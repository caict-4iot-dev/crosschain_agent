#include "async_net.h"
#include "logger.h"
#include "system.h"

//
// class utils::AsyncNetwork
//
utils::AsyncNetwork::AsyncNetwork()
{
}

utils::AsyncNetwork::~AsyncNetwork()
{
}

//
// class utils::AsyncPoll
//
utils::AsyncPoll::AsyncPoll()
{
	m_nEpoll = -1;
	m_pAppendEvents = new utils::AsyncEventList();
}

utils::AsyncPoll::~AsyncPoll()
{
	Close();
	delete m_pAppendEvents;
	m_pAppendEvents = NULL;
}

bool utils::AsyncPoll::Create( size_t nPoolSize )
{
	assert(nPoolSize > 0);
	VALIDATE_ERROR_RETURN(IsValid(), ERROR_ALREADY_EXISTS, false);
	m_nEpoll = epoll_create(nPoolSize);
	return IsValid();
}

bool utils::AsyncPoll::Close()
{
	VALIDATE_ERROR_RETURN(!IsValid(), ERROR_NOT_READY, false);
	close(m_nEpoll);
	m_nEpoll = -1;
	return true;
}

bool utils::AsyncPoll::Add( utils::Socket *lpSocket, uint32_t nEventFlag )
{
	VALIDATE_ERROR_RETURN(!IsValid(), ERROR_NOT_READY, false);

	utils::MutexGuard __access__(m_nMutex);

	utils::AsyncEvent nEvent;

	memset(&nEvent, 0, sizeof(nEvent));
	nEvent.events   = nEventFlag; //EPOLLET;
	nEvent.data.ptr = lpSocket;
	if( epoll_ctl(m_nEpoll, EPOLL_CTL_ADD, lpSocket->handle(), &nEvent) != 0 )
	{
		uint32_t nErrorCode = utils::error_code();
		if( EEXIST != nErrorCode )
		{
			return false;
		}

		// try modify if already exists
		if (epoll_ctl(m_nEpoll, EPOLL_CTL_MOD, lpSocket->handle(), &nEvent) != 0)
		{
			return false;
		}
	}

	return true;
}

bool utils::AsyncPoll::Modify( utils::Socket *lpSocket, uint32_t nEventFlag )
{
	VALIDATE_ERROR_RETURN(!IsValid(), ERROR_NOT_READY, false);
	struct epoll_event nEvent;

	memset(&nEvent, 0, sizeof(nEvent));
	nEvent.events   = nEventFlag; //EPOLLET;
	nEvent.data.ptr = lpSocket;

	if (epoll_ctl(m_nEpoll, EPOLL_CTL_MOD, lpSocket->handle(), &nEvent) != 0)
	{
		return false;
	}
	return true;
}

bool utils::AsyncPoll::Remove( utils::Socket *lpSocket )
{
	VALIDATE_ERROR_RETURN(!IsValid(), ERROR_NOT_READY, false);

	utils::MutexGuard __access__(m_nMutex);

	// remove append events if exists
	do 
	{
		for( AsyncEventList::iterator itr = m_pAppendEvents->begin(); itr != m_pAppendEvents->end(); )
		{
			AsyncEvent &nEventItem = (*itr);
			if( lpSocket == (utils::Socket *)nEventItem.data.ptr )
			{
				m_pAppendEvents->erase(itr ++);
			}
			else
			{
				itr ++;
			}
		}
	} while (false);

	struct epoll_event nEvent;

	memset(&nEvent, 0, sizeof(nEvent));
	nEvent.events   = EPOLLET;
	nEvent.data.ptr = lpSocket;

	if (epoll_ctl(m_nEpoll, EPOLL_CTL_DEL, lpSocket->handle(), &nEvent) != 0)
	{
		return false;
	}

	return true;
}

