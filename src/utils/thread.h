#ifndef UTILS_THREAD_H_
#define UTILS_THREAD_H_

#include "utils.h"

namespace utils {
	typedef std::function<void()> ThreadCallback;

	class Thread;
	class Runnable {
	public:
		Runnable() {}
		virtual ~Runnable() {}

		virtual void Run(Thread *this_thread) = 0;
	};

	//Thread 
	class Thread {
	public:
		explicit Thread() :
			target_(NULL)
			, enabled_(false)
			, running_(false)
			, handle_(Thread::INVALID_HANDLE)
			, thread_id_(0) {}

		explicit Thread(Runnable *target)
			: target_(target)
			, enabled_(false)
			, running_(false)
			, handle_(Thread::INVALID_HANDLE)
			, thread_id_(0) {}

		~Thread() {
		}

		//Stop thread, and return true if it succeeds.
		bool Stop();

		//Force to terminate the thread
		bool Terminate();

		bool Start(std::string name = "");

		//Stop and wait for the thead to be stopped
		bool JoinWithStop();

		bool enabled() const { return enabled_; };

		size_t thread_id() const { return thread_id_; };

		bool IsRunning() const { return running_; };

		//Get the current thread id
		static size_t current_thread_id();

		bool IsObjectValid() const { return Thread::INVALID_HANDLE != handle_; }

		static bool SetCurrentThreadName(std::string name);

		const std::string &GetName() const { return name_; }

	protected:
		UTILS_DISALLOW_EVIL_CONSTRUCTORS(Thread);
		std::string name_;
		static void *threadProc(void *param);
		volatile bool enabled_;
		bool running_;
		Runnable *target_;
		pthread_t handle_;
		static const pthread_t INVALID_HANDLE;
		size_t thread_id_;

	protected:
		virtual void Run();
	};

	//Thread group
	class ThreadGroup {
	public:
		ThreadGroup() {}
		~ThreadGroup() {
			JoinAll();
			for (size_t i = 0; i < _threads.size(); ++i) {
				delete _threads[i];
			}

			_threads.clear();
		}

		void AddThread(Thread *thread, std::string name = "") {
			_threads.push_back(thread);
			_thread_names.push_back(name);
		}

		void StartAll() {
			for (size_t i = 0; i < _threads.size(); ++i) {
				_threads[i]->Start(_thread_names[i]);
			}
		}

		void JoinAll() {
			for (size_t i = 0; i < _threads.size(); ++i) {
				_threads[i]->JoinWithStop();
			}
		}

		void StopAll() {
			for (size_t i = 0; i < _threads.size(); ++i) {
				_threads[i]->Stop();
			}
		}

		size_t size() const { return _threads.size(); }

	private:
		UTILS_DISALLOW_EVIL_CONSTRUCTORS(ThreadGroup);
		std::vector<Thread *> _threads;
		std::vector<std::string> _thread_names;
	};

	class Mutex {
	public:
		Mutex();
		~Mutex();

		void Lock() ;
		void Unlock() ;
		pthread_mutex_t *mutex_pointer(){ return &mutex_; }

	private:
		UTILS_DISALLOW_EVIL_CONSTRUCTORS(Mutex);

		uint32_t thread_id_;
		pthread_mutex_t mutex_;
	};

	class MutexGuard {
	public:
		MutexGuard(Mutex &mutex)
			: _mutex(mutex) {
			_mutex.Lock();
			is_locked = true;
		}

		~MutexGuard() {
			_mutex.Unlock();
			is_locked = false;
		}

		bool Lock()  {
			if (is_locked) {
				return true;
			}

			_mutex.Lock();
			is_locked = true;

			return true;
		}
		bool Unlock() {
			if (!is_locked) {
				return true;
			}

			_mutex.Unlock();
			is_locked = false;
			return true;
		}
	private:
		UTILS_DISALLOW_EVIL_CONSTRUCTORS(MutexGuard);
		Mutex &_mutex;
		bool is_locked;
	};

	class ReadWriteLock {
	public:
		ReadWriteLock();
		~ReadWriteLock();

		void ReadLock();
		void ReadUnlock();
		void WriteLock();
		void WriteUnlock();

	private:
		UTILS_DISALLOW_EVIL_CONSTRUCTORS(ReadWriteLock);
		volatile long _reads;
		Mutex _enterLock;
	};

