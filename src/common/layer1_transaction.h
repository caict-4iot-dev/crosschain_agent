#ifndef LAYER1_TRANSACTION_H_
#define LAYER1_TRANSACTION_H_
#include "proto/cpp/transaction.pb.h"
namespace agent{
    class Layer1Transaction{
    public:
        Layer1Transaction();
        ~Layer1Transaction();
    public:
        bool AddTransactionOperation(const std::string& input,int64_t amount=0);
        bool SetRandomNonce(int64_t nonce = 0);
        
        std::string GetTransactionBlob(int64_t max_seq);
        std::string GetTransactionHash();

    private:
        protocol::Transaction pb_transaction_;
    };

    typedef std::shared_ptr<Layer1Transaction> Layer1TransactionPtr;
}
#endif