bool utils::AsyncPoll::AppendEvent( utils::Socket *lpSocket, uint32_t nEventFlag )
{
	utils::MutexGuard __access__(m_nMutex);

	if( !lpSocket->IsValid() )
	{
		utils::set_error_code(ERROR_NOT_READY);
		return false;
	}

	for( utils::AsyncEventList::iterator itr = m_pAppendEvents->begin(); itr != m_pAppendEvents->end(); itr ++ )
	{
		AsyncEvent &nEventItem = (*itr);
		if( lpSocket == (utils::Socket *)nEventItem.data.ptr )
		{
			nEventItem.events |= nEventFlag;
			return true;
		}
	}

	AsyncEvent nNewEvent;

	memset(&nNewEvent, 0, sizeof(nNewEvent));
	nNewEvent.data.ptr = lpSocket;
	nNewEvent.events   = nEventFlag;

	m_pAppendEvents->push_back(nNewEvent);
	return true;
}

int utils::AsyncPoll::WaitEvent( utils::AsyncEvent *lpEvents, size_t nMaxEvents, uint32_t nTimeoutMs )
{
	//VALIDATE_ERROR_RETURN(!IsValid(), ERROR_NOT_READY, -1);

	assert(NULL != lpEvents && nMaxEvents > 0);

	if( m_pAppendEvents->size() > 0 )
	{
		utils::MutexGuard __access__(m_nMutex);

		size_t nAppendEvents = 0;

		while( m_pAppendEvents->size() > 0 && nAppendEvents < nMaxEvents )
		{
			lpEvents[nAppendEvents ++] = m_pAppendEvents->front();
			m_pAppendEvents->pop_front();
		}

		if( nAppendEvents > 0 )
		{
			return (int)nAppendEvents;
		}
	}

	int nCount = epoll_wait(m_nEpoll, lpEvents, nMaxEvents, nTimeoutMs);
	if( nCount < 0 )
	{
		if( nCount < 0 && utils::Socket::IsNomralError(utils::error_code()) )
		{
			nCount = 0;
		}

		return nCount;
	}

	return nCount;
}

//
// class utils::AsyncBuffer
//

utils::AsyncBuffer::AsyncBuffer()
{
	_Tidy();
}

utils::AsyncBuffer::AsyncBuffer( size_t nSize )
{
	_Tidy();
	if( !Allocate(nSize) )
	{
		LOG_ERROR_ERRNO("Allocate buffer(" FMT_SIZE ") failed", nSize, utils::error_code(), utils::error_desc().c_str());
	}
}

utils::AsyncBuffer::AsyncBuffer( const utils::AsyncBuffer &nCopy )
{
	_Tidy();
	*this = nCopy;
}

utils::AsyncBuffer &utils::AsyncBuffer::operator =( const utils::AsyncBuffer &nCopy )
{
	if( this == &nCopy )
	{
		return *this;
	}

	if( NULL == nCopy.m_pBuffer || 0 == nCopy.m_nSize )
	{
		Release();
		return *this;
	}

	m_bAttach = nCopy.m_bAttach;

	if( m_bAttach )
	{
		Release();

		m_pBuffer     = nCopy.m_pBuffer;
		m_nSize       = nCopy.m_nSize;
		m_nDataOffset = nCopy.m_nDataOffset;
		m_nDataSize   = nCopy.m_nDataSize;
	}
	else
	{
		if( !Allocate(nCopy.m_nSize) )
		{
			LOG_ERROR_ERRNO("Allocate buffer(" FMT_SIZE ") failed", nCopy.m_nSize, utils::error_code(), utils::error_desc().c_str());
		}

		m_nSize       = nCopy.m_nSize;
		m_nDataOffset = nCopy.m_nDataOffset;
		m_nDataSize   = nCopy.m_nDataSize;
		memcpy(m_pBuffer + nCopy.m_nDataOffset, nCopy.m_pBuffer + nCopy.m_nDataOffset, nCopy.m_nDataSize);
	}

	return *this;
}

utils::AsyncBuffer::~AsyncBuffer()
{
	Release();
}

void utils::AsyncBuffer::_Tidy()
{
	m_bAttach   = false;
	m_pBuffer   = NULL;
	m_nSize     = 0;
	m_nDataOffset = 0;
	m_nDataSize   = 0;
}

void utils::AsyncBuffer::Attach( uint8_t *pBuffer, size_t nSize )
{
	assert(NULL != pBuffer);
	assert(nSize > 0);

	Release();

	m_bAttach   = true;
	m_pBuffer   = pBuffer;
	m_nSize     = nSize;
	m_nDataOffset = 0;
	m_nDataSize   = 0;
}

