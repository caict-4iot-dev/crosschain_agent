#include "utils/logger.h"
#include "utils/timestamp.h"
#include "glue/handle_manager.h"
#include <main/configure.h>
#include "layer2_websocket_client.h"

namespace agent{

    Layer2WebSocketClient::~Layer2WebSocketClient(){
        if (thread_ptr_){
            delete thread_ptr_;
        }
    }
    bool Layer2WebSocketClient::Initialize(){
        LOG_INFO("Layer2WebSocketClient Initialize");
        ping_in_flight_ = false;
        init(Configure::Instance().layer2_configure_.ws_endpoint_);
        thread_ptr_ = new utils::Thread(this);
        if (!thread_ptr_->Start("layer2-websocket")) {
            return false;
        }
        return true;
    }

	bool Layer2WebSocketClient::Exit(){
        close();
        thread_ptr_->JoinWithStop(); 
        LOG_INFO("Layer2WebSocketClient Exit");
        return true;
    }

    void Layer2WebSocketClient::Run(utils::Thread *thread) {
        StartClient();
        return;
    }

    void Layer2WebSocketClient::on_message(websocketpp::connection_hdl hdl, server::message_ptr msg) {
        protocol::WsMessage message;
        if (!message.ParseFromString(msg->get_payload())){
            LOG_ERROR("Layer2WebSocketClient on_message ParseFromString failed");
            return;
        }
        switch (message.type()){
            case protocol::CHAIN_CONTRACT_LOG:
                HandleManager::Instance().OnLayer2SubscribeResponse(message.data());
                break;
            case protocol::OVERLAY_MSGTYPE_PING:
                on_ping(hdl);
                break;
            default:
                break;
        }
        return;
    }

    void Layer2WebSocketClient::subscribe(websocketpp::connection_hdl hdl) {
        LOG_INFO("Layer2WebSocketClient on_open send tlog");
        protocol::WsMessage message_tlog;
        protocol::TlogSubscribeRequest tlog_request;
        tlog_request.set_address(Configure::Instance().layer2_configure_.cross_contract_);

        message_tlog.set_request(true);
        message_tlog.set_type(protocol::CHAIN_CONTRACT_LOG);
        message_tlog.set_data(tlog_request.SerializeAsString());
        try{
            client_.send(hdl, message_tlog.SerializeAsString(), websocketpp::frame::opcode::binary);
        }catch (websocketpp::exception const & e) {
            LOG_INFO("Layer2WebSocketClient on_open send tlog error: %s", e.what());
        }
        LOG_INFO("Layer2WebSocketClient subscribe tlog");
        return;
    }

}