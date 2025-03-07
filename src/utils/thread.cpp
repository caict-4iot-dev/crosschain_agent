#include <sys/prctl.h>
#include "strings.h"
#include "thread.h"

const pthread_t utils::Thread::INVALID_HANDLE = (pthread_t)-1;

void *utils::Thread::threadProc(void *param)
{
	Thread *this_thread = reinterpret_cast<Thread *>(param);
	this_thread->Run();
	this_thread->thread_id_ = 0;
	this_thread->handle_ = INVALID_HANDLE;
	this_thread->running_ = false;
	return NULL;
}

bool utils::Thread::Start(std::string name) {
	name_ = name;
	if (running_) {
		return false;
	}

	bool result = false;
	enabled_ = true;
	running_ = true;
	int ret = 0;
	pthread_attr_t object_attr;
	pthread_attr_init(&object_attr);
	pthread_attr_setdetachstate(&object_attr, PTHREAD_CREATE_DETACHED);

   	//Check and keep min stack 2 Mb on linux
	size_t stacksize = 0;
	ret = pthread_attr_getstacksize(&object_attr, &stacksize);
	if(ret != 0) {
		printf("get stacksize error!:%d\n", (int)stacksize);
		pthread_attr_destroy(&object_attr);
		return false;
	}

	if(stacksize <= 2 * 1024 * 1024)
	{
		printf("linux default pthread statck size:%d\n", (int)stacksize);
		stacksize = 2 * 1024 * 1024;
		printf("set pthread statck size:%d\n", (int)stacksize);

		ret = pthread_attr_setstacksize(&object_attr, stacksize);
		if (ret != 0) {
			printf("set stacksize error!:%d\n", (int)stacksize);
			pthread_attr_destroy(&object_attr);
			return false;
		}
	}


	ret = pthread_create(&handle_, &object_attr, threadProc, (void *)this);
	result = (0 == ret);
	thread_id_ = handle_;
	pthread_attr_destroy(&object_attr);

	if (!result) {
		// restore _beginthread or pthread_create's error
		utils::set_error_code(ret);

		handle_ = Thread::INVALID_HANDLE;
		enabled_ = false;
		running_ = false;
	}
	return result;
}


bool utils::Thread::Stop() {
	if (!IsObjectValid()) {
		return false;
	}

	enabled_ = false;
	return true;
}


bool utils::Thread::Terminate() {
	if (!IsObjectValid()) {
		return false;
	}

	bool result = true;

	if (0 != pthread_cancel(thread_id_)) {
		result = false;
	}

	enabled_ = false;
	return result;
}

bool utils::Thread::JoinWithStop() {
	if (!IsObjectValid()) {
		return true;
	}

	enabled_ = false;
	while (running_) {
		utils::Sleep(10);
	}

	return true;
}

void utils::Thread::Run() {
	assert(target_ != NULL);

	SetCurrentThreadName(name_);

	target_->Run(this);
}

bool utils::Thread::SetCurrentThreadName(std::string name) {
	return 0 == prctl(PR_SET_NAME, name.c_str(), 0, 0, 0);
}

size_t utils::Thread::current_thread_id() {
	return (size_t)pthread_self();
}

utils::Mutex::Mutex()
	: thread_id_(0) {
	pthread_mutexattr_t mattr;
	pthread_mutexattr_init(&mattr);
	pthread_mutexattr_settype(&mattr, PTHREAD_MUTEX_RECURSIVE);
	pthread_mutex_init(&mutex_, &mattr);
	pthread_mutexattr_destroy(&mattr);
}

utils::Mutex::~Mutex() {
	pthread_mutex_destroy(&mutex_);
}

void utils::Mutex::Lock() {
	pthread_mutex_lock(&mutex_);
#ifdef _DEBUG
	thread_id_ = static_cast<uint32_t>(pthread_self());
#endif
}

void utils::Mutex::Unlock() {
#ifdef _DEBUG
	thread_id_ = 0;
#endif
	pthread_mutex_unlock(&mutex_);
}

utils::ReadWriteLock::ReadWriteLock()
	: _reads(0) {}

utils::ReadWriteLock::~ReadWriteLock() {}

