// SPDX-License-Identifier: MIT
pragma solidity ^0.8.0;
contract L1CrossChainBridge {
    constructor(uint32 _chainId, uint256 _timeout) {
        owner_ = msg.sender;
        chainId_ = _chainId;
        timeout_ = _timeout;
    }

    enum TxStatusEnum {
        INIT,
        ACK_SUCCESS,
        ACK_FAIL,
        ACK_TIMEOUT
    }

    enum TxRefundedEnum {
        NONE,
        TODO,
        REFUNDED
    }
    
    event StartTxEvent(bytes32 cross_tx_id);
    event SendTxEvent(bytes32 cross_tx_id);
    event SendAckedTxEvent(bytes32 cross_tx_id);
    event RefundEvent(bytes32 cross_tx_id);
    event SetOwnerEvent(address new_owner);
    event SetTimeoutEvent(uint256 new_timeout);

    address internal owner_;
    uint32 internal chainId_;
    uint256 internal timeout_;
    mapping(bytes => CrossTxInfo) internal crossTxInfo_;

    struct CrossTxInfo {
        bytes crossTxId;
        uint32 srcChainId;
        uint32 destChainId;
        address srcBid;
        address destBid;
        uint256 amount;
        TxStatusEnum status;
        TxRefundedEnum refunded;
        string errorDesc;
        uint256 createTimestamp;
        uint256 executeTimestamp;
        uint256 timeout;
    }

    modifier onlyOwner() {
        require(
            msg.sender == owner_,
            "This function is restricted to the contract's owner"
        );
        _;
    }

    function SetTimeout(uint256 _timeout) public onlyOwner {
        require(_timeout > 30*60*1000*1000 && _timeout < 12*3600*1000*1000, "SetTimeout: timeout must be greater than 30min and smaller than 12h");
        timeout_ = _timeout;
        emit SetTimeoutEvent(timeout_);
    }

    function SetOwner(address _owner) public onlyOwner {
        require(_owner != address(0), "SetOwner: _owner is invalid");
        require(_owner != owner_, "SetOwner: _owner and owner_ are equal");
        owner_ = _owner;
        emit SetOwnerEvent(owner_);
    }

    function StartTx(address src_bid, address dest_bid, uint256 amount) public payable {
        require(src_bid != address(0) && dest_bid != address(0), "StartTx: src_bid or dest_bid is invalid");
        require(src_bid == msg.sender, "StartTx: src_bid and msg.sender are not equal");
        require(amount == msg.value, "StartTx: amount and msg.value are not equal");

        bytes32 txId = sha256(abi.encode(src_bid, dest_bid, 0, chainId_, amount, block.timestamp));
        bytes memory myBytes = bytes32Tobytes(txId);
        CrossTxInfo memory crossTxInfo;
        crossTxInfo.srcChainId = 0;
        crossTxInfo.destChainId = chainId_;
        crossTxInfo.srcBid = src_bid;
        crossTxInfo.destBid = dest_bid;
        crossTxInfo.amount = amount;
        crossTxInfo.crossTxId = myBytes;
        crossTxInfo.status = TxStatusEnum.INIT;
        crossTxInfo.createTimestamp = block.timestamp;
        crossTxInfo.timeout = block.timestamp + timeout_;

        crossTxInfo_[myBytes] = crossTxInfo;
        emit StartTxEvent(txId);
    }

    function SendTx(bytes memory cross_tx_id, address src_bid, address dest_bid, uint256 amount, uint256 timeout) public payable onlyOwner {
        require(cross_tx_id.length == 32, "SendTx: invalid cross_tx_id");
        require(src_bid != address(0) && dest_bid != address(0), "SendTx: src_bid or dest_bid is invalid");
        require(amount == msg.value, "SendTx: amount and msg.value are not equal");
        require(timeout >= block.timestamp, "SendTx: timeout must be greater than current timestamp");

        CrossTxInfo memory crossTxInfo = crossTxInfo_[cross_tx_id];
        require(crossTxInfo.crossTxId.length == 0, "SendTx: cross_tx_id already exists");

        crossTxInfo.crossTxId = cross_tx_id;
        crossTxInfo.srcChainId = chainId_;
        crossTxInfo.destChainId = 0;
        crossTxInfo.srcBid = src_bid;
        crossTxInfo.destBid = dest_bid;
        crossTxInfo.amount = amount;
        crossTxInfo.executeTimestamp = block.timestamp;
        crossTxInfo.timeout = timeout;

        crossTxInfo_[cross_tx_id] = crossTxInfo;
        bytes32 tx_id_bytes32 = bytes32(cross_tx_id);
        emit SendTxEvent(tx_id_bytes32);
    }

    function SendAckedTx(bytes memory cross_tx_id, uint8 result, string memory error_desc, uint256 execute_time) public onlyOwner {
        require(cross_tx_id.length == 32, "SendAckedTx: invalid cross_tx_id");
        CrossTxInfo memory crossTxInfo = crossTxInfo_[cross_tx_id];
        require(crossTxInfo.crossTxId.length > 0, "SendAckedTx: cross_tx_id is not exists");
        require(crossTxInfo.status == TxStatusEnum.INIT, "SendAckedTx: cross_tx_id status is not INIT");
        require(result == 1 || result == 2 || result == 3, "SendAckedTx: result must be 1 or 2 or 3");
        require(bytes(error_desc).length < 2048, "SendAckedTx: error_desc length must be less than 2048");

        crossTxInfo.status = TxStatusEnum(result);
        if (result == 1) {
            require(execute_time < crossTxInfo.timeout && execute_time > crossTxInfo.createTimestamp, 
            "SendAckedTx: execute_time must be greater than createTimestamp and less than timeout");
            crossTxInfo.executeTimestamp = execute_time;
            if (crossTxInfo.srcChainId == 0) {
                address payable temp = payable(msg.sender);
                temp.transfer(crossTxInfo.amount);
                crossTxInfo_[cross_tx_id] = crossTxInfo;
                bytes32 tx_id_bytes32 = bytes32(cross_tx_id);
                emit SendAckedTxEvent(tx_id_bytes32);
            } else {
                address payable temp = payable(crossTxInfo.destBid);
                temp.transfer(crossTxInfo.amount);
                crossTxInfo_[cross_tx_id] = crossTxInfo;
            }
        } else if (result == 2) {
            crossTxInfo.refunded = TxRefundedEnum.TODO;
            crossTxInfo.errorDesc = error_desc;
            crossTxInfo_[cross_tx_id] = crossTxInfo;
        } else {
            crossTxInfo.refunded = TxRefundedEnum.TODO;
            crossTxInfo.errorDesc = "timeout";
            crossTxInfo_[cross_tx_id] = crossTxInfo;
        }
    }

    function Refund(bytes memory cross_tx_id) public {
        require(cross_tx_id.length == 32, "Refund: invalid cross_tx_id");
        CrossTxInfo memory crossTxInfo = crossTxInfo_[cross_tx_id];
        require(crossTxInfo.crossTxId.length > 0, "Refund: cross_tx_id is not exists");
        require(crossTxInfo.refunded == TxRefundedEnum.TODO, "Refund: cross_tx_id refunded is not TODO");
        require(crossTxInfo.srcBid == msg.sender, "Refund: has not permission to refund");

        address payable temp = payable(crossTxInfo.srcBid);
        temp.transfer(crossTxInfo.amount);
        crossTxInfo.refunded = TxRefundedEnum.REFUNDED;
        crossTxInfo_[cross_tx_id] = crossTxInfo;
        bytes32 tx_id_bytes32 = bytes32(cross_tx_id);
        emit RefundEvent(tx_id_bytes32);
    }

    function GetCrossTxInfo1(bytes memory cross_tx_id) public view returns (uint32 src_chain_id, uint32 dest_chain_id, bytes memory cross_tx_id_, uint256 amount_, address src_bid, address dest_bid) {
        return (crossTxInfo_[cross_tx_id].srcChainId, crossTxInfo_[cross_tx_id].destChainId, crossTxInfo_[cross_tx_id].crossTxId, crossTxInfo_[cross_tx_id].amount, crossTxInfo_[cross_tx_id].srcBid, crossTxInfo_[cross_tx_id].destBid);
    }

    function GetCrossTxInfo2(bytes memory cross_tx_id) public view returns (uint8 tx_status, uint8 tx_refunded, uint256 create_timestamp, uint256 execute_timestamp, uint256 timeout, string memory error_desc) {
        return (uint8(crossTxInfo_[cross_tx_id].status), uint8(crossTxInfo_[cross_tx_id].refunded), crossTxInfo_[cross_tx_id].createTimestamp, crossTxInfo_[cross_tx_id].executeTimestamp, crossTxInfo_[cross_tx_id].timeout, crossTxInfo_[cross_tx_id].errorDesc);
    }

    function bytes32Tobytes(bytes32 bytes32_) internal pure returns (bytes memory bytes_) {
        bytes memory myBytes = new bytes(32);  
        for (uint i = 0; i < 32; i++) {  
            myBytes[i] = bytes32_[i];  
        }   
        return myBytes;
    }
}
