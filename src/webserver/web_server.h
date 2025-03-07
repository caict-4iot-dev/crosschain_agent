#ifndef WEB_SERVER_H_
#define WEB_SERVER_H_

#include <pistache/endpoint.h>
#include <pistache/http.h>
#include <pistache/router.h>
#include <utils/singleton.h>
#include <utils/thread.h>
#include <websocket/layer1_websocket_client.h>
#include <websocket/layer2_websocket_client.h>

namespace agent {

    class WebServer : public utils::Singleton<agent::WebServer>{
        friend class utils::Singleton<agent::WebServer>;
    public:
        WebServer();
        ~WebServer();
    private:
        Pistache::Http::Endpoint *endpoint_ptr_;
        Pistache::Rest::Router router_;
        utils::Mutex running_lock_;
        bool running_;
        size_t thread_count_;
        unsigned short port_;
        Layer1WebSocketClient layer1_ws_client_;
        Layer2WebSocketClient layer2_ws_client_;

        void FileNotFound(const Pistache::Rest::Request &request, Pistache::Http::ResponseWriter response);
        void FuncOptions(const Pistache::Rest::Request &request, Pistache::Http::ResponseWriter response);
		void Hello(const Pistache::Rest::Request &request, Pistache::Http::ResponseWriter response);

        void SubmitStartTxToLayer1(const Pistache::Rest::Request &request, Pistache::Http::ResponseWriter response);
        void SubmitStartTxToLayer2(const Pistache::Rest::Request &request, Pistache::Http::ResponseWriter response);
        void sendResponse(Pistache::Http::ResponseWriter &response, const Pistache::Http::Code code, const std::string &reply);
        void ResponseInit(Pistache::Http::ResponseWriter &response);
    public:
        bool Initialize();
		bool Exit();
    };
    };  // namespace agent

#endif