#ifndef CONFIGURE_H_
#define CONFIGURE_H_

#include <common/configure_base.h>
#include <unordered_set>
#include <json/json.h>

namespace agent {


	class Layer1Configure {
	public:
		Layer1Configure();
		~Layer1Configure();
        std::vector<std::string> topics_;
        
        std::string http_endpoint_;
        std::string ws_endpoint_;
        std::string cross_contract_;
        std::string private_key_;
        std::string manager_contract_;
        std::string agent_address_;
        std::string start_tx_topic_;
        std::string send_tx_topic_;
        std::string ack_tx_topic_;
        int64_t fee_limit_;
        int64_t gas_price_;
        bool Load(const Json::Value &value);
	};

    class Layer2Configure{
        public:
            Layer2Configure();
            ~Layer2Configure();

            std::string http_endpoint_;
            std::string ws_endpoint_;
            std::string cross_contract_;
            std::string private_key_;
            std::string agent_address_;
            std::string start_tx_topic_;
            std::string send_tx_topic_;
            std::string ack_tx_topic_;
            int64_t layer2_id_;
            int64_t fee_limit_;
            int64_t gas_price_;
            bool Load(const Json::Value &value);
    };

    class WebServerConfigure {
	public:
		WebServerConfigure();
		~WebServerConfigure();

        uint32_t port_;
        uint32_t thread_count_;
		bool Load(const Json::Value &value);
	};

    class Configure : public ConfigureBase, public utils::Singleton<Configure> {
		friend class utils::Singleton<Configure>;
		Configure();
		~Configure();

	public:
		LoggerConfigure logger_configure_;
		Layer1Configure layer1_configure_;
        Layer2Configure layer2_configure_;
        WebServerConfigure webserver_configure_;
        
        virtual bool LoadFromJson(const Json::Value &values);
	};

}

#endif
