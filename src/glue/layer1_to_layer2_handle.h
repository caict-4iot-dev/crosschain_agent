#ifndef LAYER1_TO_LAYER2_HANDLE_H_
#define LAYER1_TO_LAYER2_HANDLE_H_

#include "proto/cpp/subscribe.pb.h"
#include <proto/cpp/agent.pb.h>
#include <common/general.h>
#include <utils/thread.h>


namespace agent {
    class Layer1ToLayer2Handle{
	public:
        Layer1ToLayer2Handle();
		~Layer1ToLayer2Handle(){};
    public:
        bool Initialize();
        bool HandleStartTxEvent(const std::string& cross_txid); //L1->L2
        bool HandleSendTxEvent(const std::string& cross_txid);  //l1->l2
        bool HandleSendAckdTxEvent(const std::string& cross_txid);//L1->L2
        void OnTimer(int64_t current_time);
    private: 
        bool SendTxToLayer2(protocol::CrossTxInfo cross_tx_info);
        bool SendAckToLayer1(protocol::CrossTxInfo cross_tx_info);
        bool ReplyAckToLayer2(protocol::CrossTxInfo cross_tx_info);
        
        bool SendTxToLayer2TimeOut();
        bool SendAckToLayer1TimeOut();
        bool ReplyAckToLayer2TimeOut();
       
    private:
        bool SaveSendTxInfo(CrossTxInfoPtr cross_tx_info);
        bool SaveSendAckInfo(CrossTxInfoPtr cross_tx_info);
        bool SaveReplyAckInfo(CrossTxInfoPtr cross_tx_info);
        bool makeCrossTxInfo(protocol::CrossTxInfo &cross_tx_info,const std::string cross_info1, const std::string cross_info2);
        bool ClearSendTxInfo(const std::string cross_txid);
        bool ClearSendAckInfo(const std::string cross_txid);
        bool ClearReplyAckInfo(const std::string cross_txid);
        bool handleSendTxEvent(const std::string& cross_txid,int64_t status);
    private:
        bool sendTransactionToLayer2(std::string input, int64_t amount,std::string& txhash);
        bool sendTransactionToLayer1(std::string input, int64_t amount,std::string& txhash);
        bool checkTxResponse(std::string response,std::string& txhash);

    private:
        utils::Mutex send_tx_info_lock_;
        SendTxInfoMap send_tx_info_map_; //sendtx to l2 success or failed 

        utils::Mutex send_ack_info_lock_;
        SendAckInfoMap send_ack_info_map_;  //send ack to l1 success or failed

        utils::Mutex reply_ack_info_lock_;
        SendAckInfoMap reply_ack_info_map_; //send ack to l2 success or failed

        std::string send_tx_prefix_;
        std::string send_ack_prefix_;
        std::string reply_ack_prefix_;
    };

    typedef std::shared_ptr<Layer1ToLayer2Handle> Layer1ToLayer2HandlePtr;
};  // namespace agent
#endif