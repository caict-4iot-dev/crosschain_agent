#ifndef LAYER2_WEBSOCKET_CLIENT_H_
#define LAYER2_WEBSOCKET_CLIENT_H_
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/config/asio_no_tls_client.hpp>
#include <websocketpp/config/asio.hpp>
#include <websocketpp/config/asio_client.hpp>
#include <websocketpp/client.hpp>
#include <websocketpp/server.hpp>
#include <main/configure.h>
#include "websocket_client.h"
#include <utils/thread.h>

namespace agent {
    typedef websocketpp::client<websocketpp::config::asio_client> client;
    typedef websocketpp::server<websocketpp::config::asio> server;
    class Layer2WebSocketClient : public WebSocketClient, 
        public utils::Runnable{
	public:
		Layer2WebSocketClient(){thread_ptr_ = nullptr;};
		~Layer2WebSocketClient();
		bool Initialize();
		bool Exit();
    public:
        void subscribe(websocketpp::connection_hdl hdl) override ;
        void on_message(websocketpp::connection_hdl hdl, server::message_ptr msg) override ;
    protected:
        virtual void Run(utils::Thread *thread) override;
    private:
        utils::Thread *thread_ptr_;
    };
}
#endif // WEBSOCKET_CLIENT_H_