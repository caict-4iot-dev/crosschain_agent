#include "utils.h"
#include "file.h"
#include "strings.h"
#include <termios.h>

uint32_t utils::error_code() {
	return (uint32_t)errno;
}

void utils::set_error_code(uint32_t code) {
	errno = code;
}

void utils::Sleep(int time_milli) {
	::usleep(((__useconds_t)time_milli) * 1000);
}

std::string utils::error_desc(uint32_t code) {
	uint32_t real_code = (((uint32_t)-1) == code) ? error_code() : code;
	return std::string(strerror(real_code));
}

size_t utils::GetCpuCoreCount() {
	size_t core_count = 1;
	do {
		utils::File nProcFile;

		if (!nProcFile.Open("/proc/stat", utils::File::FILE_M_READ)) {
			break;
		}

		std::string strLine;

		if (!nProcFile.ReadLine(strLine, 1024)) {
			nProcFile.Close();
			break;
		}
		utils::StringVector nValues = utils::String::Strtok(strLine, ' ');
		if (nValues.size() < 8) {
			break;
		}

		core_count = nValues.size();
	} while (false);
	return core_count;
}

time_t utils::GetStartupTime(time_t time_now) {
	time_t nStartupTime = 0;

	if (0 == time_now) {
		time_now = time(NULL);
	}

	struct sysinfo nInfo;
	memset(&nInfo, 0, sizeof(nInfo));
	sysinfo(&nInfo);
	nStartupTime = time_now - (time_t)nInfo.uptime;

	//Utils::File nProcFile;

	//if( !nProcFile.Open("/proc/uptime", Utils::File::FILE_M_READ) )
	//{
	//	return 0;
	//}

	//std::string strLine;
	//Utils::StringArray nValues;

	//if( !nProcFile.ReadLine(strLine, 1024) || Utils::String::Split(strLine, nValues, ' ', -1, true) < 1 )
	//{
	//	nProcFile.Close();
	//	return 0;
	//}
	//nProcFile.Close();

	//uint32 nTimeSecs = Utils::String::ParseNumber(nValues[0], (uint32)0);
	//nStartupTime = nTimeNow - (time_t)nTimeSecs;
	return nStartupTime;
}


void utils::SetExceptionHandle()
{
}

char *utils::Ltrim(char *line) {
    int len = strlen(line);
    int i;
    auto *tmp = new char[len + 1];
    if (nullptr != tmp) {
        strcpy(tmp, line);
        for (i = 0; i < len; i++)
            if (isspace(tmp[i]) == 0)
                break;
        strcpy(line, &tmp[i]);
        delete[] tmp;
        return line;
    }
    delete[] tmp;
    return nullptr;
}


char *utils::Rtrim(char *line) {
    int i;
    if (nullptr != line) {
        for (i = strlen(line); i > 0; i--) {
            if (isspace(line[i - 1]) != 0)
                line[i - 1] = '\0';
            else
                break;
        }
    }
    return line;
}

char *utils::Trim(char *line) {
    Ltrim(line);
    Rtrim(line);
    return line;
}

bool utils::CheckPortIsUsed(unsigned int port) {
    FILE *pf;
    char buffer[4096];
    sprintf(buffer, "/usr/bin/netstat -an|grep -w %d | grep LISTEN |wc -l", port);
    pf = popen(buffer, "r");
    memset(buffer, 0, sizeof(buffer));
    fgets(buffer, sizeof(buffer), pf);
    pclose(pf);
    Trim(buffer);
    if (atoi(buffer) > 0)
        return false;
    else
        return true;
}

std::string utils::GetCinPassword(const std::string &_prompt) {
	struct termios oflags;
	struct termios nflags;
	char password[256];

	// Disable echo in the terminal
	tcgetattr(fileno(stdin), &oflags);
	nflags = oflags;
	nflags.c_lflag &= ~ECHO;
	nflags.c_lflag |= ECHONL;

	if (tcsetattr(fileno(stdin), TCSANOW, &nflags) != 0)
		abort();

	printf("%s", _prompt.c_str());
	if (!fgets(password, sizeof(password), stdin))
		abort();
	password[strlen(password) - 1] = 0;

	// Restore terminal
	if (tcsetattr(fileno(stdin), TCSANOW, &oflags) != 0)
		abort();
	return password;
}

utils::DbValue::DbValue() {}

utils::DbValue::~DbValue() {}

void utils::DbValue::operator = (const std::string &value) {
	type_ = DbValue::stringValue;
	str_value_ = value;
}

void utils::DbValue::operator = (const char *value) {
	type_ = DbValue::stringValue;
	str_value_ = value;
}

void utils::DbValue::operator = (const uint32_t  &value) {
	type_ = DbValue::uintValue;
	values_.uint_ = value;
}
void utils::DbValue::operator = (const int32_t  &value) {
	type_ = DbValue::intValue;
	values_.int_ = value;
}

void utils::DbValue::operator = (const uint64_t  &value) {
	type_ = DbValue::uint64Value;
	values_.uint64_ = value;
}

void utils::DbValue::operator = (const int64_t  &value) {
	type_ = DbValue::int64Value;
	values_.int64_ = value;
}

void utils::DbValue::operator = (const double  &value) {
	type_ = DbValue::realValue;
	values_.real_ = value;
}

void utils::DbValue::operator = (bool  &value) {
	type_ = DbValue::booleanValue;
	values_.bool_ = value;
}

std::string utils::DbValue::AsString() const {
	if (type_ == DbValue::stringValue) return str_value_;
	return "";
}

bool utils::DbValue::AsBool() const {
	if (type_ == DbValue::booleanValue) return values_.bool_;
	return false;
}

uint32_t utils::DbValue::AsUint32() const {
	if (type_ == DbValue::uintValue) return values_.uint_;
	return 0;
}

int32_t utils::DbValue::AsInt32() const {
	if (type_ == DbValue::intValue) return values_.int_;
	return 0;
}
uint64_t utils::DbValue::AsUnt64() const {
	if (type_ == DbValue::uint64Value) return values_.uint64_;
	return 0;
}
int64_t utils::DbValue::AsInt64() const {
	if (type_ == DbValue::int64Value) return values_.int64_;
	return 0;
}

double utils::DbValue::AsDouble() const {
	if (type_ == DbValue::realValue) return values_.real_;
	return 0.0;
}

utils::DbValue::ValueType utils::DbValue::GetType() const {
	return type_;
}

extern "C"
{
	void * __wrap_memcpy(void *dest, const void *src, size_t n) {
		asm(".symver memcpy, memcpy@GLIBC_2.2.5");
		return memcpy(dest, src, n);
	}
}