	class ReadLockGuard {
	public:
		ReadLockGuard(ReadWriteLock &lock)
			: lock_(lock) {
			lock_.ReadLock();
		}
		~ReadLockGuard() { lock_.ReadUnlock(); }
	private:
		UTILS_DISALLOW_EVIL_CONSTRUCTORS(ReadLockGuard);
		ReadWriteLock &lock_;
	};

	class WriteLockGuard {
	public:
		WriteLockGuard(ReadWriteLock &lock)
			: lock_(lock) {
			lock_.WriteLock();
		}
		~WriteLockGuard() { lock_.WriteUnlock(); }
	private:
		UTILS_DISALLOW_EVIL_CONSTRUCTORS(WriteLockGuard);
		ReadWriteLock &lock_;
	};


	// Implement a spin lock using lock free.

	class SpinLock {
	public:
		/** ctor */
		SpinLock() :m_busy(SPINLOCK_FREE) {

		}

		/** dtor */
		virtual	~SpinLock() {

		}

		// Lock
		inline void Lock() {
			while (SPINLOCK_BUSY == LOCK_CAS(&m_busy, SPINLOCK_BUSY, SPINLOCK_FREE)) {
				LOCK_YIELD();
			}
		}

		// Unlock
		inline void Unlock() {
			LOCK_CAS(&m_busy, SPINLOCK_FREE, SPINLOCK_BUSY);
		}

	private:
		SpinLock(const SpinLock&);
		SpinLock& operator = (const SpinLock&);
		volatile uint32_t m_busy;
		static const int SPINLOCK_FREE = 0;
		static const int SPINLOCK_BUSY = 1;
	};

#define ReadLockGuard(x) error "Missing guard object name"
#define WriteLockGuard(x) error "Missing guard object name"

	class Semaphore {
	public:
		static const uint32_t kInfinite = UINT_MAX;
		Semaphore(int32_t num = 0);
		~Semaphore();

		// P
		bool Wait(uint32_t millisecond = kInfinite);

		// V
		bool Signal();

	private:
		UTILS_DISALLOW_EVIL_CONSTRUCTORS(Semaphore);
		sem_t sem_;
	};

	class ThreadTaskQueue {
	public:
		ThreadTaskQueue();
		~ThreadTaskQueue();

		int PutFront(Runnable *task);
		int Put(Runnable *task);
		int Size();
		Runnable *Get();

	private:
		UTILS_DISALLOW_EVIL_CONSTRUCTORS(ThreadTaskQueue);
		typedef std::list<Runnable *> Tasks;
		Tasks tasks_;
		size_t tasks_size_;
		utils::Mutex spinLock_;
		Semaphore sem_;
	};

	class ThreadPool : public Runnable {
	public:
		ThreadPool();
		~ThreadPool();

		bool Init(const std::string &name, int threadNum = kDefaultThreadNum);
		bool Exit();

		//Add a task
		void AddTask(Runnable *task);

		void JoinwWithStop();

		/// Wait all tasks to join 
		bool WaitAndJoin();

		// Get the thread's size
		size_t Size() const { return threads_.size(); }
		int GetTaskSize();

		bool WaitTaskComplete();

		//Terminate the thread
		void Terminate();

	private:
		UTILS_DISALLOW_EVIL_CONSTRUCTORS(ThreadPool);
		typedef std::vector<Thread *> ThreadVector;

		//Add a worker
		bool AddWorker(int threadNum);

		void Run(Thread *this_thread);

		ThreadVector threads_;
		ThreadTaskQueue tasks_;
		bool enabled_;
		std::string name_;

		static const int32_t kDefaultThreadNum = 10;
	};

	class Event {
		DISALLOW_COPY_AND_ASSIGN(Event);

	private:
		bool m_bValid;
		std::string m_strName;
		mutable uint32_t m_nError;
		Mutex m_nLocker;
		mutable pthread_cond_t m_nObject;
	public:
		Event();
		explicit Event(const std::string &strName);
		virtual ~Event();

		bool Create();
		bool Create(const std::string &strName);
		bool Close();
		bool IsValid() const;
		bool Wait(int nTimeoutMs);
		bool ServiceWait(int nTimeoutMs) ;
		bool Trigger() ;
		bool Reset() ;
		bool Broadcast() ;
		bool IsTimeout()const;
	};
}

#endif // _UTILS_THREAD_H_
