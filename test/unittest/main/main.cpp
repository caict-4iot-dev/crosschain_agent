
#include <gtest/gtest.h>
#include <utils/headers.h>
#include <main/configure.h>
#include <iostream>

using namespace std;


GTEST_API_ int main(int argc, char **argv){

	agent::Configure::InitInstance();

	//log
    utils::Logger::InitInstance();
	utils::Logger &logger = utils::Logger::Instance();
	logger.SetCapacity(1 * 3600 * 24, 10 * utils::BYTES_PER_MEGA);
	logger.SetExpireDays(10);
	if (!logger.Initialize(utils::LOG_DEST_FILE, utils::LOG_LEVEL_ALL, "log/bif.log", true)){
		LOG_STD_ERR("Failed to initialize logger");
		return false;
	}
	LOG_INFO("Initialized logger successfully");

	if(argc > 1)
		testing::GTEST_FLAG(filter)=argv[1]; //通过用户自定义传参数决定运行指定测试用例，多个用例的话中间以:号分隔并支持正则形式，比如SafeIntegerOpeTest.UT_SafeIntAdd:SafeIntegerOpeTest.case*
	else
		testing::GTEST_FLAG(output) = "xml:gtest_result.xml";//单元测试结果输出到文件

	testing::InitGoogleTest(&argc, argv);
	std::cout << RUN_ALL_TESTS() << std::endl;

	agent::Configure::ExitInstance();
	utils::Logger::ExitInstance();

	return 0;
}
