#include "utils.h"
#include "strings.h"
#include "file.h"
#include <unistd.h>
#include <pwd.h>
#include <fnmatch.h>

const char *utils::File::PATH_SEPARATOR = "/";
const char  utils::File::PATH_CHAR = '/';

utils::File::File() {
	handle_ = NULL;
}

utils::File::~File() {
	if (IsOpened()) Close();
}

std::string utils::File::RegularPath(const std::string &path) {
	std::string new_path = path;
	new_path = utils::String::Replace(new_path, "\\", File::PATH_SEPARATOR);
	return new_path;
}

std::string utils::File::ConvertToAbsolutePath(const std::string &path, std::string base_path) {
	if (utils::File::IsAbsolute(path)){
		return RegularPath(path);
	}

	if (base_path.empty()) {
		base_path = utils::File::GetBinHome();
	} 

	return RegularPath(base_path + "/" + path);
}

std::string utils::File::GetFileFromPath(const std::string &path) {

	std::string regular_path = path;
	regular_path = File::RegularPath(regular_path);

	size_t nPos = regular_path.rfind(File::PATH_CHAR);
	if (std::string::npos == nPos) {
		return regular_path;
	}
	else if (nPos + 1 >= regular_path.size()) {
		return std::string("");
	}
	else {
		return regular_path.substr(nPos + 1, regular_path.size() - nPos - 1);
	}
}

bool utils::File::Open(const std::string &strFile0, int nMode) {
	CHECK_ERROR_RET(IsOpened(), ERROR_ALREADY_EXISTS, false);

	file_name_ = File::RegularPath(strFile0);

	// read or write
	char szMode[64] = { 0 };
	if (nMode & FILE_M_READ)	strcpy(szMode, (nMode & FILE_M_WRITE) ? "r+" : "r");
	else if (nMode & FILE_M_WRITE) strcpy(szMode, "w");
	else if (nMode & FILE_M_APPEND) strcpy(szMode, "a");
	else strcpy(szMode, "r+");

	// text or binary
	if (nMode & FILE_M_BINARY) strcat(szMode, "b");
	else if (nMode & FILE_M_TEXT) strcat(szMode, "t");
	else strcat(szMode, "b"); // default as binary

	handle_ = fopen(file_name_.c_str(), szMode);
	if (NULL == handle_) {
		return false;
	}

	open_mode_ = nMode;
	if (nMode & FILE_M_LOCK && !LockRange(0, utils::LOW32_BITS_MASK, true)) {
		uint32_t error_code = utils::error_code();

		fclose(handle_);
		handle_ = NULL;

		utils::set_error_code(error_code);
		return false;
	}

	return NULL != handle_;
}

bool utils::File::Close() {
	CHECK_ERROR_RET(!IsOpened(), ERROR_NOT_READY, false);

	if (open_mode_ & FILE_M_LOCK) {
		UnlockRange(0, utils::LOW32_BITS_MASK);
	}

	fclose(handle_);
	handle_ = NULL;

	return true;
}

bool utils::File::Flush() {
	CHECK_ERROR_RET(!IsOpened(), ERROR_NOT_READY, false);

	return fflush(handle_) == 0;
}

bool utils::File::LockRange(uint64_t offset, uint64_t size, bool try_lock /* = false */) {
	CHECK_ERROR_RET(!IsOpened(), ERROR_NOT_READY, false);

	bool result = false;
	int file_no = fileno(handle_);

	uint64_t nOldPosition = GetPosition();
	if (nOldPosition != offset && !Seek(offset, File::FILE_S_BEGIN)) {
		return false;
	}

	int nFlags = try_lock ? F_TLOCK : F_LOCK;
	off_t nUnlockSize = (utils::LOW32_BITS_MASK == size) ? 0 : (off_t)size;
	result = (lockf(file_no, nFlags, nUnlockSize) == 0);

	if (nOldPosition != offset && !Seek(nOldPosition, File::FILE_S_BEGIN)) {
		uint32_t nErrorCode = utils::error_code();

		// unlock
		//result = false;
		result = (lockf(file_no, F_ULOCK, nUnlockSize) == 0);

		utils::set_error_code(nErrorCode);
	}

	return result;
}

