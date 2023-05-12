/*
 * Copyright 2018, University Corporation for Atmospheric Research
 * See netcdf/COPYRIGHT file for copying and redistribution conditions.
 */

#include "config.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <errno.h>

#include <windows.h>
#include <io.h>
#include <wchar.h>
#include <locale.h>
//#include <direct.h>

#ifdef _WIN32
__declspec(dllexport)
#endif
int
getmountpoint(char* keyvalue, size_t size)
{
    /* See if we can get the MSYS2|CYGWIN prefix from the registry */
    LSTATUS stat;
    const LPCSTR rpath = "SOFTWARE\\Cygwin\\setup\\";
    const LPCSTR leaf = "rootdir";
    HKEY key;
	
    if(size == 0 || keyvalue == NULL)
	return -1;

    keyvalue[0] = '\0';

    stat =  RegOpenKeyA(HKEY_LOCAL_MACHINE, rpath, &key);
    if(stat != ERROR_SUCCESS) {
//        wprintf(L"RegOpenKeyA failed. Error code: %li\n", stat);
        goto done;
    }
    stat = RegGetValueA(key, NULL, leaf, RRF_RT_REG_SZ, NULL, (PVOID)keyvalue, (LPDWORD)&size);
    if(stat != ERROR_SUCCESS) {
//        wprintf(L"RegGetValueA failed. Error code: %li\n", stat);
        goto done;
    }
done:
    return (stat == ERROR_SUCCESS ? 0 : -1);
}