void utils::ReadWriteLock::ReadLock() {
	_enterLock.Lock();
	AtomicInc(&_reads);
	_enterLock.Unlock();
}

void utils::ReadWriteLock::ReadUnlock() {
	AtomicDec(&_reads);
}

void utils::ReadWriteLock::WriteLock() {
	_enterLock.Lock();
	while (_reads > 0) {
		Sleep(0);
	}
}

void utils::ReadWriteLock::WriteUnlock() {
	_enterLock.Unlock();
}

utils::Semaphore::Semaphore(int32_t num) {
	sem_init(&sem_, 0, num);
}

utils::Semaphore::~Semaphore() {
	sem_destroy(&sem_);  
}

bool utils::Semaphore::Wait(uint32_t millisecond) {
	int32_t ret = 0;

	if (kInfinite == millisecond) {
		ret = sem_wait(&sem_);
	}
	else {
		struct timespec ts = { 0, 0 };
		//TimeUtil::getAbsTimespec(&ts, millisecond);

		ts.tv_sec = millisecond / 1000;
		ts.tv_nsec = millisecond % 1000;

		ret = sem_timedwait(&sem_, &ts);
	}

	return -1 != ret;
}

bool utils::Semaphore::Signal() {
	return -1 != sem_post(&sem_);
}

utils::ThreadTaskQueue::ThreadTaskQueue()
	:tasks_size_(0){}

utils::ThreadTaskQueue::~ThreadTaskQueue() {}

int utils::ThreadTaskQueue::PutFront(Runnable *task) {
	int ret = 0;
	spinLock_.Lock();
	if (task) {
		tasks_.push_front(task);
		tasks_size_++;
	}
	ret = (int)tasks_size_;
	spinLock_.Unlock();
	return ret;
}

int utils::ThreadTaskQueue::Put(Runnable *task) {
	int ret = 0;
	spinLock_.Lock();
	if (task) {
		tasks_.push_back(task);
		tasks_size_++;
	}
	ret = (int)tasks_size_;
	spinLock_.Unlock();
	return ret;
}


int utils::ThreadTaskQueue::Size() {
	int ret = 0;
	spinLock_.Lock();
	ret = (int)tasks_size_;
	spinLock_.Unlock();
	return tasks_size_;
};

utils::Runnable *utils::ThreadTaskQueue::Get() {
	Runnable *task = NULL;
	spinLock_.Lock();
	if (tasks_size_ > 0) {
		task = tasks_.front();
		tasks_.pop_front();
		tasks_size_--;
	}
	spinLock_.Unlock();
	return task;
}

utils::ThreadPool::ThreadPool() : enabled_(false) {}

utils::ThreadPool::~ThreadPool() {
	for (size_t i = 0; i < threads_.size(); i++) {
		if (threads_[i]) delete threads_[i];
	}
}

bool utils::ThreadPool::Init(const std::string &name, int threadNum) {
	name_ = name;
	enabled_ = true;
	return AddWorker(threadNum);
}

bool utils::ThreadPool::Exit() {
	enabled_ = false;
	for (size_t i = 0; i < threads_.size(); i++) {
		if (threads_[i]) threads_[i]->JoinWithStop();
	}

	return true;
}

void utils::ThreadPool::AddTask(Runnable *task) {
	tasks_.Put(task);
}

void utils::ThreadPool::JoinwWithStop() {
	enabled_ = false;
	for (ThreadVector::const_iterator it = threads_.begin(); it != threads_.end(); ++it) {
		(*it)->JoinWithStop();
	}
	threads_.clear();
}

bool utils::ThreadPool::WaitAndJoin() {

	while (tasks_.Size() > 0)
		Sleep(1);

	enabled_ = false;
	for (size_t i = 0; i < threads_.size(); i++) {
		if (threads_[i]) threads_[i]->JoinWithStop();
	}

	return true;
}

bool utils::ThreadPool::WaitTaskComplete() {
	while (tasks_.Size() > 0)
		Sleep(1);
	return true;
}

int utils::ThreadPool::GetTaskSize() {
	return tasks_.Size();
}

