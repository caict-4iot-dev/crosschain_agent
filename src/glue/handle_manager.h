#ifndef HANDLEMSG_MANAGER_
#define HANDLEMSG_MANAGER_

#include <utils/singleton.h>
#include <utils/thread.h>
#include <proto/cpp/subscribe.pb.h>
#include <proto/cpp/common.pb.h>
#include <proto/cpp/overlay.pb.h>
#include "layer1_to_layer2_handle.h"
#include "layer2_to_layer1_handle.h"
namespace agent {

	class HandleManager : public utils::Singleton < agent::HandleManager>,
    public TimerNotify{
		//std::shared_ptr<Consensus> consensus_;
		//std::list<ConsensusMsgPtr> pending_consensus_;

	public:
		//共识开始时间 FOR DEBUG
		int64_t consensus_start_time;
	public:
		HandleManager();
		~HandleManager();

		bool Initialize();
		bool Exit();

        bool OnLayer1SubscribeResponse(const std::string& msg);
        bool OnLayer2SubscribeResponse(const std::string& msg);
    private:
        bool OnLayer1CrossContractTlog(protocol::TlogSubscribeResponse &tlog_res);
        bool OnLayer1ManagerContractTlog(protocol::TlogSubscribeResponse &tlog_res);
        bool updatePeriodToLayer2CrossContract(int64_t period);
        bool CheckLayer1Balance();
        bool CheckLayer2Balance();
    public:
        virtual void OnTimer(int64_t current_time);
		virtual void OnSlowTimer(int64_t current_time){};
    private:
        Layer1ToLayer2HandlePtr layer1_to_layer2_handle_;
        Layer2ToLayer1HandlePtr layer2_to_layer1_handle_;

    };
};

#endif
