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

#ifndef NETWORK_ADAPTER_H_
#define NETWORK_ADAPTER_H_
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/config/asio_no_tls_client.hpp>
#include <websocketpp/config/asio.hpp>
#include <websocketpp/config/asio_client.hpp>
#include <websocketpp/server.hpp>
#include <websocketpp/client.hpp>
#include <proto/cpp/overlay.pb.h>
#include <utils/headers.h>
#include <utils/async_net.h>
#include <common/general.h>
#include "general.h"


namespace agent {

	typedef websocketpp::client<websocketpp::config::asio_client> client;
	typedef websocketpp::server<websocketpp::config::asio> server;
	typedef websocketpp::lib::shared_ptr<asio::ssl::context> context_ptr;

	using websocketpp::connection_hdl;
	using websocketpp::lib::placeholders::_1;
	using websocketpp::lib::placeholders::_2;
	using websocketpp::lib::bind;

	typedef std::set<connection_hdl, std::owner_less<connection_hdl> > con_list;
	typedef std::map<connection_hdl, int64_t, std::owner_less<connection_hdl>> ConnectHandleMap;

	typedef std::function<void(int64_t)> OpenHandle;
	typedef std::function<void(int64_t)> CloseHandle;
	typedef std::function<void(int64_t)> FailedHandle;
	typedef std::function<void(int64_t, const std::string &msg)> MsgHandle;
	typedef std::function<void(int64_t, const std::string &msg)> PongHandle;

	class SocketIo {
	protected:
		utils::ThreadGroup thread_group_;
		asio::io_service io_;
		asio::io_service::work *work_;
		int32_t thread_count_;
	public:
		SocketIo(int32_t thread_count);
		~SocketIo();

		virtual bool Create();
		virtual bool Close();
		asio::io_service &GetIoService();
	};

	class IConnectNotify {
	public:
		IConnectNotify() {};
		~IConnectNotify() {};

		virtual bool OnConnectError(int64_t id) = 0;
		virtual bool OnConnectClose(int64_t id) = 0;
		virtual bool OnSocketAccept(int64_t current_time) { return true; };
	};

	class SocketConnection;
	class SocketClient : public IConnectNotify, TimerNotify{
	protected:
		std::map<int64_t, SocketConnection *> client_conns_;
		std::map<int64_t, SocketConnection *> client_delay_delete_conns_;
		utils::Mutex lock_;
	public:
		SocketClient();
		virtual ~SocketClient(){};

		virtual void Init(SocketIo *io) {};
		virtual SocketConnection* GetConnect(const std::string uri, Result &err) { return NULL; };
		virtual SocketConnection *GetConnectFromHandle(int64_t id);
		virtual bool Close(int64_t handle, const std::string &reason, Result &ec);
		virtual bool Send(int64_t handle, const std::string &message, Result &ec) ;

		virtual bool Connect(int64_t id);

		virtual bool OnConnectError(int64_t id);
		virtual bool OnConnectClose(int64_t id);

        virtual void OnTimer(int64_t current_time);
		virtual void OnSlowTimer(int64_t current_time)  {};

	};

	class SocketServer : public IConnectNotify,TimerNotify{
	protected:
		OpenHandle open_handle_;
		FailedHandle failed_handle_;
		CloseHandle closed_handle_;
		PongHandle pong_handle_;
		MsgHandle msg_handle_;

		utils::Mutex lock_;
		std::map<int64_t, SocketConnection *> server_conns_;
		std::map<int64_t, SocketConnection *> server_delay_delete_conns_;
	public:
		SocketServer();
		virtual ~SocketServer();

		virtual void Init(SocketIo *io) {};
		virtual bool SetReuseAddr(bool reuse) { return false; };
		virtual void SetOpenHandle(OpenHandle func_handle);
		virtual void SetCloseHandle(CloseHandle func_handle);
		virtual void SetFailedHandle(FailedHandle func_handle) ;
		virtual void SetMsgHandle(MsgHandle func_handle);
		virtual void SetPongHandle(PongHandle func_handle);
		virtual utils::InetAddress GetPeerAddress() { return utils::InetAddress::Any(); };
		virtual bool Listen(const utils::InetAddress &ip, Result &err) { return false; };
		virtual uint16_t GetLocalPort() { return 0; };
		virtual bool Send(int64_t handle, const std::string &message, Result &ec);
		virtual SocketConnection *GetConnectFromHandle(int64_t id);
		virtual bool StartAccept(Result &err) { return true; };
		virtual bool Close(int64_t handle, const std::string &reason, Result &ec);
		virtual bool PauseReading(int64_t handle, Result &ec);

