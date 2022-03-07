#pragma once

#define USESVN  1

#if(USESVN)
//#define USENEON 1	// (GPL licensing!)
#define USESERF 1


#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "shfolder.lib")
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "secur32.lib")
#pragma comment(lib, "crypt32.lib")
#pragma comment(lib, "version.lib")
#pragma comment(lib, "psapi.lib")


#endif