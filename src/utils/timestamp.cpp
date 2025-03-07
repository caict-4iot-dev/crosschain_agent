#include "utils.h"
#include "timestamp.h"

utils::Timestamp::Timestamp()
	: timestamp_(0) {}

utils::Timestamp::Timestamp(int64_t usTimestamp)
	: timestamp_(usTimestamp) {}

utils::Timestamp::Timestamp(const Timestamp &ts)
	: timestamp_(ts.timestamp_) {}

std::string utils::Timestamp::ToString() const {
	char buf[32] = { 0 };
	int64_t seconds = timestamp_ / kMicroSecondsPerSecond;
	int64_t microseconds = timestamp_ % kMicroSecondsPerSecond;
#ifdef __x86_64__
	snprintf(buf, sizeof(buf), "%ld.%06ld", seconds, microseconds);
#else 
	snprintf(buf, sizeof(buf), "%lld.%06lld", seconds, microseconds);
#endif
	return buf;
}

std::string utils::Timestamp::ToFormatString(bool with_micro) const {
	char buf[32] = { 0 };
	time_t seconds = static_cast<time_t>(timestamp_ / kMicroSecondsPerSecond);
	int microseconds = static_cast<int>(timestamp_ % kMicroSecondsPerSecond);
	tm *tm_time = localtime(&seconds);

	if (with_micro) {
		snprintf(buf, sizeof(buf), "%4d-%02d-%02d %02d:%02d:%02d.%06d",
			tm_time->tm_year + 1900, tm_time->tm_mon + 1, tm_time->tm_mday,
			tm_time->tm_hour, tm_time->tm_min, tm_time->tm_sec,
			microseconds);
	}
	else {
		snprintf(buf, sizeof(buf), "%4d-%02d-%02d %02d:%02d:%02d",
			tm_time->tm_year + 1900, tm_time->tm_mon + 1, tm_time->tm_mday,
			tm_time->tm_hour, tm_time->tm_min, tm_time->tm_sec);
	}
	return buf;
}

std::string utils::Timestamp::ToFormatDateString() const {
	char buf[32] = { 0 };
	time_t seconds = static_cast<time_t>(timestamp_ / kMicroSecondsPerSecond);
	int microseconds = static_cast<int>(timestamp_ % kMicroSecondsPerSecond);
	tm *tm_time = localtime(&seconds);
	snprintf(buf, sizeof(buf), "%4d-%02d-%02d",
		tm_time->tm_year + 1900, tm_time->tm_mon + 1, tm_time->tm_mday);
	return buf;
}

time_t utils::Timestamp::ToUnixTimestamp() const {
	return static_cast<time_t>(timestamp_ / kMicroSecondsPerSecond);
}

int64_t utils::Timestamp::timestamp() const {
	return timestamp_;
}

bool utils::Timestamp::Valid() const {
	return timestamp_ > 0;
}

utils::Timestamp utils::Timestamp::Now() {
	struct timeval tv;
	gettimeofday(&tv, NULL);
	int64_t seconds = tv.tv_sec;
	return Timestamp(seconds * kMicroSecondsPerSecond + tv.tv_usec);
}

int64_t utils::Timestamp::HighResolution() {
	static int64_t uprefertime = 0;
	int64_t uptime = 0;
	struct timespec nTime = { 0, 0 };
	clock_gettime(CLOCK_MONOTONIC, &nTime);
	uptime = (int64_t)(nTime.tv_sec) * utils::MICRO_UNITS_PER_SEC + (int64_t)(nTime.tv_nsec) / utils::NANO_UNITS_PER_MICRO;
	if (uprefertime == 0) {
		uprefertime = Now().timestamp() - uptime;
	}
	return uptime + uprefertime;
}

std::string utils::Timestamp::Format(bool milli_second) const {
	char buffer[256] = { 0 };
	struct tm time_info = { 0 };
	time_t time_value = (time_t)(timestamp_ / utils::MICRO_UNITS_PER_SEC);
	int    micro_part = (int)(timestamp_ % utils::MICRO_UNITS_PER_SEC);

	GetLocalTimestamp(time_value, time_info);

	if (milli_second) {
		sprintf(buffer, "%04d-%02d-%02d %02d:%02d:%02d.%03d",
			time_info.tm_year + 1900,
			time_info.tm_mon + 1,
			time_info.tm_mday,
			time_info.tm_hour,
			time_info.tm_min,
			time_info.tm_sec,
			micro_part / 1000);
	}
	else {
		sprintf(buffer, "%04d-%02d-%02d %02d:%02d:%02d.%06d",
			time_info.tm_year + 1900,
			time_info.tm_mon + 1,
			time_info.tm_mday,
			time_info.tm_hour,
			time_info.tm_min,
			time_info.tm_sec,
			micro_part);
	}

	return std::string(buffer);
}

bool utils::Timestamp::GetLocalTimestamp(time_t timestamp, struct tm &timevalue) {
	const time_t timestampTmp = timestamp;
	return localtime_r(&timestampTmp, &timevalue) != NULL;
}

int utils::Timestamp::TimeZone() const {
	int timezone = 0;
	time_t t1, t2;
	struct tm *tm_local, *tm_utc;

	// get time
	time(&t1);
	t2 = t1;

	// local time
	tm_local = localtime(&t1);
	t1 = mktime(tm_local);

	// utc time
	tm_utc = gmtime(&t2);
	t2 = mktime(tm_utc);

	// time zone
	timezone = (t1 - t2) / 3600;

	return (int)timezone;
}
