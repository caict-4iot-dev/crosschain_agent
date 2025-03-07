#ifndef CONSISTENT_WRITE_BATCH_
#define CONSISTENT_WRITE_BATCH_

#include <rocksdb/db.h>
namespace agent {
    class ConsistentWriteBatch {
        public:
            ConsistentWriteBatch();
            ~ConsistentWriteBatch();
            bool Put(std::string key, const std::string& value);
            bool Delete(std::string key);
            bool Clear();
            rocksdb::WriteBatch& GetBatch();
        private:
            rocksdb::WriteBatch batch_;
    };
}
#endif