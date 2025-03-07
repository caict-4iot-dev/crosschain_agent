#ifndef WEBSOCKET_CLIENT_H_
#define WEBSOCKET_CLIENT_H_
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/config/asio_no_tls_client.hpp>
#include <websocketpp/config/asio.hpp>
#include <websocketpp/config/asio_client.hpp>
#include <websocketpp/client.hpp>
#include <websocketpp/server.hpp>
#include <utils/singleton.h>
#include <main/configure.h>

namespace agent {
    typedef websocketpp::client<websocketpp::config::asio_client> client;
    typedef websocketpp::server<websocketpp::config::asio> server;
    class WebSocketClient
	{
	public:
		WebSocketClient(){};
		~WebSocketClient(){};
    public:
        void StartClient();
    protected:
        void init(std::string url);
        void run();
        void connect();
        void start_ping(websocketpp::connection_hdl hdl);
        void reconnect();
        void on_open(websocketpp::connection_hdl hdl);
        void on_close(websocketpp::connection_hdl hdl);
        
        void on_fail(websocketpp::connection_hdl hdl);
        void on_pong(websocketpp::connection_hdl hdl);
        void on_ping(websocketpp::connection_hdl hdl);
        void on_pong_timeout(websocketpp::connection_hdl hdl);
        void close();

    public:
        virtual void subscribe(websocketpp::connection_hdl hdl){};
        virtual void on_message(websocketpp::connection_hdl hdl, server::message_ptr msg) {};
    public:
        bool IsConnected() { return connected_; };

    protected:
        client client_;
        std::atomic<bool> connected_;
        std::atomic<bool> ping_in_flight_;
        std::string url_;
        int64_t last_receive_time_;
    };
}
#endif // WEBSOCKET_CLIENT_H_