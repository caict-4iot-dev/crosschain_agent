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

#include "network_adapter.h"

namespace agent {

	class IoThread : public utils::Thread {
	public:
		asio::io_service *p_io_service_;
		IoThread(asio::io_service *p_io_service) : p_io_service_(p_io_service) {};
		void Run() {
			p_io_service_->run();
		}
	};

	SocketIo::SocketIo(int32_t thread_count) {
		thread_count_ = thread_count;
		work_ = NULL;
	}

	SocketIo::~SocketIo() {
		if (work_) {
			delete work_;
		}
	}

	bool SocketIo::Create() {
		for (int32_t i = 0; i < thread_count_; i++) {
			thread_group_.AddThread(new IoThread(&io_), utils::String::Format("network%d", i + 1));
		}
		thread_group_.StartAll();
		work_ = new asio::io_service::work(io_);

		return true;
	}

	bool SocketIo::Close() {
		io_.stop();
		thread_group_.JoinAll();
		return true;
	}

	asio::io_service &SocketIo::GetIoService() {
		return io_;
	}

	SocketClient::SocketClient() : TimerNotify("SocketClient", 2 * utils::MICRO_UNITS_PER_SEC){
		TimerNotify::RegisterModule(this);
	}

	SocketConnection *SocketClient::GetConnectFromHandle(int64_t id) {
		utils::MutexGuard guard(lock_);
		std::map<int64_t, SocketConnection *>::iterator iter = client_conns_.find(id);
		if (iter != client_conns_.end()){
			return iter->second;
		}
		return NULL;
	}

	bool SocketClient::Connect(int64_t id) {
		utils::MutexGuard guard(lock_);
		std::map<int64_t, SocketConnection *>::iterator iter = client_conns_.find(id);
		if (iter != client_conns_.end()) {
			return iter->second->Connect();
		}

		return false;
	}

	bool SocketClient::OnConnectClose(int64_t id) {
		/*
		utils::MutexGuard guard(lock_);
		std::map<int64_t, SocketConnection *>::iterator iter = conns_.find(id);
		if (iter != conns_.end()) {
			delete iter->second;
			conns_.erase(iter);
		}
		*/
		
		return true;
	}

	bool SocketClient::OnConnectError(int64_t id) {
		/*
		utils::MutexGuard guard(lock_);
		std::map<int64_t, SocketConnection *>::iterator iter = conns_.find(id);
		if (iter != conns_.end()) {
			delete iter->second;
			conns_.erase(iter);
		}
		*/

		return true;
	}

	void SocketClient::OnTimer(int64_t current_time){
		utils::MutexGuard guard(lock_);
		for (auto iter = client_delay_delete_conns_.begin(); iter != client_delay_delete_conns_.end();) {
			if (iter->first < current_time) {
				delete iter->second;
				iter = client_delay_delete_conns_.erase(iter);
			}
			else {
				iter++;
			}
		}
	}

	bool SocketClient::Close(int64_t handle, const std::string &reason, Result &ec){
		utils::MutexGuard guard(lock_);
		std::map<int64_t, SocketConnection *>::iterator iter = client_conns_.find(handle);
		if (iter == client_conns_.end()) {
			return false;
		}

		client_delay_delete_conns_.insert(std::make_pair(utils::Timestamp::HighResolution() + 15 * utils::MICRO_UNITS_PER_SEC, iter->second));
		bool result = iter->second->Close(reason, ec);
		client_conns_.erase(iter);
		return result;
	}

	bool SocketClient::Send(int64_t handle, const std::string &message, Result &ec) {
		utils::MutexGuard guard(lock_);
		std::map<int64_t, SocketConnection *>::iterator iter = client_conns_.find(handle);
		if (iter != client_conns_.end()) {
			return iter->second->Send(message, ec);
		}

		return false;
	}

	SocketServer::SocketServer() : TimerNotify("SocketServer", 2 * utils::MICRO_UNITS_PER_SEC){
		TimerNotify::RegisterModule(this);
	}

	SocketServer::~SocketServer() {

	}

	bool SocketServer::Send(int64_t handle, const std::string &message, Result &ec) {
		utils::MutexGuard guard(lock_);
		std::map<int64_t, SocketConnection *>::iterator iter = server_conns_.find(handle);
		if (iter != server_conns_.end()) {
			return iter->second->Send(message, ec);
		}

		return false;
	}

