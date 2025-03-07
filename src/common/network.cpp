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

#include <utils/timestamp.h>
#include <utils/logger.h>
#include "network.h"
#include <system_error>

#define OVERLAY_PING 1
namespace agent {

	Connection::Connection(SocketServer *server_h, SocketClient *client_h,
		int64_t hdl, const std::string &uri, int64_t id) :
		server_(server_h),
		client_(client_h),
		handle_(hdl),
		uri_(uri), 
		id_(id), 
		sequence_(0){
		connect_start_time_ = 0;
		connect_end_time_ = 0;
		last_receive_time_ = 0;
		last_send_time_ = 0;
		delay_ = 0;

		std::error_code ec;
		last_receive_time_ = connect_start_time_ = utils::Timestamp::HighResolution();
		if (server_){
			in_bound_ = true;
			connect_end_time_ = connect_start_time_;
			
			SocketConnection * con = server_->GetConnectFromHandle(handle_);
			if (con) peer_address_ = con->GetConnRemoteAddr();

		}
		else {
			in_bound_ = false;
			if (client_) {
				SocketConnection *con = client_->GetConnectFromHandle(handle_);
				if (con) peer_address_ = con->GetConnRemoteAddr();
			}
		}
	}
	Connection::~Connection() {}

	utils::InetAddress Connection::GetPeerAddress() const {
		return peer_address_;
	}

	void Connection::TouchReceiveTime() {
		last_receive_time_ = utils::Timestamp::HighResolution();
	}

	bool Connection::NeedPing(int64_t interval) {
		return connect_end_time_ > 0 && utils::Timestamp::HighResolution() - last_send_time_ > interval;
	}

	void Connection::SetConnectTime() {
		connect_end_time_ = utils::Timestamp::HighResolution();
		last_receive_time_ = connect_end_time_;
	}

	void Connection::SetDelay() {
		delay_ = (utils::Timestamp::HighResolution() - last_send_time_) / 2;
	}

	int64_t Connection::GetDelay() const {
		return delay_;
	}

	int64_t Connection::GetId() const{
		return id_;
	}

	int64_t Connection::GetHandle() const {
		return handle_;
	}

	Result Connection::GetErrorCode() const {
		Result ret;
		std::error_code ec;
		if (in_bound_) {
			if (server_) {
				SocketConnection *con = server_->GetConnectFromHandle(handle_);
				if (!con) {
					ret = con->GetError();
				}
			}
		}
		else {
			if (client_) {
				SocketConnection *con = client_->GetConnectFromHandle(handle_);
				if (!con) {
					ret = con->GetError();
				}
			}
		}

		return ret;
	}

	bool Connection::InBound() const {
		return in_bound_;
	}

	bool Connection::SendByteMessage(const std::string &message, std::error_code &ec) {
		//if (message.size() < 150 && message.size() > 90) {
		//	return true;
		//}
		std::error_code ec1;
		Result ret;
        int retry = 3;
        int try_i = 0;
        do {
            try_i++;
	    	if (in_bound_){
	    		if (server_) {
	    			server_->Send(handle_, message, ret);
	    		}
	    	} else{
	    		if (client_) {
	    			client_->Send(handle_, message, ret);
	    		}
	    	}
        } while (try_i < retry && ret.code() != 0);
        LOG_TRACE("TEST SendByteMessage ec1:%d  ret:%d retry:%d try_i:%d", ec1.value(), ret.code(), retry, try_i);
		if (ret.code() == 0) {
			return true;
		} else{
			LOG_ERROR("Failed to send msg ec:%d", ret.code());
			return false;
		}
	}

	bool Connection::Ping(std::error_code &ec) {
        LOG_INFO("Connection::Ping");
        do {
			std::error_code ec1;
			std::string payload = utils::String::Format("%s - %d", utils::Timestamp::Now().ToFormatString(true).c_str(), rand());
			Result ret;
			if (in_bound_) {
				if (server_) {
					SocketConnection *conn = server_->GetConnectFromHandle(handle_);
					if (!conn) break;
					conn->Ping(payload, ret);
				}
			}
			else {
				if (client_) {
					SocketConnection* conn = client_->GetConnectFromHandle(handle_);
					if (!conn) break;
					conn->Ping(payload, ret);
				}
			}

			last_send_time_ = utils::Timestamp::HighResolution();
		} while (false);

		return ec.value() == 0;
	}

