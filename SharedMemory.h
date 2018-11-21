#pragma once
#include <sys/types.h>
#include <sys/shm.h>
#include <memory.h>

enum Permission {
	CREATE_RW,
	CREATE_R,
	OPEN_RW,
	OPEN_R,
};

template<typename T>
class SharedMemory
{
public:
	SharedMemory(const char* s_name, int s_id, Permission s_permission);
	SharedMemory(key_t s_key, Permission s_permission);
	~SharedMemory();

	bool read(T* output);
	bool write(T* input);
private:
	bool s_writeable;
	key_t s_fileHandle;
	int s_mapHandle;
	T* s_sharedData;

	void init();
};

template<typename T>
SharedMemory<T>::SharedMemory(const char* s_name, int s_id, Permission s_permission)
	: s_writeable(s_permission == CREATE_RW || s_permission == OPEN_RW)
{
    /*Here the file must exist */
    if ((s_fileHandle = ftok(s_name, s_id)) == -1) 
    {
        throw -1;
    }
    init();
}

template<typename T>
SharedMemory<T>::SharedMemory(key_t s_key, Permission s_permission)
	: s_writeable(s_permission == CREATE_RW || s_permission == OPEN_RW)
{
    s_fileHandle = s_key;
    init();
}

template<typename T>
SharedMemory<T>::~SharedMemory()
{
    if (s_sharedData !=nullptr)
    {
        shmdt(s_sharedData);
    }

    if (s_mapHandle != -1)
    {
        shmctl(s_mapHandle,IPC_RMID, NULL);
    }
};

template<typename T>
bool SharedMemory<T>::read(T* output) {
	if (s_sharedData == nullptr) {
		return false;
	}

	memcpy(output, s_sharedData, sizeof(T));
	return true;
};

template<typename T>
bool SharedMemory<T>::write(T* input) {
	if (input == nullptr) {
		return false;
	}

	if (!s_writeable) {
		return false;
	}

	memcpy(s_sharedData, input, sizeof(T));
	return 0;
};

template<typename T>
void SharedMemory<T>::init() {
    s_mapHandle = shmget(s_fileHandle ,sizeof(T), IPC_CREAT);
    if (s_mapHandle == -1)
    {
        throw -2;
    }

    s_sharedData = (T*)shmat(s_mapHandle, NULL, 0);
    if (s_sharedData == nullptr)
    {
        throw -3;
    }

    memset(s_sharedData, 0, sizeof(T));
};
