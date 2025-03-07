#ifndef DATABASE_PROXY_H_
#define DATABASE_PROXY_H_

#include <utils/headers.h>
#include <common/configure_base.h>
#include <main/configure.h>
#include "database_manager.h"
#include <memory>
#include "storage_lock.h"

namespace agent {
	class DatabaseProxy : public utils::Singleton<agent::DatabaseProxy>{
		friend class utils::Singleton<DatabaseProxy>;
	private:
		DatabaseProxy();
		~DatabaseProxy();

        std::shared_ptr<DatabaseManager> database_manager_;

    public:
        bool Initialize();
        bool Exit(); 

	public:
        //batch
        std::shared_ptr<agent::BatchIndex> NewWriteBatch();
        void ReleaseBatch(int64_t batch_index);
        bool BatchPut(std::shared_ptr<agent::BatchIndex> batch_index, std::string key, const std::string& value);
        bool BatchDelete(std::shared_ptr<agent::BatchIndex> batch_index, std::string key);
        bool BatchClear(std::shared_ptr<agent::BatchIndex> batch_index);
        bool WriteBatch(std::shared_ptr<agent::BatchIndex> batch_index);
        //DB Read
        bool DBGet(std::string key, std::string& value);
        //DB Write
        bool DBPut(std::string key, std::string& value);
        //DB Delete
        bool DBDelete(std::string key);
        bool GetAllValueByPrefix(std::string prefix, std::set<std::string>& values);
	};
}

#endif