bool utils::AsyncBuffer::Allocate( size_t nSize )
{
	Release();

	m_bAttach = false;
	m_pBuffer = (uint8_t *)malloc(nSize);
	if( NULL == m_pBuffer )
	{
		return false;
	}

	memset(m_pBuffer, 0, nSize);

	m_nSize       = nSize;
	m_nDataOffset = 0;
	m_nDataSize   = 0;

	return true;
}

void utils::AsyncBuffer::Release()
{
	if( !m_bAttach && NULL != m_pBuffer )
	{
		free(m_pBuffer);
		m_pBuffer = NULL;
	}

	_Tidy();
}

namespace utils
{
};

//
// class utils::AsyncCustomSocket
//
utils::AsyncVxSocket::AsyncVxSocket( utils::AsyncVxIo *lpAsyncIo, uint32_t nEventFlag )
{
	assert(NULL != lpAsyncIo);

	m_nIoIndex     = -1;
	m_lpAsyncIo    = lpAsyncIo;
	m_nEventFlag   = nEventFlag;

	m_bConnected   = false;
	m_bListening   = false;
	m_bPrepared    = false;

	m_nTimerCalled   = 0;
	m_nTimerInterval = -1;
}

utils::AsyncVxSocket::~AsyncVxSocket()
{
	if( m_nIoIndex >= 0 && NULL != m_lpAsyncIo )
	{
		m_lpAsyncIo->RemoveChannel(this);
		m_nIoIndex = -1;
	}

	m_lpAsyncIo = NULL;
}

bool utils::AsyncVxSocket::Attach( utils::SocketHandle nHandle, int nType, bool bConnected, bool bListening, bool bPrepared )
{
	if( !utils::Socket::Attach(nHandle, nType, false) )
	{
		return false;
	}

	m_bConnected = bConnected;
	m_bListening = bListening;
	m_bPrepared  = bPrepared;

	m_nIoIndex = -1;
	if( NULL != m_lpAsyncIo && !m_lpAsyncIo->AddChannel(this) )
	{
		return false;
	}

	return true;
}

bool utils::AsyncVxSocket::Detach()
{
	VALIDATE_ERROR_RETURN(!IsValid(), ERROR_NOT_READY, false);

	if( m_nIoIndex >= 0 && NULL != m_lpAsyncIo )
	{
		m_lpAsyncIo->RemoveChannel(this);
		m_nIoIndex = -1;
	}

	m_bConnected   = false;
	m_bListening   = false;

	return utils::Socket::Detach();
}

bool utils::AsyncVxSocket::Create( utils::Socket::SocketType nType, int nProtocol, const utils::InetAddress &nAddress, bool bReuseAddress /* = false */ )
{
	VALIDATE_ERROR_RETURN(IsValid(), ERROR_ALREADY_EXISTS, false);

	if (!Socket::Create(nType, nAddress, bReuseAddress))
	{
		return false;
	}

	m_bConnected   = false;
	m_bListening   = false;

	bool bResult = false;
	do 
	{
		if( !SetBlocking(false) )
		{
			break;
		}

		assert(-1 == m_nIoIndex);

		//m_nIoIndex = -1;
		if( !m_lpAsyncIo->AddChannel(this) )
		{
			break;
		}

		bResult = true;
	} while (false);

	if( !bResult )
	{
		uint32_t nErrorCode = utils::error_code();
		Close();
		utils::set_error_code(nErrorCode);
	}

	return bResult;
}

bool utils::AsyncVxSocket::Close()
{
	if( m_nIoIndex >= 0 && NULL != m_lpAsyncIo )
	{
		m_lpAsyncIo->RemoveChannel(this);
		m_nIoIndex = -1;
	}

	m_bConnected   = false;
	m_bListening   = false;
	m_bPrepared    = false;

	return utils::Socket::Close();
}

bool utils::AsyncVxSocket::Listen( int nBackLog /* = SOMAXCONN */ )
{
	if( !Socket::Listen(nBackLog) )
	{
		return false;
	}

	m_bListening = true;
	return true;
}