	bool SocketServer::Close(int64_t handle, const std::string &reason, Result &ec) {
		utils::MutexGuard guard(lock_);
		std::map<int64_t, SocketConnection *>::iterator iter = server_conns_.find(handle);
		if (iter == server_conns_.end()) {
			return false;
		}

		server_delay_delete_conns_.insert(std::make_pair(utils::Timestamp::HighResolution() + 15 * utils::MICRO_UNITS_PER_SEC, iter->second));
		bool result = iter->second->Close(reason, ec);
		server_conns_.erase(iter);
		return result;
	}

	bool SocketServer::PauseReading(int64_t handle, Result &ec) {
		utils::MutexGuard guard(lock_);
		std::map<int64_t, SocketConnection *>::iterator iter = server_conns_.find(handle);
		if (iter != server_conns_.end()) {
			return iter->second->PauseReading(ec);
		}

		return false;
	}

	void SocketServer::OnTimer(int64_t current_time){
		utils::MutexGuard guard(lock_);
		for (auto iter = server_delay_delete_conns_.begin(); iter != server_delay_delete_conns_.end();) {
			if (iter->first < current_time) {
				delete iter->second;
				iter = server_delay_delete_conns_.erase(iter);
			}
			else {
				iter++;
			}
		}
	}

	SocketConnection *SocketServer::GetConnectFromHandle(int64_t id) {
		utils::MutexGuard guard(lock_);
		std::map<int64_t, SocketConnection *>::iterator iter = server_conns_.find(id);
		if (iter != server_conns_.end()) {
			return iter->second;
		}

		return NULL;
	}

	void SocketServer::SetOpenHandle(OpenHandle func_handle) {
		open_handle_ = func_handle;
	}

	void SocketServer::SetCloseHandle(CloseHandle func_handle) {
		closed_handle_ = func_handle;
	}

	void SocketServer::SetFailedHandle(FailedHandle func_handle) {
		failed_handle_ = func_handle;
	}

	void SocketServer::SetMsgHandle(MsgHandle func_handle) {
		msg_handle_ = func_handle;
	}

	void SocketServer::SetPongHandle(PongHandle func_handle) {
		pong_handle_ = func_handle;
	}

	static int64_t g_id = 1;
	static utils::Mutex g_id_mutex;
	SocketConnection::SocketConnection(IConnectNotify *conn_notify) {
		utils::MutexGuard guard(g_id_mutex);
		id_ = g_id++;
		conn_notify_ = conn_notify;
	}

	SocketConnection::~SocketConnection() {}

	void SocketConnection::SetUri(const std::string &uri) {
		uri_ = uri;
	}

	int64_t SocketConnection::GetHandle() {
		return id_;
	}

	void SocketConnection::SetOpenHandle(OpenHandle func_handle) {
		open_handle_ = func_handle;
	}

	void SocketConnection::SetCloseHandle(CloseHandle func_handle) {
		closed_handle_ = func_handle;
	}

	void SocketConnection::SetFailedHandle(FailedHandle func_handle) {
		failed_handle_ = func_handle;
	}

	void SocketConnection::SetMsgHandle(MsgHandle func_handle) {
		msg_handle_ = func_handle;
	}

	void SocketConnection::SetPongHandle(PongHandle func_handle) {
		pong_handle_ = func_handle;
	}

	WssConnection::WssConnection(client::connection_ptr c_con, server::connection_ptr s_con, connection_hdl hdl, IConnectNotify *conn_notify) :
		SocketConnection(conn_notify) {
		hdl_ = hdl;
		conn_notify_ = conn_notify;
		c_con_ = c_con;
		s_con_ = s_con;
	}

	WssConnection::~WssConnection() {

	}

	void WssConnection::SetOpenHandle(OpenHandle func_handle) {
		SocketConnection::SetOpenHandle(func_handle);
		if (c_con_) c_con_->set_open_handler(bind(&WssConnection::OnClientOpen, this, _1));
	}

	void WssConnection::SetCloseHandle(CloseHandle func_handle) {
		SocketConnection::SetCloseHandle(func_handle);
		if (c_con_) c_con_->set_close_handler(bind(&WssConnection::OnClose, this, _1));
	}

	void WssConnection::SetFailedHandle(FailedHandle func_handle) {
		SocketConnection::SetFailedHandle(func_handle);
		if (c_con_) c_con_->set_fail_handler(bind(&WssConnection::OnFailed, this, _1));
	}

	void WssConnection::SetMsgHandle(MsgHandle func_handle) {
		SocketConnection::SetMsgHandle(func_handle);
		if (c_con_) c_con_->set_message_handler(bind(&WssConnection::OnMessage, this, _1, _2));
	}

