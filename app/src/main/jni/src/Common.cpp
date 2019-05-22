#include "Common.h"
#include <sys/statfs.h>
#include<sys/stat.h>
#include<fcntl.h>
#include <vector>
#include <unordered_map>
#include <algorithm>
size_t getFreeSpace(const char* disk){
	size_t freeSpace = 0;
	struct statfs disk_statfs;
	if( statfs(disk, &disk_statfs) >= 0 ){
		freeSpace = disk_statfs.f_bsize  * disk_statfs.f_bfree / 1024;
	}
	return freeSpace;
}
bool isHasEnoughSpace(const char* disk){
    return getFreeSpace(disk)>=RESERVE_SPACE_SIZE;
}
int64_t systemnanotime() {
    timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    return now.tv_sec * 1000000000LL + now.tv_nsec;
}
int64_t systemmicrotime() {
    timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    return now.tv_sec * 1000LL + now.tv_nsec/1000000LL;
}
unsigned long get_file_size(const char *path)  {
    unsigned long filesize = -1;
    struct stat statbuff;
    if(stat(path, &statbuff) < 0){
        return filesize;
    }else{
        filesize = statbuff.st_size;
    }
    return filesize;
}
std::string getFileExtension(const std::string& filePath)
{
    std::string fileExtension;
    size_t pos = filePath.find_last_of('.');
    if (pos != std::string::npos)
    {
        fileExtension = filePath.substr(pos, filePath.length());

        std::transform(fileExtension.begin(), fileExtension.end(), fileExtension.begin(), ::tolower);
    }

    return fileExtension;
}


