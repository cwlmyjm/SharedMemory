#pragma once
#include <Windows.h>
#include <functional>

#ifdef _DEBUG
#include <assert.h>
#else
#define assert(exp)
#endif

#ifdef UNICODE
#define _CHAR wchar_t
#define Format(str) L##str
#else
#define _CHAR char
#define Format(str) str
#endif

namespace SharedMemoryTemplate{

	enum Permission {
		CREATE_RW,
		CREATE_R,
		OPEN_RW,
		OPEN_R,
	};

	enum ErrorCode{
		Error_None = 0,
		Error_NullMapHandle = 1,
		Error_NullMapView = 2,
		Error_NullDefault = 3,
	};


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
		R apply(std::function<R(T*)> fun);
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
		if (s_permission == CREATE_RW) {
			s_mapHandle = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, sizeof(T), s_name);
		}
		else if (s_permission == CREATE_R) {
			s_mapHandle = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READONLY, 0, sizeof(T), s_name);
		}
		else if(s_permission == OPEN_RW){
			s_mapHandle = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, s_name);
		}
		else if (s_permission == OPEN_R){
			s_mapHandle = OpenFileMapping(FILE_MAP_READ, FALSE, s_name);
		}
		if (s_mapHandle == nullptr) {
			throw ErrorCode::Error_NullMapHandle;
		}

		if (s_writeable) {
			s_sharedData = (T*)MapViewOfFile(s_mapHandle, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(T));
		}
		else {
			s_sharedData = (T*)MapViewOfFile(s_mapHandle, FILE_MAP_READ, 0, 0, sizeof(T));
		}
		if (s_sharedData == nullptr) {
			throw ErrorCode::Error_NullMapView;
		}
	};

	template<typename T>
	SharedMemory<T>::SharedMemory(const _CHAR* s_name, Permission s_permission, T* s_default)
		: SharedMemory(s_name, s_permission)
	{
		if (s_default == nullptr)
		{
			throw ErrorCode::Error_NullDefault;
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

		assert(output != nullptr);

		if (output == nullptr) {
			return false;
		}

		memcpy(output, s_sharedData, sizeof(T));
		return true;

	}

	template<typename T>
	bool SharedMemory<T>::write(T* input) {

		assert(input != nullptr);

		if (input == nullptr) {
			return false;
		}

		assert(s_writeable);

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
		: public SharedMemory < T >
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

	template<typename T, int C>
	class SharedMemoryBuffer
	{
	public:
		SharedMemoryBuffer(const _CHAR* s_name, const _CHAR* sem_name, Permission s_permission);
		~SharedMemoryBuffer();

		bool read(T* output);
		bool write(T* input);

	private:
		bool s_writeable;
		HANDLE s_mapHandle;
		T* s_sharedData;
		int readIndex = 0;
		int writeIndex = 0;
		HANDLE readSemHandle = NULL;
		HANDLE writeSemHandle = NULL;
	};

	template<typename T, int C>
	SharedMemoryBuffer<T, C>::SharedMemoryBuffer(const _CHAR* s_name, const _CHAR* sem_name, Permission s_permission)
		: s_writeable(s_permission == CREATE_RW || s_permission == OPEN_RW)
	{
		if (s_permission == CREATE_RW) {
			s_mapHandle = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, sizeof(T) * C, s_name);
		}
		else if (s_permission == CREATE_R) {
			s_mapHandle = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READONLY, 0, sizeof(T) * C, s_name);
		}
		else if (s_permission == OPEN_RW){
			s_mapHandle = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, s_name);
		}
		else if (s_permission == OPEN_R){
			s_mapHandle = OpenFileMapping(FILE_MAP_READ, FALSE, s_name);
		}
		if (s_mapHandle == nullptr) {
			throw ErrorCode::Error_NullMapHandle;
		}

		if (s_writeable) {
			s_sharedData = (T*)MapViewOfFile(s_mapHandle, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(T) * C);
		}
		else {
			s_sharedData = (T*)MapViewOfFile(s_mapHandle, FILE_MAP_READ, 0, 0, sizeof(T) * C);
		}
		if (s_sharedData == nullptr) {
			throw ErrorCode::Error_NullMapView;
		}

		_CHAR readSemName[128] = { 0 };
		_CHAR writeSemName[128] = { 0 };
		wsprintf(readSemName, Format("%sRead"), sem_name);
		wsprintf(writeSemName, Format("%sWrite"), sem_name);
		readSemHandle = CreateSemaphore(NULL, 0, C, readSemName);
		writeSemHandle = CreateSemaphore(NULL, C, C, writeSemName);
	}

	template<typename T, int C>
	SharedMemoryBuffer<T, C>::~SharedMemoryBuffer()
	{
		// 释放共享内存
		if (s_sharedData != nullptr) UnmapViewOfFile(s_sharedData);

		// 关闭对象句柄
		if (s_mapHandle != nullptr) CloseHandle(s_mapHandle);

		CloseHandle(readSemHandle);
		CloseHandle(writeSemHandle);
	}

	template<typename T, int C>
	bool SharedMemoryBuffer<T, C>::read(T* output)
	{
		assert(output != nullptr);

		if (output == nullptr) {
			return false;
		}

		WaitForSingleObject(readSemHandle, INFINITE);
		{
			memcpy(output, s_sharedData + readIndex, sizeof(T));
			readIndex = (readIndex + 1) % C;
		}
		ReleaseSemaphore(writeSemHandle, 1, NULL);
		return true;
	}

	template<typename T, int C>
	bool SharedMemoryBuffer<T, C>::write(T* input)
	{
		assert(input != nullptr);
		
		if (input == nullptr) {
			return false;
		}

		assert(s_writeable);

		if (!s_writeable) {
			return false;
		}

		WaitForSingleObject(writeSemHandle, INFINITE);
		{
			memcpy(s_sharedData + writeIndex, input, sizeof(T));
			writeIndex = (writeIndex + 1) % C;
		}
		ReleaseSemaphore(readSemHandle, 1, NULL);
		return true;
	}
}