	void WssConnection::SetPongHandle(PongHandle func_handle) {
		SocketConnection::SetPongHandle(func_handle);
		if (c_con_) c_con_->set_pong_handler(bind(&WssConnection::OnPong, this, _1, _2));
	}

	void WssConnection::OnClientOpen(connection_hdl hdl) {
		open_handle_(id_);
	}

	void WssConnection::OnClose(connection_hdl hdl) {
		closed_handle_(id_);
		conn_notify_->OnConnectClose(id_);
	}

	void WssConnection::OnMessage(connection_hdl hdl, server::message_ptr msg) {
		msg_handle_(id_, msg->get_payload());
	}

	void WssConnection::OnFailed(connection_hdl hdl) {
		failed_handle_(id_);
		conn_notify_->OnConnectError(id_);
	}

	void WssConnection::OnPong(connection_hdl hdl, std::string payload) {
        LOG_INFO("OnPong");
        pong_handle_(id_, payload);
	}

	connection_hdl WssConnection::GetWssHandle() {
		return hdl_;
	}

	client::connection_ptr WssConnection::GetClientConPtr() {
		return c_con_;
	}

	utils::InetAddress WssConnection::GetConnRemoteAddr() {
		utils::InetAddress remote_addr = utils::InetAddress::None();
		if (c_con_){
			remote_addr = utils::InetAddress(c_con_->get_host(), c_con_->get_port());
		}
		else if (s_con_) {
			remote_addr = utils::InetAddress(s_con_->get_remote_endpoint());
		}
		return remote_addr;
	}

	void WssConnection::Ping(const std::string &payload, Result &err) {
        LOG_DEBUG("WssConnection::Ping");
        std::error_code ec;
		if (c_con_){
			c_con_->ping(payload, ec);
			err.set_code(ec.value());
			err.set_desc(ec.message());
		}
		else if (s_con_) {
			s_con_->ping(payload, ec);
			err.set_code(ec.value());
			err.set_desc(ec.message());
		}
	}

	Result WssConnection::GetError() {
		std::error_code err;
		if (c_con_) {
			err = c_con_->get_ec();
		}
		else if (s_con_) {
			err = s_con_->get_ec();
		}

		Result ret;
		ret.set_code(err.value());
		ret.set_desc(err.message());
		return ret;
	}

	WssSocketServer::WssSocketServer() {

	}

	WssSocketServer::~WssSocketServer() {
		utils::MutexGuard guard(lock_);
		for(auto iter = server_conns_.begin(); iter != server_conns_.end();) {
			if (iter->second != nullptr) {
				delete iter->second;
				iter->second = nullptr;
			}
			iter = server_conns_.erase(iter);
		}
		for(auto iter = server_delay_delete_conns_.begin(); iter != server_delay_delete_conns_.end();) {
			if (iter->second != nullptr) {
				delete iter->second;
				iter->second = nullptr;
			}
			iter = server_delay_delete_conns_.erase(iter);
		}
	}

	void WssSocketServer::Init(SocketIo *io) {
		server_.init_asio(&io->GetIoService());
		server_.clear_access_channels(websocketpp::log::alevel::all);
		server_.clear_error_channels(websocketpp::log::elevel::all);
		server_.set_open_handler(bind(&WssSocketServer::OnOpen, this, _1));
		server_.set_close_handler(bind(&WssSocketServer::OnClose, this, _1));
		server_.set_fail_handler(bind(&WssSocketServer::OnFailed, this, _1));
		server_.set_message_handler(bind(&WssSocketServer::OnMessage, this, _1, _2));
		server_.set_pong_handler(bind(&WssSocketServer::OnPong, this, _1, _2));
	}

	bool WssSocketServer::SetReuseAddr(bool reuse) {
		server_.set_reuse_addr(reuse);
		return true;
	}

	bool WssSocketServer::Listen(const utils::InetAddress &ip, Result &err) {
		std::error_code ec;
		server_.listen(ip.tcp_endpoint(), ec);
		err.set_code(ec.value());
		err.set_desc(ec.message());

		return ec.value() == 0;
	}

	bool WssSocketServer::StartAccept(Result &err) {
		std::error_code ec;
		server_.start_accept(ec);
		err.set_code(ec.value());
		err.set_desc(ec.message());

		return ec.value() == 0;
	}

	uint16_t WssSocketServer::GetLocalPort() {
        websocketpp::lib::error_code ec;
		return server_.get_local_endpoint(ec).port();
	}

