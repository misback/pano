#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <string>
#ifdef __cplusplus
extern "C" {
#endif
    bool saveToJPG(unsigned char* _data, bool bAlpha, int _width, int _height, const std::string& filePath);
    bool saveToPNG(unsigned char* _data, bool bAlpha, int _width, int _height, const std::string& filePath);
    bool saveToJPG_FlipY(unsigned char* _data, bool bAlpha, int _width, int _height, const std::string& filePath);
    bool read_JPEG_file_data(unsigned char*imageData, ssize_t unpackedLen, unsigned char* &_data, int& _width, int& _height, ssize_t& _dataLen);
    bool read_JPEG_file (const char* filePath, unsigned char* &_data, int& _width, int& _height, int& _dataLen);
    bool read_PNG_file (const char* filePath, unsigned char* &_data, int& _width, int& _height, int& _dataLen);
    bool read_PNG_file_data (unsigned char*imageData, ssize_t unpackedLen, unsigned char* &_data, int& _width, int& _height, ssize_t& _dataLen);
#ifdef __cplusplus
}
#endif