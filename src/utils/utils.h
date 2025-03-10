﻿#ifndef UTIS_UTILS_H_
#define UTIS_UTILS_H_

#include <stack>
#include "common.h"
#include <sys/sysinfo.h>

namespace utils {
	// Seconds
	static const int64_t  MILLI_UNITS_PER_SEC = 1000;   // Milliseconds
	static const int64_t  MICRO_UNITS_PER_MILLI = 1000; // Microseconds
	static const int64_t  NANO_UNITS_PER_MICRO = 1000;  // Nanoseconds
	static const int64_t  MICRO_UNITS_PER_SEC = MICRO_UNITS_PER_MILLI * MILLI_UNITS_PER_SEC;
	static const int64_t  NANO_UNITS_PER_SEC = NANO_UNITS_PER_MICRO * MICRO_UNITS_PER_SEC;

	static const time_t SECOND_UNITS_PER_MINUTE = 60;
	static const time_t MINUTE_UNITS_PER_HOUR = 60;
	static const time_t HOUR_UNITS_PER_DAY = 24;
	static const time_t SECOND_UNITS_PER_HOUR = SECOND_UNITS_PER_MINUTE * MINUTE_UNITS_PER_HOUR;
	static const time_t SECOND_UNITS_PER_DAY = SECOND_UNITS_PER_HOUR * HOUR_UNITS_PER_DAY;

	static const size_t  BYTES_PER_KILO = 1024;
	static const size_t  KILO_PER_MEGA = 1024;
	static const size_t  BYTES_PER_MEGA = BYTES_PER_KILO * KILO_PER_MEGA;

	static const size_t MAX_OPERATIONS_NUM_PER_TRANSACTION = 100;

	static const uint16_t  MAX_UINT16 = 0xFFFF;
	static const uint32_t  MAX_UINT32 = 0xFFFFFFFF;
	static const int32_t   MAX_INT32 = 0x7FFFFFFF;
	static const int64_t   MAX_INT64 = 0x7FFFFFFFFFFFFFFF;

	// Low-high
	static const uint64_t LOW32_BITS_MASK = 0xffffffffULL;
	static const uint64_t HIGH32_BITS_MASK = 0xffffffff00000000ULL;

	static const size_t ETH_MAX_PACKET_SIZE = 1600;

	uint32_t error_code();
	void set_error_code(uint32_t code);

	std::string error_desc(uint32_t code = -1);

#define LOCK_CAS(mem, with, cmp) __sync_val_compare_and_swap(mem, cmp, with)
#define LOCK_YIELD()             pthread_yield();

	inline int32_t AtomicInc(volatile int32_t *value) {
		__sync_fetch_and_add(value, 1);
		return *value;
	}

	inline int64_t AtomicInc(volatile int64_t *value) {
		__sync_fetch_and_add(value, 1);
		return *value;
	}

	inline int32_t AtomicDec(volatile int32_t *value) {
		__sync_fetch_and_sub(value, 1);
		return *value;
	}

	inline int64_t AtomicDec(volatile int64_t *value) {
		__sync_fetch_and_sub(value, 1);
		return *value;
	}

	template<typename T>
	class AtomicInteger {
	public:
		AtomicInteger()
			: value_(0) {}

		T Inc() {
			return AtomicInc(&value_);
		}
		T Dec() {
			return AtomicDec(&value_);
		}
		T value() const {
			return value_;
		}
	private:
		T value_;
	};

	typedef AtomicInteger<int32_t> AtomicInt32;
	typedef AtomicInteger<int64_t> AtomicInt64;

	void Sleep(int nMillSecs);

	size_t GetCpuCoreCount();
	time_t GetStartupTime(time_t time_now = 0);
	std::string GetCinPassword(const std::string &_prompt);

	void SetExceptionHandle();

	char *Ltrim(char *line);
	char *Rtrim(char *line);
	char *Trim(char *line);
	bool CheckPortIsUsed(unsigned int port);

#if __cplusplus >= 201402L || (defined(_MSC_VER) && _MSC_VER >= 1900)

	using std::make_unique;

#else
	template <typename T, typename... Args>
	std::unique_ptr<T>
		make_unique(Args&&... args) {
		return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
	}
#endif


	class ObjectExit {
	public:
		typedef std::function<bool()> ExitHandler;
		ObjectExit() {};
		~ObjectExit() {
			while (!s_.empty()) {
				s_.top()();
				s_.pop();
			}
		}
		void Push(ExitHandler fun) { s_.push(fun); };

	private:
		std::stack<ExitHandler> s_;
	};

	class DbValue {
	public:
		enum ValueType {
			nullValue = 0, ///< 'null' value
			intValue,      ///< signed integer value
			uintValue,     ///< unsigned integer value
			int64Value,    ///< signed big integer value
			uint64Value,   ///< unsigned big integer value
			realValue,     ///< double value
			stringValue,   ///< UTF-8 string value
			booleanValue  ///< bool value
		};
	private:
		union {
			int32_t int_;
			uint32_t uint_;
			int64_t int64_;
			uint64_t uint64_;
			double real_;
			bool bool_;
		}values_;
		std::string str_value_;
		ValueType type_;
	public:
		DbValue();
		~DbValue();

		void operator = (const std::string &value);
		void operator = (const uint32_t  &value);
		void operator = (const int32_t  &value);
		void operator = (const uint64_t  &value);
		void operator = (const int64_t  &value);
		void operator = (const double  &value);
		void operator = (bool  &value);
		void operator = (const char *value);

		std::string AsString() const ;
		bool AsBool() const;
		uint32_t AsUint32() const;
		int32_t AsInt32() const;
		uint64_t AsUnt64() const;
		int64_t AsInt64() const;
		double AsDouble() const;

		ValueType GetType() const;
	};

	typedef std::map<std::string, DbValue> DbRow;
	typedef std::vector<DbRow> DbResult;
}
#endif

