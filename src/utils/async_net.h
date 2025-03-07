#ifndef __utils_ASYNC_NETWORK_H_INCLUDE__
#define __utils_ASYNC_NETWORK_H_INCLUDE__

#include "net.h"
#include "thread.h"
#include "timestamp.h"
#include <sys/epoll.h>


namespace utils
{
	class AsyncNetwork
	{
	public:
		AsyncNetwork();
		virtual ~AsyncNetwork();
	};

	typedef struct epoll_event AsyncEvent;

	typedef enum tagASYNC_EVENT_CONSTS
	{
		ASYNC_EVENT_IN   = EPOLLIN,
		ASYNC_EVENT_OUT  = EPOLLOUT,
		ASYNC_EVENT_ERR  = EPOLLERR,
#ifndef ANDROID
		ASYNC_EVENT_ONESHOT = EPOLLONESHOT,
#endif
		ASYNC_EVENT_ET      = EPOLLET
	}ASYNC_EVENT_CONSTS;

	typedef std::list<utils::AsyncEvent> AsyncEventList;
	typedef std::map<utils::SocketHandle, utils::AsyncEvent> AsyncEventMap;

	class AsyncPoll
	{
	public:
		static const uint32_t DEFAULT_EVENTS = utils::ASYNC_EVENT_IN|utils::ASYNC_EVENT_OUT|utils::ASYNC_EVENT_ERR|utils::ASYNC_EVENT_ET;

	private:
		AsyncPoll( const AsyncPoll & ){}
		AsyncPoll &operator =( const AsyncPoll & ){ return *this; }
		int m_nEpoll;
		utils::Mutex m_nMutex;
		utils::AsyncEventList *m_pAppendEvents;

	public:
		AsyncPoll();
		virtual ~AsyncPoll();

		inline bool IsValid()
		{
			return -1 != m_nEpoll; 
		}

		bool Create( size_t nPoolSize );
		bool Close();
		bool Add( utils::Socket *lpSocket, uint32_t nEventFlag );
		bool Modify( utils::Socket *lpSocket, uint32_t nEventFlag );
		bool Remove( utils::Socket *lpSocket );
		bool AppendEvent( utils::Socket *lpSocket, uint32_t nEventFlag );

		int WaitEvent( utils::AsyncEvent *lpEvents, size_t nMaxEvents, uint32_t nTimeoutMs );
	};

	typedef std::list<utils::AsyncPoll *> AsyncPollPtrList;
	typedef std::vector<utils::AsyncPoll *> AsyncPollPtrArray;

	class AsyncBuffer
	{
	public:
		static const size_t DEFAULT_SIZE = 2048;

	private:
		bool   m_bAttach;
		uint8_t *m_pBuffer;
		size_t m_nSize;
		size_t m_nDataOffset;
		size_t m_nDataSize;

		void _Tidy();

	public:
		AsyncBuffer();
		explicit AsyncBuffer( size_t nSize );
		explicit AsyncBuffer( const utils::AsyncBuffer &nCopy );
		utils::AsyncBuffer &operator =( const utils::AsyncBuffer &nCopy );
		virtual ~AsyncBuffer();

		void Attach( uint8_t *pBuffer, size_t nSize );
		bool Allocate( size_t nSize );
		void Release();

		inline uint8_t *GetBuffer(){ return m_pBuffer; }
		inline size_t GetSize() const{ return m_nSize; }
		inline uint8_t *GetData(){ return m_pBuffer + m_nDataOffset; }
		inline size_t GetDataOffset() const{ return m_nDataOffset; }
		inline size_t GetDataSize() const{ return m_nDataSize; }
		inline void   SetDataOffset( size_t nDataOffset ){ m_nDataOffset = nDataOffset; }
		inline void   SetDataSize( size_t nDataSize ){ m_nDataSize = nDataSize; }
	};

	class AsyncVxIo;

