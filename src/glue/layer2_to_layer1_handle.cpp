#include "layer2_to_layer1_handle.h"
#include <proto/cpp/agent.pb.h>
#include <database/database_proxy.h>
#include <common/layer1_transaction.h>
#include <common/layer2_transaction.h>
#include <common/web_client.h>
#include <common/private_key.h>
namespace agent {
    Layer2ToLayer1Handle::Layer2ToLayer1Handle(){
        start_tx_prefix_ = "L2_StartTx_";
        send_tx_prefix_ = "L2_SendTx_";
        send_ack_prefix_ = "L2_SendAck_";
        reply_ack_prefix_ = "L2_ReplyAck_";
    }

    bool Layer2ToLayer1Handle::Initialize(){

        std::set<std::string> start_tx_list;
        if(DatabaseProxy::Instance().GetAllValueByPrefix(start_tx_prefix_, start_tx_list)){
            for(auto& start_tx: start_tx_list){
                protocol::CrossTxInfo cross_tx_info;
                if(cross_tx_info.ParseFromString(start_tx)){
                    CrossTxInfoPtr cross_send_ackd = std::make_shared<CrossTx>(cross_tx_info);
                    utils::MutexGuard guard(start_tx_info_lock_);
                    start_tx_info_map_.emplace(cross_send_ackd->cross_tx_id_,cross_send_ackd);
                    LOG_DEBUG("Layer2ToLayer1Handle Initialize start_tx_info_map_ cross_txid is :%s",cross_send_ackd->cross_tx_id_.c_str());
                }else{
                    LOG_ERROR("Layer2ToLayer1Handle Start tx ParseFromString failed");
                    continue;
                }
            }
        }

        std::set<std::string> send_tx_list;
        if(DatabaseProxy::Instance().GetAllValueByPrefix(send_tx_prefix_, send_tx_list)){
            for(auto& send_tx: send_tx_list){
                protocol::CrossTxInfo cross_tx_info;
                if(cross_tx_info.ParseFromString(send_tx)){
                    CrossTxInfoPtr cross_send_ackd = std::make_shared<CrossTx>(cross_tx_info);
                    utils::MutexGuard guard(send_tx_info_lock_);
                    send_tx_info_map_.emplace(cross_send_ackd->cross_tx_id_,cross_send_ackd);
                    LOG_DEBUG("Layer2ToLayer1Handle Initialize send_tx_info_map_ cross_txid is :%s",cross_send_ackd->cross_tx_id_.c_str());
                }else{
                    LOG_ERROR("Layer2ToLayer1Handle Send tx ParseFromString failed");
                    continue;
                }
            }
        }

        std::set<std::string> send_ack_list;
        if(DatabaseProxy::Instance().GetAllValueByPrefix(send_ack_prefix_, send_ack_list)){
            for(auto& send_ack: send_ack_list){
                protocol::CrossTxInfo cross_tx_info;
                if(cross_tx_info.ParseFromString(send_ack)){
                    CrossTxInfoPtr cross_send_ackd = std::make_shared<CrossTx>(cross_tx_info);
                    cross_send_ackd->send_ack_timeout_ = utils::Timestamp::HighResolution() + 60 * utils::MICRO_UNITS_PER_SEC;
                    utils::MutexGuard guard(send_ack_info_lock_);
                    send_ack_info_map_.emplace(cross_send_ackd->cross_tx_id_,cross_send_ackd);
                    LOG_DEBUG("Layer2ToLayer1Handle Initialize send_ack_info_map_ cross_txid is :%s",cross_send_ackd->cross_tx_id_.c_str());
                }else{
                    LOG_ERROR("Layer2ToLayer1Handle Send ack ParseFromString failed");
                    continue;
                }
            }
        }
        std::set<std::string> reply_ack_list;
        if(DatabaseProxy::Instance().GetAllValueByPrefix(reply_ack_prefix_, reply_ack_list)){
            for(auto& reply_ack: reply_ack_list){
                protocol::CrossTxInfo cross_tx_info;
                if(cross_tx_info.ParseFromString(reply_ack)){
                    CrossTxInfoPtr cross_reply_ackd = std::make_shared<CrossTx>(cross_tx_info);
                    cross_reply_ackd->send_ack_timeout_ = utils::Timestamp::HighResolution() + 60 * utils::MICRO_UNITS_PER_SEC;
                    utils::MutexGuard guard(reply_ack_info_lock_);
                    reply_ack_info_map_.emplace(cross_reply_ackd->cross_tx_id_,cross_reply_ackd);
                    LOG_DEBUG("Layer2ToLayer1Handle Initialize send_ack_info_map_ cross_txid is :%s",cross_reply_ackd->cross_tx_id_.c_str());
                }else{
                    LOG_ERROR("Layer2ToLayer1Handle Reply ack ParseFromString failed");
                    continue;
                }
            }
        }
        LOG_INFO("Layer2ToLayer1Handle StartTx map size is %d,SendTx map size is %d,ReplyTx map size is %d",
        start_tx_info_map_.size(), send_ack_info_map_.size(),reply_ack_info_map_.size());
        return true;
    }

