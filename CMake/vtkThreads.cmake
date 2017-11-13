include(FindThreads)
set(VTK_USE_WIN32_THREADS 0)
set(VTK_USE_PTHREADS 0)
# if win32 threads and pthreads are available figure out which
# one the compiler is setup to use.  If you can not figure it
# out default to pthreads.
if(CMAKE_USE_PTHREADS_INIT AND CMAKE_USE_WIN32_THREADS_INIT)
  if(DEFINED VTK_THREAD_MODEL)
    set(output "${VTK_THREAD_MODEL}")
  else()
    execute_process(COMMAND "${CMAKE_C_COMPILER}" -v OUTPUT_VARIABLE output
      ERROR_VARIABLE output RESULT_VARIABLE result TIMEOUT 10)
  endif()
  if(output MATCHES "Thread model: posix")
    set(VTK_THREAD_MODEL "Thread model: posix" CACHE STRING
      "Thread model used by gcc.")
    set(CMAKE_USE_WIN32_THREADS_INIT 0)
  elseif(output MATCHES "Thread model: win32")
    set(VTK_THREAD_MODEL "Thread model: win32" CACHE STRING
      "Thread model used by gcc.")
    set(CMAKE_USE_PTHREADS_INIT 0)
  else()
    set(VTK_THREAD_MODEL "Thread model: posix" CACHE STRING
      "Thread model used by gcc.")
    set(CMAKE_USE_WIN32_THREADS_INIT 0)
  endif()
endif()
mark_as_advanced(VTK_THREAD_MODEL)
if(CMAKE_USE_WIN32_THREADS_INIT)
  set(VTK_USE_WIN32_THREADS 1)
  set(CMAKE_THREAD_LIBS_INIT "")
elseif(CMAKE_USE_PTHREADS_INIT)
  set(VTK_USE_PTHREADS 1)
endif()
set(CMAKE_THREAD_LIBS "${CMAKE_THREAD_LIBS_INIT}" CACHE STRING "Thread library used.")
mark_as_advanced(CMAKE_THREAD_LIBS)
set(VTK_MAX_THREADS "64" CACHE STRING
  "Max number of threads vtkMultiThreader will allocate.")
mark_as_advanced(VTK_MAX_THREADS)
