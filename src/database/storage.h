#ifndef STORAGE_H_
#define STORAGE_H_

#include <unordered_map>
#include <utils/headers.h>
#include <utils/thread.h>
#include <json/json.h>
#include <common/configure_base.h>
#include <main/configure.h>
#include "keyvalue_db.h"
#include <proto/cpp/common.pb.h>
#include <atomic>
#include "storage_lock.h"
namespace agent {
	class Storage {
	public:
		Storage();
		~Storage();
    private:
		std::shared_ptr<KeyValueDb> db_;
		std::shared_ptr<DomianBatchVector> batch_;
		std::string db_path_;

	public:
		static std::shared_ptr<KeyValueDb> NewKeyValueDb();
	public:
		bool CreateDb();
        void CloseDb();

		std::shared_ptr<agent::BatchIndex> NewWriteBatch();
		void ReleaseBatch(uint32_t batch_index);
		bool BatchPut( uint32_t batch_index, std::string key, const std::string& value);
        bool BatchDelete(uint32_t batch_index, std::string key);
        bool BatchClear(uint32_t batch_index);
		bool WriteBatch(uint32_t batch_index);

		int32_t DBGet(std::string key, std::string& value);
		int32_t DBPut(std::string key, const std::string& value);
		int32_t DBDelete(std::string key);

        bool GetAllValueByPrefix(std::string prefix, std::set<std::string>& values);
	};
}

#endif
