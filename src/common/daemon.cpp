
#include <utils/timestamp.h>
#include <utils/logger.h>
#include "daemon.h"
#include <utils/file.h>
#include <fcntl.h>

namespace utils {
	Daemon::Daemon() : TimerNotify("Daemon", 0) {
		last_write_time_ = 0;
		shared = NULL;
	}

	Daemon::~Daemon() {}

	//void Daemon::GetModuleStatus(Json::Value &data) const {
	//}

	bool Daemon::Initialize(int32_t key) {
		agent::TimerNotify::RegisterModule(this);
		//Initialize mutex
		int fd;
		pthread_mutexattr_t mattr;
		fd = open("/dev/zero", O_RDWR, 0);
		mptr = (pthread_mutex_t*)mmap(0, sizeof(pthread_mutex_t), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
		close(fd);
		pthread_mutexattr_init(&mattr);
		pthread_mutexattr_setpshared(&mattr, PTHREAD_PROCESS_SHARED);
		pthread_mutex_init(mptr, &mattr);

		//Allocate shared memory
		shmid = shmget((key_t)key, sizeof(int64_t), 0666 | IPC_CREAT);
		if (shmid == -1) {
			LOG_ERROR("Failed to initialize daemon, invalid shmget");
			return true;
		}
		//Attach the shared memory at the the current thread 
		shm = shmat(shmid, (void*)0, 0);
		if (shm == (void*)-1) {
			LOG_ERROR("Failed to initialize daemon, invalid shmget");
			return false;
		}
		LOG_INFO("Attached to shared memory address at %lx\n", (unsigned long int)shm);
		//Set the shared memory
		shared = (int64_t*)shm;
		return true;
	}

	bool Daemon::Exit() {
		//Detach the shared memory from the current thread
		if (shmdt(shm) == -1) {
			LOG_ERROR("Failed to exit daemon,shmdt failed");
			return false;
		}
		return true;
	}

	void Daemon::OnTimer(int64_t current_time) {
		//int64_t now_time = utils::Timestamp::GetLocalTimestamp(,);
		//int64_t now_time = utils::Timestamp::Now().timestamp();
		if (current_time - last_write_time_ > 500000) {
			pthread_mutex_lock(mptr);
			if (shared) *shared = current_time;
			last_write_time_ = current_time;
			pthread_mutex_unlock(mptr);
		}
	}

	void Daemon::OnSlowTimer(int64_t current_time) {

	}
}