bool utils::AsyncVxSocket::Connect( const utils::InetAddress &nPeerAddress )
{
	if( !Socket::Connect(nPeerAddress) )
	{
		return false;
	}

	m_bConnected = true;
	return true;
}

bool utils::AsyncVxSocket::Connect( const utils::InetAddress &nPeerAddress, int nTimeoutMillis )
{
	if( !Socket::Connect(nPeerAddress, nTimeoutMillis) )
	{
		return false;
	}

	m_bConnected = true;
	return true;
}

bool utils::AsyncVxSocket::SetTimer( int64_t nInterval )
{
	if( m_lpAsyncIo->SetTimer(this, nInterval) )
	{
		m_nTimerInterval = nInterval;
		return true;
	}
	else
	{
		return false;
	}
}

//bool utils::AsyncCustomSocket::SendBuffer()
//{
//	assert(m_nType == Socket::TYPE_TCP);
//	VALIDATE_ERROR_RETURN(!IsValid(), ERROR_NOT_READY, false);
//
//	size_t nSizeSent = 0;
//
//	while( m_pSendBuffer->GetDataSize() > 0 )
//	{
//		int nResult = send(m_nHandle, (const char *)m_pSendBuffer->GetData(), (int)m_pSendBuffer->GetDataSize(), MSG_NOSIGNAL);
//		if( nResult == utils::Socket::ERROR_VALUE )
//		{
//			if( EAGAIN != utils::error_code() )
//			{
//				return false;
//			}
//			break;
//		}
//
//		nSizeSent = nResult;
//
//		m_pSendBuffer->SetDataOffset(m_pSendBuffer->GetDataOffset() + nSizeSent);
//		m_pSendBuffer->SetDataSize(m_pSendBuffer->GetDataSize() - nSizeSent);
//	}
//
//	return true;
//}
//
//bool utils::AsyncCustomSocket::SendBufferTo( const utils::InetAddress &nAddress )
//{
//	assert(m_nType != Socket::TYPE_TCP);
//	VALIDATE_ERROR_RETURN(!IsValid(), ERROR_NOT_READY, false);
//
//	size_t nSizeSent = 0;
//	while( m_pSendBuffer->GetDataSize() > 0 )
//	{
//		int nResult = sendto(m_nHandle, (const char *)m_pSendBuffer->GetData(), (int)m_pSendBuffer->GetDataSize(), MSG_NOSIGNAL,
//			nAddress.GetAddress(), sizeof(*nAddress.GetAddress()));
//		if( nResult == utils::Socket::ERROR_VALUE )
//		{
//			if( EAGAIN != utils::error_code() )
//			{
//				return false;
//			}
//			break;
//		}
//
//		nSizeSent = nResult;
//
//		m_pSendBuffer->SetDataOffset(m_pSendBuffer->GetDataOffset() + nSizeSent);
//		m_pSendBuffer->SetDataSize(m_pSendBuffer->GetDataSize() - nSizeSent);
//	}
//
//	return true;
//}
//
//bool utils::AsyncCustomSocket::RecvBuffer()
//{
//	assert(m_nType == Socket::TYPE_TCP);
//	VALIDATE_ERROR_RETURN(!IsValid(), ERROR_NOT_READY, false);
//
//	m_pRecvBuffer->SetDataOffset(0);
//	m_pRecvBuffer->SetDataSize(0);
//
//	size_t nSizeRecv = 0;
//
//	int nResult = recv(m_nHandle, (char *)m_pRecvBuffer->GetBuffer(), (int)m_pRecvBuffer->GetSize(), MSG_NOSIGNAL);
//	if( nResult == utils::Socket::ERROR_VALUE )
//	{
//		if( EAGAIN != utils::error_code() )
//		{
//			return false;
//		}
//	}
//	else
//	{
//		nSizeRecv = nResult;
//	}
//
//	m_pRecvBuffer->SetDataSize(nSizeRecv);
//	return true;
//}
//
//bool utils::AsyncCustomSocket::RecvBufferFrom()
//{
//	assert(m_nType != Socket::TYPE_TCP);
//	VALIDATE_ERROR_RETURN(!IsValid(), ERROR_NOT_READY, false);
//
//	m_pRecvBuffer->SetDataOffset(0);
//	m_pRecvBuffer->SetDataSize(0);
//
//	size_t nSizeRecv = 0;
//	socklen_t nAddrLen = sizeof(* m_nAddressFrom.GetAddress());
//
//	int nResult = recvfrom(m_nHandle, (char *)m_pRecvBuffer->GetBuffer(), (int)m_pRecvBuffer->GetSize(), MSG_NOSIGNAL,
//		m_nAddressFrom.GetAddress(), &nAddrLen);
//	if( nResult == utils::Socket::ERROR_VALUE )
//	{
//		if( EAGAIN != utils::error_code() )
//		{
//			return false;
//		}
//	}
//	else
//	{
//		nSizeRecv = nResult;
//	}
//
//	m_pRecvBuffer->SetDataSize(nSizeRecv);
//	return true;
//}

