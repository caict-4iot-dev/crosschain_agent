#include <pistache/net.h>
#include <pistache/tcp.h>
#include "web_server.h"
#include <common/web_client.h>
#include <main/configure.h>
#include <websocket/layer1_websocket_client.h>
#include <websocket/layer2_websocket_client.h>
#include <utils/system.h>
#include <utils/logger.h>
#include <utils/timestamp.h>
namespace agent {
    WebServer::WebServer() :
		endpoint_ptr_(NULL),
		running_(NULL),
		thread_count_(0),
        port_(0)
	{
	}

	WebServer::~WebServer() {
	}

    bool WebServer::Initialize() {
        layer1_ws_client_.Initialize();
        layer2_ws_client_.Initialize();
        thread_count_ = Configure::Instance().webserver_configure_.thread_count_;
		if (thread_count_ == 0) {
			thread_count_ = utils::System::GetCpuCoreCount() * 4;
		}
        try {
            int64_t max_request_size = 100 * 1024 * 1024; //100M
            int64_t max_response_size = 100 * 1024 * 1024; //100M
            port_  = Configure::Instance().webserver_configure_.port_;
			Pistache::Address addr(Pistache::Ipv4::any(), Pistache::Port(port_));
			endpoint_ptr_ = new Pistache::Http::Endpoint(addr);

            auto opts = Pistache::Http::Endpoint::options();
            opts.threads(thread_count_);
            opts.maxRequestSize(max_request_size);
            opts.maxResponseSize(max_response_size);

            Pistache::Tcp::Options tcpopt   = Pistache::Tcp::Options::ReuseAddr | Pistache::Tcp::Options::ReusePort;
            opts.flags(tcpopt);
			endpoint_ptr_->init(opts);
		} catch (std::exception& e) {
			LOG_ERROR("Failed to initialize web server, %s", e.what());
			return false;
		}

        Pistache::Rest::Routes::NotFound(router_, Pistache::Rest::Routes::bind(&WebServer::FileNotFound, this));
        Pistache::Rest::Routes::Options(router_, "/*",Pistache::Rest::Routes::bind(&WebServer::FuncOptions, this));
        Pistache::Rest::Routes::Get(router_, "/hello", Pistache::Rest::Routes::bind(&WebServer::Hello, this));
        Pistache::Rest::Routes::Post(router_, "/submitTxToLayer1", Pistache::Rest::Routes::bind(&WebServer::SubmitStartTxToLayer1, this));
        Pistache::Rest::Routes::Post(router_, "/submitTxToLayer2", Pistache::Rest::Routes::bind(&WebServer::SubmitStartTxToLayer2, this));

        try{
			endpoint_ptr_->setHandler(router_.handler());
			endpoint_ptr_->serveThreaded();
		} catch (std::exception& e) {
			LOG_ERROR("Failed to initialize web server, %s", e.what());
			return false;
		}
		running_ = true;
        LOG_INFO("Webserver started, thread count(" FMT_SIZE ") listen port at %d", thread_count_, port_);
		return true;
    }

    bool WebServer::Exit(){
        LOG_INFO("WebServer stoping...");
        layer1_ws_client_.Exit();
        layer2_ws_client_.Exit();
        if (endpoint_ptr_) {
			endpoint_ptr_->shutdown();
			delete endpoint_ptr_;
			endpoint_ptr_ = NULL;
		}

		LOG_INFO("WebServer stop [OK]");
		return true;
    }

    void WebServer::FileNotFound(const Pistache::Rest::Request &request, Pistache::Http::ResponseWriter response) {
        ResponseInit(response);
		sendResponse(response, Pistache::Http::Code::Not_Found, "File not found");
	}
    
    void WebServer::FuncOptions(const Pistache::Rest::Request &request, Pistache::Http::ResponseWriter response){
        ResponseInit(response);
        response.send(Pistache::Http::Code::Ok);
    }

    void WebServer::Hello(const Pistache::Rest::Request &request, Pistache::Http::ResponseWriter response) {
        ResponseInit(response);

		Json::Value reply_json = Json::Value(Json::objectValue);
		reply_json["current_time"] = utils::Timestamp::HighResolution();
		reply_json["layer2_id_"] =  Configure::Instance().layer2_configure_.layer2_id_;
		sendResponse(response, Pistache::Http::Code::Ok, reply_json.toFastString());
	}

    void WebServer::ResponseInit(Pistache::Http::ResponseWriter &response){
        WebServerConfigure &web_config = Configure::Instance().webserver_configure_;
		response.headers().add<Pistache::Http::Header::AccessControlAllowOrigin>("*");
        response.headers().add<Pistache::Http::Header::AccessControlAllowHeaders>("Content-Type,X-Requested-With");
        response.headers().add<Pistache::Http::Header::AccessControlAllowMethods>("GET, POST, DELETE,OPTIONS,PUT");
    }

    void WebServer::sendResponse(Pistache::Http::ResponseWriter &response, const Pistache::Http::Code code, const std::string &reply){
		try{
			response.send(code, reply,MIME(Application, Json));
		}catch (const std::exception& e) {
			response.send(Pistache::Http::Code::Internal_Server_Error, e.what());
		}
	}

    void WebServer::SubmitStartTxToLayer1(const Pistache::Rest::Request &request, Pistache::Http::ResponseWriter response){
        ResponseInit(response);
        if(layer1_ws_client_.IsConnected() && layer2_ws_client_.IsConnected() ){
            httplib::Result resp;
            std::string body = request.body();
            std::shared_ptr<HttpClient> httpClient = std::make_shared<HttpClient>(Configure::Instance().layer1_configure_.http_endpoint_);
            httpClient->SubmitLayer1Transaction(body,resp);
            sendResponse(response, Pistache::Http::Code::Ok, resp.value().body);
            LOG_DEBUG("SubmitStartTxToLayer1 response is %s",resp.value().body.c_str());
        }else{
            sendResponse(response, Pistache::Http::Code::Service_Unavailable, "Service Unavailable");
        }
        
    }

    void WebServer::SubmitStartTxToLayer2(const Pistache::Rest::Request &request, Pistache::Http::ResponseWriter response){
        ResponseInit(response);
        if(layer1_ws_client_.IsConnected() && layer2_ws_client_.IsConnected()){
            std::shared_ptr<HttpClient> httpClient = std::make_shared<HttpClient>(Configure::Instance().layer2_configure_.http_endpoint_);
            httplib::Result resp;
            std::string body = request.body();
            httpClient->SubmitLayer2Transaction(body,resp);
            sendResponse(response, Pistache::Http::Code::Ok, resp.value().body);
            LOG_DEBUG("SubmitStartTxToLayer2 response is %s",resp.value().body.c_str());
        }else{
            sendResponse(response, Pistache::Http::Code::Service_Unavailable, "Service Unavailable");
        }
    }
}