	void WssSocketServer::OnOpen(connection_hdl hdl) {

		int64_t out_hdl = -1;
		//create connection
		do {
			utils::MutexGuard guard(lock_);
			std::error_code err;
			server::connection_ptr ws_con = server_.get_con_from_hdl(hdl, err);
			if (err) {
				break;
			}

			WssConnection *con = new WssConnection(NULL, ws_con, hdl, this);

			hdl_to_ids_[hdl] = con->GetHandle();
			server_conns_[con->GetHandle()] = con;
			out_hdl = con->GetHandle();

		} while (false);
		if (open_handle_ && out_hdl >= 0) {
			open_handle_(out_hdl);
		}
	}

	void WssSocketServer::OnClose(connection_hdl hdl) {
		int64_t out_hdl = -1;
		do {
			utils::MutexGuard guard(lock_);
			ConnectHandleMap::iterator iter = hdl_to_ids_.find(hdl);
			if (iter == hdl_to_ids_.end()) {
				break;
			}

			out_hdl = iter->second;
		} while (false);

		if (closed_handle_ && out_hdl >= 0) {
			closed_handle_(out_hdl);
		}
	}

	void WssSocketServer::OnMessage(connection_hdl hdl, server::message_ptr msg) {
		int64_t out_hdl = -1;
		do {
			utils::MutexGuard guard(lock_);
			ConnectHandleMap::iterator iter = hdl_to_ids_.find(hdl);
			if (iter == hdl_to_ids_.end()) {
				break;
			}

			out_hdl = iter->second;
		} while (false);
		if (msg_handle_ && out_hdl >= 0) {
			msg_handle_(out_hdl, msg->get_payload());
		}
	}

	void WssSocketServer::OnFailed(connection_hdl hdl) {
		int64_t out_hdl = -1;
		do {
			utils::MutexGuard guard(lock_);
			std::map<connection_hdl, int64_t>::iterator iter = hdl_to_ids_.find(hdl);
			if (iter == hdl_to_ids_.end()) {
				break;
			}
			out_hdl = iter->second;
		} while (false);
		if (failed_handle_) {
			failed_handle_(out_hdl);
		}
	}

	void WssSocketServer::OnPong(connection_hdl hdl, std::string payload) {
		int64_t out_hdl = -1;
		do {
			utils::MutexGuard guard(lock_);
			ConnectHandleMap::iterator iter = hdl_to_ids_.find(hdl);
			if (iter == hdl_to_ids_.end()) {
				break;
			}

			out_hdl = iter->second;
		} while (false);
		if (pong_handle_) {
			pong_handle_(out_hdl, payload);
		}
	}

	bool WssSocketServer::OnConnectError(int64_t id) {
		SocketServer::OnConnectError(id);

		utils::MutexGuard guard(lock_);
		for (ConnectHandleMap::iterator iter = hdl_to_ids_.begin();
			iter != hdl_to_ids_.end();
			iter++) {
			if ( iter->second == id){
				hdl_to_ids_.erase(iter);
				break;
			} 
		}

		return true;
	}

	bool WssSocketServer::OnConnectClose(int64_t id) {
		SocketServer::OnConnectClose(id);

		utils::MutexGuard guard(lock_);
		for (ConnectHandleMap::iterator iter = hdl_to_ids_.begin();
			iter != hdl_to_ids_.end();
			iter++) {
			if (iter->second == id) {
				hdl_to_ids_.erase(iter);
				break;
			}
		}

		return true;
	}

	bool WssSocketServer::Send(int64_t handle, const std::string &message, Result &ec) {
		utils::MutexGuard guard(lock_);
		std::map<int64_t, SocketConnection *>::iterator iter = server_conns_.find(handle);
		if (iter != server_conns_.end()) {
			std::error_code ec1;
			WssConnection * wss_con = (WssConnection *)iter->second;
			server_.send(wss_con->GetWssHandle(), message, websocketpp::frame::opcode::BINARY, ec1);
			ec.set_code(ec1.value());
			ec.set_desc(ec1.message());
			return ec.code() == 0;
		} 

		return false;
	}

	bool WssSocketServer::Close(int64_t handle, const std::string &reason, Result &ec) {
		utils::MutexGuard guard(lock_);
		std::map<int64_t, SocketConnection *>::iterator iter = server_conns_.find(handle);
		if (iter == server_conns_.end()) {
			return false;
		}
		
		std::error_code ec1;
		WssConnection * wss_con = (WssConnection *)iter->second;
		server_.close(wss_con->GetWssHandle(), 0, reason, ec1);
		ec.set_code(ec1.value());
		ec.set_desc(ec1.message());

		server_delay_delete_conns_.insert(std::make_pair(utils::Timestamp::HighResolution() + 15 * utils::MICRO_UNITS_PER_SEC, iter->second));
		server_conns_.erase(iter);
		return ec.code() == 0;
	}