//
// utils::AsyncCustomIoThread
//
utils::AsyncCustomIoThread::AsyncCustomIoThread( utils::AsyncVxIo *lpOwner, size_t nIdx, int nThreadPriority )
{
	assert(NULL != lpOwner);

	//m_nThreadPriority = nThreadPriority;

	m_lpOwner = lpOwner;
	m_nIoIdx  = nIdx;
}

utils::AsyncCustomIoThread::~AsyncCustomIoThread()
{
}

void utils::AsyncCustomIoThread::Run()
{
	assert(m_nPoll.IsValid());

	const int WAIT_TIMEOUT_MS = 10;
	int64_t nMicroTime = 0;

	LOG_INFO("I/O index(" FMT_SIZE ") thread start", m_nIoIdx);

	const size_t WAIT_EVENT_SIZE = 100;
	utils::AsyncEvent nEvents[WAIT_EVENT_SIZE];

	while( enabled_ )
	{
		//// schedule timer
		//if( m_nTimers.size() > 0 )
		//{
		//	utils::MutexGuard __access__(&m_nMutex);
		//
		//	int64_t nMicroTime = utils::Timestamp::HighResolution();
		//	for( utils::AsyncCustomSocketSet::iterator itr = m_nTimers.begin(); itr != m_nTimers.end(); )
		//	{
		//		utils::AsyncCustomSocket *pSocket = (*itr);
		//		if( pSocket->m_nTimerCalled < nMicroTime && pSocket->m_nTimerCalled + pSocket->m_nTimerInterval > nMicroTime )
		//		{
		//			itr ++;
		//			continue;
		//		}

		//		pSocket->m_nTimerCalled = nMicroTime;
		//		if( !pSocket->OnTimer() )
		//		{
		//			m_nTimers.erase(itr ++);
		//		}
		//		else
		//		{
		//			itr ++;
		//		}
		//	}
		//}

		// wait for events
		int nResult = m_nPoll.WaitEvent(nEvents, WAIT_EVENT_SIZE, WAIT_TIMEOUT_MS);

		nMicroTime = utils::Timestamp::HighResolution();
		if( nResult < 0 )
		{
			/**
			uint32_t nErrorCode = utils::error_code();
			if( EINTR == nErrorCode )
			{
				continue;
			}
			**/

			LOG_ERROR_ERRNO( "Query events failed", utils::error_code(), utils::error_desc().c_str());
			
			utils::Sleep(5000);
			continue;
			//break;
		}
		
		for( int n = 0; n < nResult; n ++ )
		{
			utils::AsyncEvent  &nEvent  = nEvents[n];
			utils::AsyncVxSocket *pSocket = (utils::AsyncVxSocket *)nEvent.data.ptr;
			const int nSerialNumber = pSocket->m_nSerailNumber;

			//change the order for http recieve immediatly rest after data.
			if( pSocket->IsPrepared() )
			{
				// check if event is valid
				if( (nEvent.events & ASYNC_EVENT_OUT) && (nSerialNumber == pSocket->m_nSerailNumber) )
				{
					int64_t nPreTime = utils::Timestamp::HighResolution();
					utils::ISocketControl *lpSocketCtrl = pSocket->m_pCustomControl;

					if( pSocket->IsConnectable() && pSocket->m_bConnectBlocked )
					{
						pSocket->m_bConnectBlocked = false;

						uint32_t nConnectError = pSocket->GetSocketError();
						pSocket->m_bConnected = (ERROR_SUCCESS == nConnectError);
						if( pSocket->m_bConnected && NULL != lpSocketCtrl )
						{
							lpSocketCtrl->OnConnect(nMicroTime);
						}
						else
						{
							pSocket->OnConnect(nMicroTime, nConnectError);
						}
					}
					else
					{
						if( lpSocketCtrl ) lpSocketCtrl->OnSend(nMicroTime);
						else pSocket->OnSend(nMicroTime);
					}

					int64_t nUseTime = (utils::Timestamp::HighResolution() - nPreTime) / utils::MICRO_UNITS_PER_MILLI;
					if ( nUseTime > 10 )
					{
						LOG_WARN( "Async out use time(" FMT_I64 "ms) ,id(" FMT_SIZE ")",
							nUseTime, m_nIoIdx);
					}
				}

				// check if event is valid
				if( (nEvent.events & ASYNC_EVENT_IN) && (nSerialNumber == pSocket->m_nSerailNumber) )
				{
					int64_t nPreTime = utils::Timestamp::HighResolution();
					utils::ISocketControl *lpSocketCtrl = pSocket->m_pCustomControl;

					if( pSocket->m_bListening )
					{
						if( lpSocketCtrl ) lpSocketCtrl->Accept(0);
						else pSocket->OnAccept(nMicroTime);
					}
					else
					{
						if( lpSocketCtrl ) lpSocketCtrl->OnReceive(nMicroTime);
						else pSocket->OnReceive(nMicroTime);
					}

					int64_t nUseTime = (utils::Timestamp::HighResolution() - nPreTime) / utils::MICRO_UNITS_PER_MILLI;
					if ( nUseTime > 10 )
					{
						//__ULOG_WARNING(__ULOG_FMT("utils::AsyncCustomIoThread", "Async in use time("FMT_I64"ms) ,id("FMT_SIZE")"),
						//	nUseTime, m_nIoIdx);
					}
				}
			}

			if( (nEvent.events & ASYNC_EVENT_ERR) && (nSerialNumber == pSocket->m_nSerailNumber) )
			{
				pSocket->OnError(nMicroTime);
			}
		}
	}

	LOG_INFO("I/O index(" FMT_SIZE ") thread exit", m_nIoIdx);
}

