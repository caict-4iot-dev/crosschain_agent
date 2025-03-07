#include <utils/headers.h>
#include <glue/handle_manager.h>
#include <websocket/layer1_websocket_client.h>
#include <websocket/layer2_websocket_client.h>
#include "configure.h"
#include <database/database_proxy.h>
#include <iostream>
#include <algorithm>
#include <utils/file.h>
#include <utils/logger.h>
#include <common/general.h>
#include <common/argument.h>
#include <utils/timer.h>
#include <common/daemon.h>
#include <webserver/web_server.h>

using namespace std;
void RunLoop();

int main(int argc, char *argv[]){
    utils::Daemon::InitInstance();
    utils::Timer::InitInstance();
	agent::Configure::InitInstance();
    agent::DatabaseProxy::InitInstance();
    agent::AddressPrefix::InitInstance();
	utils::Logger::InitInstance();
    agent::HandleManager::InitInstance();
    agent::WebServer::InitInstance();
    agent::Argument arg;
    if (arg.Parse(argc, argv)){
		//do nothing
		//std::cout << "main Parse Exec Success" << std::endl;
	} else {
	    do {
            utils::ObjectExit object_exit;
            agent::InstallSignal();

            utils::Daemon &daemon = utils::Daemon::Instance();
			if (!agent::g_enable_ || !daemon.Initialize((int32_t)1234))
			{
				LOG_STD_ERRNO("Failed to initialize daemon", STD_ERR_CODE, STD_ERR_DESC);
				break;
			}
			object_exit.Push(std::bind(&utils::Daemon::Exit, &daemon));

            agent::Configure &config = agent::Configure::Instance();
            std::string config_path = "config/agent.json";
            if (!config.Load(utils::File::ConvertToAbsolutePath(config_path))) {
                LOG_STD_ERRNO("Failed to load configuration", STD_ERR_CODE, STD_ERR_DESC);
                break;
            }
            agent::AddressPrefix::Instance().Initialize();

            std::string log_path = utils::File::ConvertToAbsolutePath(config.logger_configure_.path_);
            const agent::LoggerConfigure &logger_config = agent::Configure::Instance().logger_configure_;
            utils::Logger &logger = utils::Logger::Instance();
            logger.SetCapacity(logger_config.time_capacity_, logger_config.size_capacity_);
            logger.SetExpireDays(logger_config.expire_days_);
            if (!agent::g_enable_ || !logger.Initialize((utils::LogDest)(logger_config.dest_),
                                                    (utils::LogLevel)logger_config.level_, log_path, true)) {
                LOG_STD_ERR("Failed to initialize logger");
                break;
            }
            object_exit.Push(std::bind(&utils::Logger::Exit, &logger));
            LOG_INFO("Loaded configure successfully");
            LOG_INFO("Initialized logger successfully");

            // database proxy
            agent::DatabaseProxy &database_proxy = agent::DatabaseProxy::Instance();
            if (!agent::g_enable_ || !database_proxy.Initialize()) {
                LOG_ERROR("Failed to initialize database proxy");
                break;
            }
            object_exit.Push(std::bind(&agent::DatabaseProxy::Exit, &database_proxy));
            LOG_INFO("Initialized proxy manager successfully");

            agent::HandleManager &handlemsg = agent::HandleManager::Instance();
            if (!agent::g_enable_ || !handlemsg.Initialize()) {
                LOG_ERROR("Failed to initialize handlemsg manager");
                break;
            }
            object_exit.Push(std::bind(&agent::HandleManager::Exit, &handlemsg));
            LOG_INFO("Initialized handlemsg manager successfully");

            agent::WebServer &web_server = agent::WebServer::Instance();
            if (!agent::g_enable_ || !web_server.Initialize()) {
                LOG_ERROR("Failed to initialize web server");
                break;
            }
            object_exit.Push(std::bind(&agent::WebServer::Exit, &web_server));
            LOG_INFO("Initialized web server successfully");

            RunLoop();

            LOG_INFO("Process begins to quit...");
			
		} while (false);
	}
    agent::WebServer::ExitInstance();
    agent::HandleManager::ExitInstance();
    agent::Configure::ExitInstance();
    agent::DatabaseProxy::ExitInstance();
    utils::Timer::ExitInstance();
    utils::Logger::ExitInstance();
	utils::Daemon::ExitInstance();

    google::protobuf::ShutdownProtobufLibrary();
    // printf("process exit\n");
}

void RunLoop(){
	int64_t check_module_interval = 5 * utils::MICRO_UNITS_PER_SEC;
	int64_t last_check_module = 0;
	while (agent::g_enable_){
        int64_t current_time = utils::Timestamp::HighResolution();
        
        for(auto item : agent::TimerNotify::notifys_){
            item->TimerWrapper(utils::Timestamp::HighResolution());
            if(item->IsExpire(utils::MICRO_UNITS_PER_SEC)){
                LOG_WARN("The execution time(" FMT_I64 " us) for the timer(%s) is expired after 1s elapses", item->GetLastExecuteTime(), item->GetTimerName().c_str());
            }
        }
        utils::Timer::Instance().OnTimer(current_time);
		utils::Logger::Instance().CheckExpiredLog();

		utils::Sleep(1);
	}
}


