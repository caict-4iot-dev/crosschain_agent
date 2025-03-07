#ifndef LEVEL_ROCKS_DB_H_
#define LEVEL_ROCKS_DB_H_

#include "keyvalue_db.h"
#include <rocksdb/db.h>
namespace agent {
	class RocksDbDriver : public KeyValueDb {
	private:
		rocksdb::DB* db_;

	public:
		RocksDbDriver();
		~RocksDbDriver();

		bool Open(const std::string &db_path, int max_open_files);
		bool OpenReadOnly(const std::string &db_path);
		bool Close();
		int32_t Get(const std::string &key, std::string &value);
		bool Put(const std::string &key, const std::string &value);
		bool Delete(const std::string &key);
		bool GetOptions(Json::Value &options);
		bool WriteBatch(WRITE_BATCH &values);
        void *NewIterator();
        bool GetAllByIterator(std::string leaf_prefix, std::set<std::string>& valuelist);
	};
}

#endif