    bool Layer2ToLayer1Handle::HandleStartTxEvent(const std::string& cross_txid){
        std::shared_ptr<HttpClient> httpClient = std::make_shared<HttpClient>(Configure::Instance().layer2_configure_.http_endpoint_);
        std::string cross_txinfo1,cross_txinfo2;
        if(!httpClient->GetLayer2CrossTxInfo(cross_txid, cross_txinfo1,cross_txinfo2)){
            LOG_ERROR("Layer2ToLayer1Handle  GetLayer1CrossTxInfo failed");
            return false;
        }

        protocol::CrossTxInfo cross_tx_info;
        if(!makeCrossTxInfo(cross_tx_info, cross_txinfo1,cross_txinfo2)){
            LOG_ERROR("Layer2ToLayer1Handle  makeCrossTxInfo failed");
            return false;
        }
        int64_t layer2_seq = 0;
        if(!httpClient->GetLayerSeq(layer2_seq)){
            LOG_ERROR("get layer2 seq failed");
            return false;
        }
        cross_tx_info.set_confirmseq(layer2_seq);
        CrossTxInfoPtr cross_start_tx = std::make_shared<CrossTx>(cross_tx_info);
        LOG_DEBUG("Debug:HandleStartTxEvent cross_txid is %s, cross_tx_info is %s",cross_txid.c_str(),cross_tx_info.DebugString().c_str());
        SaveStartTxInfo(cross_start_tx);
        return true;
    }

    bool Layer2ToLayer1Handle::HandleSendTxEvent(const std::string& cross_txid){
        //从L1获取cross_tx_info, 调用L2合约的SendAcked方法
        return handleSendTxEvent(cross_txid,CROSS_RESULT_SUCCESS);
    }

    bool Layer2ToLayer1Handle::HandleSendAckdTxEvent(const std::string& cross_txid){
        //从L2获取cross_tx_info, 调用L1合约的SendAckdTx方法
        std::shared_ptr<HttpClient> httpClient = std::make_shared<HttpClient>(Configure::Instance().layer2_configure_.http_endpoint_);
        std::string cross_txinfo1,cross_txinfo2;
        if(!httpClient->GetLayer2CrossTxInfo(cross_txid, cross_txinfo1,cross_txinfo2)){
            LOG_ERROR("Layer2ToLayer1Handle  GetLayer1CrossTxInfo failed");
            return false;
        }
        protocol::CrossTxInfo cross_tx_info;
        if(!makeCrossTxInfo(cross_tx_info, cross_txinfo1,cross_txinfo2)){
            LOG_ERROR("Layer2ToLayer1Handle  makeCrossTxInfo failed");
            return false;
        }
        if(ReplyAckToLayer1(cross_tx_info)){
            ClearSendAckInfo(cross_txid);
            return true;
        }else{
            LOG_ERROR("Layer2ToLayer1Handle  ReplyAckToLayer1 failed");
            return false;
        }
        return true;
    }
    
