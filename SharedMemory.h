#pragma once
#include <Windows.h>

template<typename T>
class SharedMemory
{
public:
	SharedMemory(const char* s_name, bool s_writeable);
	~SharedMemory();

	bool read(T* output);
	bool write(T* input);
private:
	bool s_writeable;
	HANDLE s_fileHandle;
	HANDLE s_mapHandle;
	T* s_sharedData;
};

template<typename T>
SharedMemory<T>::SharedMemory(const char* s_name, bool s_writeable) : s_writeable(s_writeable)
{
	if (s_writeable){
		s_fileHandle = CreateFile(s_name, GENERIC_READ | GENERIC_WRITE,
			0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

		if (s_fileHandle == nullptr) {
			throw - 1;
		}

		s_mapHandle = CreateFileMapping(s_fileHandle, NULL, PAGE_READWRITE, 0, sizeof(T), s_name);

		if (s_fileHandle == nullptr) {
			throw - 2;
		}

		s_sharedData = (T*)MapViewOfFile(s_mapHandle, FILE_MAP_WRITE, 0, 0, sizeof(T));

		if (s_sharedData == nullptr) {
			throw - 3;
		}
	}
	else{
		s_mapHandle = OpenFileMapping(FILE_MAP_READ, FALSE, s_name);

		if (s_mapHandle == nullptr) {
			throw - 1;
		}

		s_sharedData = (T*)MapViewOfFile(s_mapHandle, FILE_MAP_READ, 0, 0, sizeof(T));

		if (s_sharedData == nullptr) {
			throw - 2;
		}
	}

};

template<typename T>
SharedMemory<T>::~SharedMemory()
{
	// 释放共享内存
	if (s_shareData != nullptr) UnmapViewOfFile(s_shareData);

	// 关闭对象句柄
	if (s_mapHandle != nullptr) CloseHandle(s_mapHandle);

	// 关闭文件句柄
	if (s_fileHandle != nullptr) CloseHandle(s_fileHandle);

};

template<typename T>
bool SharedMemory<T>::read(T* output){
	if (s_sharedData == nullptr) {
		return false;
	}

	memcpy(output, s_sharedData, sizeof(T));
	return true;

}

template<typename T>
bool SharedMemory<T>::write(T* input){
	if (input == nullptr) {
		return false;
	}

	if (!s_writeable) {
		return false;
	}

	memcpy(s_sharedData, input, sizeof(T));
	return 0;
}

