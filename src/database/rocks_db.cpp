#include "rocks_db.h"
#include <rocksdb/db.h>
#include <rocksdb/table.h>
#include <rocksdb/filter_policy.h>
#include <rocksdb/cache.h>
#include <rocksdb/utilities/checkpoint.h>
#include <utils/timestamp.h>
#include <memory>
#include "utils/logger.h"
#include <stdio.h>


namespace agent {
	RocksDbDriver::RocksDbDriver()
	{
		db_ = nullptr;
	}

	RocksDbDriver::~RocksDbDriver() {
		Close();
	}

	bool RocksDbDriver::Open(const std::string &db_path, int max_open_files) {
		rocksdb::Options options;
		options.max_open_files = 500; 
        rocksdb::BlockBasedTableOptions table_options;
        table_options.filter_policy.reset(rocksdb::NewBloomFilterPolicy(10, false));
        auto cache = rocksdb::NewLRUCache(1024 * 1024 *  8); //8M
        table_options.block_cache = cache;
        table_options.block_size = 1024 * 16;//16KB
        table_options.cache_index_and_filter_blocks = true;
        options.table_factory.reset(rocksdb::NewBlockBasedTableFactory(table_options));
		options.create_if_missing = true;
		rocksdb::Status status = rocksdb::DB::Open(options, db_path, &db_);
		if (!status.ok()) {
			utils::MutexGuard guard(mutex_);
			error_desc_ = status.ToString();
			error_code_ = (KeyValueErrorCode)status.code();
		}
		return status.ok();
	}

	bool RocksDbDriver::OpenReadOnly(const std::string &db_path) {
		rocksdb::Options options;
		options.max_open_files = 100; 
		rocksdb::BlockBasedTableOptions table_options;
		table_options.no_block_cache = true;
		table_options.block_restart_interval = 4;
		options.table_factory.reset(NewBlockBasedTableFactory(table_options));
		rocksdb::Status status = rocksdb::DB::OpenForReadOnly(options, db_path, &db_);
		if (!status.ok()) {
			utils::MutexGuard guard(mutex_);
			error_desc_ = status.ToString();
			error_code_ = (KeyValueErrorCode)status.code();
		}
		return status.ok();
	}

	bool RocksDbDriver::Close() {
		if (db_ != nullptr) {
			delete db_;
			db_ = nullptr;
		}
		return true;
	}

	int32_t RocksDbDriver::Get(const std::string &key, std::string &value) {
		assert(db_ != nullptr);
		rocksdb::Status status = db_->Get(rocksdb::ReadOptions(), key, &value);
		if (status.ok()) {
			return 1;
		}
		else if (status.IsNotFound()) {
			return 0;
		}
		else {
			utils::MutexGuard guard(mutex_);
			error_desc_ = status.ToString();
			return -1;
		}
	}

	bool RocksDbDriver::Put(const std::string &key, const std::string &value) {
		assert(db_ != nullptr);
		rocksdb::WriteOptions opt;
		opt.sync = false;
		rocksdb::Status status = db_->Put(opt, key, value);
		if (!status.ok()) {
			utils::MutexGuard guard(mutex_);
			error_desc_ = status.ToString();
		}
		return status.ok();
	}

	bool RocksDbDriver::Delete(const std::string &key) {
		assert(db_ != nullptr);
		rocksdb::WriteOptions opt;
		opt.sync = false;
		rocksdb::Status status = db_->Delete(opt, key);
		if (!status.ok()) {
			utils::MutexGuard guard(mutex_);
			error_desc_ = status.ToString();
		}
		return status.ok();
	}

	bool RocksDbDriver::WriteBatch(WRITE_BATCH &write_batch) {

		rocksdb::WriteOptions opt;
		opt.sync = false;
		rocksdb::Status status = db_->Write(opt, &write_batch);
		if (!status.ok()) {
			utils::MutexGuard guard(mutex_);
			error_desc_ = status.ToString();
		}
		return status.ok();
	}

	void* RocksDbDriver::NewIterator() {
		rocksdb::ReadOptions options;
  		options.fill_cache = false; // read data not fill in LRU cache
		return db_->NewIterator(options);
	}

	bool RocksDbDriver::GetOptions(Json::Value &options) {
		std::string out;
		db_->GetProperty("rocksdb.estimate-table-readers-mem", &out);
		options["rocksdb.estimate-table-readers-mem"] = out;

		db_->GetProperty("rocksdb.cur-size-all-mem-tables", &out);
		options["rocksdb.cur-size-all-mem-tables"] = out;

		db_->GetProperty("rocksdb.stats", &out);
		options["rocksdb.stats"] = out;
		return true;
	}

    bool RocksDbDriver::GetAllByIterator(std::string leaf_prefix, std::set<std::string>& valuelist) {
        assert(db_ != nullptr);
		rocksdb::ReadOptions options;
  		options.fill_cache = false; // read data not fill in LRU cache
		auto dbIter = std::shared_ptr<rocksdb::Iterator>(db_->NewIterator(options));
		if (!dbIter->status().ok()) {
			utils::MutexGuard guard(mutex_);
			error_desc_ = dbIter->status().ToString();
			return false;
		}

		for (dbIter->Seek(leaf_prefix); dbIter->Valid() && dbIter->key().starts_with(leaf_prefix); dbIter->Next()) {
            valuelist.insert(dbIter->value().ToString());
		}
		return true;	
    }
}