bool utils::File::UnlockRange(uint64_t offset, uint64_t size) {
	CHECK_ERROR_RET(!IsOpened(), ERROR_NOT_READY, false);

	bool result = false;
	int file_no = fileno(handle_);

#ifndef ANDROID
	uint64_t nOldPosition = GetPosition();

	if (nOldPosition != offset && !Seek(offset, File::FILE_S_BEGIN)) {
		return false;
	}

	off_t nUnlockSize = (utils::LOW32_BITS_MASK == size) ? 0 : (off_t)size;
	bool bResult = (lockf(file_no, F_ULOCK, nUnlockSize) == 0);

	if (nOldPosition != offset && !Seek(nOldPosition, File::FILE_S_BEGIN)) {
		bResult = false;
	}
#endif
	return result;
}

size_t utils::File::ReadData(std::string &data, size_t max_count) {
	CHECK_ERROR_RET(!IsOpened(), ERROR_NOT_READY, 0);

	const size_t buffer_size = 1024 * 1024;
	size_t read_bytes_size = 0;
	static char nTmpBuffer[buffer_size];

	while (read_bytes_size < max_count) {
		size_t nCount = MIN(max_count - read_bytes_size, buffer_size);
		size_t nRetBytes = fread(nTmpBuffer, 1, nCount, handle_);

		if (nRetBytes > 0) {
			read_bytes_size += nRetBytes;
			data.append(nTmpBuffer, nRetBytes);
		}
		else {
			break;
		}
	}

	return read_bytes_size;
}

size_t utils::File::Read(void *pBuffer, size_t nChunkSize, size_t nCount) {
	CHECK_ERROR_RET(!IsOpened(), ERROR_NOT_READY, 0);

	return fread(pBuffer, nChunkSize, nCount, handle_);
}

bool utils::File::ReadLine(std::string &strLine, size_t nMaxCount) {
	CHECK_ERROR_RET(!IsOpened(), ERROR_NOT_READY, 0);

	strLine.resize(nMaxCount + 1, 0);

	char *pszBuffer = (char *)strLine.c_str();
	pszBuffer[nMaxCount] = 0;
	if (fgets(pszBuffer, nMaxCount, handle_) == NULL) {
		strLine.clear();
		return false;
	}

	size_t nNewLen = strlen(pszBuffer);
	strLine.resize(nNewLen);
	return true;
}

size_t utils::File::Write(const void *pBuffer, size_t nChunkSize, size_t nCount) {
	CHECK_ERROR_RET(!IsOpened(), ERROR_NOT_READY, 0);

	return fwrite(pBuffer, nChunkSize, nCount, handle_);
}

uint64_t  utils::File::GetPosition() {
	return 0;
}

bool utils::File::Seek(uint64_t offset, FILE_SEEK_MODE nMode) {
	return true;
}

bool utils::File::IsAbsolute(const std::string &path) {
	std::string regular_path = File::RegularPath(path);
	return regular_path.size() > 0 && '/' == regular_path[0];
}

std::string utils::File::GetBinPath() {
	std::string path;
	char szpath[File::MAX_PATH_LEN * 4] = { 0 };
	ssize_t path_len = readlink("/proc/self/exe", szpath, File::MAX_PATH_LEN * 4 - 1);
	if (path_len >= 0) {
		szpath[path_len] = '\0';
		path = szpath;
	}
	return path;
}

std::string utils::File::GetBinDirecotry() {
	std::string path = File::GetBinPath();

	return GetUpLevelPath(path);
}

std::string utils::File::GetBinHome() {
	return GetUpLevelPath(GetBinDirecotry());
}

std::string utils::File::GetUpLevelPath(const std::string &path) {
	std::string normal_path = File::RegularPath(path);

	size_t nPos = normal_path.rfind(File::PATH_CHAR);
	if (std::string::npos == nPos) {
		return std::string("");
	}

	if (0 == nPos && normal_path.size() > 0 && normal_path[0] == File::PATH_CHAR) {
		nPos++;
	}

	return normal_path.substr(0, nPos);
}