	bool WssSocketServer::PauseReading(int64_t handle, Result &ec) {
		utils::MutexGuard guard(lock_);
		std::map<int64_t, SocketConnection *>::iterator iter = server_conns_.find(handle);
		if (iter != server_conns_.end()) {
			std::error_code ec1;
			WssConnection * wss_con = (WssConnection *)iter->second;
			server_.pause_reading(wss_con->GetWssHandle(), ec1);
			ec.set_code(ec1.value());
			ec.set_desc(ec1.message());
			return ec.code() == 0;
		}

		return false;
	}

	WssSocketClient::WssSocketClient() {

	}

	WssSocketClient::~WssSocketClient() {
		utils::MutexGuard guard(lock_);
		for(auto iter = client_conns_.begin(); iter != client_conns_.end();) {
			if (iter->second != nullptr) {
				delete iter->second;
				iter->second = nullptr;
			}
			iter = client_conns_.erase(iter);
		}

		for (auto iter = client_delay_delete_conns_.begin(); iter != client_delay_delete_conns_.end();) {
			if (iter->second != nullptr) {
				delete iter->second;
				iter->second = nullptr;
			}
			iter = client_delay_delete_conns_.erase(iter);
		}
	}

	void WssSocketClient::Init(SocketIo *io) {
		client_.init_asio(&io->GetIoService());
		client_.clear_access_channels(websocketpp::log::alevel::all);
		client_.clear_error_channels(websocketpp::log::elevel::all);
	}

	SocketConnection* WssSocketClient::GetConnect(const std::string uri, Result &err) {
		utils::MutexGuard guard(lock_);
		std::error_code ecc;
		client::connection_ptr con = client_.get_connection(uri, ecc);
		if (!con) {
			err.set_code(ecc.value());
			err.set_desc(ecc.message());
			return NULL;
		}

		WssConnection *wss_con = new WssConnection(con, NULL, con->get_handle(), this);
		wss_con->SetUri(uri);
		client_conns_[wss_con->GetHandle()] = wss_con;
		return wss_con;
	}

	bool WssSocketClient::Send(int64_t handle, const std::string &message, Result &ec) {
		utils::MutexGuard guard(lock_);
		std::map<int64_t, SocketConnection *>::iterator iter = client_conns_.find(handle);
		if (iter != client_conns_.end()) {
			std::error_code ec1;
			WssConnection * wss_con = (WssConnection *)iter->second;
			client_.send(wss_con->GetWssHandle(), message, websocketpp::frame::opcode::BINARY, ec1);
			ec.set_code(ec1.value());
			ec.set_desc(ec1.message());
			return ec.code() == 0;
		}

		return false;
	}

	bool WssSocketClient::Close(int64_t handle, const std::string &reason, Result &ec) {
		utils::MutexGuard guard(lock_);
		std::map<int64_t, SocketConnection *>::iterator iter = client_conns_.find(handle);
		if (iter == client_conns_.end()) {
			return false;
		}


		std::error_code ec1;
		WssConnection * wss_con = (WssConnection *)iter->second;
		client_.close(wss_con->GetWssHandle(), 0, reason, ec1);
		ec.set_code(ec1.value());
		ec.set_desc(ec1.message());

		client_delay_delete_conns_.insert(std::make_pair(utils::Timestamp::HighResolution() + 300 * utils::MICRO_UNITS_PER_SEC, iter->second));
		client_conns_.erase(iter);
		return ec.code() == 0;
	}

	bool WssSocketClient::Connect(int64_t id) {
		utils::MutexGuard guard(lock_);
		std::map<int64_t, SocketConnection *> ::iterator iter = client_conns_.find(id);
		if (iter != client_conns_.end()) {
			std::error_code ec1;
			WssConnection * wss_con = (WssConnection *)iter->second;
			client_.connect(wss_con->GetClientConPtr());
			return true;
		} 

		return false;
	}

	bool WssSocketClient::OnConnectError(int64_t id) {
		return SocketClient::OnConnectError(id);
		
	}

	bool WssSocketClient::OnConnectClose(int64_t id) {
		return SocketClient::OnConnectClose(id);
	}
}
