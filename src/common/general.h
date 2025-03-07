#ifndef ADDRESS_PREFIX_H_
#define ADDRESS_PREFIX_H_
#include <utils/singleton.h>
#include <string>
#include <utils/timestamp.h>
#include <proto/cpp/agent.pb.h>
namespace agent {
    class AddressPrefix : public utils::Singleton<agent::AddressPrefix> {
		std::string chain_code_;
		std::string address_prefix_;

	public:
		AddressPrefix();
		~AddressPrefix();

		bool Initialize();
		void UpdatePrefix(const std::string &address_prefix) { address_prefix_ = address_prefix; };
		void UpdateChainCode(const std::string &chain_code) { chain_code_ = chain_code; };

		std::string chain_code() const { return chain_code_; };
		std::string address_prefix() const { return address_prefix_; }
		std::string Encode(const std::string &address) const;
		std::string Decode(const std::string &address) const;
		bool CheckPrefix(const std::string &address) const;
	};

    class Result {
		int32_t code_;
		std::string desc_;

	public:
		Result();
		Result(const Result &result);
		~Result();

		void Reset(){
			code_ = 0;
			desc_ = "";
		}

		int32_t code() const;
		std::string desc() const;

		void set_code(int32_t code);
		void set_desc(const std::string desc);

		bool operator=(const Result &result);
	};

    class TimerNotify {
	private:
		int64_t last_check_time_;
		int64_t last_slow_check_time_;

		int64_t check_interval_;

		int64_t last_execute_complete_time_;
		int64_t last_slow_execute_complete_time_;

	public:
		static std::list<TimerNotify *> notifys_;
		static bool RegisterModule(TimerNotify *module) { notifys_.push_back(module); return true; };

		TimerNotify(const std::string &timer_name, int64_t check_interval) :last_check_time_(0),
			last_slow_check_time_(0), 
			check_interval_(check_interval),
			last_execute_complete_time_(0),
			last_slow_execute_complete_time_(0),
			timer_name_(timer_name){};
		~TimerNotify() {};

		void TimerWrapper(int64_t current_time) {
			last_execute_complete_time_ = 0; //clear first
			if (current_time > last_check_time_ + check_interval_) {
				last_check_time_ = current_time;
				OnTimer(current_time);
				last_execute_complete_time_ = utils::Timestamp::HighResolution();
			}
		};

		void SlowTimerWrapper(int64_t current_time) {
			last_slow_execute_complete_time_ = 0;//clear first
			if (current_time > last_slow_check_time_ + check_interval_) {
				last_slow_check_time_ = current_time;
				OnSlowTimer(current_time);
				last_slow_execute_complete_time_ = utils::Timestamp::HighResolution();
			}
		};

		bool IsSlowExpire(int64_t time_out) {
			return last_slow_execute_complete_time_ - last_slow_check_time_ > time_out;
		}

		bool IsExpire(int64_t time_out) {
			return last_execute_complete_time_ - last_check_time_ > time_out;
		}

		int64_t GetSlowLastExecuteTime() {
			return last_slow_execute_complete_time_ - last_slow_check_time_;
		}

		int64_t GetLastExecuteTime() {
			return last_execute_complete_time_ - last_check_time_;
		}

		std::string GetTimerName() {
			return timer_name_;
		}

		virtual void OnTimer(int64_t current_time) = 0;
		virtual void OnSlowTimer(int64_t current_time) = 0;

	private:
		std::string timer_name_;
	};

    const int64_t CROSS_RESULT_INIT = 0;
    const int64_t CROSS_RESULT_SUCCESS = 1;
    const int64_t CROSS_RESULT_FAILED = 2;
    const int64_t CROSS_RESULT_TIMEOUT = 3;

    class CrossTx {
	public:
        CrossTx(protocol::CrossTxInfo cross_tx_info);
        CrossTx()=default;
		~CrossTx()=default;
    public:
        std::string cross_tx_id_;
        int64_t status_;
        int64_t confirm_seq_;
        int64_t send_tx_timeout_;
        int64_t send_ack_timeout_;
        std::string send_tx_hash_;
        std::string send_ack_to_l1_hash_;
        std::string send_ack_to_l2_hash_;
        std::string cross_tx_info_;
        
    };
    typedef std::shared_ptr<CrossTx> CrossTxInfoPtr;
    typedef std::map<std::string , CrossTxInfoPtr> StartTxInfoMap; //key:cross_txid
    typedef std::map<std::string , CrossTxInfoPtr> SendTxInfoMap; //key:cross_txid
    typedef std::map<std::string, CrossTxInfoPtr> SendAckInfoMap; //key:cross_txid

}
#endif 
    
    