    bool Layer2ToLayer1Handle::HandleConfirmSeqEvent(const int64_t confirm_seq){
        LOG_DEBUG("handle confirm seq event, confirm_seq=%ld",confirm_seq);
        utils::MutexGuard guard(start_tx_info_lock_);
        for(const auto& start_tx : start_tx_info_map_){
            int64_t current_time = utils::Timestamp::HighResolution();
            protocol::CrossTxInfo cross_tx_info;
            cross_tx_info.ParseFromString(start_tx.second->cross_tx_info_);
            LOG_DEBUG("Debug:HandleConfirmSeqEvent cross_txid is %s, cross_tx_info is %s",start_tx.first.c_str(),cross_tx_info.DebugString().c_str());
            if(cross_tx_info.confirmseq() <= confirm_seq){
                LOG_DEBUG("cross_tx_info confirmseq=(%ld) <= confirm_seq=(%ld)",cross_tx_info.confirmseq(),confirm_seq);
                if(cross_tx_info.timeout() < current_time){
                    LOG_ERROR("cross_tx_info timeout(%ld) < current_time(%ld)",cross_tx_info.timeout(),current_time);
                    cross_tx_info.set_status(CROSS_RESULT_TIMEOUT);
                    SendAckToLayer2(cross_tx_info);
                }else{
                    if(SendTxToLayer1(cross_tx_info)){
                        ClearStartTxInfo(cross_tx_info.crosstxid());
                    }
                } 
            }
        }
        return true;
    }
    
    bool Layer2ToLayer1Handle::SendTxToLayer1(protocol::CrossTxInfo cross_tx_info){
        std::string txhash;
        Json::Value input;
        input["function"] = "SendTx(bytes,address,address,uint256,uint256)";//TODO
        input["args"]=  utils::String::Format("'%s',%s,%s,%ld,%ld",
        cross_tx_info.crosstxid().c_str(),cross_tx_info.srcbid().c_str(),
        cross_tx_info.destbid().c_str(),cross_tx_info.amount(),cross_tx_info.timeout());
        if(!sendTransactionToLayer1(input.toStyledString(),cross_tx_info.amount(),txhash)){
            LOG_ERROR("Layer2ToLayer1Handle  SendTxToLayer1 failed");
            return false;
        }
        cross_tx_info.set_sendtxhash(txhash);
        CrossTxInfoPtr cross_send_tx = std::make_shared<CrossTx>(cross_tx_info);
        SaveSendTxInfo(cross_send_tx);
        LOG_INFO("SendTxToLayer1 success,cross_tx id is:%s, txhash:%s",cross_tx_info.crosstxid().c_str(), txhash.c_str());
        return true;
    }

    bool Layer2ToLayer1Handle::SendAckToLayer2(protocol::CrossTxInfo cross_tx_info){
        //1.get sendtx hash ,TODO check @liuyuanchao
        std::string txhash;
        Json::Value input;
        std::string error_desc = cross_tx_info.status() > CROSS_RESULT_SUCCESS ? "send tx failed" : "";
        input["function"] = "SendAckedTx(bytes,uint8,string,uint256)";//TODO checkout args @liuyuanchao
        input["args"]=  utils::String::Format("'%s',%d,'%s',%ld",
        cross_tx_info.crosstxid().c_str(),cross_tx_info.status(),error_desc.c_str(),cross_tx_info.executetimestamp());
        if(!sendTransactionToLayer2(input.toStyledString(),0,txhash)){
            LOG_ERROR("Layer2ToLayer1Handle SendAckToLayer2 failed");
            return false;
        }
        cross_tx_info.set_sendacktol2hash(txhash);
        CrossTxInfoPtr cross_send_ackd = std::make_shared<CrossTx>(cross_tx_info);
        SaveSendAckInfo(cross_send_ackd);
        LOG_INFO("SendAckToLayer2 status is %d,cross_tx id is:%s, txhash:%s",cross_tx_info.status(),cross_tx_info.crosstxid().c_str(), txhash.c_str());
        return true;
    }

