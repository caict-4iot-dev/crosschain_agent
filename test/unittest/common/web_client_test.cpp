#include "gtest/gtest.h"
#include <utils/headers.h>
#include <json/json.h>
#include <common/web_client.h>
#include <utils/file.h>
#include <proto/cpp/overlay.pb.h>
#include <main/configure.h>
#include <common/general.h>
using namespace agent;
class WebClientTest:public testing::Test{
public:
	WebClientTest(){}
protected:

	// Sets up the test fixture.
	virtual void SetUp() {
        agent::AddressPrefix::InitInstance();
		agent::AddressPrefix::Instance().Initialize();
        //init config
        agent::Configure::InitInstance();
        agent::Configure &config = agent::Configure::Instance();
        std::string config_path = "config/agent.json";
        config.Load(utils::File::ConvertToAbsolutePath(config_path));
	}

	// Tears down the test fixture.
	virtual void TearDown() {
        agent::Configure::ExitInstance();
        agent::AddressPrefix::ExitInstance();
	}
protected:
	void UT_getLayer1Seq(); 
    void UT_getLayer2Seq();
    void UT_getBalance();
};

TEST_F(WebClientTest, UT_getLayer1Seq) {
    UT_getLayer1Seq();
}

TEST_F(WebClientTest, UT_getLayer2Seq) {
    UT_getLayer2Seq();
}

TEST_F(WebClientTest, UT_getBalance){
    UT_getBalance();
}

void WebClientTest::UT_getLayer1Seq() {
    std::shared_ptr<HttpClient> httpClient = std::make_shared<HttpClient>(Configure::Instance().layer1_configure_.http_endpoint_);
    int64_t seq = 0;
    bool ret = httpClient->GetLayerSeq(seq);
    ASSERT_EQ(ret,true);
    ASSERT_GT(seq,0);
}

void WebClientTest::UT_getLayer2Seq() {
    std::shared_ptr<HttpClient> httpClient = std::make_shared<HttpClient>(Configure::Instance().layer2_configure_.http_endpoint_);
    int64_t seq = 0;
    bool ret = httpClient->GetLayerSeq(seq);
    ASSERT_EQ(ret,true);
    ASSERT_GT(seq,0); 
}

void WebClientTest::UT_getBalance() {
    std::shared_ptr<HttpClient> httpClient = std::make_shared<HttpClient>(Configure::Instance().layer1_configure_.http_endpoint_);
    std::string address = Configure::Instance().layer1_configure_.agent_address_;
    int64_t balance = 0;
    int64_t min_balance = 100 * Configure::Instance().layer1_configure_.fee_limit_ * Configure::Instance().layer1_configure_.gas_price_;
    bool ret = httpClient->GetBalance(balance, address);
    ASSERT_EQ(ret,true);
    ASSERT_GT(balance,min_balance);
}