	bool Connection::PingCustom(std::error_code &ec) {
		protocol::Ping ping;
		ping.set_nonce(utils::Timestamp::HighResolution());
		bool ret = SendRequest(OVERLAY_PING, ping.SerializeAsString(), ec);
		last_send_time_ = utils::Timestamp::HighResolution();
		return !ec;
	}

	bool Connection::SendMsg(int64_t type, bool request, int64_t sequence, const std::string &data, std::error_code &ec) {
		protocol::WsMessage message;
		message.set_type(type);
		message.set_request(request);
		message.set_sequence(sequence);
		message.set_data(data);
		return SendByteMessage(message.SerializeAsString(), ec);
	}

	bool Connection::SendRequest(int64_t type, const std::string &data, std::error_code &ec) {
		protocol::WsMessage message;
		message.set_type(type);
		message.set_request(true);
		message.set_sequence(sequence_++);
		message.set_data(data);

		return SendByteMessage(message.SerializeAsString(), ec);
	}

	bool Connection::SendResponse(const protocol::WsMessage &req_message, const std::string &data, std::error_code &ec) {
		return SendMsg(req_message.type(), false, req_message.sequence(), data, ec);
	}

	bool Connection::Close(const std::string &reason) {
		std::error_code ec1;
		Result ret;
		if (in_bound_) {
			if (server_) {
				server_->PauseReading(handle_, ret);
				server_->Close(handle_, reason, ret);
			}
		} else{
			if (client_) {
				client_->Close(handle_, reason, ret);
			}
		}

		return ec1.value() == 0;
	}

	bool Connection::IsConnectExpired(int64_t time_out) const {
		return connect_end_time_ == 0 &&
			utils::Timestamp::HighResolution() - connect_start_time_ > time_out &&
			!in_bound_;
	}

	bool Connection::IsDataExpired(int64_t time_out) const {
		return connect_end_time_ > 0 && utils::Timestamp::HighResolution() - last_receive_time_ > time_out;
	}

	void Connection::ToJson(Json::Value &status) const {
		status["id"] = id_;
		status["in_bound"] = in_bound_;
		status["peer_address"] = GetPeerAddress().ToIpPort();
		status["last_receive_time"] = last_receive_time_;
	}

	bool Connection::OnNetworkTimer(int64_t current_time) { return true; }

	Network::Network() : next_id_(0), enabled_(false), thread_inited_(false){
		last_check_time_ = 0;
		connect_time_out_ = 60 * utils::MICRO_UNITS_PER_SEC;
		std::error_code err;
		io_ = new SocketIo(4);
		server_ = new WssSocketServer();
		client_ = new WssSocketClient();
		
		server_->Init(io_);
		server_->SetReuseAddr(true);
		server_->SetOpenHandle(bind(&Network::OnOpen, this, _1));
		server_->SetCloseHandle(bind(&Network::OnClose, this, _1));
		server_->SetFailedHandle(bind(&Network::OnFailed, this, _1));
		server_->SetMsgHandle(bind(&Network::OnMessage, this, _1, _2));
		server_->SetPongHandle(bind(&Network::OnPong, this, _1, _2));
        client_->Init(io_);

		if (err.value() != 0){
			LOG_ERROR_ERRNO("Failed to initiate websocket network", err.value(), err.message().c_str());
		}

		//Register function
		request_methods_[OVERLAY_PING] = std::bind(&Network::OnRequestPing, this, std::placeholders::_1, std::placeholders::_2);
		response_methods_[OVERLAY_PING] = std::bind(&Network::OnResponsePing, this, std::placeholders::_1, std::placeholders::_2);
	}

	Network::~Network() {
		for (ConnectionMap::iterator iter = connections_.begin();
			iter != connections_.end();
			iter++) {
			iter->second = nullptr;
		}

		for (ConnectionMap::iterator iter = connections_delete_.begin();
			iter != connections_delete_.end();
			iter++) {
			iter->second = nullptr;
		}

		if (server_) delete server_;
		if (client_) delete client_;
		if (io_) delete io_;
	}