//
// utils::AsyncCustomIo
//

utils::AsyncVxIo::AsyncVxIo()
{
	m_bEnabled   = false;
	m_pThreads   = new utils::AsyncCustomIoThreadArray();
	m_nNextIoIdx = 0;
}

utils::AsyncVxIo::~AsyncVxIo()
{
	assert(m_pThreads->size() == 0);

	Close();

	delete m_pThreads;
	m_pThreads = NULL;
}

bool utils::AsyncVxIo::Create( size_t nPoolSize, size_t nThreadCount, int nThreadPriority )
{
	if( nThreadCount > utils::AsyncVxIo::MAX_IO_COUNT )
	{
		utils::set_error_code(ERROR_INVALID_PARAMETER);
		return false;
	}
	else if( m_pThreads->size() > 0 )
	{
		utils::set_error_code(ERROR_ALREADY_EXISTS);
		return false;
	}

	if( nThreadCount == 0 )
	{
		utils::System nSystem(false);
		nSystem.UpdateProcessor();
		nThreadCount = (int)nSystem.GetProcessor().core_count_;
	}

	nThreadCount = MAX(nThreadCount, 1);
	m_pThreads->resize(nThreadCount, NULL);

	bool bSuccess = false;

	do 
	{
		size_t nCreateIoHandles = 0;

		for( size_t n = 0; n < nThreadCount; n ++ )
		{
			utils::AsyncCustomIoThread *pNewThread = new utils::AsyncCustomIoThread(this, n, nThreadPriority);

			if( !pNewThread->m_nPoll.Create(nPoolSize) )
			{
				LOG_ERROR_ERRNO("Create io poll(" FMT_SIZE ") failed", n, utils::error_code(), utils::error_desc().c_str());

				delete pNewThread;
				break;
			}

			if( !pNewThread->Start() )
			{
				LOG_ERROR_ERRNO("Start io thread failed", utils::error_code(), utils::error_desc().c_str());

				delete pNewThread;
				break;
			}

			(*m_pThreads)[n] = pNewThread;
			nCreateIoHandles ++;
		}

		if( nCreateIoHandles != nThreadCount )
		{
			break;
		}

		bSuccess = true;
	} while (false);

	if( !bSuccess )
	{
		uint32_t nErrorCode = utils::error_code();

		// clean
		Close();

		utils::set_error_code(nErrorCode);
		return false;
	}

	return true;
}