void utils::ThreadPool::Terminate() {
	for (ThreadVector::const_iterator it = threads_.begin(); it != threads_.end(); ++it) {
		(*it)->Terminate();
	}
}

bool utils::ThreadPool::AddWorker(int threadNum) {
	for (int i = 0; i < threadNum; ++i) {
		Thread *thread = new Thread(this);
		threads_.push_back(thread);
		bool ret = thread->Start(utils::String::Format("worker-%s-%d", name_.c_str(),i));
		if ( !ret ){
			return false;
		} 
	}

	return true;
}

void utils::ThreadPool::Run(Thread *this_thread) {
	while (enabled_) {
		utils::Runnable *task = tasks_.Get();
		if (task) task->Run(this_thread);
		else utils::Sleep(1);
	}
}

utils::Event::Event() {
	m_bValid = false;
	m_nError = ERROR_SUCCESS;

	Create();
}

utils::Event::Event(const std::string &strName) {
	m_bValid = false;
	m_nError = ERROR_SUCCESS;
	m_strName = strName;

	Create();
}

utils::Event::~Event() {
	if (m_bValid) {
		Close();
	}
}

bool utils::Event::Create() {
	VALIDATE_ERROR_RETURN(m_bValid, ERROR_ALREADY_EXISTS, false);
	m_nError = (uint32_t)pthread_cond_init(&m_nObject, NULL);
	m_bValid = (0 == m_nError);
	return m_bValid;
}

bool utils::Event::Create(const std::string &strName) {
	VALIDATE_ERROR_RETURN(m_bValid, ERROR_ALREADY_EXISTS, false);

	m_strName = strName;
	return Create();
}

bool utils::Event::Close() {
	VALIDATE_ERROR_RETURN(!m_bValid, ERROR_NOT_READY, false);
	pthread_cond_destroy(&m_nObject);
	m_bValid = false;
	return true;
}

bool utils::Event::IsValid() const {
	return m_bValid;
}

bool utils::Event::Wait(int nTimeoutMs) {
	VALIDATE_ERROR_RETURN(!m_bValid, ERROR_NOT_READY, false);
	m_nLocker.Lock();

	if (nTimeoutMs < 0) {
		m_nError = pthread_cond_wait(&m_nObject, m_nLocker.mutex_pointer());
	}
	else {
		const long n1E9 = 1000 * 1000 * 1000;
		timeval nTimeNow = { 0, 0 };
		timespec nTimeout = { 0, 0 };
		gettimeofday(&nTimeNow, 0);

		nTimeout.tv_sec = nTimeNow.tv_sec + nTimeoutMs / 1000;
		nTimeout.tv_nsec = nTimeNow.tv_usec * 1000 + (nTimeoutMs % 1000) * 1000 * 1000;

		if (nTimeout.tv_nsec > n1E9) {
			nTimeout.tv_sec += nTimeout.tv_nsec / n1E9;
			nTimeout.tv_nsec %= n1E9;
		}

		m_nError = pthread_cond_timedwait(&m_nObject, m_nLocker.mutex_pointer(), &nTimeout);
	}

	m_nLocker.Unlock();

	return 0 == m_nError;
}

bool utils::Event::ServiceWait(int nTimeoutMs) {
	// we do sleep under non-windows because of daemon wait for signal only, no event
	usleep(((__useconds_t)nTimeoutMs) * 1000);
	return false;
}

bool utils::Event::Trigger() {
	VALIDATE_ERROR_RETURN(!m_bValid, ERROR_NOT_READY, false);
	return 0 == pthread_cond_signal(&m_nObject);
}

bool utils::Event::Reset()  {
	VALIDATE_ERROR_RETURN(!m_bValid, ERROR_NOT_READY, false);
	// no need for linux, auto release
	// even pthread_cond_signal will not set the signal when no thread wait.
	return true;
}

bool utils::Event::Broadcast() {
	VALIDATE_ERROR_RETURN(!m_bValid, ERROR_NOT_READY, false);
	return 0 == pthread_cond_broadcast(&m_nObject);
}

bool utils::Event::IsTimeout() const {
	return ETIMEDOUT == m_nError;
}


