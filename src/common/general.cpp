#include <utils/headers.h>
#include <utils/sm3.h>
#include "general.h"
#include <proto/cpp/common.pb.h>
namespace agent {
    AddressPrefix::AddressPrefix() {
		chain_code_ = "";
		address_prefix_ = "did:bid:";
	}
	AddressPrefix::~AddressPrefix() {}

	bool AddressPrefix::Initialize() {
		chain_code_ = "";
		address_prefix_ = "did:bid:";
		return true;
	}

	std::string AddressPrefix::Encode(const std::string &address) const {
		std::string temp_addr = address_prefix_;
		if (chain_code_.empty()){
			temp_addr.append(address);
		} else{
			temp_addr = utils::String::AppendFormat(temp_addr, "%s:%s", chain_code_.c_str(), address.c_str());
		}
		
		return temp_addr;
	}

	std::string AddressPrefix::Decode(const std::string &address) const {
		std::string temp_addr = address;
		size_t pos = temp_addr.rfind(":");
		if (pos != std::string::npos) {
			temp_addr = temp_addr.substr(pos + 1);
		} 

		return temp_addr;
	}

	bool AddressPrefix::CheckPrefix(const std::string &address) const {

		std::string temp_addr = address;
		std::string prefix;
		do {
			size_t pos = temp_addr.rfind(":");
			if (pos == std::string::npos) {
				break;
			}

			prefix = temp_addr.substr(0, pos+1);

			std::string local_prefix = address_prefix_;
			if (!chain_code_.empty()) {
				local_prefix.append(chain_code_);
				local_prefix.append(":");
			} 

			return prefix == local_prefix;
		} while (false);

		return false;
	}

    Result::Result(){
		code_ = protocol::ERRCODE_SUCCESS;
	}

	Result::Result(const Result &result) {
		code_ = result.code_;
		desc_ = result.desc_;
	}

	Result::~Result(){};

	int32_t Result::code() const{
		return code_;
	}

	std::string Result::desc() const{
		return desc_;
	}


	void Result::set_code(int32_t code){
		code_ = code;
	}

	void Result::set_desc(const std::string desc){
		desc_ = desc;
	}

	bool Result::operator=(const Result &result){
		code_ = result.code();
		desc_ = result.desc();
		return true;
	}
    std::list<TimerNotify *> TimerNotify::notifys_;

    CrossTx::CrossTx(protocol::CrossTxInfo cross_tx_info){
        cross_tx_info_ = cross_tx_info.SerializeAsString();
        cross_tx_id_ = cross_tx_info.crosstxid();
        status_ = cross_tx_info.status();
        confirm_seq_ = cross_tx_info.confirmseq();
        send_tx_timeout_ = cross_tx_info.timeout();
        send_tx_hash_ = cross_tx_info.sendtxhash();
        send_ack_to_l1_hash_ = cross_tx_info.sendacktol1hash();
        send_ack_to_l2_hash_ = cross_tx_info.sendacktol2hash();
    }
}