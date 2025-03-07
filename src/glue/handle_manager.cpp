#include <utils/headers.h>
#include <main/configure.h>
#include "handle_manager.h"
#include <utils/thread.h>
#include <common/web_client.h>
#include <proto/cpp/subscribe.pb.h>
#include <proto/cpp/common.pb.h>
#include <database/database_proxy.h>
#include <common/private_key.h>
#include <common/layer2_transaction.h>


namespace agent {

    HandleManager::HandleManager(): TimerNotify("HandleManager", 60 * utils::MICRO_UNITS_PER_SEC){
        layer1_to_layer2_handle_ = std::make_shared<Layer1ToLayer2Handle>();
        layer2_to_layer1_handle_ = std::make_shared<Layer2ToLayer1Handle>();
	}

	HandleManager::~HandleManager() {}

	bool HandleManager::Initialize() {
        if(CheckLayer2Balance() == false){
            return false;
        }
        if(CheckLayer1Balance() == false){
            return false;
        }
        layer1_to_layer2_handle_->Initialize();
        layer2_to_layer1_handle_->Initialize();
        TimerNotify::RegisterModule(this);
        return true;
	}
	
    bool HandleManager::Exit() {
		return true;
	}

    bool HandleManager::OnLayer1SubscribeResponse(const std::string& msg){
       LOG_DEBUG("OnLayer1SubscribeResponse  msg %s",msg.c_str());
        protocol::TlogSubscribeResponse tlog_res;
        if(!tlog_res.ParseFromString(msg)){
            LOG_ERROR("OnLayer1SubscribeResponse  ParseFromString failed");
            return false;
        }
        LOG_DEBUG("TEST tlog response is %s",tlog_res.DebugString().c_str());
        std::string contract_address = tlog_res.address();
        if(contract_address == Configure::Instance().layer1_configure_.cross_contract_){
            return OnLayer1CrossContractTlog(tlog_res);
        }else if(contract_address == Configure::Instance().layer1_configure_.manager_contract_){
            return OnLayer1ManagerContractTlog(tlog_res);
        }else{
            LOG_ERROR("OnLayer1SubscribeResponse  address not match,tlog_address is %s,cross_contract_address is %s", contract_address.c_str(),Configure::Instance().layer1_configure_.cross_contract_.c_str());
            return false;
        }

        return true;
    }
   
    bool HandleManager::OnLayer2SubscribeResponse(const std::string& msg){
        LOG_DEBUG("OnLayer2SubscribeResponse  msg %s",msg.c_str());
        protocol::TlogSubscribeResponse tlog_res;
        if(!tlog_res.ParseFromString(msg)){
            LOG_ERROR("OnLayer2SubscribeResponse  ParseFromString failed");
            return false;
        }
        LOG_DEBUG("TEST tlog response is %s",tlog_res.DebugString().c_str());
        std::string contract_address = tlog_res.address();
        if(contract_address != Configure::Instance().layer2_configure_.cross_contract_){
            LOG_ERROR("OnLayer2SubscribeResponse  address not match,tlog_address is %s,manager_contract_address is %s", contract_address.c_str(),Configure::Instance().layer2_configure_.cross_contract_.c_str());
            return false;
        }

        const protocol::Tlog tlog = tlog_res.tlogs(0);
        bool ret = true;
        //1.根据topic判断tlog类型
        std::string topic = tlog.topic();
        auto cross_tx_id = tlog.tlog(0);
        if(topic == Configure::Instance().layer2_configure_.start_tx_topic_){
            ret = layer2_to_layer1_handle_->HandleStartTxEvent(cross_tx_id);
        }else if(topic == Configure::Instance().layer2_configure_.send_tx_topic_){
            ret = layer1_to_layer2_handle_->HandleSendTxEvent(cross_tx_id);
        }else if(topic == Configure::Instance().layer2_configure_.ack_tx_topic_){
            ret = layer2_to_layer1_handle_->HandleSendAckdTxEvent(cross_tx_id);
        }else{
            LOG_ERROR("OnLayer2SubscribeResponse  topic (%s) not match",topic.c_str());
            ret = false;
        }
        return ret;
    }
    
