#ifndef LAYER2_MSG_HANDLE_H_
#define LAYER2_MSG_HANDLE_H_

#include <proto/cpp/agent.pb.h>
#include <common/general.h>
#include <utils/thread.h>


namespace agent {

	class Layer2ToLayer1Handle { 
	public:
        Layer2ToLayer1Handle();
		~Layer2ToLayer1Handle(){};
    public:
        bool Initialize();
        bool HandleStartTxEvent(const std::string& cross_txid); //L2->L1
        bool HandleSendTxEvent(const std::string& cross_txid); //L2->L1
        bool HandleSendAckdTxEvent(const std::string& cross_txid);//L2->L1
        bool HandleConfirmSeqEvent(const int64_t confirm_seq);
        void OnTimer(int64_t current_time);

    private:
        bool SendTxToLayer1(protocol::CrossTxInfo cross_tx_info);
        bool SendAckToLayer2(protocol::CrossTxInfo cross_tx_info);
        bool ReplyAckToLayer1(protocol::CrossTxInfo cross_tx_info);
        
        bool StartTxTimeOut();
        bool SendTxToLayer1TimeOut();
        bool SendAckToLayer2TimeOut();
        bool ReplyAckToLayer1TimeOut();
    private:
        bool SaveStartTxInfo(CrossTxInfoPtr cross_tx_info);
        bool SaveSendTxInfo(CrossTxInfoPtr cross_tx_info);
        bool SaveSendAckInfo(CrossTxInfoPtr cross_tx_info);
        bool SaveReplyAckInfo(CrossTxInfoPtr cross_tx_info);

        bool makeCrossTxInfo(protocol::CrossTxInfo &cross_tx_info,const std::string cross_info1, const std::string cross_info2);
        
        bool ClearStartTxInfo(const std::string cross_txid);
        bool ClearSendTxInfo(const std::string cross_txid);
        bool ClearSendAckInfo(const std::string cross_txid);
        bool ClearReplyAckInfo(const std::string cross_txid);
        bool handleSendTxEvent(const std::string& cross_txid, int64_t suatus);

    private:
        bool sendTransactionToLayer2(std::string input, int64_t amount,std::string& txhash);
        bool sendTransactionToLayer1(std::string input, int64_t amount,std::string& txhash);
        bool checkTxResponse(std::string response,std::string& txhash);

    private:
        utils::Mutex start_tx_info_lock_;
        StartTxInfoMap start_tx_info_map_; //sendtx to l1 success or failed 

        utils::Mutex send_tx_info_lock_;
        SendTxInfoMap send_tx_info_map_; //sendtx to l1 success or failed 

        utils::Mutex send_ack_info_lock_;
        SendAckInfoMap send_ack_info_map_;  //send ack to l2 success or failed

        utils::Mutex reply_ack_info_lock_;
        SendAckInfoMap reply_ack_info_map_; //send ack to l1 success or failed

        std::string start_tx_prefix_;
        std::string send_tx_prefix_;
        std::string send_ack_prefix_;
        std::string reply_ack_prefix_;
    };

    typedef std::shared_ptr<Layer2ToLayer1Handle> Layer2ToLayer1HandlePtr;
    };  // namespace agent
#endif