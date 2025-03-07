#include <gtest/gtest.h>
#include <common/layer1_transaction.h>
#include <utils/basen.h>
#include <main/configure.h>
#include <proto/cpp/overlay.pb.h>
#include <utils/file.h>
#include <common/general.h>
#include <common/data_secret_key.h>
#include <common/private_key.h>
using namespace agent;

class Layer1TransactionTest:public testing::Test{
public:
	Layer1TransactionTest(){}
protected:

	// Sets up the test fixture.
	virtual void SetUp() {
        agent::AddressPrefix::InitInstance();
		agent::AddressPrefix::Instance().Initialize();
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
    void UT_getTransactionBlob();
};



TEST_F(Layer1TransactionTest, UT_GetTransactionBlob){
    UT_getTransactionBlob();
}



void Layer1TransactionTest::UT_getTransactionBlob() {
    Layer1Transaction tran;
    int64_t nonce = 1;
    int64_t max_seq = 100;
    protocol::InputSubmitLayer2State pb_submit_param;
    pb_submit_param.set_id(Configure::Instance().layer2_configure_.layer2_id_);
    pb_submit_param.set_bid(Configure::Instance().layer2_configure_.layer2_bid_);
    auto param = pb_submit_param.SerializeAsString();
    Json::Value input;
    input["method"] = "submitlayer2state";
    input["params"] = utils::String::BinToHexString(param);
    tran.AddTransactionOperation(input.toStyledString());
    tran.SetRandomNonce(nonce);
    auto blob = tran.GetTransactionBlob(max_seq);
    protocol::Transaction pb_tx;
    std::string bin_blog;
    utils::String::HexStringToBin(blob,bin_blog);
    auto ret = pb_tx.ParseFromString(bin_blog);
    EXPECT_EQ(ret,true);
}