    bool HandleManager::OnLayer1CrossContractTlog(protocol::TlogSubscribeResponse &tlog_res){
        const protocol::Tlog tlog = tlog_res.tlogs(0);
        bool ret = true;
        //1.根据topic判断tlog类型
        auto topic = tlog.topic();
        auto cross_tx_id = tlog.tlog(0);
        if(topic == Configure::Instance().layer1_configure_.start_tx_topic_){
            ret = layer1_to_layer2_handle_->HandleStartTxEvent(cross_tx_id);
        }else if(topic == Configure::Instance().layer1_configure_.send_tx_topic_){
            ret = layer2_to_layer1_handle_->HandleSendTxEvent(cross_tx_id);
        }else if(topic == Configure::Instance().layer1_configure_.ack_tx_topic_){
            ret = layer1_to_layer2_handle_->HandleSendAckdTxEvent(cross_tx_id);
        }else{
            LOG_ERROR("OnLayer2SubscribeResponse  topic (%s) not match",topic.c_str());
            ret = false;
        }

        return ret;
    }

    bool HandleManager::OnLayer1ManagerContractTlog(protocol::TlogSubscribeResponse &tlog_res){
        LOG_DEBUG("HandleManager handle_tlog");
        const protocol::Tlog tlog = tlog_res.tlogs(0);
        for (int i = 0; i < tlog.tlog_size(); i++){
            protocol::ManagerContractTlog manager_log;
            if(!manager_log.ParseFromString(utils::String::HexStringToBin(tlog.tlog(i)))){
                LOG_ERROR("HandleManager handle_tlog ParseFromString failed");
                return false;
            }
            LOG_DEBUG("TEST manager_log is %s",manager_log.DebugString().c_str());
            switch (manager_log.tlog_type()){
                case protocol::ManagerContractTlog_TopicType_TOPIC_SUBMIT:
                {
                    LOG_DEBUG("dealtlog type submit");
                    const protocol::TlogSubmit submit = manager_log.submit();
                    int64_t confirm_seq = submit.confirm_seq();
                    layer2_to_layer1_handle_->HandleConfirmSeqEvent(confirm_seq);
                    break;
                }
                case protocol::ManagerContractTlog_TopicType_TOPIC_UPDATE_PERIOD:
                {
                    const protocol::TlogUpdatePeriod update_period = manager_log.update_period();
                    int64_t period = update_period.period();
                    updatePeriodToLayer2CrossContract(period);
                    break;
                }
                default:
                    LOG_ERROR("dealTlog unknown message type");
                    break;
            }
        }
        return true;
    }
   