	void Network::OnOpen(int64_t hdl) {
		utils::MutexGuard guard_(conns_list_lock_);
		int64_t new_id = next_id_++;
		auto conn = CreateConnectObject(server_, NULL, hdl, "", new_id);
		connections_.insert(std::make_pair(new_id, conn));
		connection_handles_.insert(std::make_pair(hdl, new_id));

		LOG_INFO("Accepted a new connection, ip(%s), network id:" FMT_I64 " hdl:" FMT_I64, conn->GetPeerAddress().ToIpPort().c_str(), new_id, hdl);

		if (!OnConnectOpen(conn)) { //delete
			conn->Close("connections exceed");
			RemoveConnection(conn);
		}
	}

	void Network::OnClose(int64_t hdl) {
		utils::MutexGuard guard_(conns_list_lock_);
		std::shared_ptr<Connection>conn = GetConnectionFromHandle(hdl);
		if (conn) {
			LOG_INFO("Closed a connection, ip(%s)", conn->GetPeerAddress().ToIpPort().c_str());
			OnDisconnect(conn);
			RemoveConnection(conn);
		} 
	}

	void Network::OnFailed(int64_t hdl) {
		utils::MutexGuard guard_(conns_list_lock_);
		std::shared_ptr<Connection>conn = GetConnectionFromHandle(hdl);
		if (conn) {
			LOG_ERROR("Got a network failed event, ip(%s), error desc(%s)", conn->GetPeerAddress().ToIpPort().c_str(), conn->GetErrorCode().desc().c_str());
			OnDisconnect(conn);
			RemoveConnection(conn);
		}
	}

	void Network::OnMessage(int64_t hdl, const std::string &msg) {
		protocol::WsMessage message;
		try {
			message.ParseFromString(msg);
		}
		catch (std::exception const e) {
			LOG_ERROR("Failed to parse websocket message (%s)", e.what());
			return;
		}
        LOG_INFO("Received a message, type:%d",message.type());
		int64_t conn_id = -1;
		do {
			utils::MutexGuard guard(conns_list_lock_);
			std::shared_ptr<Connection>conn = GetConnectionFromHandle(hdl);
			if (!conn) { 
				LOG_ERROR("hdl " FMT_I64 " Not found", hdl);
				return; 
			}

			conn->TouchReceiveTime();
			conn_id = conn->GetId();
		} while (false);

		do {
           
            MessageConnPoc proc;
			if (message.request()) {
				MessageConnPocMap::iterator iter = request_methods_.find(message.type());
				if (iter == request_methods_.end()) {
                    LOG_INFO("Request method not found, type:%d,conn_id is %ld",message.type(),conn_id);
                    break; 
                } // methond not found, break;
				proc = iter->second;
			} else{
				MessageConnPocMap::iterator iter = response_methods_.find(message.type());
				if (iter == response_methods_.end()) { 
                    LOG_INFO("Response method not found, type:%d,conn_id is %ld",message.type(),conn_id);
                    break; 
                } // methond not found, break;
				proc = iter->second;
			}

			if (proc(message, conn_id)) {
                LOG_INFO("Request method success, type:%d,conn_id is %ld",message.type(),conn_id);
                break; //Break if returned true;
            }
            

			LOG_ERROR("Failed to process message, the method type (" FMT_I64 ") (%s) handles exceptions, need to delete it here",
				message.type(), message.request() ? "true" : "false");
			// Delete the connection if returned false.
			do {
				utils::MutexGuard guard(conns_list_lock_);
				std::shared_ptr<Connection>conn = GetConnectionFromHandle(hdl);
				if (!conn) {
					LOG_ERROR("Failed to process network message. Handle not found");
					break;  //Not found
				}
				OnDisconnect(conn);
			} while (false);

			RemoveConnection(conn_id);
		} while (false);
	}

	void Network::Stop() {
		enabled_ = false;
	}

	bool Network::ThreadStarted() {
		//waiting for finish single of network module
		while (!thread_inited_) {
			utils::Sleep(100);
		}

		return enabled_;
	}
	
		//test code
	class IoThread : public utils::Thread {
	public :
		asio::io_service *p_io_service_;
		IoThread(asio::io_service *p_io_service) : p_io_service_(p_io_service) {};
		void Run() {
			p_io_service_->run();
		}
	};