    bool Layer2ToLayer1Handle::ReplyAckToLayer1(protocol::CrossTxInfo cross_tx_info){
        //1.get sendtx hash ,TODO check @liuyuanchao
        std::string txhash;
        Json::Value input;
        std::string error_desc = cross_tx_info.status() > CROSS_RESULT_SUCCESS ? "send tx failed" : "";
        input["function"] = "SendAckedTx(bytes,uint8,string,uint256)";//TODO checkout args @liuyuanchao
        input["args"]=  utils::String::Format("'%s',%d,'%s',%ld",
        cross_tx_info.crosstxid().c_str(),cross_tx_info.status(),error_desc.c_str(),cross_tx_info.executetimestamp());
        if(!sendTransactionToLayer1(input.toStyledString(),0,txhash)){
            LOG_ERROR("Layer2ToLayer1Handle  ReplyAckToLayer1 failed");
            return false;
        }
        cross_tx_info.set_sendacktol1hash(txhash);
        CrossTxInfoPtr cross_send_ackd = std::make_shared<CrossTx>(cross_tx_info);
        SaveReplyAckInfo(cross_send_ackd);
        LOG_INFO("ReplyAckToLayer1 then clear send ack info, status is %d, cross_tx id is:%s, txhash:%s",cross_tx_info.status(),cross_tx_info.crosstxid().c_str(), txhash.c_str());
        return true;
    }

    bool Layer2ToLayer1Handle::makeCrossTxInfo(protocol::CrossTxInfo &cross_tx_info,const std::string cross_info1, const std::string cross_info2){
        if(cross_info1.empty() || cross_info2.empty()){
            LOG_ERROR("cross_info1 or cross_info2 is empty");
            return false;
        }
        LOG_DEBUG("Debug:cross_info1:%s,cross_info2:%s",cross_info1.c_str(),cross_info2.c_str());
        //1.parse cross_info1 & cross_info2
        //cross_info1 eg:"[0,100,0f665d34b2bc8bbb5f713284efe9fb0c15bae2385b603678151cd33c2ee6084e,100,did:bid:efkdYHzcgLiHHCq1SKayMtqVHpxveSDD,did:bid:efkdYHzcgLiHHCq1SKayMtqVHpxveSDD]"
        std::string info1 = cross_info1.substr(1,cross_info1.size()-2);
        std::vector<std::string> info1_list;
        std::stringstream ss(info1);
        std::string item;
        while (std::getline(ss, item, ',')) {
            info1_list.push_back(item);
        }
        //cross_info2 eg:"[0,0,1724897213598443,0,1724897813598443]"
        std::string info2 = cross_info2.substr(1,cross_info2.size()-2);
        std::vector<std::string> info2_list;
        std::stringstream ss2(info2);
        while (std::getline(ss2, item, ',')) {
            info2_list.push_back(item);
        }
        //2.init cross_tx_info
        cross_tx_info.set_crosstxid(info1_list[2]);
        cross_tx_info.set_amount(utils::String::Stol(info1_list[3]));
        cross_tx_info.set_srcbid(info1_list[4]);
        cross_tx_info.set_destbid(info1_list[5]);
        cross_tx_info.set_status(utils::String::Stoi(info2_list[0]));
        cross_tx_info.set_refunded(utils::String::Stoi(info2_list[1]));
        cross_tx_info.set_createtimestamp(utils::String::Stol(info2_list[2]));
        cross_tx_info.set_executetimestamp(utils::String::Stol(info2_list[3]));
        cross_tx_info.set_timeout(utils::String::Stol(info2_list[4]));
        if(info2_list.size() > 5){
            cross_tx_info.set_errordesc(info2_list[5]);
        }else{
            cross_tx_info.set_errordesc("");
        }
        LOG_DEBUG("makeCrossTxInfo success,cross_tx_info is %s",cross_tx_info.ShortDebugString().c_str());
        return true;
    }

