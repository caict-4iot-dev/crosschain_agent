#include "database_proxy.h"
#include "database_manager.h"
namespace agent {
    DatabaseProxy::DatabaseProxy() {    
        database_manager_ = std::make_shared<DatabaseManager>();
    }

    DatabaseProxy::~DatabaseProxy() {
    }

    bool DatabaseProxy::Initialize() {
        //init database manager
        if (!database_manager_->Initialize()) {
            LOG_ERROR("DatabaseProxy::initProxy database manager init failed.");
            return false;
        }
        return true;
    }

    bool DatabaseProxy::Exit() {

		LOG_INFO("Database proxy stoped. [OK]");
		return true;
    }
    
    //batch
    std::shared_ptr<agent::BatchIndex> DatabaseProxy::NewWriteBatch() {
        return database_manager_->NewWriteBatch();
    }

    void DatabaseProxy::ReleaseBatch(int64_t batch_index) {
        database_manager_->ReleaseBatch(batch_index);
    }

    bool DatabaseProxy::BatchPut(std::shared_ptr<agent::BatchIndex> batch_index, std::string key, const std::string& value) {
        return database_manager_->BatchPut(batch_index->batch_index_, key, value);
    }
    
    bool DatabaseProxy::BatchDelete(std::shared_ptr<agent::BatchIndex> batch_index, std::string key){
        return database_manager_->BatchDelete(batch_index->batch_index_, key);
    }

    bool DatabaseProxy::BatchClear(std::shared_ptr<agent::BatchIndex> batch_index) {
        return database_manager_->BatchClear(batch_index->batch_index_);
    }

    bool DatabaseProxy::WriteBatch(std::shared_ptr<agent::BatchIndex> batch_index){
        return database_manager_->WriteBatch(batch_index->batch_index_);
    }

    //db read
    bool DatabaseProxy::DBGet(std::string key, std::string& value) {
        return database_manager_->DBGet(key, value);
    }

    bool DatabaseProxy::DBPut(std::string key, std::string& value){
        return database_manager_->DBPut(key,value);
    }

    bool DatabaseProxy::DBDelete(std::string key){
        return database_manager_->DBDelete(key);
    }

    bool DatabaseProxy::GetAllValueByPrefix(std::string prefix, std::set<std::string>& values){
        return database_manager_->GetAllValueByPrefix(prefix, values);
    }

}