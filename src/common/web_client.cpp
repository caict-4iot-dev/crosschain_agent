#include "web_client.h"
#include "utils/logger.h"
#include <main/configure.h>
#include <utils/utils.h>

namespace agent {
    HttpClient::HttpClient(std::string url) {
        
        client = std::make_shared<httplib::Client>(url);
    }

    HttpClient::~HttpClient() {
        
    }

    void HttpClient::makePostRequest(const std::string& request, const std::string& body, httplib::Result &result) {
        LOG_DEBUG("make post request: %s, body: %s",request.c_str(),body.c_str());
        result = client->Post(request, body, "application/json");
        return;
    }

    void HttpClient::makeGetRequest(const std::string& request,httplib::Result &result) {
        LOG_DEBUG("make get request: %s",request.c_str());
        if(! client->is_valid()){
            LOG_ERROR("client is invalid");
            return;
        }
        result = client->Get(request);
        return ;
    }

    bool HttpClient::GetLayerSeq(int64_t& seq){
        auto request = "/getLedger";
        httplib::Result resp;
        makeGetRequest(request, resp);
        if(resp.error() != httplib::Error::Success){
            LOG_ERROR("get Layer1 seq failed, error: %d",resp.error());
            return false;
        }
        LOG_DEBUG("get seq resp: %s",resp.value().body.c_str());
        Json::Value res;
        if(!res.fromString(resp.value().body)){
            LOG_ERROR("get seq resp parse failed");
            return false;
        }
        int64_t error_code = res["error_code"].asInt64();
        if(error_code != 0){
            LOG_ERROR("get ledger seq failed, error_code: %d",error_code);
            return false;
        }
        if (res["result"].isMember("header")){
            const Json::Value &header = res["result"]["header"];
            if (header.isMember("seq")){
                seq = header["seq"].asInt64();
                return true;
            }else{
                LOG_ERROR("Invalid response: missing 'seq' field");
                return false;
            }
        }else{
            LOG_ERROR("Invalid response: missing 'header' field");
            return false;
        }
        
        return false;
    }

    bool HttpClient::SubmitLayer1Transaction(std::string &requestBody,httplib::Result &result){
        auto request = "/submitTransaction";
        makePostRequest(request, requestBody, result);
        return true;
    }

    bool HttpClient::SubmitLayer2Transaction(std::string &requestBody,httplib::Result &result){
        auto request = "/submitTransaction";
        makePostRequest(request, requestBody, result);
        return true;
    }

    bool HttpClient::GetLayer1CrossTxInfo(const std::string cross_tx_id,std::string &crossTxInfo1, std::string &crossTxInfo2){      
        if(!getCrossTxInfo1(cross_tx_id,crossTxInfo1,true)){
            LOG_ERROR("get layer1 cross tx info1 failed");
            return false;
        }
        if(!getCrossTxInfo2(cross_tx_id,crossTxInfo2,true)){
            LOG_ERROR("get layer1 cross tx info2 failed");
            return false;
        }
        return true;
    }

    bool HttpClient::GetLayer2CrossTxInfo(const std::string cross_tx_id,std::string &crossTxInfo1, std::string &crossTxInfo2){      
        if(!getCrossTxInfo1(cross_tx_id,crossTxInfo1,false)){
            LOG_ERROR("get layer2 cross tx info1 failed");
            return false;
        }
        if(!getCrossTxInfo2(cross_tx_id,crossTxInfo2,false)){
            LOG_ERROR("get layer2 cross tx info2 failed");
            return false;
        }
        return true;
    }

    bool HttpClient::getCrossTxInfo1(const std::string cross_tx_id,std::string &data, bool is_layer1){
        std::string contract_addresss;
        if(is_layer1){
            contract_addresss = Configure::Instance().layer1_configure_.cross_contract_;
        }else{
            contract_addresss = Configure::Instance().layer2_configure_.cross_contract_;
        }
        // 1. 构建请求体
        Json::Value input;
        input["function"] = "GetCrossTxInfo1(bytes)";
        input["return"] = "returns(uint32,uint32,bytes,uint256,address,address)";
        input["args"] = "'" + cross_tx_id + "'";

        Json::Value requestBody;
        requestBody["contract_address"] = contract_addresss;
        requestBody["code"] = "";
        requestBody["opt_type"] = 2;
        requestBody["source_address"] = "";
        requestBody["input"] = input.toStyledString();

        std::string requestBodyString = requestBody.toStyledString();
        LOG_DEBUG("Debug:GetCrossTxInfo requestBodyString: %s",requestBodyString.c_str());
        // 2. 发送请求
        httplib::Result result;
        auto request = "/callContract";
        makePostRequest(request, requestBodyString, result);

        if (!result || result.error() != httplib::Error::Success) {
            LOG_ERROR("Failed to get agent info, HTTP error: %d", result.error());
            return false;
        }

        // 3. 解析响应
        Json::Value res;
        if(!res.fromString(result.value().body)){
            LOG_ERROR("GetLayer1CrossTxInfo resp parse failed");
            return false;
        }

        // 4. 检查响应中的错误字段
        const Json::Value &query_result = res["result"];
        if (query_result.isMember("query_rets") && query_result["query_rets"].isArray()){
            const Json::Value &query_rets = query_result["query_rets"][int(0)];
           // 检查是否有错误数据
            if (query_rets["error"].isMember("data") && query_rets["error"]["data"].isNull()){
                // 5. 提取所需数据
                data = query_rets["result"]["data"].asString();
                LOG_INFO("Debug:GetLayer1CrossTxInfo crossTxid (%s), crossTxInfo: %s",cross_tx_id.c_str(), data.c_str());
                return true;
            }else{
                LOG_ERROR("Invalid response: missing 'data' field in 'error'");
                return false;
            }
        }else{
            LOG_ERROR("Invalid response: missing 'query_rets' field");
            return false;
        }
        return false;   
    }