bool utils::File::GetAttribue(const std::string &strFile0, FileAttribute &nAttr) {
	std::string file1 = RegularPath(strFile0);
	struct stat nFileStat;
	if (stat(file1.c_str(), &nFileStat) == 0) {
		nAttr.is_directory_ = (nFileStat.st_mode & S_IFDIR) != 0;
		nAttr.create_time_ = nFileStat.st_ctime;
		nAttr.modify_time_ = nFileStat.st_mtime;
		nAttr.access_time_ = nFileStat.st_atime;
		nAttr.size_ = nFileStat.st_size;

		return true;
	} else {
		return false;
	}
}

bool utils::File::GetFileList(const std::string &strDirectory0, const std::string &strPattern, utils::FileAttributes &nFiles, bool bFillAttr, size_t nMaxCount) {
	std::string strNormalPath = File::RegularPath(strDirectory0);
	DIR *pDir = opendir(strNormalPath.c_str());
	if (NULL == pDir) {
		return false;
	}

	// Clean old files
	nFiles.clear();

	struct dirent *pItem = NULL;
	while ((pItem = readdir(pDir)) != NULL) {
		if (strcmp(pItem->d_name, ".") == 0 ||
			strcmp(pItem->d_name, "..") == 0) {
			// self or parent
			continue;
		}

		if (!strPattern.empty() && fnmatch(strPattern.c_str(), pItem->d_name, FNM_FILE_NAME | FNM_PERIOD) != 0) {
			// not match the pattern
			continue;
		}

		std::string strName(pItem->d_name);
		utils::FileAttribute &nAttr = nFiles[strName];

		if (bFillAttr) {
			std::string strFilePath = utils::String::Format("%s/%s", strNormalPath.c_str(), strName.c_str());
			File::GetAttribue(strFilePath, nAttr);
		}

		if (nMaxCount > 0 && nFiles.size() >= nMaxCount) {
			break;
		}
	}

	closedir(pDir);
	return true;
}

bool utils::File::GetFileList(const std::string &strDirectory0, utils::FileAttributes &nFiles, bool bFillAttr, size_t nMaxCount) {
	return utils::File::GetFileList(strDirectory0, std::string(""), nFiles, bFillAttr, nMaxCount);
}

uint64_t utils::File::GetDirectorySize(const std::string &strDirectory) {
	utils::FileAttributes nFiles;
	bool result = utils::File::GetFileList(strDirectory, std::string(""), nFiles);
	if (!result || nFiles.size() == 0) {
		return 0;
	}

	uint64_t ret = 0;
	for (auto i = nFiles.begin(); i != nFiles.end(); i++) {
		ret += i->second.size_;
	}

	return ret;
}

utils::FileAttribute utils::File::GetAttribue(const std::string &strFile0) {
	utils::FileAttribute nAttr;
	std::string strNormalFile = File::RegularPath(strFile0);

	File::GetAttribue(strNormalFile, nAttr);
	return nAttr;
}

bool utils::File::Move(const std::string &strSource, const std::string &strDest, bool bOverwrite) {
	std::string strNormalSource = File::RegularPath(strSource);
	std::string strNormalDest = File::RegularPath(strDest);

	if (bOverwrite && utils::File::IsExist(strDest)) {
		utils::File::Delete(strDest);
	}

	return rename(strNormalSource.c_str(), strNormalDest.c_str()) == 0;
}