	void Network::Start(const utils::InetAddress &ip) {
		//try {
			if (!ip.IsNone()) {
				bool init_res = false;
				do {
					websocketpp::lib::error_code ec;
					Result err;
                    // Listen on a specified port.
					server_->Listen(ip, err);
					if (err.code() != 0) {
						LOG_ERROR("Failed to listen ip(%s), error(%d:%s)", ip.ToIpPort().c_str(), err.code(), err.desc().c_str());
						break;
					}
					// Start the TLS server.
					server_->StartAccept(err);
					if (err.code() != 0) {
						LOG_ERROR("Failed to accept ip(%s), error(%d:%s)", ip.ToIpPort().c_str(), err.code(), err.desc().c_str());
						break;
					}
					listen_port_ = server_->GetLocalPort();

					init_res = true;
					LOG_INFO("Network listen at ip(%s:%d)", ip.ToIp().c_str(), listen_port_);
				} while (false);

				if (!init_res){
					thread_inited_ = true;
					return;
				}
			}
			
			enabled_ = true;
			thread_inited_ = true;

			// Start the ASIO io_service run loop.

			io_->Create();
			int64_t last_check_time = 0;
			while (enabled_) {
				//io_.poll();

				utils::Sleep(10);

				int64_t now = utils::Timestamp::HighResolution();
				if (now - last_check_time > utils::MICRO_UNITS_PER_SEC) {

					utils::MutexGuard guard_(conns_list_lock_);
					//Ping the client to see if the connectin times out.
					std::list<std::shared_ptr<Connection>> delete_list;
					for (ConnectionMap::iterator iter = connections_.begin();
						iter != connections_.end();
						iter++) {

						if (iter->second->NeedPing(connect_time_out_ / 4)) {
							iter->second->PingCustom(ec_);
						}

						if (iter->second->IsDataExpired(connect_time_out_)) {
							iter->second->Close("expired");
							delete_list.push_back(iter->second);
							LOG_ERROR("Failed to process data by network module.Peer(%s) data receive timeout", iter->second->GetPeerAddress().ToIpPort().c_str());
						}

						//Check application timer.
						if (!iter->second->OnNetworkTimer(now)) {
							iter->second->Close("app error");
							delete_list.push_back(iter->second);
						} 
					}

					//Remove the current connection to delete array.
					for (std::list<std::shared_ptr<Connection>>::iterator iter = delete_list.begin();
						iter != delete_list.end();
						iter++) {
						LOG_INFO("Connection is closed as expired, ip(%s)", (*iter)->GetPeerAddress().ToIpPort().c_str());
						OnDisconnect(*iter);
						RemoveConnection(*iter);
					}

					//Check if the connections are deleted.
					for (ConnectionMap::iterator iter = connections_delete_.begin();
						iter != connections_delete_.end();) {
						if (iter->first < now) {
							iter = connections_delete_.erase(iter);
						}
						else {
							iter++;
						}
					}

					last_check_time = now;
				}
			}
	//	}
		//catch (const std::exception & e) {
		//	LOG_ERROR("%s", e.what());
		//}
		io_->Close();
		enabled_ = false;
		LOG_INFO("Network listen server(%s) has exited", ip.ToIpPort().c_str());
	}
	
	uint16_t Network::GetListenPort() const {
		return listen_port_;
	}

	bool Network::Connect(const std::string &uri) {
		websocketpp::lib::error_code ec;
		Result ret;

		client::connection_ptr con = NULL;
		int64_t handle;
		 {
			SocketConnection* con = client_->GetConnect(uri, ret);
			if (con) {
				con->SetOpenHandle(bind(&Network::OnClientOpen, this, _1));
				con->SetCloseHandle(bind(&Network::OnClose, this, _1));
				con->SetMsgHandle(bind(&Network::OnMessage, this, _1, _2));
				con->SetFailedHandle(bind(&Network::OnFailed, this, _1));
				con->SetPongHandle(bind(&Network::OnPong, this, _1, _2));
				handle = con->GetHandle();
			}
			else {
				LOG_ERROR("Failed to connect network.Url(%s), error(%s)", uri.c_str(), ec.message().c_str());
				return false;
			}
		}

		if (ec) {
			LOG_ERROR("Failed to connect uri(%s), error(%s)", uri.c_str(), ec.message().c_str());
			return false;
		}

		utils::MutexGuard guard_(conns_list_lock_);
		int64_t new_id = next_id_++;
		std::shared_ptr<Connection> peer = CreateConnectObject(NULL, client_, handle, uri, new_id);
		connections_.insert(std::make_pair(new_id, peer));
		connection_handles_.insert(std::make_pair(handle, new_id));
        client_->Connect(handle);

		LOG_INFO("Connecting uri(%s), network id(" FMT_I64 "), hdl:" FMT_I64, uri.c_str(), new_id, handle);
		return true;
	}

