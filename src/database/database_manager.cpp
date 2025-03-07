#include <utils/logger.h>
#include "database_manager.h"
#include "rocks_db.h"
namespace agent {

	DatabaseManager::DatabaseManager() {
	}

	DatabaseManager::~DatabaseManager() {
		
	}

	bool DatabaseManager::Initialize() {	

        storage_ = std::make_shared<Storage>();
        return storage_->CreateDb();
	}

	std::shared_ptr<agent::BatchIndex> DatabaseManager::NewWriteBatch() {
		return storage_->NewWriteBatch();
	}

	void DatabaseManager::ReleaseBatch(uint32_t batch_index) {
		storage_->ReleaseBatch(batch_index);
	}

	bool DatabaseManager::BatchPut(uint32_t batch_index, std::string key, const std::string& value) {
		return storage_->BatchPut(batch_index, key, value);
	}

	bool DatabaseManager::BatchDelete(uint32_t batch_index, std::string key) {
		return storage_->BatchDelete(batch_index, key);
	}

	bool DatabaseManager::BatchClear(uint32_t batch_index) {
		return storage_->BatchClear(batch_index);
	}

	bool DatabaseManager::WriteBatch(uint32_t batch_index) {
		return storage_->WriteBatch(batch_index);
	}

	bool DatabaseManager::DBGet(std::string key, std::string& value) {
		auto ret =  storage_->DBGet(key, value);
        if (ret == 1){
            return true;
        }
        return false;
	}

	bool DatabaseManager::DBPut(std::string key, const std::string& value) {
		auto ret =  storage_->DBPut(key, value);
        if (ret == 1){
            return true;
        }
        return false;
	}
	
	bool DatabaseManager::DBDelete(std::string key) {
		auto ret =  storage_->DBDelete(key);
        if (ret == 1){
            return true;
        }
        return false;
	}

    bool DatabaseManager::GetAllValueByPrefix(std::string prefix, std::set<std::string>& values){
        return storage_->GetAllValueByPrefix(prefix, values);
    }
}
