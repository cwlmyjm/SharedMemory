#pragma once
#include <Windows.h>

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
	~SharedMemory();

	bool read(T* output);
	bool write(T* input);
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
T* SharedMemory<T>::get() {
	return s_sharedData;
};