    bool HandleManager::updatePeriodToLayer2CrossContract(int64_t period){
        int64_t period_second = period * 60;
        int64_t timeout = period_second * utils::MICRO_UNITS_PER_SEC + 30 * utils::SECOND_UNITS_PER_MINUTE * utils::MICRO_UNITS_PER_SEC;
        Layer2TransactionPtr layer2_transaction = std::make_shared<Layer2Transaction>();
        Json::Value input;
        input["function"] = "SetTimeout(uint256)";
        input["args"] = utils::String::ToString(timeout);
        layer2_transaction->AddTransactionOperation(input.toStyledString());
        LOG_DEBUG("Debug:update Period To Layer2 CrossContract, input:%s",input.toStyledString().c_str());
        std::shared_ptr<HttpClient> httpClient = std::make_shared<HttpClient>(Configure::Instance().layer2_configure_.http_endpoint_);
        int64_t layer2_seq = 0;
        if(!httpClient->GetLayerSeq(layer2_seq)){
            LOG_ERROR("get layer2 seq failed");
            return false;
        }
        
        int64_t max_seq = layer2_seq + 200;
        LOG_DEBUG("set tx max_ledger_seq is %ld",max_seq);
        std::string tran_blob = layer2_transaction->GetTransactionBlob(max_seq);
        PrivateKeyV4 prikey(Configure::Instance().layer2_configure_.private_key_);

        //l1-agent sign tarn_blob & send to l1-node
        Json::Value requestBody;
        Json::Value &items = requestBody["items"];
		items = Json::Value(Json::arrayValue);
        Json::Value &body_item = items[items.size()];
        body_item["transaction_blob"] = tran_blob;
        Json::Value& signature_list = body_item["signatures"];
        signature_list = Json::Value(Json::arrayValue);
        Json::Value& signature = signature_list[signature_list.size()];
        auto bin_tran_blob = utils::String::HexStringToBin(tran_blob);
        signature["public_key"] = prikey.GetEncPublicKey();
        signature["sign_data"] = utils::String::BinToHexString(prikey.Sign(bin_tran_blob));
        std::string requestBodyString = requestBody.toStyledString();
        
        httplib::Result response;
        httpClient->SubmitLayer2Transaction(requestBodyString,response);
        if(response.error() != httplib::Error::Success){
            LOG_ERROR("update Period To Layer2 CrossContract failed");
            return false;
        }
        auto body = response.value().body;
        LOG_DEBUG("update Period To Layer2 CrossContract response is %s",body.c_str());
        Json::Value responseBody;
        Json::Reader reader;
        if (!responseBody.fromString(body)){
            LOG_ERROR("Failed to parse response body");
            return false;
        }
        auto error_code = responseBody["results"]["error_code"].asInt();
        if(error_code != 0){
            LOG_ERROR("update Period To Layer2 CrossContract failed, error code is %d, error message:%s",error_code,responseBody["results"]["error_desc"].asString().c_str());
            return false;
        }
        auto txhash = responseBody["results"]["hash"].asString();
        LOG_INFO("update Period To Layer2 CrossContract success,error code is %d, txhash:%s",error_code, txhash.c_str());
        return true;
    }

    bool HandleManager::CheckLayer1Balance(){
        std::shared_ptr<HttpClient> httpClient = std::make_shared<HttpClient>(Configure::Instance().layer1_configure_.http_endpoint_);
        std::string address = Configure::Instance().layer1_configure_.agent_address_;
        int64_t balance = 0;
        int64_t min_balance = 100 * Configure::Instance().layer1_configure_.fee_limit_ * Configure::Instance().layer1_configure_.gas_price_;
        if(!httpClient->GetBalance(balance,address)){
            LOG_ERROR("get balance failed");
            return false;
        }else{
            if(balance < min_balance){
                LOG_ERROR("layer1 agent account balance is not enough, need %ld, but only have %ld",min_balance,balance);
                return false;
            }
        }
        return true;
    }
    
    bool HandleManager::CheckLayer2Balance(){
        if(Configure::Instance().layer2_configure_.gas_price_ == 0){
            return true;
        }
        std::shared_ptr<HttpClient> httpClient = std::make_shared<HttpClient>(Configure::Instance().layer2_configure_.http_endpoint_);
        std::string address = Configure::Instance().layer2_configure_.agent_address_;
        int64_t balance = 0;
        int64_t min_balance = 100 * Configure::Instance().layer2_configure_.fee_limit_ * Configure::Instance().layer2_configure_.gas_price_;
        if(!httpClient->GetBalance(balance,address)){
            LOG_ERROR("get balance failed");
            return false;
        }else{
            if(balance < min_balance){
                LOG_ERROR("layer2 agent account balance is not enough, need %ld, but only have %ld",min_balance,balance);
                return false;
            }
        }
        return true;
    }

    void HandleManager::OnTimer(int64_t current_time){
        layer1_to_layer2_handle_->OnTimer(current_time);
        layer2_to_layer1_handle_->OnTimer(current_time);
        return;
    }
}
