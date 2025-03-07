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

#ifndef CHANNEL_H_
#define CHANNEL_H_

#include <utils/net.h>
#include <utils/strings.h>
#include <utils/net.h>
#include <json/value.h>
#include <proto/cpp/common.pb.h>
#include <proto/cpp/overlay.pb.h>
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/config/asio_no_tls_client.hpp>
#include <websocketpp/config/asio.hpp>
#include <websocketpp/config/asio_client.hpp>
#include <websocketpp/server.hpp>
#include <websocketpp/client.hpp>
#include <common/general.h>
#include "network_adapter.h"

namespace agent {
	class Connection {
	private:
		SocketServer *server_;
		SocketClient *client_;
		int64_t handle_;

		//Status
		int64_t connect_end_time_;

		int64_t last_receive_time_;

		std::string uri_;
		int64_t id_;
		bool in_bound_;
		utils::InetAddress peer_address_;

	protected:
		int64_t connect_start_time_;
		int64_t sequence_;
		int64_t last_send_time_;
		int64_t delay_;

	public:
		Connection(SocketServer *server_h, SocketClient *client_h,
			int64_t hdl, const std::string &uri, int64_t id);
		virtual ~Connection();
		
		bool SendByteMessage(const std::string &message, std::error_code &ec);
		bool SendMsg(int64_t type, bool request, int64_t sequence, const std::string &data, std::error_code &ec);
		bool SendRequest(int64_t type, const std::string &data, std::error_code &ec);
		bool SendResponse(const protocol::WsMessage &req_message, const std::string &data, std::error_code &ec);
		bool Ping(std::error_code &ec);
		virtual bool PingCustom(std::error_code &ec);
		bool Close(const std::string &reason);

		bool NeedPing(int64_t interval);
		void TouchReceiveTime();
		void SetConnectTime();
		utils::InetAddress GetPeerAddress() const;
		int64_t GetId() const;
		int64_t GetHandle() const;
		Result GetErrorCode() const;
		bool InBound() const;

		// delay
		void SetDelay();
		int64_t GetDelay() const;

		//get status
		bool IsConnectExpired(int64_t time_out) const;
		bool IsDataExpired(int64_t time_out) const;
		virtual void ToJson(Json::Value &status) const;
		virtual bool OnNetworkTimer(int64_t current_time);
	};

	class CertParameter {
	public:
		CertParameter() {
			memset(id_, 0, 128);
			memset(start_time_, 0, 20);
			memset(end_time_, 0, 20);
		}
		char id_[128];
		char start_time_[20];
		char end_time_[20];
	};

	class Certs {
	public:
		Certs() {
			conn_id_ = -1;
			memset(root_code, 0, 65);
			memset(license_time_, 0, 64);
			memset(root_common_name_, 0, 128);
			memset(root_organization_unit_, 0, 128);
			memset(root_email_, 0, 128);
			//memset(&pkey_, 0, sizeof(pkey_));
			pkey_ = NULL;
		}
		int64_t conn_id_;
		char root_code[65];
		char root_common_name_[128];
		char root_organization_unit_[128];
		char root_email_[128];
		//EVP_PKEY pkey_;
		EVP_PKEY *pkey_;
		CertParameter verify_cert_;
		CertParameter entity_cert_;
		char license_time_[64];
	};

	typedef std::map<int64_t, std::shared_ptr<Connection>> ConnectionMap;
	typedef std::map<int64_t, int64_t> ConnectHandleToIdMap;
	typedef std::map<connection_hdl, Certs, std::owner_less<connection_hdl>> ConnectCertMap;
	typedef std::function<bool(protocol::WsMessage &message, int64_t conn_id)> MessageConnPoc;
	typedef std::map<int64_t, MessageConnPoc> MessageConnPocMap;

	class Network {
	protected:
		int64_t last_check_time_;
		int64_t connect_time_out_;

		SocketIo     *io_;
		SocketServer *server_;
		SocketClient *client_;

		ConnectionMap connections_;
		ConnectionMap connections_delete_;
		ConnectHandleToIdMap connection_handles_;
		ConnectCertMap connection_certs_;

		int64_t next_id_;
		bool enabled_;
		bool thread_inited_;


		std::error_code ec_;
		utils::Mutex conns_list_lock_;
        uint16_t listen_port_;
	public:
		Network();
		virtual ~Network();

		void Start(const utils::InetAddress &ip);
		void Stop();
		//For client
		bool Connect(std::string const & uri);
		uint16_t GetListenPort() const;
		bool ThreadStarted();
	protected:
		//For server
		void OnOpen(int64_t hdl);
		void OnClose(int64_t hdl);
		virtual void OnMessage(int64_t hdl, const std::string &msg);
		void OnFailed(int64_t hdl);

		//For client
		void OnClientOpen(int64_t hdl);
		void OnPong(int64_t hdl, std::string payload);
	

		//The thread is not safe when you get a peer object.
		std::shared_ptr<Connection> GetConnection(int64_t id);
		std::shared_ptr<Connection> GetConnectionFromHandle(int64_t hdl);

		//The thread is not safe when you remove or close a network connection.
		void RemoveConnection(std::shared_ptr<Connection> conn);
		void RemoveConnection(int64_t conn_id);

		//Mapp message type to function.
		MessageConnPocMap request_methods_;
		MessageConnPocMap response_methods_;

		//Send custom message.
		bool OnRequestPing(protocol::WsMessage &message, int64_t conn_id);
		bool OnResponsePing(protocol::WsMessage &message, int64_t conn_id);

		//When requested, create a connection.
		virtual std::shared_ptr<Connection> CreateConnectObject(SocketServer *server_h, SocketClient *client_h, 
			int64_t hdl, const std::string &uri, int64_t id);
		virtual void OnDisconnect(std::shared_ptr<Connection> conn) {};
		virtual bool OnConnectOpen(std::shared_ptr<Connection> conn) { return true; };
	};

}
#endif