    bool Layer2ToLayer1Handle::StartTxTimeOut(){
        auto current_time = utils::Timestamp::HighResolution();
        utils::MutexGuard guard(start_tx_info_lock_);
        for(const auto& starttx : start_tx_info_map_){
            LOG_INFO("Debug:starttx.first:%s,starttx.second->send_tx_timeout_:%ld,current_time:%ld",starttx.first.c_str(),starttx.second->send_tx_timeout_,current_time);
            if(starttx.second->send_tx_timeout_ < current_time){
                protocol::CrossTxInfo cross_tx_info;
                cross_tx_info.ParseFromString(starttx.second->cross_tx_info_);
                cross_tx_info.set_status(CROSS_RESULT_FAILED);
                if(SendAckToLayer2(cross_tx_info)){
                    ClearStartTxInfo(starttx.first);
                }
                LOG_ERROR("Start_Tx TimeOut, Send Ack To Layer2,please check cross_tx_id is (%s) .",starttx.first.c_str());
            }
        }
        return true;
    }

    bool Layer2ToLayer1Handle::SendTxToLayer1TimeOut(){
        auto current_time = utils::Timestamp::HighResolution();
        utils::MutexGuard guard(send_tx_info_lock_);
        for(const auto& sendtx : send_tx_info_map_){
            if(sendtx.second->send_tx_timeout_ < current_time){
                //1.update status by send tx hash
                std::string send_tx_hash = sendtx.second->send_tx_hash_;
                std::shared_ptr<HttpClient> httpClient = std::make_shared<HttpClient>(Configure::Instance().layer1_configure_.http_endpoint_);
                if(httpClient->GetTxResult(send_tx_hash)){
                    LOG_INFO("SendTxToLayer1 success,cross_tx_id is (%s), tx hash is %s",sendtx.second->cross_tx_id_.c_str(), send_tx_hash.c_str());
                    //send ackd tx to layer1
                    handleSendTxEvent(sendtx.second->cross_tx_id_,CROSS_RESULT_SUCCESS);
                }else{
                    LOG_INFO("SendTxToLayer2 failed, cross_tx_id is (%s), tx hash is %s",sendtx.second->cross_tx_id_.c_str(), send_tx_hash.c_str());
                    protocol::CrossTxInfo cross_tx_info;
                    cross_tx_info.ParseFromString(sendtx.second->cross_tx_info_);
                    cross_tx_info.set_status(CROSS_RESULT_TIMEOUT);
                    if(SendAckToLayer2(cross_tx_info)){
                        ClearSendTxInfo(cross_tx_info.crosstxid().c_str());
                    }
                }               
            }
        }
        return true;
    }
   
    bool Layer2ToLayer1Handle::SendAckToLayer2TimeOut(){
        auto current_time = utils::Timestamp::HighResolution();
        utils::MutexGuard guard(send_ack_info_lock_);
        for(const auto& acktx : send_ack_info_map_){
            if(acktx.second->send_ack_timeout_ < current_time){
                protocol::CrossTxInfo cross_tx_info;
                cross_tx_info.ParseFromString(acktx.second->cross_tx_info_);
                //if status is success, need resend
                if(cross_tx_info.status() == CROSS_RESULT_SUCCESS){
                    SendAckToLayer2(cross_tx_info);
                    acktx.second->send_ack_timeout_ = current_time + utils::MICRO_UNITS_PER_SEC * 60;
                    LOG_INFO("SendAckToLayer2 timeout and resend,cross_tx_id is (%s), status is success",cross_tx_info.crosstxid().c_str());
                }else {   //status is fail, need check tx hash,because contract not send tlog when status is fail
                    //if tx hash is ok, delete it
                    std::string tx_hash = acktx.second->send_ack_to_l2_hash_;
                    std::shared_ptr<HttpClient> httpClient = std::make_shared<HttpClient>(Configure::Instance().layer2_configure_.http_endpoint_);
                    if(httpClient->GetTxResult(tx_hash)){
                        LOG_INFO("SendAckToLayer2 success and clear send ack info,cross_tx_id is (%s), status is failed",cross_tx_info.crosstxid().c_str());
                        ClearSendAckInfo(acktx.first);
                    }else{ //resend
                        auto status = httpClient->GetCrossTxStatus(acktx.first);
                        if(status != CROSS_RESULT_INIT){
                            ClearSendAckInfo(acktx.first);
                            LOG_INFO("Already send ack to layer2,cross_tx_id is (%s), status is (%d),then clear send ack info",acktx.first.c_str(),status);
                        }else{
                            SendAckToLayer2(cross_tx_info);
                            acktx.second->send_ack_timeout_ = current_time + utils::MICRO_UNITS_PER_SEC * 60;
                            LOG_INFO("SendAckToLayer2 timeout and resend,cross_tx_id is (%s), status is failed, new_send_ack_hash is %s",cross_tx_info.crosstxid().c_str(),cross_tx_info.sendacktol2hash().c_str());
                       }
                    }
                }
            }
        }
        return true;
    }
    
