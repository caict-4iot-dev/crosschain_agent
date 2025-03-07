#include <utils/file.h>
#include <utils/logger.h>
#include "configure_base.h"

namespace agent {

	LoggerConfigure::LoggerConfigure() {
		dest_ = utils::LOG_DEST_OUT | utils::LOG_DEST_FILE;
		level_ = utils::LOG_LEVEL_ALL;
		time_capacity_ = 30;
		size_capacity_ = 100;
		expire_days_ = 10;
	}

	LoggerConfigure::~LoggerConfigure() {}

	bool LoggerConfigure::Load(const Json::Value &value) {
		ConfigureBase::GetValue(value, "path", path_);
		ConfigureBase::GetValue(value, "dest", dest_str_);
		ConfigureBase::GetValue(value, "level", level_str_);
		ConfigureBase::GetValue(value, "time_capacity", time_capacity_);
		ConfigureBase::GetValue(value, "size_capacity", size_capacity_);
		ConfigureBase::GetValue(value, "expire_days", expire_days_);

		time_capacity_ *= (3600 * 24);
		size_capacity_ *= utils::BYTES_PER_MEGA;

		//Parse the type string
		utils::StringVector dests, levels;
		dest_ = utils::LOG_DEST_NONE;
		dests = utils::String::Strtok(dest_str_, '|');

		for (auto &dest : dests) {
			std::string destitem = utils::String::ToUpper(dest);

			if (destitem == "ALL")         dest_ = utils::LOG_DEST_ALL;
			else if (destitem == "STDOUT") dest_ |= utils::LOG_DEST_OUT;
			else if (destitem == "STDERR") dest_ |= utils::LOG_DEST_ERR;
			else if (destitem == "FILE")   dest_ |= utils::LOG_DEST_FILE;
			else if (destitem == "PERFORMANCE")   dest_ |= utils::LOG_DEST_PERFORMANCE;
		}

		// Parse the level string
		level_ = utils::LOG_LEVEL_NONE;
		levels = utils::String::Strtok(level_str_, '|');

		for (auto &level : levels) {
			std::string levelitem = utils::String::ToUpper(level);

			if (levelitem == "ALL")          level_ = utils::LOG_LEVEL_ALL;
			else if (levelitem == "TRACE")   level_ |= utils::LOG_LEVEL_TRACE;
			else if (levelitem == "DEBUG")   level_ |= utils::LOG_LEVEL_DEBUG;
			else if (levelitem == "INFO")    level_ |= utils::LOG_LEVEL_INFO;
			else if (levelitem == "WARNING") level_ |= utils::LOG_LEVEL_WARN;
			else if (levelitem == "ERROR")   level_ |= utils::LOG_LEVEL_ERROR;
			else if (levelitem == "FATAL")   level_ |= utils::LOG_LEVEL_FATAL;
			else if (levelitem == "PERFORMANCE")   level_ |= utils::LOG_LEVEL_PERFORMANCE;
		}

		return true;
	}

	ConfigureBase::ConfigureBase() {}
	ConfigureBase::~ConfigureBase() {}

	void ConfigureBase::GetValue(const Json::Value &object, const std::string &key, std::string &value) {
		if (object.isMember(key)) {
			value = object[key].asString();
		}
	}

	void ConfigureBase::GetValue(const Json::Value &object, const std::string &key, uint32_t &value) {
		if (object.isMember(key)) {
			value = object[key].asUInt();
		}
	}

	void ConfigureBase::GetValue(const Json::Value &object, const std::string &key, int32_t &value) {
		if (object.isMember(key)) {
			value = object[key].asInt();
		}
	}

	void ConfigureBase::GetValue(const Json::Value &object, const std::string &key, int64_t &value) {
		if (object.isMember(key)) {
			value = object[key].asInt64();
		}
	}

	void ConfigureBase::GetValue(const Json::Value &object, const std::string &key, utils::StringList &list) {
		if (object.isMember(key)) {
			const Json::Value &array_value = object[key];
			for (size_t i = 0; i < array_value.size(); i++) {
				list.push_back(array_value[i].asString());
			}
		}
	}

	void ConfigureBase::GetValue(const Json::Value &object, const std::string &key, bool &value) {
		if (object.isMember(key)) {
			value = object[key].asBool();
		}
	}

	bool ConfigureBase::Load(const std::string &config_file_path) {
		do {
			utils::File config_file;
			if (!config_file.Open(config_file_path, utils::File::FILE_M_READ)) {
				break;
			}

			std::string data;
			config_file.ReadData(data, utils::BYTES_PER_MEGA);

			Json::Reader reader;
			Json::Value values;
			if (!reader.parse(data, values)) {
				LOG_STD_ERR("Failed to parse config file, (%s)", reader.getFormatedErrorMessages().c_str());
				break;
			}

			return LoadFromJson(values);
		} while (false);

		return false;
	}
}