bool utils::AsyncVxIo::Close()
{
	for( size_t n = 0; n < m_pThreads->size(); n ++ )
	{
		utils::Thread *pThread = m_pThreads->at(n);
		if( NULL == pThread ) continue;

		if( pThread->thread_id() == utils::Thread::current_thread_id() )
		{
			LOG_ERROR("Can't close in event thread(" FMT_SIZE ")", n);

			utils::set_error_code(ERROR_ACCESS_DENIED);
			return false;
		}
	}

	for( size_t n = 0; n < m_pThreads->size(); n ++ )
	{
		utils::Thread *pThread = m_pThreads->at(n);
		if( NULL == pThread ) continue;

		if( pThread->IsRunning() )
		{
			pThread->Stop();
			while( pThread->IsRunning() )
			{
				utils::Sleep(10);
			}

		}

		delete pThread;
	}

	m_pThreads->clear();

	return true;
}

bool utils::AsyncVxIo::AddChannel( utils::AsyncVxSocket *pSocket )
{
	VALIDATE_ERROR_RETURN(!IsValid(), ERROR_NOT_READY, false);

	size_t nIoIdx = ((pSocket->m_nIoIndex >= 0) ? (size_t)pSocket->m_nIoIndex : (m_nNextIoIdx ++)) % m_pThreads->size();

	pSocket->m_nIoIndex = (int)nIoIdx;
	utils::AsyncCustomIoThread *pThread = m_pThreads->at(nIoIdx);

	do 
	{
		//utils::MutexGuard __access__(&pThread->m_nMutex);

		if( !pThread->m_nPoll.Add(pSocket, pSocket->m_nEventFlag) )
		{
			LOG_ERROR_ERRNO("Add socket to poll(" FMT_SIZE ") failed", 
				nIoIdx, utils::error_code(), utils::error_desc().c_str());
			return false;
		}
	} while (false);

	return true;
}

bool utils::AsyncVxIo::RemoveChannel( utils::AsyncVxSocket *pSocket )
{
	VALIDATE_ERROR_RETURN(!IsValid(), ERROR_NOT_READY, false);

	int nIoIndex = pSocket->m_nIoIndex;
	if( nIoIndex < 0 || nIoIndex >= (int)m_pThreads->size() )
	{
		//assert(false);
		return false;
	}

	utils::AsyncCustomIoThread *pThread = m_pThreads->at(nIoIndex);
	do 
	{
		//utils::MutexGuard __access__(&pThread->m_nMutex);

		//// remove timer
		//pThread->m_nTimers.erase(pSocket);
		//pSocket->m_nTimerInterval = -1;

		if( !pThread->m_nPoll.Remove(pSocket) )
		{
			LOG_ERROR_ERRNO("Remove socket from poll(%d) failed", 
				nIoIndex, utils::error_code(), utils::error_desc().c_str());
		}

	} while (false);

	return true;
}

bool utils::AsyncVxIo::SetTimer( utils::AsyncVxSocket *pSocket, int64_t nInterval )
{
	VALIDATE_ERROR_RETURN(!IsValid(), ERROR_NOT_READY, false);

	int nIoIndex = pSocket->m_nIoIndex;
	if( nIoIndex < 0 || nIoIndex >= (int)m_pThreads->size() )
	{
		return false;
	}

	//utils::AsyncCustomIoThread *pThread = m_pThreads->at(nIoIndex);
	//do 
	//{
	//	utils::MutexGuard __access__(&pThread->m_nMutex);

	//	if( nInterval > 0 ) pThread->m_nTimers.insert(pSocket);
	//	else pThread->m_nTimers.erase(pSocket);
	//} while (false);

	return true;
}