    bool Layer2ToLayer1Handle::ReplyAckToLayer1TimeOut(){
        auto current_time = utils::Timestamp::HighResolution();
        utils::MutexGuard guard(reply_ack_info_lock_);
        for(const auto& acktx : reply_ack_info_map_){
            if(acktx.second->send_ack_timeout_ < current_time){
                protocol::CrossTxInfo cross_tx_info;
                cross_tx_info.ParseFromString(acktx.second->cross_tx_info_);
                //if tx hash is ok, delete it
                std::string tx_hash = acktx.second->send_ack_to_l1_hash_;
                std::shared_ptr<HttpClient> httpClient = std::make_shared<HttpClient>(Configure::Instance().layer1_configure_.http_endpoint_);
                if(httpClient->GetTxResult(tx_hash)){
                    LOG_INFO("ReplyAckToLayer1 success and clear reply ack info,cross_tx_id is (%s), txhash is %s",cross_tx_info.crosstxid().c_str(),tx_hash.c_str());
                    ClearReplyAckInfo(acktx.first);
                }else{ //resend
                    auto status = httpClient->GetCrossTxStatus(acktx.first);
                    if(status != CROSS_RESULT_INIT){
                        ClearReplyAckInfo(acktx.first);
                        LOG_INFO("Already reply ack to layer1,cross_tx_id is (%s), status is (%d),then clear send ack info",acktx.first.c_str(),status);
                    }else{
                        ReplyAckToLayer1(cross_tx_info);
                        acktx.second->send_ack_timeout_ = current_time + utils::MICRO_UNITS_PER_SEC * 60;
                        LOG_INFO("ReplyAckToLayer1 failed and resend ack info,cross_tx_id is (%s), txhash is %s",cross_tx_info.crosstxid().c_str(),cross_tx_info.sendacktol1hash().c_str());
                    }
                }
            }
        }
        return true;
    }

    void Layer2ToLayer1Handle::OnTimer(int64_t current_time){
        //1.start tx timeout
        StartTxTimeOut();
        //2.check if send tx timeout
        SendTxToLayer1TimeOut();
        //3.check if send ackd tx to layer2 timeout
        SendAckToLayer2TimeOut();
        //4.check if send ackd tx to layer1 timeout
        ReplyAckToLayer1TimeOut();
        return;
    }

    bool Layer2ToLayer1Handle::SaveStartTxInfo(CrossTxInfoPtr cross_tx_info){
        if(cross_tx_info){
            auto dbkey = start_tx_prefix_ + cross_tx_info->cross_tx_id_;
            DatabaseProxy::Instance().DBPut(dbkey,cross_tx_info->cross_tx_info_);
            utils::MutexGuard guard(start_tx_info_lock_);
            start_tx_info_map_.emplace(cross_tx_info->cross_tx_id_,cross_tx_info); 
        }
        return true;
    }
    
    bool Layer2ToLayer1Handle::ClearStartTxInfo(const std::string cross_txid){
        auto dbkey = start_tx_prefix_ + cross_txid;
        DatabaseProxy::Instance().DBDelete(dbkey);
        utils::MutexGuard guard(start_tx_info_lock_);
        start_tx_info_map_.erase(cross_txid);
        return true;
    }