	class AsyncVxSocket : public utils::Socket
	{
		friend class AsyncVxIo;
		friend class AsyncCustomIoThread;

	protected:
		int m_nIoIndex;
		uint32_t m_nEventFlag;
		utils::AsyncVxIo *m_lpAsyncIo;

		bool m_bConnected;
		bool m_bListening;
		bool m_bPrepared;

		int64_t m_nTimerCalled;
		int64_t m_nTimerInterval;

	public:
		AsyncVxSocket( utils::AsyncVxIo *lpAsyncIo, uint32_t nEventFlag );
		virtual ~AsyncVxSocket();

		// initialize methods
		virtual bool Attach( SocketHandle nHandle, int nType, bool bConnected, bool bListening, bool bPrepared );
		virtual bool Detach();
		virtual bool Create(utils::Socket::SocketType nType, int nProtocol, const utils::InetAddress &nAddress, bool bReuseAddress = false);
		virtual bool Close(); 

		// action methods
		virtual bool Listen( int nBackLog = SOMAXCONN );
		virtual bool Connect( const utils::InetAddress &nPeerAddress );
		virtual bool Connect( const utils::InetAddress &nPeerAddress, int nTimeoutMillis );

		// event methods
		virtual void OnAccept( int64_t nMicroTime ){}
		virtual void OnConnect( int64_t nMicroTime, uint32_t nErrorCode ){}
		virtual void OnReceive( int64_t nMicroTime ){}
		virtual void OnSend( int64_t nMicroTime ){}
		virtual void OnError( int64_t nMicroTime ){}
		virtual bool OnTimer(){ return true; }

		bool SetTimer( int64_t nInterval );
		inline void SetPrepared( bool bPrepared ){ m_bPrepared = bPrepared; }
		inline bool IsPrepared(){ return m_bPrepared; }
	};

	typedef std::set<utils::AsyncVxSocket *> AsyncCustomSocketSet;

	class AsyncCustomIoThread : public utils::Thread
	{
		friend class AsyncVxIo;

	public:
		AsyncCustomIoThread( utils::AsyncVxIo *lpOwner, size_t nIdx, int nThreadPriority );
		virtual ~AsyncCustomIoThread();

	private:
		size_t                m_nIoIdx;
		utils::AsyncVxIo       *m_lpOwner;
		utils::AsyncPoll      m_nPoll;

		// disable timers
		//utils::Mutex          m_nMutex;
		//utils::AsyncCustomSocketSet m_nTimers;

		virtual void Run();
	};

	typedef std::vector<AsyncCustomIoThread *> AsyncCustomIoThreadArray;

	class AsyncVxIo
	{
		friend class AsyncCustomIoThread;
		friend class AsyncVxSocket;

	public:
		static const size_t MAX_IO_COUNT = 32;

	private:
		bool m_bEnabled;

		AsyncVxIo( const utils::AsyncVxIo & ){}
		utils::AsyncVxIo &operator =( const utils::AsyncVxIo & ){ return *this; }

	protected:
		utils::AsyncCustomIoThreadArray *m_pThreads;
		size_t m_nNextIoIdx;

	public:
		AsyncVxIo();
		virtual ~AsyncVxIo();

		bool AddChannel( utils::AsyncVxSocket *pSocket );
		bool RemoveChannel( utils::AsyncVxSocket *pSocket );
		bool SetTimer( utils::AsyncVxSocket *pSocket, int64_t nInterval );

		inline bool IsValid(){ return m_pThreads->size() > 0; }
		bool Create( size_t nPoolSize, size_t nThreadCount = 0, int nThreadPriority = -1 );
		bool Close();
	};

	typedef std::list<utils::AsyncVxIo *>   AsyncIoPtrList;
	typedef std::vector<utils::AsyncVxIo *> AsyncIoPtrArray;
};

#endif //__utils_ASYNC_NETWORK_H_INCLUDE__
