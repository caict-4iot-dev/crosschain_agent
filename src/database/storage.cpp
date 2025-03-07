#include <utils/strings.h>
#include <utils/logger.h>
#include <utils/file.h>
#include "storage.h"
#include "rocks_db.h"
#include <proto/cpp/common.pb.h>
#include "consistent_writebatch.h"
#include "database_proxy.h"
namespace agent {
	Storage::Storage() {
        batch_ = std::make_shared<DomianBatchVector>();
    }

	Storage::~Storage() {
		CloseDb();
	}

	bool Storage::CreateDb() {
		do {
			std::string db_path = "data";
			db_path = utils::String::Format("%s/%s", utils::File::GetBinHome().c_str(), db_path.c_str());
            db_path = db_path + "/" + "ledger.db";
            auto db = NewKeyValueDb();
            if(!db->Open(db_path, -1)){
                LOG_ERROR("Failed to open db path(%s), the reason is(%s)\n",
                              db_path.c_str(), db->error_desc().c_str());
                break;
            }
            db_ = db;
            return true;
		} while (false);
		return false;
	}

	void Storage::CloseDb() {
        db_ = nullptr;
        batch_ = nullptr;
    }

	std::shared_ptr<KeyValueDb> Storage::NewKeyValueDb() {
		std::shared_ptr<KeyValueDb> db = nullptr;
		db = std::make_shared<RocksDbDriver>();
		return db;
	}

	std::shared_ptr<agent::BatchIndex> Storage::NewWriteBatch() {
		std::shared_ptr<agent::BatchIndex> batch_index_ptr = std::make_shared<agent::BatchIndex>();
		if (batch_ != nullptr) {
			auto index = batch_->ThreadSafeGetIndex();
			batch_index_ptr->batch_index_ = index;
			std::shared_ptr<ConsistentWriteBatch> batch_ptr = std::make_shared<ConsistentWriteBatch>();
			batch_->ThreadSafeAdd(index, batch_ptr);
		} else {
			batch_index_ptr->batch_index_ = 0;
			batch_ = std::make_shared<DomianBatchVector>();
			std::shared_ptr<ConsistentWriteBatch> batch_ptr = std::make_shared<ConsistentWriteBatch>();
			batch_->ThreadSafeAdd(0, batch_ptr);
		}

		LOG_INFO("Storage::NewWriteBatch batch index(%d).", batch_index_ptr->batch_index_);
		return batch_index_ptr;
	}

	void Storage::ReleaseBatch(uint32_t batch_index) {
		batch_->ThreadSafeAdd(batch_index, nullptr);
		LOG_DEBUG("Storage::ReleaseBatch batch index(%d)", batch_index);
	}

	bool Storage::BatchPut(uint32_t batch_index, std::string key, const std::string& value) {
		auto batch_ptr = batch_->ThreadSafeGetByIndex(batch_index);
		if (batch_ptr == nullptr) {
			LOG_ERROR("Storage::BatchPut batch index(%d) batch is nullptr.",batch_index);
			return false;
		}	
		return batch_ptr->Put(key, value);
	}

	bool Storage::BatchDelete(uint32_t batch_index, std::string key) {
		auto batch_ptr = batch_->ThreadSafeGetByIndex(batch_index);
		if (batch_ptr == nullptr) {
			LOG_ERROR("Storage::BatchDelete batch index(%d) batch is nullptr.", batch_index);
			return false;
		}
        return batch_ptr->Delete(key);	
	}

	bool Storage::BatchClear(uint32_t batch_index) {
		auto batch_ptr = batch_->ThreadSafeGetByIndex(batch_index);
		if (batch_ptr == nullptr) {
			return true;
		}	
		return batch_ptr->Clear();
	}

	bool Storage::WriteBatch(uint32_t batch_index) {
		if (batch_ == nullptr) {
			LOG_ERROR("Storage::WriteBatch batch_ is nullptr.");
			return false;
		}
        auto batch_ptr = batch_->ThreadSafeGetByIndex(batch_index);
		if (batch_ptr == nullptr) {
				LOG_ERROR("Storage::WriteBatch batch is nullptr.");
				return false;
		}
		auto ret = db_->WriteBatch(batch_ptr->GetBatch());
		if(!ret){
			LOG_ERROR("Storage::WriteBatch write failed, error(%s).", db_->error_desc().c_str());
			return false;
		}
		batch_ptr->Clear();
		LOG_INFO("Storage::WriteBatch, batch index(%d).",batch_index);
        return true; 
	}

	int32_t Storage::DBGet(std::string key, std::string& value) {        		
		return db_->Get(key, value);
	}

	int32_t Storage::DBPut(std::string key, const std::string& value) {
		auto stat = db_->Put(key, value);
		if (!stat) {
			return -1;
		}
		return 1;
	}
	
	int32_t Storage::DBDelete(std::string key) {		
		auto stat = db_->Delete(key);
		if (!stat) {
			return -1;
		}
		return 1;
	}

    bool  Storage::GetAllValueByPrefix(std::string prefix, std::set<std::string>& values){
        if (false == db_->GetAllByIterator(prefix, values)) {
			LOG_ERROR("GetAllValueByPrefix failed, prefix is (%s), error(%s).",prefix.c_str(), db_->error_desc().c_str());
			return false;
		}
        return true;
    }
}
