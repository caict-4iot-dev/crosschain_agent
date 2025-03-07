#include "storage_lock.h"
#include "database_proxy.h"

#define MAX_BATCH_COUNT 10000
namespace agent {
	BatchIndex::BatchIndex() {
		batch_index_ = -1;
	}

	BatchIndex::~BatchIndex() {
		DatabaseProxy::Instance().ReleaseBatch(batch_index_);
	}

	DomianBatchVector::DomianBatchVector() : domain_batch_record_(100) {}

	DomianBatchVector::~DomianBatchVector() {}

	void DomianBatchVector::ThreadSafeAdd(int32_t index, std::shared_ptr<ConsistentWriteBatch> batch) {
		utils::MutexGuard guard(domain_batch_lock_);
		domain_batch_record_[index] = batch;
	}

	int32_t DomianBatchVector::ThreadSafeGetIndex() {
		utils::MutexGuard guard(domain_batch_lock_);
		uint32_t batch_size = domain_batch_record_.size();
		uint32_t index = -1;
		for (int32_t i = 0; i < batch_size; i++) {
			if (domain_batch_record_[i] == nullptr) {
				index = i;
				break;
			}
		}

		if (index == -1) {
			index = batch_size;
		} 

		if (index == MAX_BATCH_COUNT) {
			PROCESS_EXIT("More than 100 active batches.");
		}
		return index;
	}

	std::shared_ptr<ConsistentWriteBatch> DomianBatchVector::ThreadSafeGetByIndex(uint32_t index) {
		utils::MutexGuard guard(domain_batch_lock_);
		if (index >= MAX_BATCH_COUNT) {
			return nullptr;
		}
		return domain_batch_record_[index];
	}
}