#include "os_util.h"
#ifdef _WIN32
#include <windows.h>
void os_mkdir(const char *path) { CreateDirectoryA(path, NULL); }
#else
#include <sys/stat.h>
void os_mkdir(const char *path) { mkdir(path, 0755); }
#endif
