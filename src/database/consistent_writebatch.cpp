#include "database_proxy.h"
namespace agent {
    ConsistentWriteBatch::ConsistentWriteBatch(){
        batch_.Clear();
    }

    ConsistentWriteBatch::~ConsistentWriteBatch(){
        batch_.Clear();
    }

    bool ConsistentWriteBatch::Put(std::string key, const std::string& value) {
        batch_.Put(key, value);
        return true;
    }

    bool ConsistentWriteBatch::Delete(std::string key){
        batch_.Delete(key);
        return true;
    }

    bool ConsistentWriteBatch::Clear() {
       batch_.Clear();
        return true;
    }

    rocksdb::WriteBatch& ConsistentWriteBatch::GetBatch(){
        return batch_;
    }
}