#pragma once
#include <Windows.h>
#include <functional>

enum Permission {
	CREATE_RW,
	CREATE_R,
	OPEN_RW,
	OPEN_R,
};

#ifdef UNICODE
#define _CHAR wchar_t
#else
#define _CHAR char
#endif

template<typename T>
class SharedMemory
{
public:
	SharedMemory(const _CHAR* s_name, Permission s_permission);
	SharedMemory(const _CHAR* s_name, Permission s_permission, T* s_default);
	~SharedMemory();

	bool read(T* output);
	bool write(T* input);

	template<typename R>
	R apply(std::function<R (T*)> fun);
	T* get();

private:
	bool s_writeable;
	HANDLE s_mapHandle;
	T* s_sharedData;
};

template<typename T>
SharedMemory<T>::SharedMemory(const _CHAR* s_name, Permission s_permission)
	: s_writeable(s_permission == CREATE_RW || s_permission == OPEN_RW)
{
	if (s_permission == CREATE_RW || s_permission == CREATE_R) {
		s_mapHandle = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, sizeof(T), s_name);
	}
	else {
		s_mapHandle = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, s_name);
	}
	if (s_mapHandle == nullptr) {
		throw - 2;
	}

	if (s_writeable) {
		s_sharedData = (T*)MapViewOfFile(s_mapHandle, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(T));
	}
	else {
		s_sharedData = (T*)MapViewOfFile(s_mapHandle, FILE_MAP_READ, 0, 0, sizeof(T));
	}
	if (s_sharedData == nullptr) {
		throw - 3;
	}
};

template<typename T>
SharedMemory<T>::SharedMemory(const _CHAR* s_name, Permission s_permission, T* s_default)
	: SharedMemory(s_name, s_permission)
{
	if (s_default == nullptr)
	{
		throw - 4;
	}
	memcpy(s_sharedData, s_default, sizeof(T));
};

template<typename T>
SharedMemory<T>::~SharedMemory()
{
	// 释放共享内存
	if (s_sharedData != nullptr) UnmapViewOfFile(s_sharedData);

	// 关闭对象句柄
	if (s_mapHandle != nullptr) CloseHandle(s_mapHandle);
};

template<typename T>
bool SharedMemory<T>::read(T* output) {
	if (s_sharedData == nullptr) {
		return false;
	}

	memcpy(output, s_sharedData, sizeof(T));
	return true;

}

template<typename T>
bool SharedMemory<T>::write(T* input) {
	if (input == nullptr) {
		return false;
	}

	if (!s_writeable) {
		return false;
	}

	memcpy(s_sharedData, input, sizeof(T));
	return true;
}

template<typename T>
template<typename R>
R SharedMemory<T>::apply(std::function<R(T*)> fun)
{
	return fun(s_sharedData);
};

template<typename T>
T* SharedMemory<T>::get() {
	return s_sharedData;
};

template<typename T>
class MutexSharedMemory
	: public SharedMemory<T>
{
public:
	MutexSharedMemory(const _CHAR* s_name, const _CHAR* m_name, Permission s_permission);
	MutexSharedMemory(const _CHAR* s_name, const _CHAR* m_name, Permission s_permission, T* s_default);
	~MutexSharedMemory();

	bool mutex_read(T* output);
	bool mutex_write(T* input);

	template<typename R>
	R mutex_apply(std::function<R(T*)> fun);
private:
	HANDLE mutexHandle = NULL;
};

template<typename T>
MutexSharedMemory<T>::MutexSharedMemory(const _CHAR* s_name, const _CHAR* m_name, Permission s_permission)
	: SharedMemory<T>(s_name, s_permission)
{
	mutexHandle = CreateMutex(NULL, false, m_name);
};

template<typename T>
MutexSharedMemory<T>::MutexSharedMemory(const _CHAR* s_name, const _CHAR* m_name, Permission s_permission, T* s_default)
	: SharedMemory<T>(s_name, s_permission, s_default)
{
	mutexHandle = CreateMutex(NULL, false, m_name);
};

template<typename T>
MutexSharedMemory<T>::~MutexSharedMemory()
{
	if (mutexHandle != NULL)
	{
		CloseHandle(mutexHandle);
	}
};

template<typename T>
bool MutexSharedMemory<T>::mutex_read(T* output)
{
	bool result = false;
	WaitForSingleObject(mutexHandle, INFINITE);
	{
		result = SharedMemory<T>::read(output);
	}
	ReleaseMutex(mutexHandle);
	return result;
};

template<typename T>
bool MutexSharedMemory<T>::mutex_write(T* input)
{
	bool result = false;
	WaitForSingleObject(mutexHandle, INFINITE);
	{
		result = SharedMemory<T>::write(input);
	}
	ReleaseMutex(mutexHandle);
	return result;
};

template<typename T>
template<typename R>
R MutexSharedMemory<T>::mutex_apply(std::function<R(T*)> fun)
{
	R result;
	WaitForSingleObject(mutexHandle, INFINITE);
	{
		result = SharedMemory<T>::apply(fun);
	}
	ReleaseMutex(mutexHandle);
	return result;
}