    bool HttpClient::getCrossTxInfo2(const std::string cross_tx_id,std::string &data, bool is_layer1){
        std::string contract_addresss;
        if(is_layer1){
            contract_addresss = Configure::Instance().layer1_configure_.cross_contract_;
        }else{
            contract_addresss = Configure::Instance().layer2_configure_.cross_contract_;
        }
        // 1. 构建请求体
        Json::Value input;
        input["function"] = "GetCrossTxInfo2(bytes)";
        input["return"] = "returns(uint8,uint8,uint256,uint256,uint256,string)";
        input["args"] = "'" + cross_tx_id + "'";

        Json::Value requestBody;
        requestBody["contract_address"] = contract_addresss;
        requestBody["code"] = "";
        requestBody["opt_type"] = 2;
        requestBody["source_address"] = "";
        requestBody["input"] = input.toStyledString();

        std::string requestBodyString = requestBody.toStyledString();
        LOG_DEBUG("Debug:GetCrossTxInfo requestBodyString: %s",requestBodyString.c_str());
        // 2. 发送请求
        httplib::Result result;
        auto request = "/callContract";
        makePostRequest(request, requestBodyString, result);

        if (!result || result.error() != httplib::Error::Success) {
            LOG_ERROR("Failed to get agent info, HTTP error: %d", result.error());
            return false;
        }

        // 3. 解析响应
        Json::Value res;
        if(!res.fromString(result.value().body)){
            LOG_ERROR("GetLayer1CrossTxInfo resp parse failed");
            return false;
        }

        // 4. 检查响应中的错误字段
        const Json::Value &query_result = res["result"];
        if (query_result.isMember("query_rets") && query_result["query_rets"].isArray()){
            const Json::Value &query_rets = query_result["query_rets"][int(0)];
           // 检查是否有错误数据
            if (query_rets["error"].isMember("data") && query_rets["error"]["data"].isNull()){
                // 5. 提取所需数据
                data = query_rets["result"]["data"].asString();
                LOG_DEBUG("Debug:GetLayer1CrossTxInfo crossTxid (%s), crossTxInfo: %s",cross_tx_id.c_str(), data.c_str());
                return true;
            }else{
                LOG_ERROR("Invalid response: missing 'data' field in 'error'");
                return false;
            }
        }else{
            LOG_ERROR("Invalid response: missing 'query_rets' field");
            return false;
        }
        return false;   
    }

    bool HttpClient::GetTxResult(const std::string tx_hash){
        auto request = utils::String::Format("/getTransactionHistory?hash=%s",tx_hash.c_str());
        httplib::Result resp;
        makeGetRequest(request, resp);
        if(resp.error() != httplib::Error::Success){
            LOG_ERROR("get transaction history failed, error: %d",resp.error());
            return false;
        }
        LOG_DEBUG("get transaction history resp: %s",resp.value().body.c_str());
        Json::Value res;
        if(!res.fromString(resp.value().body)){
            LOG_ERROR("get transaction history resp parse failed");
            return false;
        }
        int64_t error_code = res["error_code"].asInt64();
        if(error_code != 0){
            LOG_ERROR("get transaction history failed, error_code: %d",error_code);
            return false;
        }
        if (res["result"].isMember("transactions")){
            const Json::Value &transactions = res["result"]["transactions"];
            if (transactions.isArray() && transactions.size() > 0){
                for (const auto &tx : transactions){
                    if (tx.isMember("error_code") && tx["error_code"].asInt64() == 0){
                        return true;
                    }else{
                        LOG_INFO("Debug:GetTxResult tx_hash (%s), error_code: %d",tx_hash.c_str(), tx["error_code"].asInt64());
                        return false;
                    }
                }
            }
        }else{
            LOG_ERROR("Invalid response: missing 'transactions' field");
            return false;
        }

        return false;
    }

    bool HttpClient::GetBalance(int64_t& balance, const std::string address){
        auto request = "/getAccountBase?address=" + address;
        httplib::Result resp;
        makeGetRequest(request, resp);
        if(resp.error() != httplib::Error::Success){
            LOG_ERROR("get account failed, error: %d",resp.error());
            return false;
        }
        LOG_DEBUG("get account resp: %s",resp.value().body.c_str());
        Json::Value res;
        if(!res.fromString(resp.value().body)){
            LOG_ERROR("get layer1 seq resp parse failed");
            return false;
        }
        int64_t error_code = res["error_code"].asInt64();
        if(error_code != 0){
            LOG_ERROR("get transaction history failed, error_code: %d",error_code);
            return false;
        }
        if (res["result"].isMember("balance")){
           balance = res["result"]["balance"].asInt64();
           return true;
        }else{
            balance = 0;
            LOG_ERROR("Invalid response: missing 'balance' field");
            return false;
        }
        
        return false;
    }

    int32_t HttpClient::GetCrossTxStatus(const std::string cross_tx_id){
        std::string crossTxInfo2 = "";
        if(!getCrossTxInfo2(cross_tx_id,crossTxInfo2,false)){
            LOG_ERROR("get layer2 cross tx info2 failed");
            return false;
        }
        std::string info2 = crossTxInfo2.substr(1,crossTxInfo2.size()-2);
        std::vector<std::string> info2_list;
        std::stringstream ss2(info2);
        std::string item;
        while (std::getline(ss2, item, ',')) {
            info2_list.push_back(item);
        }
        return utils::String::Stoi(info2_list[0]);
    }
}