	std::shared_ptr<Connection> Network::GetConnection(int64_t id) {
		ConnectionMap::iterator iter = connections_.find(id);
		if (iter != connections_.end()){
			return iter->second;
		}

		return NULL;
	}

	std::shared_ptr<Connection> Network::GetConnectionFromHandle(int64_t hdl) {
		ConnectHandleToIdMap::iterator iter = connection_handles_.find(hdl);
		if (iter == connection_handles_.end()) {
			return NULL;
		}

		return GetConnection(iter->second);
	}

	void Network::RemoveConnection(int64_t conn_id) {
		utils::MutexGuard guard(conns_list_lock_);
		std::shared_ptr<Connection> conn = GetConnection(conn_id);
		if(conn) RemoveConnection(conn);
	}

	void Network::RemoveConnection(std::shared_ptr<Connection> conn) {
		LOG_INFO("Remove connection id(" FMT_I64 "), peer ip(%s)", conn->GetId(), conn->GetPeerAddress().ToIpPort().c_str());
		conn->Close("no reason");
        connections_delete_.insert(std::make_pair(utils::Timestamp::HighResolution() + 5 * utils::MICRO_UNITS_PER_SEC,
			conn));
		connections_.erase(conn->GetId());
		connection_handles_.erase(conn->GetHandle());
	}

	void Network::OnClientOpen(int64_t hdl) {
		utils::MutexGuard guard_(conns_list_lock_);
		std::shared_ptr<Connection> conn = GetConnectionFromHandle(hdl);
		if (conn) {
			LOG_INFO("Peer is connected, ip(%s), hdl(" FMT_I64 ")", conn->GetPeerAddress().ToIpPort().c_str(), hdl);
			conn->SetConnectTime();

			if (!OnConnectOpen(conn)) { //delete
				conn->Close("no reason");
				RemoveConnection(conn);
			} 
			//conn->Ping(ec_);
		}
	}

	void Network::OnPong(int64_t hdl, std::string payload) {
		std::shared_ptr<Connection>peer = GetConnectionFromHandle(hdl);
		if (peer){
			peer->TouchReceiveTime();
			LOG_INFO("Recv pong, payload(%s) from ip(%s)", payload.c_str(), peer->GetPeerAddress().ToIpPort().c_str());
		} 
	}

	bool Network::OnRequestPing(protocol::WsMessage &message, int64_t conn_id) {
		protocol::Ping ping;
		if (!ping.ParseFromString(message.data())){
			LOG_ERROR("Failed to parse ping");
			return false;
		}

		utils::MutexGuard guard_(conns_list_lock_);
		std::shared_ptr<Connection>con = GetConnection(conn_id);
		if (!con) {
			LOG_ERROR("Failed to get connection by id(" FMT_I64 ")", conn_id);
			return false;
		} 

		protocol::Pong pong;
		pong.set_nonce(ping.nonce());
		return con->SendResponse(message, pong.SerializeAsString(), ec_);
	}

	bool Network::OnResponsePing(protocol::WsMessage &message, int64_t conn_id) {
		protocol::Pong pong;
		if (!pong.ParseFromString(message.data())) {
			LOG_ERROR("Failed to parse pong");
			return false;
		}
        utils::MutexGuard guard_(conns_list_lock_);
		std::shared_ptr<Connection>conn = GetConnection(conn_id);
		if (conn) {
			conn->TouchReceiveTime();
			conn->SetDelay();
		}

		return true;
	}

	std::shared_ptr<Connection> Network::CreateConnectObject(SocketServer *server_h, SocketClient *client_h,
		int64_t hdl, const std::string &uri, int64_t id) {
		return  std::make_shared<Connection>(server_h, client_h,hdl, uri, id);
	}

}
