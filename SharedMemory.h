/*!
* @file   SharedMemory.h
* @author Jinming YAO
* @param  Email: me@ysyy.xyz
* @date   2018_04_26
* @template for shared memory
*/

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
		SharedMemory(const _CHAR* name, Permission permission);
		SharedMemory(const _CHAR* name, Permission permission, T* pDefault);
		SharedMemory(const SharedMemory<T>& other) = delete;
		~SharedMemory();
		SharedMemory<T>& operator=(const SharedMemory<T>& other) = delete;

		bool read(T* pOutput);
		bool write(T* pInput);

		bool read_void(void* pOutput);
		bool write_void(void* pInput);

		template<typename R>
		R apply(std::function<R(T*)> fun);
		T* get();

	private:
		bool m_writeable;
		HANDLE m_mapHandle;
		T* m_sharedData;
	};

	template<typename T>
	SharedMemory<T>::SharedMemory(const _CHAR* name, Permission permission)
		: m_writeable(permission == CREATE_RW || permission == OPEN_RW)
	{
		if (permission == CREATE_RW) {
			m_mapHandle = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, sizeof(T), name);
		}
		else if (permission == CREATE_R) {
			m_mapHandle = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READONLY, 0, sizeof(T), name);
		}
		else if(permission == OPEN_RW){
			m_mapHandle = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, name);
		}
		else if (permission == OPEN_R){
			m_mapHandle = OpenFileMapping(FILE_MAP_READ, FALSE, name);
		}
		if (m_mapHandle == nullptr) {
			throw ErrorCode::Error_NullMapHandle;
		}

		if (m_writeable) {
			m_sharedData = (T*)MapViewOfFile(m_mapHandle, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(T));
		}
		else {
			m_sharedData = (T*)MapViewOfFile(m_mapHandle, FILE_MAP_READ, 0, 0, sizeof(T));
		}
		if (m_sharedData == nullptr) {
			throw ErrorCode::Error_NullMapView;
		}
	};

	template<typename T>
	SharedMemory<T>::SharedMemory(const _CHAR* name, Permission permission, T* pDefault)
		: SharedMemory(name, permission)
	{
		if (pDefault == nullptr)
		{
			throw ErrorCode::Error_NullDefault;
		}
		memcpy(m_sharedData, pDefault, sizeof(T));
	};

	template<typename T>
	SharedMemory<T>::~SharedMemory()
	{
		// 释放共享内存
		if (m_sharedData != nullptr) UnmapViewOfFile(m_sharedData);

		// 关闭对象句柄
		if (m_mapHandle != nullptr) CloseHandle(m_mapHandle);
	};

	template<typename T>
	bool SharedMemory<T>::read(T* pOutput) {

		assert(pOutput != nullptr);

		if (pOutput == nullptr) {
			return false;
		}

		memcpy(pOutput, m_sharedData, sizeof(T));
		return true;

	}

	template<typename T>
	bool SharedMemory<T>::write(T* pInput) {

		assert(pInput != nullptr);

		if (pInput == nullptr) {
			return false;
		}

		assert(m_writeable);

		if (!m_writeable) {
			return false;
		}

		memcpy(m_sharedData, pInput, sizeof(T));
		return true;
	}

	template<typename T>
	bool SharedMemory<T>::read_void(void* pOutput) {

		assert(pOutput != nullptr);

		if (pOutput == nullptr) {
			return false;
		}

		memcpy(pOutput, m_sharedData, sizeof(T));
		return true;

	}

	template<typename T>
	bool SharedMemory<T>::write_void(void* pInput) {

		assert(pInput != nullptr);

		if (pInput == nullptr) {
			return false;
		}

		assert(m_writeable);

		if (!m_writeable) {
			return false;
		}

		memcpy(m_sharedData, pInput, sizeof(T));
		return true;
	}

	template<typename T>
	template<typename R>
	R SharedMemory<T>::apply(std::function<R(T*)> fun)
	{
		return fun(m_sharedData);
	};

	template<typename T>
	T* SharedMemory<T>::get() {
		return m_sharedData;
	};

	template<typename T>
	class MutexSharedMemory
		: public SharedMemory < T >
	{
	private:
		class Mutex
		{
		private:
			HANDLE m_handle;
		public:
			Mutex(HANDLE handle)
				: m_handle(handle)
			{
				WaitForSingleObject(m_handle, INFINITE);
			}

			~Mutex()
			{
				ReleaseMutex(m_handle);
			}
		};

	public:
		MutexSharedMemory(const _CHAR* name, const _CHAR* mutexName, Permission permission);
		MutexSharedMemory(const _CHAR* name, const _CHAR* mutexName, Permission permission, T* pDefault);
		MutexSharedMemory(const MutexSharedMemory<T>& other) = delete;
		~MutexSharedMemory();
		MutexSharedMemory<T>& operator=(const MutexSharedMemory<T>& other) = delete;

		bool mutex_read(T* pOutput);
		bool mutex_write(T* pInput);

		bool mutex_read_void(void* pOutput);
		bool mutex_write_void(void* pInput);

		template<typename R>
		R mutex_apply(std::function<R(T*)> fun);
	private:
		HANDLE m_mutexHandle = NULL;
	};

	template<typename T>
	MutexSharedMemory<T>::MutexSharedMemory(const _CHAR* name, const _CHAR* mutexName, Permission permission)
		: SharedMemory<T>(name, permission)
	{
		m_mutexHandle = CreateMutex(NULL, false, mutexName);
	};

	template<typename T>
	MutexSharedMemory<T>::MutexSharedMemory(const _CHAR* name, const _CHAR* mutexName, Permission permission, T* pDefault)
		: SharedMemory<T>(name, permission, pDefault)
	{
		m_mutexHandle = CreateMutex(NULL, false, mutexName);
	};

	template<typename T>
	MutexSharedMemory<T>::~MutexSharedMemory()
	{
		if (m_mutexHandle != NULL)
		{
			CloseHandle(m_mutexHandle);
		}
	};

	template<typename T>
	bool MutexSharedMemory<T>::mutex_read(T* pOutput)
	{
		Mutex mutex(m_mutexHandle);
		return SharedMemory<T>::read(pOutput);
	};

	template<typename T>
	bool MutexSharedMemory<T>::mutex_write(T* pInput)
	{
		Mutex mutex(m_mutexHandle);
		return SharedMemory<T>::write(pInput);
	};

	template<typename T>
	bool MutexSharedMemory<T>::mutex_read_void(void* pOutput)
	{
		Mutex mutex(m_mutexHandle);
		return SharedMemory<T>::read_void(pOutput);
	};

	template<typename T>
	bool MutexSharedMemory<T>::mutex_write_void(void* pInput)
	{
		Mutex mutex(m_mutexHandle);
		return SharedMemory<T>::write_void(pInput);
	};

	template<typename T>
	template<typename R>
	R MutexSharedMemory<T>::mutex_apply(std::function<R(T*)> fun)
	{
		Mutex mutex(m_mutexHandle);
		return SharedMemory<T>::apply(fun);
	}

	template<typename T, int C>
	class SharedMemoryBuffer
	{
	public:
		SharedMemoryBuffer(const _CHAR* name, const _CHAR* semName, Permission permission);
		SharedMemoryBuffer(const SharedMemoryBuffer<T, C>& other) = delete;
		~SharedMemoryBuffer();
		SharedMemoryBuffer<T, C>& operator=(const SharedMemoryBuffer<T, C>& other) = delete;

		bool read(T* pOutput);
		bool write(T* pInput);

	private:
		bool m_writeable;
		HANDLE m_mapHandle;
		T* m_sharedData;
		int m_readIndex = 0;
		int m_writeIndex = 0;
		HANDLE m_readSemHandle = NULL;
		HANDLE m_writeSemHandle = NULL;
	};

	template<typename T, int C>
	SharedMemoryBuffer<T, C>::SharedMemoryBuffer(const _CHAR* name, const _CHAR* semName, Permission permission)
		: m_writeable(permission == CREATE_RW || permission == OPEN_RW)
	{
		if (permission == CREATE_RW) {
			m_mapHandle = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, sizeof(T) * C, name);
		}
		else if (permission == CREATE_R) {
			m_mapHandle = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READONLY, 0, sizeof(T) * C, name);
		}
		else if (permission == OPEN_RW){
			m_mapHandle = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, name);
		}
		else if (permission == OPEN_R){
			m_mapHandle = OpenFileMapping(FILE_MAP_READ, FALSE, name);
		}
		if (m_mapHandle == nullptr) {
			throw ErrorCode::Error_NullMapHandle;
		}

		if (m_writeable) {
			m_sharedData = (T*)MapViewOfFile(m_mapHandle, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(T) * C);
		}
		else {
			m_sharedData = (T*)MapViewOfFile(m_mapHandle, FILE_MAP_READ, 0, 0, sizeof(T) * C);
		}
		if (m_sharedData == nullptr) {
			throw ErrorCode::Error_NullMapView;
		}

		_CHAR readSemName[128] = { 0 };
		_CHAR writeSemName[128] = { 0 };
		wsprintf(readSemName, Format("%sRead"), semName);
		wsprintf(writeSemName, Format("%sWrite"), semName);
		m_readSemHandle = CreateSemaphore(NULL, 0, C, readSemName);
		m_writeSemHandle = CreateSemaphore(NULL, C, C, writeSemName);
	}

	template<typename T, int C>
	SharedMemoryBuffer<T, C>::~SharedMemoryBuffer()
	{
		// 释放共享内存
		if (m_sharedData != nullptr) UnmapViewOfFile(m_sharedData);

		// 关闭对象句柄
		if (m_mapHandle != nullptr) CloseHandle(m_mapHandle);

		CloseHandle(m_readSemHandle);
		CloseHandle(m_writeSemHandle);
	}

	template<typename T, int C>
	bool SharedMemoryBuffer<T, C>::read(T* pOutput)
	{
		assert(pOutput != nullptr);

		if (pOutput == nullptr) {
			return false;
		}

		WaitForSingleObject(m_readSemHandle, INFINITE);
		{
			memcpy(pOutput, m_sharedData + m_readIndex, sizeof(T));
			m_readIndex = (m_readIndex + 1) % C;
		}
		ReleaseSemaphore(m_writeSemHandle, 1, NULL);
		return true;
	}

	template<typename T, int C>
	bool SharedMemoryBuffer<T, C>::write(T* pInput)
	{
		assert(pInput != nullptr);
		
		if (pInput == nullptr) {
			return false;
		}

		assert(m_writeable);

		if (!m_writeable) {
			return false;
		}

		WaitForSingleObject(m_writeSemHandle, INFINITE);
		{
			memcpy(m_sharedData + m_writeIndex, pInput, sizeof(T));
			m_writeIndex = (m_writeIndex + 1) % C;
		}
		ReleaseSemaphore(m_readSemHandle, 1, NULL);
		return true;
	}
}