		virtual bool OnConnectError(int64_t id) { return false; };
		virtual bool OnConnectClose(int64_t id) { return false; };

		virtual void OnTimer(int64_t current_time) ;
		virtual void OnSlowTimer(int64_t current_time)  {};
	};
	class SocketConnection {
	protected:
		int64_t id_;
		OpenHandle open_handle_;
		FailedHandle failed_handle_;
		CloseHandle closed_handle_;
		PongHandle pong_handle_;
		MsgHandle msg_handle_;

		IConnectNotify *conn_notify_;
		std::string uri_;
	public:
		SocketConnection(IConnectNotify *conn_notify);
		virtual ~SocketConnection();
		virtual bool Send(const std::string &message, Result &ec) { return false; };
		virtual bool Close(const std::string &reason, Result &ec) { return false; };
		virtual bool PauseReading(Result &ec) { return false; };
		int64_t GetHandle();
		void SetUri(const std::string &uri);
		void Ping(const std::string &payload, Result &err) {};

		virtual void SetOpenHandle(OpenHandle func_handle);
		virtual void SetCloseHandle(CloseHandle func_handle);
		virtual void SetFailedHandle(FailedHandle func_handle);
		virtual void SetMsgHandle(MsgHandle func_handle);
		virtual void SetPongHandle(PongHandle func_handle);

		virtual utils::InetAddress GetConnRemoteAddr() { return utils::InetAddress::None(); };

		virtual Result GetError() { return Result(); };
		virtual bool Connect() { return false; };
	};

	typedef struct tagPeerMsgHearder {
		//char type[16];
		uint8_t version;
		uint8_t ext;
		uint8_t type;  // 0:data, 1:ping, 2:pong
		uint8_t compress_type; //0: no 1:gzip
		uint32_t data_len;
	}PeerMsgHearder;


	class WssConnection : public SocketConnection {
		client::connection_ptr c_con_;
		server::connection_ptr s_con_;
		connection_hdl hdl_;
		IConnectNotify *conn_notify_;
	public:
		WssConnection(client::connection_ptr c_con, server::connection_ptr s_con, connection_hdl hdl, IConnectNotify *conn_notify);
		~WssConnection();

		virtual void SetOpenHandle(OpenHandle func_handle);
		virtual void SetCloseHandle(CloseHandle func_handle);
		virtual void SetFailedHandle(FailedHandle func_handle);
		virtual void SetMsgHandle(MsgHandle func_handle);
		virtual void SetPongHandle(PongHandle func_handle);

		void OnClientOpen(connection_hdl hdl);
		void OnClose(connection_hdl hdl);
		void OnMessage(connection_hdl hdl, server::message_ptr msg);
		void OnFailed(connection_hdl hdl);
		void OnPong(connection_hdl hdl, std::string payload);

		connection_hdl GetWssHandle();
		client::connection_ptr GetClientConPtr();

		virtual utils::InetAddress GetConnRemoteAddr();
		virtual Result GetError();
		void Ping(const std::string &payload, Result &err);
	};

	class WssSocketClient : public SocketClient {
		client client_;
	public:
		WssSocketClient();
		~WssSocketClient();

		virtual void Init(SocketIo *io);
		virtual SocketConnection* GetConnect(const std::string uri, Result &err);
		virtual bool Close(int64_t handle, const std::string &reason, Result &ec);
		virtual bool Send(int64_t handle, const std::string &message, Result &ec);

		virtual bool Connect(int64_t id);

		virtual bool OnConnectError(int64_t id);
		virtual bool OnConnectClose(int64_t id);
	};

	class WssSocketServer : public SocketServer {
		server server_;
		ConnectHandleMap hdl_to_ids_;
	public:
		WssSocketServer();
		~WssSocketServer();

		virtual void Init(SocketIo *io);
		virtual bool SetReuseAddr(bool reuse);
		virtual bool Listen(const utils::InetAddress &ip, Result &err);
		virtual uint16_t GetLocalPort();
		virtual bool StartAccept(Result &err);

		void OnOpen(connection_hdl hdl);
		void OnClose(connection_hdl hdl);
		virtual void OnMessage(connection_hdl hdl, server::message_ptr msg);
		void OnFailed(connection_hdl hdl);
		void OnPong(connection_hdl hdl, std::string payload);

		virtual bool OnConnectError(int64_t id);
		virtual bool OnConnectClose(int64_t id);

		virtual bool Send(int64_t handle, const std::string &message, Result &ec);
		virtual bool Close(int64_t handle, const std::string &reason, Result &ec);
		virtual bool PauseReading(int64_t handle, Result &ec);
	};
}

#endif