bool utils::File::Copy(const std::string &strSource, const std::string &strDest, bool bOverwrite) {
	std::string strNormalSource = File::RegularPath(strSource);
	std::string strNormalDest = File::RegularPath(strDest);
	if (strNormalSource == strNormalDest) {
		utils::set_error_code(ERROR_ALREADY_EXISTS);
		return false;
	}
	else if (!bOverwrite && utils::File::IsExist(strNormalDest)) {
		utils::set_error_code(ERROR_ALREADY_EXISTS);
		return false;
	}

	utils::File nSource, nDest;
	uint32_t nErrorCode = ERROR_SUCCESS;
	bool bSuccess = false;
	char *pDataBuffer = NULL;
	const size_t nBufferSize = 102400;

	do {
		pDataBuffer = (char *)malloc(nBufferSize);
		if (NULL == pDataBuffer) {
			nErrorCode = utils::error_code();
			break;
		}

		if (!nSource.Open(strNormalSource, utils::File::FILE_M_READ | utils::File::FILE_M_BINARY) ||
			!nDest.Open(strNormalDest, utils::File::FILE_M_WRITE | utils::File::FILE_M_BINARY)) {
			nErrorCode = utils::error_code();
			break;
		}

		while (true) {
			size_t nSizeRead = nSource.Read(pDataBuffer, 1, nBufferSize);
			if (nSizeRead == 0) {
				bSuccess = true;
				break;
			}

			if (nDest.Write(pDataBuffer, 1, nSizeRead) != nSizeRead) {
				nErrorCode = utils::error_code();
				break;
			}
		}
	} while (false);

	if (NULL != pDataBuffer) free(pDataBuffer);
	if (nSource.IsOpened()) nSource.Close();
	if (nDest.IsOpened()) nDest.Close();

	if (ERROR_SUCCESS != nErrorCode) {
		utils::set_error_code(nErrorCode);
	}

	return bSuccess;
}

bool utils::File::IsExist(const std::string &strFile) {
	std::string strNormalFile = File::RegularPath(strFile);
	struct stat nFileStat;
	return stat(strNormalFile.c_str(), &nFileStat) == 0 || errno != ENOENT;
}

bool utils::File::Delete(const std::string &strFile) {
	std::string strNormalFile = File::RegularPath(strFile);
	return unlink(strNormalFile.c_str()) == 0;
}

void RecurveRemoveDir() {
	DIR* cur_dir = opendir(".");
	struct dirent *ent = NULL;
	struct stat st;
	if (!cur_dir) {
		//	LOG_ERROR("");
		return;
	}
	while ((ent = readdir(cur_dir)) != NULL) {
		stat(ent->d_name, &st);
		if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0) {
			continue;
		}

		if (S_ISDIR(st.st_mode)) {
			chdir(ent->d_name);
			RecurveRemoveDir();
			chdir("..");
		}
		remove(ent->d_name);
	}
	closedir(cur_dir);
}

bool utils::File::DeleteFolder(const std::string &path) {
	std::string strNormalFile = File::RegularPath(path);
	char old_path[100];

	if (!path.c_str()) {
		return false;
	}

	getcwd(old_path, 100);

	if (chdir(path.c_str()) == -1) {
		//LOG_ERROR("not a dir or access error\n");
		return false;
	}

	//LOG_INFO("path_raw : %s\n", path_raw);
	RecurveRemoveDir();
	chdir(old_path);
	unlink(old_path);
	return rmdir(strNormalFile.c_str()) == 0;
}

std::string utils::File::GetExtension(const std::string &path) {
	std::string normal_path = RegularPath(path);

	// Check whether url is like "*****?url=*.*.*.*"
	size_t end_pos = normal_path.find('?');
	if (end_pos != std::string::npos) {
		normal_path = normal_path.substr(0, end_pos);
	}

	size_t nPos = normal_path.rfind('.');
	if (std::string::npos == nPos || (nPos + 1) == normal_path.size()) {
		return std::string("");
	}

	return normal_path.substr(nPos + 1);
}

std::string utils::File::GetTempDirectory() {
	std::string strPath;
	strPath = std::string("/tmp");
	while (strPath.size() > 0 && File::PATH_CHAR == strPath[strPath.size() - 1]) {
		strPath.erase(strPath.size() - 1, 1);
	}

	return strPath;
}

bool utils::File::CreateDir(const std::string &path) {
	std::string strNormalFile = File::RegularPath(path);
	return mkdir(strNormalFile.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) == 0;
}
