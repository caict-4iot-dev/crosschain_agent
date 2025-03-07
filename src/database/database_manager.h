#ifndef STORAGE_CONSISTENT_H_
#define STORAGE_CONSISTENT_H_

#include <utils/headers.h>
#include "keyvalue_db.h"
#include "storage.h"
#include "storage_lock.h"
namespace agent {
	class DatabaseManager {
	public:
		DatabaseManager();
		~DatabaseManager();
	public:
		bool Initialize();

		std::shared_ptr<agent::BatchIndex> NewWriteBatch();
		void ReleaseBatch(uint32_t batch_index);
        bool BatchPut(uint32_t batch_index, std::string key, const std::string& value);
        bool BatchDelete(uint32_t batch_index, std::string key);
        bool BatchClear(uint32_t batch_index);
		bool WriteBatch(uint32_t batch_index);
		
		bool DBGet(std::string key, std::string& value);
		bool DBPut(std::string key, const std::string& value);
		bool DBDelete(std::string key);
        bool GetAllValueByPrefix(std::string prefix, std::set<std::string>& values);
	private:
		std::shared_ptr<Storage> storage_;
	};
}

#endif