    bool Layer2ToLayer1Handle::SaveSendTxInfo(CrossTxInfoPtr cross_tx_info){
        if(cross_tx_info){
            auto dbkey = send_tx_prefix_ + cross_tx_info->cross_tx_id_;
            auto ret = DatabaseProxy::Instance().DBPut(dbkey,cross_tx_info->cross_tx_info_);
            LOG_DEBUG("SaveSendTxInfo: cross id is (%s),ret is (%d),dbkey is (%s)",cross_tx_info->cross_tx_id_.c_str(),ret,dbkey.c_str());
            utils::MutexGuard guard(send_tx_info_lock_);
            send_tx_info_map_.emplace(cross_tx_info->cross_tx_id_,cross_tx_info);
        }
        return true;
    }

    bool Layer2ToLayer1Handle::ClearSendTxInfo(const std::string cross_txid){
        auto dbkey = send_tx_prefix_ + cross_txid;
        auto ret = DatabaseProxy::Instance().DBDelete(dbkey);
        LOG_DEBUG("ClearSendTxInfo: cross id is (%s),ret is (%d),dbkey is (%s)",cross_txid.c_str(),ret,dbkey.c_str());
        utils::MutexGuard guard(send_tx_info_lock_);
        send_tx_info_map_.erase(cross_txid);
        return true;
    }

    bool Layer2ToLayer1Handle::SaveSendAckInfo(CrossTxInfoPtr cross_tx_info){
        if(cross_tx_info){
            cross_tx_info->send_ack_timeout_ = utils::Timestamp::HighResolution() + utils::MICRO_UNITS_PER_SEC * 60;
            auto dbkey = send_ack_prefix_ + cross_tx_info->cross_tx_id_;
            auto ret = DatabaseProxy::Instance().DBPut(dbkey,cross_tx_info->cross_tx_info_);
            LOG_DEBUG("SaveSendAckInfo: cross id is (%s),ret is (%d),send_ack_prefix_ is (%s),dbkey is (%s)",cross_tx_info->cross_tx_id_.c_str(),ret,send_ack_prefix_.c_str(),dbkey.c_str());
            utils::MutexGuard guard(send_ack_info_lock_);
            send_ack_info_map_.emplace(cross_tx_info->cross_tx_id_,cross_tx_info);
        }
        return true;
    }

    bool Layer2ToLayer1Handle::ClearSendAckInfo(const std::string cross_txid){
        auto dbkey = send_ack_prefix_ + cross_txid;
        auto ret = DatabaseProxy::Instance().DBDelete(dbkey);
        LOG_DEBUG("ClearSendAckInfo: cross id is (%s),ret is (%d),dbkey is (%s)",cross_txid.c_str(),ret,dbkey.c_str());
        utils::MutexGuard guard(send_ack_info_lock_);
        send_ack_info_map_.erase(cross_txid);
        return true;
    }

    bool Layer2ToLayer1Handle::SaveReplyAckInfo(CrossTxInfoPtr cross_tx_info){
        if(cross_tx_info){
            cross_tx_info->send_ack_timeout_ = utils::Timestamp::HighResolution() + utils::MICRO_UNITS_PER_SEC * 60;
            auto dbkey = utils::String::Format("%s%s",reply_ack_prefix_.c_str(),cross_tx_info->cross_tx_id_.c_str());
            auto ret = DatabaseProxy::Instance().DBPut(dbkey,cross_tx_info->cross_tx_info_);
            LOG_DEBUG("SaveReplyAckInfo: cross id is (%s),ret is (%d),dbkey is (%s)",cross_tx_info->cross_tx_id_.c_str(),ret,dbkey.c_str());
            utils::MutexGuard guard(reply_ack_info_lock_);
            reply_ack_info_map_.emplace(cross_tx_info->cross_tx_id_,cross_tx_info);
        }
        return true;
    }

    bool Layer2ToLayer1Handle::ClearReplyAckInfo(const std::string cross_txid){
        auto dbkey = utils::String::Format("%s%s",reply_ack_prefix_.c_str(),cross_txid.c_str());
        auto ret = DatabaseProxy::Instance().DBDelete(dbkey);
        LOG_DEBUG("ClearReplyAckInfo: cross id is (%s),ret is (%d),dbkey is (%s)",cross_txid.c_str(),ret,dbkey.c_str());
        utils::MutexGuard guard(reply_ack_info_lock_);
        reply_ack_info_map_.erase(cross_txid);
        return true;
    }

