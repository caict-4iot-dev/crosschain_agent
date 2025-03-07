#include <utils/utils.h>
#include <utils/file.h>
#include <utils/strings.h>
#include <utils/logger.h>
#include "configure.h"
#include <signal.h>
#include <utils/crypto.h>
#include <common/data_secret_key.h>
#include <common/private_key.h>
namespace agent {


	Layer1Configure::Layer1Configure() {
        
    }

	Layer1Configure::~Layer1Configure() {
	}

	bool Layer1Configure::Load(const Json::Value &value) {
		Configure::GetValue(value, "http_endpoint", http_endpoint_);
        Configure::GetValue(value, "ws_endpoint", ws_endpoint_);
        Configure::GetValue(value, "cross_contract", cross_contract_);
        Configure::GetValue(value, "fee_limit", fee_limit_);
        Configure::GetValue(value, "gas_price", gas_price_);
        Configure::GetValue(value, "private_key",private_key_);
        if (private_key_.substr(0, 3) != "pri") {
			private_key_ = utils::Aes::HexDecrypto(private_key_, agent::GetDataSecuretKey());
		}
        PrivateKey priv(private_key_);
		if (!priv.IsValid()) {
			LOG_STD_ERR("Layer1Configure load private_key_ error");
			return false;
		}
        agent_address_ = priv.GetEncAddress();
        if(gas_price_ < 1){
            gas_price_ = 1;
        }
        if(fee_limit_ < 20000){
            fee_limit_ = 20000;
        }
        manager_contract_ = "did:bid:efEnXEGWYjHRw1CzK4KpWTdusnaRokk8";
        auto tlog_topic = "RegisterAgent|FreezeAgent|UnfreezeAgent|StopUsingAgent|Submit";
        topics_ = utils::String::Strtok(tlog_topic, '|');
        if(value.isMember("cross_topic")){
            const Json::Value &cross_topic = value["cross_topic"];
            Configure::GetValue(cross_topic, "start_tx", start_tx_topic_);
            Configure::GetValue(cross_topic, "send_tx", send_tx_topic_);
            Configure::GetValue(cross_topic, "send_ack", ack_tx_topic_);
        }
        if(start_tx_topic_.empty()){
            start_tx_topic_ = "2b543a7f49dd6ca6d30b8c2491cdce5a52e11bf1169943cd065a4f64b3f50e6b";
        }
        if(send_tx_topic_.empty()){
            send_tx_topic_ = "484087f6a046ffc37d32313f7ef1ad396194834491546b43547d08f91b9ed773";
        }
        if(ack_tx_topic_.empty()){
            ack_tx_topic_ = "424ebb38adf5561feffc826f8b8e6073045b5665e75cbfee188470bfb6ad6af5";
        }
        return true;
	}

    Layer2Configure::Layer2Configure() {
        
    }

    Layer2Configure::~Layer2Configure(){}

    bool Layer2Configure::Load(const Json::Value &value) {
        Configure::GetValue(value, "http_endpoint", http_endpoint_);
        Configure::GetValue(value, "ws_endpoint", ws_endpoint_);
        Configure::GetValue(value, "cross_contract", cross_contract_);
        Configure::GetValue(value, "layer2_id", layer2_id_);
        Configure::GetValue(value, "fee_limit", fee_limit_);
        Configure::GetValue(value, "gas_price", gas_price_);
        Configure::GetValue(value, "private_key",private_key_);
        if (private_key_.substr(0, 3) != "pri") {
			private_key_ = utils::Aes::HexDecrypto(private_key_, agent::GetDataSecuretKey());
		}
        PrivateKey priv(private_key_);
		if (!priv.IsValid()) {
			LOG_STD_ERR("ProtocolConfigure load private_key_ error");
			return false;
		}
        agent_address_ = priv.GetEncAddress();
        if(gas_price_ < 1){
            gas_price_ = 1;   
        }
        if(fee_limit_ < 20000){
            fee_limit_ = 20000;
        }
        if(value.isMember("cross_topic")){
            const Json::Value &cross_topic = value["cross_topic"];
            Configure::GetValue(cross_topic, "start_tx", start_tx_topic_);
            Configure::GetValue(cross_topic, "send_tx", send_tx_topic_);
            Configure::GetValue(cross_topic, "send_ack", ack_tx_topic_);
        }
        if(start_tx_topic_.empty()){
            start_tx_topic_ = "2b543a7f49dd6ca6d30b8c2491cdce5a52e11bf1169943cd065a4f64b3f50e6b";
        }
        if(send_tx_topic_.empty()){
            send_tx_topic_ = "484087f6a046ffc37d32313f7ef1ad396194834491546b43547d08f91b9ed773";
        }
        if(ack_tx_topic_.empty()){
            ack_tx_topic_ = "424ebb38adf5561feffc826f8b8e6073045b5665e75cbfee188470bfb6ad6af5";
        }
        return true;
	}

    WebServerConfigure::WebServerConfigure(){
        thread_count_ = 8;
        port_ = 8080;
    }

    WebServerConfigure::~WebServerConfigure() {
    }

    bool WebServerConfigure::Load(const Json::Value &value) {
        Configure::GetValue(value, "listen_port", port_);
        Configure::GetValue(value, "thread_count", thread_count_);
        return true;
    }


	Configure::Configure() {}

	Configure::~Configure() {}

	bool Configure::LoadFromJson(const Json::Value &values){
		if (!values.isMember("logger") ||
            !values.isMember("layer1") ||
            !values.isMember("webserver") ||
            !values.isMember("layer2") ) {
			LOG_STD_ERR("Some configuration not exist");
			return false;
		}

		do {
			if (!logger_configure_.Load(values["logger"])) break;
			if (!layer1_configure_.Load(values["layer1"])) break;
			if (!layer2_configure_.Load(values["layer2"])) break;
            if (!webserver_configure_.Load(values["webserver"])) break;
			return true;
		} while (false);
		return false;
	}
}