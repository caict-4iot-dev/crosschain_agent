#ifndef STORAGE_LOCK_H_
#define STORAGE_LOCK_H_

#include <unordered_map>
#include "consistent_writebatch.h"
#include <memory>
#include <utils/thread.h>

namespace agent {
    class BatchIndex {
	public:
		BatchIndex();
		~BatchIndex();
    public:
        uint32_t batch_index_;
    };

	class DomianBatchVector {
    public:
		DomianBatchVector();
		~DomianBatchVector();
    public:
        void ThreadSafeAdd(int32_t index, std::shared_ptr<ConsistentWriteBatch> batch);
        int32_t ThreadSafeGetIndex();
        std::shared_ptr<ConsistentWriteBatch> ThreadSafeGetByIndex(uint32_t index);
    private:        
        utils::Mutex domain_batch_lock_;
        std::vector<std::shared_ptr<ConsistentWriteBatch>> domain_batch_record_;
    };

} // namespace agent

#endif