    bool Layer2ToLayer1Handle::sendTransactionToLayer2(std::string input,int64_t amount, std::string& txhash){
        Layer2TransactionPtr layer2_transaction = std::make_shared<Layer2Transaction>();
        layer2_transaction->AddTransactionOperation(input,amount);
        LOG_DEBUG("Debug:send Tx to L2 Cross Contract, input:%s",input.c_str());
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
            LOG_ERROR("submitTransaction failed");
            return false;
        }
        LOG_DEBUG("submitTransaction response is %s",response.value().body.c_str());
        if(!checkTxResponse(response.value().body,txhash)){
            LOG_ERROR("check tx response failed");
            return false;
        }
        return true;
    }

    bool Layer2ToLayer1Handle::sendTransactionToLayer1(std::string input,int64_t amount, std::string& txhash){
        Layer1TransactionPtr layer1_transaction = std::make_shared<Layer1Transaction>();
        layer1_transaction->AddTransactionOperation(input,amount);
        LOG_DEBUG("Debug:send Tx to L1Cross Contract, input:%s",input.c_str());
        std::shared_ptr<HttpClient> httpClient = std::make_shared<HttpClient>(Configure::Instance().layer1_configure_.http_endpoint_);
        int64_t layer1_seq = 0;
        if(!httpClient->GetLayerSeq(layer1_seq)){
            LOG_ERROR("get layer1 seq failed");
            return false;
        }
        
        int64_t max_seq = layer1_seq + 200;
        LOG_DEBUG("set tx max_ledger_seq is %ld",max_seq);
        std::string tran_blob = layer1_transaction->GetTransactionBlob(max_seq);
        PrivateKeyV4 prikey(Configure::Instance().layer1_configure_.private_key_);

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
        httpClient->SubmitLayer1Transaction(requestBodyString,response);
        if(response.error() != httplib::Error::Success){
            LOG_ERROR("Layer1SendAckdTx failed");
            return false;
        }
        LOG_DEBUG("Layer1SendAckdTx response is %s",response.value().body.c_str());
        if(!checkTxResponse(response.value().body,txhash)){
            LOG_ERROR("check tx response failed");
            return false;
        }
        return true;
    }

    bool Layer2ToLayer1Handle::checkTxResponse(std::string response,std::string& txhash){
        Json::Value responseBody;
        Json::Reader reader;
        if (!responseBody.fromString(response)){
            LOG_ERROR("Failed to parse response body");
            return false;
        }
        const Json::Value& results = responseBody["results"];
        if(results.isArray() && results.size() > 0){
            for(const auto &result : results){
                 auto error_code = result["error_code"].asInt();
                 if(error_code != 0){
                    LOG_ERROR("check Tx Response failed, error code is %d, error message:%s",error_code,result["error_desc"].asString().c_str());
                    return false;
                }
                txhash = result["hash"].asString();
            }
        }
       
        return true;
    }

    bool Layer2ToLayer1Handle::handleSendTxEvent(const std::string& cross_txid, int64_t suatus){
        std::shared_ptr<HttpClient> httpClient = std::make_shared<HttpClient>(Configure::Instance().layer1_configure_.http_endpoint_);
        std::string cross_txinfo1,cross_txinfo2;
        if(!httpClient->GetLayer1CrossTxInfo(cross_txid, cross_txinfo1,cross_txinfo2)){
            LOG_ERROR("Layer2ToLayer1Handle  GetLayer1CrossTxInfo failed");
            return false;
        }
        protocol::CrossTxInfo cross_tx_info;
        if(!makeCrossTxInfo(cross_tx_info, cross_txinfo1,cross_txinfo2)){
            LOG_ERROR("Layer2ToLayer1Handle  makeCrossTxInfo failed");
            return false;
        }
        cross_tx_info.set_status(suatus);
        if(SendAckToLayer2(cross_tx_info)){
            ClearSendTxInfo(cross_txid);
            return true;
        }else{
            LOG_ERROR("Layer2ToLayer1Handle  SendAckToLayer2 failed");
            return false;
        }
        return true;
    }
}