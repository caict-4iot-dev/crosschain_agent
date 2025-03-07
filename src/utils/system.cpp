/*
	bif is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	bif is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with bif.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "system.h"
#include <mntent.h>

namespace utils{
	SystemProcessor::SystemProcessor() {
		core_count_ = 0;
		cpu_type_ = "";
		user_time_ = 0;
		nice_time_ = 0;
		system_time_ = 0;
		idle_time_ = 0;
		io_wait_time_ = 0;
		irq_time_ = 0;
		soft_irq_time_ = 0;
		usage_percent_ = 0;
	}
	SystemProcessor::~SystemProcessor() {

	}

	uint64_t SystemProcessor::GetTotalTime() {
		return user_time_+nice_time_+system_time_+idle_time_+io_wait_time_+irq_time_+soft_irq_time_;
	}
	uint64_t SystemProcessor::GetUsageTime() {
		return user_time_+nice_time_+system_time_;
	}

	System::System(bool with_processors) {
		with_processors_ = with_processors;
		processor_list_ = NULL;
	}

	System::~System() {
		delete processor_list_;
	}

	bool System::UpdateProcessor() {
		SystemProcessor nold_processer = processor_;
		SystemProcessorVector old_process_list;

		if (with_processors_) {
			if (NULL != processor_list_) {
				old_process_list.assign(processor_list_->begin(), processor_list_->end());
				processor_list_->clear();
			}
			else {
				processor_list_ = new SystemProcessorVector();
			}
		}

		File proce_file;

		if (!proce_file.Open("/proc/stat", File::FILE_M_READ))
			return false;

		std::string strline;
		StringVector values;

		if (!proce_file.ReadLine(strline, 1024)){
			proce_file.Close();
			return false;
		}

		values = String::split(strline, " ");
		if (values.size() < 8){
			proce_file.Close();
			return false;
		}

		processor_.user_time_ = String::Stoi64(values[1]);
		processor_.nice_time_ = String::Stoi64(values[2]);
		processor_.system_time_ = String::Stoi64(values[3]);
		processor_.idle_time_ = String::Stoi64(values[4]);
		processor_.io_wait_time_ = String::Stoi64(values[5]);
		processor_.irq_time_ = String::Stoi64(values[6]);
		processor_.soft_irq_time_ = String::Stoi64(values[7]);
		processor_.core_count_ = 0;

		while (proce_file.ReadLine(strline, 1024)) {
			values = String::split(strline, " ");
			if (values.size() < 8)
				break;
			if (std::string::npos == values[0].find("cpu"))
				break;

			processor_.core_count_++;
		}
		proce_file.Close();

		if (nold_processer.system_time_ > 0) {
			int64_t totalTime1 = nold_processer.GetTotalTime();
			int64_t usageTime1 = nold_processer.GetUsageTime();
			int64_t totalTime2 = processor_.GetTotalTime();
			int64_t usageTime2 = processor_.GetUsageTime();
			if (totalTime2 > totalTime1 && usageTime2 > usageTime1) {
				processor_.usage_percent_ = double(usageTime2 - usageTime1) / double(totalTime2 - totalTime1)*100.0;
			}
			else {
				processor_.usage_percent_ = 0;
			}
		}
		else {
			processor_.usage_percent_ = double(processor_.GetUsageTime()) / double(processor_.GetTotalTime()) * 100;
		}

		return true;
	}

	bool System::GetPhysicalDisk(const std::string &path, PhysicalDisk &disk) {
		struct statfs ndisk_stat;

		if (statfs(path.c_str(), &ndisk_stat) != 0) {
			return false;
		}

		disk.total_bytes_ = (uint64_t)(ndisk_stat.f_blocks) * (uint64_t)(ndisk_stat.f_frsize);
		disk.available_bytes_ = (uint64_t)(ndisk_stat.f_bavail) * (uint64_t)(ndisk_stat.f_bsize);
		// Default as root
		disk.free_bytes_ = disk.available_bytes_;

		if (disk.total_bytes_ > disk.free_bytes_) {
			disk.usage_percent_ = double(disk.total_bytes_ - disk.free_bytes_) / double(disk.total_bytes_) * 100.0;
		}

		return true;
	}

	bool System::GetDynamicDisk(DynamicDisk &dDisk){
		std::string os_bit = "";

		FILE *fstream = NULL;
		char buff[1024];
		memset(buff, 0, sizeof(buff));
		// calculate bytes per second, see more https://www.unix.com/man-page/redhat/1/sar/
		if (NULL == (fstream = popen("sar -b 1 1 |grep Average|awk '{print $5*512,$6*512}'", "r"))) {
			os_bit = "error";
			return false;
		}
		if (NULL != fgets(buff, sizeof(buff), fstream)) {
			os_bit = buff;
		}
		else{
			os_bit = "error";
		}
		char a[256] = { 0 }, b[256] = { 0 };
		std::sscanf(os_bit.c_str(), "%s %s", a, b);
		dDisk.read_bytes_ = std::string(a);
		dDisk.write_bytes_ = std::string(b);
		pclose(fstream);

		return true;
	}
// linux loadavg
	std::string System::GetLoadAvg(){
		static std::string loadAvg_ = "1";
		std::string os_bit = "";
		FILE *fstream = NULL;
		char buff[1024];
		memset(buff, 0, sizeof(buff));
		if (NULL == (fstream = popen("cat /proc/loadavg|awk '{print $1}'|sed 's/,//g'", "r"))) {
			os_bit = "error";
			return os_bit;
		}
		if (NULL != fgets(buff, sizeof(buff), fstream)) {
			if (buff[strlen(buff) - 1] == '\n') {
				buff[strlen(buff) - 1] = '\0';
			}
			os_bit = buff;
		}
		else{
			os_bit = "error";
		}
		loadAvg_ = os_bit;
		pclose(fstream);
		return loadAvg_;
	}

// linux cpu usage
	std::string System::GetCpuUsage(){
		static std::string cpu_usage = "0.00";
		std::string os_bit = "";
		FILE *fstream = NULL;
		char buff[1024];
		memset(buff, 0, sizeof(buff));
		if (NULL == (fstream = popen("sar -u 1 1|grep Average|awk '{print 100-$8}'", "r"))) {
			os_bit = "error";
			return os_bit;
		}
		if (NULL != fgets(buff, sizeof(buff), fstream)) {
			if (buff[strlen(buff) - 1] == '\n') {
				buff[strlen(buff) - 1] = '\0';
			}
			os_bit = buff;
		}
		else{
			os_bit = "error";
		}
		cpu_usage = os_bit;
		pclose(fstream);
		return cpu_usage;
	}

	bool utils::System::GetDynamicTraffic(DynamicTraffic &traffic){
		std::string os_bit = "";
		FILE *fstream = NULL;
		char buff[1024];
		memset(buff, 0, sizeof(buff));
		if (NULL == (fstream = popen("sar -n DEV 1 1|grep Average|awk 'NR!=1'|awk 'BEGIN{sum_u=0;sum_d=0}{sum_u+=$5;sum_d+=$6}END{print sum_u,sum_d}'", "r"))) {
			os_bit = "error";
			return false;
		}
		if (NULL != fgets(buff, sizeof(buff), fstream)) {
			os_bit = buff;
		}
		else{
			os_bit = "error";
		}
		char a[256] = { 0 }, b[256] = { 0 };
		std::sscanf(os_bit.c_str(), "%s %s", a, b);
		traffic.downTraffic_bytes_ = std::string(a);
		traffic.upTraffic_bytes_ = std::string(b);
		pclose(fstream);
		return true;
	}

	std::string utils::System::GetTcpConnectionsCount(){
		static std::string tcpCount_ = "1";
		std::string os_bit = "";
		FILE *fstream = NULL;
		char buff[1024];
		memset(buff, 0, sizeof(buff));
		if (NULL == (fstream = popen("netstat -ant |grep ESTABLISHED|wc -l ", "r"))) {
			os_bit = "error";
			return os_bit;
		}
		if (NULL != fgets(buff, sizeof(buff), fstream)) {
			if (buff[strlen(buff) - 1] == '\n') {
				buff[strlen(buff) - 1] = '\0';
			}
			os_bit = buff;
		}
		else{
			os_bit = "error";
		}
		tcpCount_ = os_bit;
		pclose(fstream);
		return tcpCount_;
	}

	bool System::GetPhysicalMemory(PhysicalMemory &memory) {
		File proc_file;

		if (!proc_file.Open("/proc/meminfo", File::FILE_M_READ)) {
			return false;
		}

		std::string strline;
		while (proc_file.ReadLine(strline, 1024)) {
			StringVector values;
			values=String::split(strline,":");
			String::Trim(values[0]);
			String::Trim(values[1]);
			const std::string &strkey = values[0];
			const std::string &strvalue = values[1];
			if ( strkey.compare("MemTotal")==0) {
				memory.total_bytes_=String::Stoui64( values[1])*1024;
			}
			else if (strkey.compare("MemFree") == 0) {
				memory.free_bytes_=String::Stoui64( values[1])*1024;
			}
			else if (strkey.compare("Buffers") == 0) {
				memory.buffers_bytes_=String::Stoui64( values[1])*1024;
			}
			else if (strkey.compare("Cached") == 0) {
				memory.cached_bytes_=String::Stoui64( values[1])*1024;
			}
		}
		proc_file.Close();

		memory.available_bytes_ = memory.free_bytes_ + memory.buffers_bytes_ + memory.cached_bytes_;
		if (memory.total_bytes_ > memory.available_bytes_) {
			memory.usage_percent_ = double(memory.total_bytes_ - memory.available_bytes_) / double(memory.total_bytes_) * (double)100.0;
		}

		return true;
	}

	time_t System::GetStartupTime(time_t time_now) {
		time_t startup_time = 0;
		if (0 == time_now) {
			time_now = time(NULL);
		}
		struct sysinfo info;
		memset(&info, 0, sizeof(info));
		sysinfo(&info);
		startup_time = time_now - (time_t)info.uptime;
		return startup_time;

	}

	size_t System::GetCpuCoreCount() {
		size_t core_count = 1;
		core_count = get_nprocs();
		return core_count;
	}

	bool System::GetHardwareAddress(std::string& hard_address, char* out_msg) {
		bool bret = false;
		do {
			std::string cpu_id;
			if (!GetCpuId(cpu_id)) {
				strcpy(out_msg, "get cpu id failed");
				break;
			}
			std::list<std::string> macs;
			if (!GetMac("%02x%02x%02x%02x%02x%02x", macs)) {
				strcpy(out_msg, "get mac address failed");
				break;
			}

			std::set<std::string> sort_mac;
			std::list<std::string>::iterator iter;
			for (iter = macs.begin(); iter != macs.end(); iter++) {
				sort_mac.insert(iter->c_str());
			}

			std::string mac;
			std::set<std::string>::iterator iter1;
			for (iter1 = sort_mac.begin(); iter1 != sort_mac.end(); iter1++) {
				mac += iter1->c_str();
			}
			std::string hard_info = cpu_id + mac;
			hard_address = utils::MD5::GenerateMD5ToHex(hard_info);
			bret = true;
		} while (false);
		return bret;
	}


	bool System::GetCpuAddress(std::string& hard_address, char* out_msg) {
		bool bret = false;
		do {
			std::string cpu_id;
			if (!GetCpuId(cpu_id)) {
				strcpy(out_msg, "get cpu id failed");
				break;
			}

			hard_address = utils::MD5::GenerateMD5ToHex(cpu_id);
			bret = true;
		} while (false);
		return bret;
	}

	bool System::GetCpuId(std::string& cpu_id) {
		bool bret = false;
		return bret;

	}

	bool System::GetMac(const char *format, std::list<std::string> &macs) {
		bool bret = false;
		int fd;
		do {
			int interfaceNum = 0;
			struct ifreq buf[16];
			struct ifconf ifc;
			if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) break;
			ifc.ifc_len = sizeof(buf);
			ifc.ifc_buf = (caddr_t)buf;
			if (ioctl(fd, SIOCGIFCONF, (char *)&ifc)) break;
			interfaceNum = ifc.ifc_len / sizeof(struct ifreq);
			while (interfaceNum-- > 0) {
				char ac_mac[32] = { 0 };
				struct ifreq ifrcopy;
				//Ignore the interface that is not up or not runing
				ifrcopy = buf[interfaceNum];
				if (ioctl(fd, SIOCGIFFLAGS, &ifrcopy)) continue;
				//Get the mac of this interface
				if (ioctl(fd, SIOCGIFHWADDR, (char *)(&buf[interfaceNum]))) continue;
				snprintf(ac_mac, sizeof(ac_mac), format,
					(unsigned char)buf[interfaceNum].ifr_hwaddr.sa_data[0],
					(unsigned char)buf[interfaceNum].ifr_hwaddr.sa_data[1],
					(unsigned char)buf[interfaceNum].ifr_hwaddr.sa_data[2],
					(unsigned char)buf[interfaceNum].ifr_hwaddr.sa_data[3],
					(unsigned char)buf[interfaceNum].ifr_hwaddr.sa_data[4],
					(unsigned char)buf[interfaceNum].ifr_hwaddr.sa_data[5]);
				macs.push_back(std::string(ac_mac));
				memset(ac_mac, 0, sizeof(ac_mac));
				bret = true;
			}
		} while (false);
		close(fd);
		//macs.rsort();
		return bret;
	}

	std::string System::GetHostName() {
		char host_name[128];
		if (gethostname(host_name, 128) != 0)
			host_name[0] = '\0';
		return std::string(host_name);
	}


	std::string System::GetOsVersion() {
		std::string os_version;
		struct utsname unix_name;

		memset(&unix_name, 0, sizeof(unix_name));
		if (uname(&unix_name) != 0) {
			os_version = "Unknown";
		}
		else {
			os_version = String::Format("%s %s %s %s",
				unix_name.sysname,
				unix_name.release,
				unix_name.machine,
				unix_name.version);
		}
		return os_version;
	}

	std::string System::GetOsBits(){
		std::string os_bit = "";
		FILE *fstream=NULL;
		char buff[1024];
		memset(buff, 0, sizeof(buff));
		if (NULL == (fstream = popen("getconf LONG_BIT", "r"))) {
			os_bit = "error";
			return os_bit;
		}
		if (NULL != fgets(buff, sizeof(buff), fstream)) {
			buff[2] = '\0';
			os_bit = buff;
		}
		else {
			os_bit = "error";
		}
		pclose(fstream);
		return os_bit;
	}

	uint64_t System::GetLogsSize(const std::string path) {

		uint64_t log_size = 0;

		std::string out_file_name = path + "-out";
		std::string err_file_name = path + "-err";

		std::string file_ext = File::GetExtension(path);
		if (file_ext.size() > 0 && (file_ext.size() + 1) < path.size())
		{
			std::string sub_path = path.substr(0, path.size() - file_ext.size() - 1);

			out_file_name = String::Format("%s-out.%s", sub_path.c_str(), file_ext.c_str());
			err_file_name = String::Format("%s-err.%s", sub_path.c_str(), file_ext.c_str());
		}

		log_size = GetLogSize(out_file_name.c_str());
		log_size += GetLogSize(err_file_name.c_str());

		return log_size;
	}

	uint64_t System::GetLogSize(const char* path) {

		uint64_t log_size = 0;

		struct stat buff;
		if (stat(path, &buff) < 0) {
			return log_size;
		}
		else {
			log_size = buff.st_size;
		}

		return log_size;
	}

	bool System::GetPhysicalPartition(uint64_t &total_bytes, PhysicalPartitionVector &nPartitionList) {
		total_bytes = 0;
		nPartitionList.clear();
		FILE* mount_table;
		struct mntent *mount_entry;
		struct statfs s;
		unsigned long blocks_used;
		unsigned blocks_percent_used;
		const char *disp_units_hdr = NULL;
		mount_table = NULL;
		mount_table = setmntent("/etc/mtab", "r");
		if (!mount_table) {
			return false;
		}
		PhysicalPartition nPartition;
		while (1) {
			const char *device;
			const char *mount_point;
			if (mount_table) {
				mount_entry = getmntent(mount_table);
				if (!mount_entry) {
					endmntent(mount_table);
					break;
				}
			} 
			else
				continue;
			device = mount_entry->mnt_fsname;
			mount_point = mount_entry->mnt_dir;
			if (statfs(mount_point, &s) != 0)  {
				continue;
			}
			if ((s.f_blocks > 0) || !mount_table )  {
				blocks_used = s.f_blocks - s.f_bfree;
				blocks_percent_used = 0;
				if (blocks_used + s.f_bavail) {
					blocks_percent_used = (blocks_used * 100ULL
						+ (blocks_used + s.f_bavail) / 2
						) / (blocks_used + s.f_bavail);
				}
				if (strcmp(device, "rootfs") == 0)
					continue;
				char s1[20];
				char s2[20];
				char s3[20];
				nPartition.total_bytes_ = s.f_blocks * s.f_bsize;
				nPartition.free_bytes_ = s.f_bfree * s.f_bsize;
				nPartition.available_bytes_ = s.f_bavail * s.f_bsize;
				nPartition.describe_ = mount_point;
				nPartition.usage_percent_ = blocks_percent_used;

				nPartitionList.push_back(nPartition);
				total_bytes += nPartition.total_bytes_;
			}
		}
		return true;
	}

}
