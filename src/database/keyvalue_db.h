#ifndef KEYVALUE_DB_H_
#define KEYVALUE_DB_H_

#include <utils/headers.h>
#include <json/json.h>
#include <rocksdb/db.h>
#include <proto/cpp/common.pb.h>
namespace agent {
#define WRITE_BATCH rocksdb::WriteBatch
#define SLICE rocksdb::Slice

	struct KeyValue {
		std::string key;
		std::string value;
	};

	class KeyValueDb {
	public:
		typedef enum tagKeyValueErrorCode{
			kOk = 0,
			kNotFound = 1,
			kCorruption = 2,
			kNotSupported = 3,
			kInvalidArgument = 4,
			kIOError = 5,
			kMergeInProgress = 6,
			kIncomplete = 7,
			kShutdownInProgress = 8,
			kTimedOut = 9,
			kAborted = 10
		}KeyValueErrorCode;

	protected:
		utils::Mutex mutex_;
		std::string error_desc_;
		KeyValueErrorCode error_code_;

	public:
		KeyValueDb();
		virtual ~KeyValueDb();
		virtual bool Open(const std::string &db_path, int max_open_files) = 0;
		virtual bool OpenReadOnly(const std::string &db_path) = 0;
		virtual bool Close() = 0;
		virtual int32_t Get(const std::string &key, std::string &value) = 0;
		virtual bool Put(const std::string &key, const std::string &value) = 0;
		virtual bool Delete(const std::string &key) = 0;
		virtual bool GetOptions(Json::Value &options) = 0;
        virtual bool GetAllByIterator(std::string leaf_prefix, std::set<std::string>& valuelist) = 0;
        virtual void* NewIterator() = 0;
		std::string error_desc() { return error_desc_; }
		KeyValueErrorCode error_code() { return error_code_; }

		virtual bool WriteBatch(WRITE_BATCH &values) = 0;
	};
}
#endif
