#include "websocket_client.h"
#include "utils/logger.h"
#include "utils/timestamp.h"
#include <proto/cpp/overlay.pb.h>
#include <proto/cpp/subscribe.pb.h>
#include <proto/cpp/common.pb.h>
namespace agent{
    void WebSocketClient::init(std::string url){
        connected_ = false;
        url_ = url;
        client_.set_access_channels(websocketpp::log::alevel::none);
        client_.clear_access_channels(websocketpp::log::alevel::none);

        client_.init_asio();

        client_.set_open_handler(std::bind(&WebSocketClient::on_open, this, std::placeholders::_1));
        client_.set_close_handler(std::bind(&WebSocketClient::on_close, this, std::placeholders::_1));
        client_.set_fail_handler(std::bind(&WebSocketClient::on_fail, this, std::placeholders::_1));
        client_.set_message_handler(std::bind(&WebSocketClient::on_message, this, std::placeholders::_1, std::placeholders::_2));
        client_.set_pong_handler(std::bind(&WebSocketClient::on_pong, this, std::placeholders::_1));
        client_.set_pong_timeout_handler(std::bind(&WebSocketClient::on_pong_timeout, this, std::placeholders::_1));

    }

    void WebSocketClient::on_open(websocketpp::connection_hdl hdl){
        LOG_INFO("WebSocketClient on_open subscribe tlog");
        connected_ = true;
        subscribe(hdl);
        return;
    }

    void WebSocketClient::on_close(websocketpp::connection_hdl hdl){
        connected_ = false;
        LOG_INFO("WebSocketClient on_close");
        reconnect();
        return;
    }

    void WebSocketClient::on_fail(websocketpp::connection_hdl hdl){
        connected_ = false;
        LOG_INFO("WebSocketClient connection failed");
        reconnect();
        return;
    }

    void WebSocketClient::connect(){
        try{
            websocketpp::lib::error_code ec;
            client::connection_ptr con = client_.get_connection(url_, ec);
            LOG_INFO("WebSocketClient connect: %s", url_.c_str());
            if (ec) {
                LOG_INFO("Could not create connection because: %s",ec.message().c_str());
                return;
            }
            client_.connect(con);
        }catch (websocketpp::exception const & e){
            LOG_INFO("WebSocketClient connect failed: %s", e.what());
        }
        return;
    }

    void WebSocketClient::run(){
        try{
            client_.run();   
        }catch (websocketpp::exception const & e){
            LOG_INFO("WebSocketClient run failed: %s", e.what());
        }
        return;
    }

    void WebSocketClient::on_ping(websocketpp::connection_hdl hdl){
        protocol::WsMessage message;
		message.set_type(protocol::OVERLAY_MSGTYPE_PING);
		message.set_request(false);
		message.set_sequence(0);
		message.set_data("");
        try{
            client_.send(hdl, message.SerializeAsString(), websocketpp::frame::opcode::text);
        }catch (websocketpp::exception const & e){
            LOG_INFO("WebSocketClient on_ping failed: %s", e.what());
        }

        //ping_in_flight_ = false;
        last_receive_time_ = utils::Timestamp::HighResolution();
        return;
    }

    void WebSocketClient::on_pong(websocketpp::connection_hdl hdl){
        LOG_INFO("WebsocketClient on_pong");
        ping_in_flight_ = false;
        last_receive_time_ = utils::Timestamp::HighResolution();
        return;
    }

    void WebSocketClient::start_ping(websocketpp::connection_hdl hdl){
        std::thread([this, hdl]() {
            while (connected_) {
                if (!ping_in_flight_) {
                    ping_in_flight_ = true;
                    client_.ping(hdl, "");
                }
                std::this_thread::sleep_for(std::chrono::seconds(10));
            }
        }).detach();
        return;
    }

    void WebSocketClient::on_pong_timeout(websocketpp::connection_hdl hdl){
        LOG_INFO("WebsocketClient on_pong_timeout");
        connected_ = false;
        ping_in_flight_ = false;
        reconnect();
        return;
    }

    void WebSocketClient::reconnect(){
        std::this_thread::sleep_for(std::chrono::seconds(5));
        LOG_INFO("Reconnecting...");
        connect();
        run();
    }


    void WebSocketClient::StartClient(){
        connect();
        run();
    }

    void WebSocketClient::close(){
        client_.stop();
        return;
    }

}