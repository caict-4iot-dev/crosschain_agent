#ifndef UTILS_TIME_H_
#define UTILS_TIME_H_

#include "common.h"

namespace utils {

	class Timestamp {
	public:
		Timestamp();
		Timestamp(int64_t usTimestamp);
		Timestamp(const Timestamp &ts);

		std::string ToString() const;
		std::string ToFormatString(bool with_micro) const;
		std::string ToFormatDateString() const;
		std::string Format(bool milli_second) const;
		int TimeZone() const;

		time_t ToUnixTimestamp() const;
		int64_t timestamp() const;
		bool Valid() const;
		static Timestamp Now();
		static int64_t HighResolution();
		static bool GetLocalTimestamp(time_t timestamp, struct tm &timevalue);

		static Timestamp Invalid() { return Timestamp(); }

		static const int kMicroSecondsPerSecond = 1000 * 1000;

	private:
		int64_t timestamp_;
	};

	inline bool operator<(Timestamp lhs, Timestamp rhs) {
		return lhs.timestamp() < rhs.timestamp();
	}

	inline bool operator<=(Timestamp lhs, Timestamp rhs) {
		return lhs.timestamp() <= rhs.timestamp();
	}

	inline bool operator==(Timestamp lhs, Timestamp rhs) {
		return lhs.timestamp() == rhs.timestamp();
	}
}

#endif
