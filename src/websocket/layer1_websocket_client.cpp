#include "layer1_websocket_client.h"
#include "utils/logger.h"
#include "utils/timestamp.h"
#include "glue/handle_manager.h"
#include <main/configure.h>

namespace agent{
    Layer1WebSocketClient::~Layer1WebSocketClient(){
        if (thread_ptr_){
            delete thread_ptr_;
        }
    }

    bool Layer1WebSocketClient::Initialize()
    {
        LOG_INFO("Layer1WebSocketClient Initialize");
        ping_in_flight_ = false;
        init(Configure::Instance().layer1_configure_.ws_endpoint_);
        thread_ptr_ = new utils::Thread(this);
        if (!thread_ptr_->Start("layer1-websocket")) {
            return false;
        }
        return true;
    }

	bool Layer1WebSocketClient::Exit(){
        close();
        thread_ptr_->JoinWithStop(); 
        LOG_INFO("Layer1WebSocketClient Exit");
        return true;
    }

    void Layer1WebSocketClient::on_message(websocketpp::connection_hdl hdl, server::message_ptr msg){
        protocol::WsMessage message;
        if (!message.ParseFromString(msg->get_payload())){
            LOG_ERROR("Layer1WebSocketClient on_message ParseFromString failed");
            return;
        }
        switch (message.type()){
            case protocol::CHAIN_CONTRACT_LOG:
                HandleManager::Instance().OnLayer1SubscribeResponse(message.data());
                break;
            case protocol::OVERLAY_MSGTYPE_PING:
                on_ping(hdl);
                break;
            default:
                break;
        }
        return;
    }
    void Layer1WebSocketClient::subscribe(websocketpp::connection_hdl hdl){
        subscribe_crosscontract(hdl);
        subscribe_managercontract(hdl);
        return;
    }

    void Layer1WebSocketClient::subscribe_crosscontract(websocketpp::connection_hdl hdl){
        LOG_INFO("Layer1WebSocketClient on_open send tlog");
        protocol::WsMessage message_tlog;
        protocol::TlogSubscribeRequest tlog_request;
        tlog_request.set_address(Configure::Instance().layer1_configure_.cross_contract_);
        message_tlog.set_request(true);
        message_tlog.set_type(protocol::CHAIN_CONTRACT_LOG);
        message_tlog.set_data(tlog_request.SerializeAsString());
        try{
            client_.send(hdl, message_tlog.SerializeAsString(), websocketpp::frame::opcode::binary);
        }catch (websocketpp::exception const & e) {
            LOG_INFO("Layer1WebSocketClient on_open send tlog error: %s", e.what());
        }
        LOG_INFO("Layer1WebSocketClient subscribe tlog");
        return;
    }

    void Layer1WebSocketClient::subscribe_managercontract(websocketpp::connection_hdl hdl){
        LOG_INFO("subscribe_managercontract on_open send tlog");
        protocol::WsMessage message_tlog;
        protocol::TlogSubscribeRequest tlog_request;
        tlog_request.set_address(Configure::Instance().layer1_configure_.manager_contract_);
        auto agent_id_str = utils::String::ToString(Configure::Instance().layer2_configure_.layer2_id_);
        for(auto &item : Configure::Instance().layer1_configure_.topics_){
             auto topic = item + "_" + agent_id_str;
             tlog_request.add_topics(topic);
             LOG_INFO("subscribe topic: %s",topic.c_str());
        }
        message_tlog.set_request(true);
        message_tlog.set_type(protocol::CHAIN_CONTRACT_LOG);
        message_tlog.set_data(tlog_request.SerializeAsString());
        try{
            client_.send(hdl, message_tlog.SerializeAsString(), websocketpp::frame::opcode::binary);
        }catch (websocketpp::exception const & e) {
            LOG_INFO("subscribe_managercontract on_open send tlog error: %s", e.what());
        }
        LOG_INFO("subscribe_managercontract subscribe tlog");
        return;
    }

    void Layer1WebSocketClient::Run(utils::Thread *thread) {
        StartClient();
        return;
    }

}