#include "vtkDynamicLoader.h"
// This file is actually 3 different implementations.
// 1. HP machines which uses shl_load
// 2. Apple OSX which uses NSLinkModule
// 3. Windows which uses LoadLibrary
// 4. Most unix systems which use dlopen (default )
// Each part of the ifdef contains a complete implementation for
// the static methods of vtkDynamicLoader.  



// ---------------------------------------------------------------
// 1. Implementation for HPUX  machines
#ifdef __hpux
#define VTKDYNAMICLOADER_DEFINED 1
#include <dl.h>

vtkLibHandle vtkDynamicLoader::OpenLibrary(const char* libname )
{
  return shl_load(libname, BIND_DEFERRED | DYNAMIC_PATH, 0L);
}

int vtkDynamicLoader::CloseLibrary(vtkLibHandle lib)
{
  return 0;
}

void* vtkDynamicLoader::GetSymbolAddress(vtkLibHandle lib, const char* sym)
{ 
  void* addr;
  int status;
  
  status = shl_findsym (&lib, sym, TYPE_PROCEDURE, &addr);
  return (status < 0) ? (void*)0 : addr;
}

const char* vtkDynamicLoader::LibPrefix()
{ 
  return "lib";
}

const char* vtkDynamicLoader::LibExtension()
{
  return ".sl";
}

const char* vtkDynamicLoader::LastError()
{
  return 0;
}
#endif



// ---------------------------------------------------------------
// 2. Implementation for Darwin (including OSX) Machines

#ifdef __APPLE__
#define VTKDYNAMICLOADER_DEFINED
#include <mach-o/dyld.h>

vtkLibHandle vtkDynamicLoader::OpenLibrary(const char* libname )
{
  NSObjectFileImageReturnCode rc;
  NSObjectFileImage image;

  rc = NSCreateObjectFileImageFromFile(libname, &image);
  return NSLinkModule(image, libname, TRUE);
}

int vtkDynamicLoader::CloseLibrary(vtkLibHandle lib)
{
  return 0;
}

void* vtkDynamicLoader::GetSymbolAddress(vtkLibHandle lib, const char* sym)
{
    void *result=0;
    if(NSIsSymbolNameDefined(sym)){
         cout << sym << " is defined!" << endl;
         NSSymbol symbol= NSLookupAndBindSymbol(sym);
         if(symbol){
                result = NSAddressOfSymbol(symbol);
         }
  }else{
        cout << sym << " is not defined!" << endl;
 }
  return result;
}

const char* vtkDynamicLoader::LibPrefix()
{
  return "";
}

const char* vtkDynamicLoader::LibExtension()
{
  return ".dylib";
}

const char* vtkDynamicLoader::LastError()
{
  return 0;
}

#endif




// ---------------------------------------------------------------
// 3. Implementation for Windows win32 code
#ifdef _WIN32
#include <windows.h>
#define VTKDYNAMICLOADER_DEFINED 1

vtkLibHandle vtkDynamicLoader::OpenLibrary(const char* libname )
{
  return LoadLibrary(libname);
}

int vtkDynamicLoader::CloseLibrary(vtkLibHandle lib)
{
  return (int)FreeLibrary(lib);
}

void* vtkDynamicLoader::GetSymbolAddress(vtkLibHandle lib, const char* sym)
{ 
  return GetProcAddress(lib, sym);
}

const char* vtkDynamicLoader::LibPrefix()
{ 
  return "";
}

const char* vtkDynamicLoader::LibExtension()
{
  return ".dll";
}

const char* vtkDynamicLoader::LastError()
{
   DWORD errcode = GetLastError();
  LPVOID lpMsgBuf;

  FormatMessage( 
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL,
		GetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
		(LPTSTR) &lpMsgBuf,
		0,
		NULL 
		);
  
  // Free the buffer.
  LocalFree( lpMsgBuf );
  static char* str = 0;
  delete [] str;
  str = strcpy(new char[strlen((char*)lpMsgBuf)+1], (char*)lpMsgBuf);
  return str;
}

#endif

// ---------------------------------------------------------------
// 4. Implementation for default UNIX machines.
// if nothing has been defined then use this
#ifndef VTKDYNAMICLOADER_DEFINED
#define VTKDYNAMICLOADER_DEFINED
// Setup for most unix machines
#include <dlfcn.h>

vtkLibHandle vtkDynamicLoader::OpenLibrary(const char* libname )
{
  return dlopen(libname, RTLD_LAZY);
}

int vtkDynamicLoader::CloseLibrary(vtkLibHandle lib)
{
  return (int)dlclose(lib);
}

void* vtkDynamicLoader::GetSymbolAddress(vtkLibHandle lib, const char* sym)
{ 
  return dlsym(lib, sym);
}

const char* vtkDynamicLoader::LibPrefix()
{ 
  return "lib";
}

const char* vtkDynamicLoader::LibExtension()
{
  return ".so";
}

const char* vtkDynamicLoader::LastError()
{
  return dlerror(); 
}
#endif
