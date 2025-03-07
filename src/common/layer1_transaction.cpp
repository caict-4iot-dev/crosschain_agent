#include "layer1_transaction.h"
#include <main/configure.h>
#include <utils/basen.h>
#include <chrono>
#include <common/private_key.h>


namespace agent {
    Layer1Transaction::Layer1Transaction(){
        pb_transaction_.set_source_address(Configure::Instance().layer1_configure_.agent_address_);
        pb_transaction_.set_fee_limit(Configure::Instance().layer1_configure_.fee_limit_);
        pb_transaction_.set_gas_price(Configure::Instance().layer1_configure_.gas_price_);
        pb_transaction_.set_chain_id(0);
        pb_transaction_.set_nonce_type(protocol::Transaction_TxType_RANDOM_NONCE);
    }

    Layer1Transaction::~Layer1Transaction(){

    }

    bool Layer1Transaction::AddTransactionOperation(const std::string& input,int64_t amount){
        protocol::Operation* operation = pb_transaction_.add_operations();
        operation->set_type(protocol::Operation_Type_PAY_COIN);
        protocol::OperationPayCoin* paycoin = operation->mutable_pay_coin();
        paycoin->set_dest_address(Configure::Instance().layer1_configure_.cross_contract_);
        paycoin->set_input(input);
        paycoin->set_amount(amount);
        LOG_DEBUG("AddTransactionOperation: operation size is %d,input: %s",pb_transaction_.operations_size(), input.c_str());
        return true;
    }

    std::string Layer1Transaction::GetTransactionBlob(int64_t max_seq){
        if(pb_transaction_.nonce() == 0){
            SetRandomNonce();
        }
        pb_transaction_.set_max_ledger_seq(max_seq);
        std::string SerializeString;
        LOG_DEBUG("Debug:transaction is %s:",pb_transaction_.DebugString().c_str());
        pb_transaction_.SerializeToString(&SerializeString);
        auto blob = utils::encode_b16(SerializeString);
        return blob;
    }

    bool Layer1Transaction::SetRandomNonce(int64_t nonce){
        if (nonce != 0){
             pb_transaction_.set_nonce(nonce);
        }else{
            auto now = std::chrono::system_clock::now();
            auto duration = now.time_since_epoch();
            auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
            pb_transaction_.set_nonce(millis);
        }
        
        return true;
    }

    std::string Layer1Transaction::GetTransactionHash(){
        auto SerializeString = pb_transaction_.SerializeAsString();
        auto hash = utils::encode_b16(HashWrapper::Crypto(SerializeString));
        return hash;
    }
}