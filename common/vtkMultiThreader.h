// .NAME vtkMultiThreader.h - A class for performing multithreaded execution
// .SECTION Description
// vtkMultithreader is a class that provides support for multithreaded
// execution using sproc() on an SGI, or pthread_create on any platform
// supporting POSIX threads.  This class can be used to execute a single
// method on multiple threads, or to specify a method per thread. 

#ifndef __vtkMultiThreader_h
#define __vtkMultiThreader_h

#include "vtkObject.h"

#ifdef USE_SPROC
#include <sys/types.h>
#include <unistd.h>
#endif

#ifdef USE_PTHREADS
#include <sys/types.h>
#include <unistd.h>
#endif

// If USE_SPROC is defined, then sproc() will be used to create
// multiple threads on an SGI. If USE_PTHREADS is defined, then
// pthread_create() will be used to create multiple threads (on
// a sun, for example)

// The maximum number of threads allowed
#ifdef USE_SPROC
#define VTK_MAX_THREADS              32
#endif

#ifdef USE_PTHREADS
#define VTK_MAX_THREADS              32
#endif

#ifdef _WIN32
#define VTK_MAX_THREADS              8
#endif

#ifndef _WIN32
#ifndef USE_SPROC
#ifndef USE_PTHREADS
#define VTK_MAX_THREADS              1
#endif
#endif
#endif

// If USE_PTHREADS is defined, then the multithreaded
// function is of type void *, and returns NULL
// Otherwise the type is void which is correct for WIN32
// and SPROC
//BTX
#ifdef USE_PTHREADS
typedef void *(*vtkThreadFunctionType)(void *);
#define VTK_THREAD_RETURN_VALUE  NULL
#define VTK_THREAD_RETURN_TYPE   void *
#endif

#ifdef _WIN32
typedef LPTHREAD_START_ROUTINE vtkThreadFunctionType;
#define VTK_THREAD_RETURN_VALUE 0
#define VTK_THREAD_RETURN_TYPE DWORD __stdcall
#endif

#ifndef _WIN32
#ifndef USE_PTHREADS
typedef void (*vtkThreadFunctionType)(void *);
#define VTK_THREAD_RETURN_VALUE
#define VTK_THREAD_RETURN_TYPE void
#endif
#endif
//ETX

// Description:
// This is the structure that is passed to the SingleMethod or
// MultipleMethod that is called during SingleMethodExecute or
// MultipleMethodExecute. It is passed in as a void *, and it is
// up to the method to cast correctly and extract the information.
// The ThreadID is a number between 0 and ThreadCount-1 that indicates
// the id of this thread. The ThreadCount is this->ThreadCount.
// The UserData is obtained from the SetSingleMethod or SetMultipleMethod
// call.
struct ThreadInfoStruct
{
  int                 ThreadID;
  int                 ThreadCount;
  void                *UserData;
};

class VTK_EXPORT vtkMultiThreader : public vtkObject 
{
public:
  vtkMultiThreader();
  ~vtkMultiThreader();
  char *GetClassName() {return "vtkMultiThreader";};
  void PrintSelf(ostream& os, vtkIndent indent);


  // Description:
  // Get/Set the number of threads to create
  // It will be clamped to the range 1 - VTK_MAX_THREADS, so the
  // caller of this method should check that the requested number
  // of threads was accepted.
  vtkSetClampMacro( ThreadCount, int, 1, VTK_MAX_THREADS );
  vtkGetMacro( ThreadCount, int );

  // These methods are excluded from Tcl wrapping 1) because the
  // wrapper barfs on them and 2) because they really shouldn't be
  // called from a tcl script anyway.
  //BTX 
  
  // Description:
  // Execute the SingleMethod (as define by SetSingleMethod) using
  // this->ThreadCount threads.
  void SingleMethodExecute();

  // Description:
  // Execute the MultipleMethods (as define by calling SetMultipleMethod
  // for each of the required this->ThreadCount methods) using
  // this->ThreadCount threads.
  void MultipleMethodExecute();

  // Description:
  // Set the SingleMethod to f() and the UserData field of the
  // ThreadInfoStruct that is passed to it will be data.
  // This method (and all the methods passed to SetMultipleMethod)
  // must be of type vtkThreadFunctionType and must take a single argument of
  // type void *.
  void SetSingleMethod(vtkThreadFunctionType, void *data );
 
  // Description:
  // Set the MultipleMethod at the given index to f() and the UserData 
  // field of the ThreadInfoStruct that is passed to it will be data.
  void SetMultipleMethod( int index, vtkThreadFunctionType, 
			  void *data ); 

protected:
  // The number of threads to use
  int                        ThreadCount;

  // An array of thread info containing a thread id
  // (0, 1, 2, .. VTK_MAX_THREADS-1), the thread count, and a pointer
  // to void so that user data can be passed to each thread
  ThreadInfoStruct           ThreadInfoArray[VTK_MAX_THREADS];

  // The methods
  vtkThreadFunctionType      SingleMethod;
  vtkThreadFunctionType      MultipleMethod[VTK_MAX_THREADS];
//ETX

  // Internal storage of the data
  void                       *SingleData;
  void                       *MultipleData[VTK_MAX_THREADS];
};

#endif





