#include "keyvalue_db.h"
#include "utils/file.h"

namespace agent {
	KeyValueDb::KeyValueDb()
	{
		error_code_ = kOk;
	}

	KeyValueDb::~KeyValueDb() {}

}
