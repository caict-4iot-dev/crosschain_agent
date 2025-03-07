#include <gtest/gtest.h>
#include <utils/crypto.h>
#include <utils/strings.h>
#include <common/private_key.h>
#include <main/configure.h>
#include <common/general.h>
#include <common/data_secret_key.h>

class PrivateKeyTest :public testing::Test{
protected:
	virtual void SetUp(){
		agent::AddressPrefix::InitInstance();
		agent::AddressPrefix::Instance().Initialize();
		priv_key = new agent::PrivateKey(agent::SIGNTYPE_ED25519);
	}
	virtual void TearDown(){
		delete priv_key;
		priv_key = NULL;
		agent::AddressPrefix::ExitInstance();
	}
	agent::PrivateKey* priv_key;
public:
	void UT_encode_decode();
	void UT_sign_verify();
    void UT_load_aesPri();
};

TEST_F(PrivateKeyTest, KeyNotNULL){
	EXPECT_STRNE("", priv_key->GetEncPublicKey().c_str());
	EXPECT_STRNE("", priv_key->GetEncPrivateKey().c_str());
	EXPECT_STRNE("", priv_key->GetEncAddress().c_str());
};

TEST_F(PrivateKeyTest,PrivateKey2){
	agent::PrivateKey priv_key2(priv_key->GetEncPrivateKey());
	EXPECT_STRNE("", priv_key2.GetEncPublicKey().c_str());
	EXPECT_STRNE("", priv_key2.GetEncPrivateKey().c_str());
	EXPECT_STRNE("", priv_key2.GetEncAddress().c_str());
};

TEST_F(PrivateKeyTest, Sign){
	EXPECT_STRNE("", priv_key->Sign("").c_str());
};

void PrivateKeyTest::UT_load_aesPri(){
    std::string aes_pri_key = "5542d66dd4cd2055fc95d64e31dae3ed344d96e046bf681cbe4efc01f5552bcf110ea9771b130bb155ac1c53782faa15cd668b1bf514f6080f60521ac1353245";
    std::string assert_pri = "priSPKn3KaXzKKnPg5DBdST9BLERM9x9XdX1effgkmDfNNW575";
    std::string pri_key = utils::Aes::HexDecrypto(aes_pri_key, agent::GetDataSecuretKey());
    ASSERT_EQ(pri_key,assert_pri);
    agent::PrivateKey priv(pri_key);
    ASSERT_EQ(priv.IsValid(), true);
}

void PrivateKeyTest::UT_encode_decode(){
    std::string public_key = priv_key->GetEncPublicKey();
    std::string private_key = priv_key->GetEncPrivateKey();
    std::string public_address = priv_key->GetEncAddress();
	std::cout << "public_key:" << public_key << std::endl;
	std::cout << "private_key:" << private_key << std::endl;
	std::cout << "public_address:" << public_address << std::endl;
         
    agent::PrivateKey priv_key1(private_key);
    std::string public_key1 = priv_key1.GetEncPublicKey();
    std::string private_key1 = priv_key1.GetEncPrivateKey();
    std::string public_address1 = priv_key1.GetEncAddress();

    ASSERT_EQ(public_key, public_key1);
    ASSERT_EQ(private_key, private_key1);
    ASSERT_EQ(public_address, public_address1);

    agent::PublicKey pubkey(public_key);
    std::string addr = pubkey.GetEncAddress();
    std::cout << "public_address:" << addr << std::endl;
    ASSERT_EQ(addr, public_address);

}

void PrivateKeyTest::UT_sign_verify(){
    std::string strpubkey = priv_key->GetEncPublicKey();
    
    for (int i = 0; i < 1; i++){
        std::string data = "hello" + std::to_string(i);
        std::string sig = priv_key->Sign(data);
        ASSERT_EQ(agent::PublicKey::Verify(data, sig, strpubkey), true);
        std::cout << data << ":" << agent::PublicKey::Verify(data, sig, strpubkey) << std::endl;
    }
}

TEST_F(PrivateKeyTest, encode_decode){
    UT_encode_decode();
}

TEST_F(PrivateKeyTest, sign_verify){
    UT_sign_verify();
}

TEST_F(PrivateKeyTest, sign_aes){
    UT_load_aesPri();
}

