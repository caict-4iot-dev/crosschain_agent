#ifndef WEB_CLIENT_
#define WEB_CLIENT_
#include <iostream>
#include <memory>
#include <httplib.h>

namespace agent{
    class HttpClient 
    {
    public:
        HttpClient() = delete;
        HttpClient(std::string url);
        ~HttpClient();
    public:
        //layer1
        bool SubmitLayer1Transaction(std::string &requestBody,httplib::Result &result);
        bool SubmitLayer2Transaction(std::string &requestBody,httplib::Result &result);
        bool GetLayerSeq(int64_t& seq);
        bool GetLayer1CrossTxInfo(const std::string cross_tx_id,std::string &crossTxInfo1, std::string &crossTxInfo2);
        bool GetLayer2CrossTxInfo(const std::string cross_tx_id,std::string &crossTxInfo1, std::string &crossTxInfo2);
        bool GetTxResult(const std::string tx_hash);
        bool GetBalance(int64_t& balance, const std::string address);
        int32_t GetCrossTxStatus(const std::string cross_tx_id);

    private:
        bool getCrossTxInfo1(const std::string cross_tx_id,std::string &data, bool is_layer1);
        bool getCrossTxInfo2(const std::string cross_tx_id,std::string &data, bool is_layer1);
        void makePostRequest(const std::string& request, const std::string& body, httplib::Result &result);
        void makeGetRequest(const std::string& request,httplib::Result &result);
        std::shared_ptr<httplib::Client> client = nullptr;
    };
}

#endif // WEB_CLIENT_