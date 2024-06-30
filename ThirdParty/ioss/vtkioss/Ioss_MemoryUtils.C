// Copyright(C) 1999-2023 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

#include "Ioss_MemoryUtils.h"

// For memory utilities...
#if defined(__IOSS_WINDOWS__)
#include <windows.h>
#elif defined(__unix__) || defined(__unix) || defined(unix) ||                                     \
    (defined(__APPLE__) && defined(__MACH__))
#include <sys/resource.h>
#include <unistd.h>

#if defined(__APPLE__) && defined(__MACH__) && (defined(__arm__) || defined(__arm64__))
#include <mach/arm/kern_return.h>
#endif

#if defined(__APPLE__) && defined(__MACH__)
#include <mach/kern_return.h>
#include <mach/mach_init.h>
#include <mach/message.h>
#include <mach/task.h>
#include <mach/task_info.h>

#elif (defined(_AIX) || defined(__TOS__AIX__)) ||                                                  \
    (defined(__sun__) || defined(__sun) || defined(sun) && (defined(__SVR4) || defined(__svr4__)))
#include <fcntl.h>
#include <procfs.h>

#elif defined(__linux__) || defined(__linux) || defined(linux) || defined(__gnu_linux__)
#include <stdio.h>
#endif
#endif

#if defined(BGQ_LWK) && defined(__linux__)
#include <spi/include/kernel/location.h>
#include <spi/include/kernel/memory.h>
#endif

size_t Ioss::MemoryUtils::get_memory_info()
{
  // Code from http://nadeausoftware.com/sites/NadeauSoftware.com/files/getRSS.c
  size_t memory_usage = 0;
#if defined(__IOSS_WINDOWS__)
#if 0
  /* Windows -------------------------------------------------- */
  PROCESS_MEMORY_COUNTERS info;
  GetProcessMemoryInfo(GetCurrentProcess(), &info, sizeof(info));
  memory_usage = (size_t)info.WorkingSetSize;
#else
  memory_usage = 0;
#endif

#elif defined(__APPLE__) && defined(__MACH__)
  kern_return_t               error;
  mach_msg_type_number_t      outCount;
  mach_task_basic_info_data_t taskinfo{};

  taskinfo.virtual_size = 0;
  outCount              = MACH_TASK_BASIC_INFO_COUNT;
  error                 = task_info(mach_task_self(), MACH_TASK_BASIC_INFO,
                                    reinterpret_cast<task_info_t>(&taskinfo), &outCount);
  if (error == KERN_SUCCESS) {
    memory_usage = taskinfo.resident_size;
  }
#elif __linux__
#if defined(BGQ_LWK)
  uint64_t heap;
  Kernel_GetMemorySize(KERNEL_MEMSIZE_HEAP, &heap);
  memory_usage = heap;
#else
  // On Linux, the /proc pseudo-file system contains a directory for
  // each running or zombie process. The /proc/[pid]/stat,
  // /proc/[pid]/statm, and /proc/[pid]/status pseudo-files for the
  // process with id [pid] all contain a process's current resident
  // set size, among other things. But the /proc/[pid]/statm
  // pseudo-file is the easiest to read since it contains a single
  // line of text with white-space delimited values:
  //
  // * total program size
  // * resident set size
  // * shared pages
  // * text (code) size
  // * library size
  // * data size (heap + stack)
  // * dirty pages
  //
  // The second value provides the process's current resident set size
  // in pages. To get the field for the current process, open
  // /proc/self/statm and parse the second integer value. Multiply the
  // field by the page size from sysconf( ).

  long  rss = 0L;
  FILE *fp  = NULL;
  if ((fp = fopen("/proc/self/statm", "r")) == NULL)
    return (size_t)0L; // Can't open? */
  if (fscanf(fp, "%*s%ld", &rss) != 1) {
    fclose(fp);
    return (size_t)0L; // Can't read? */
  }
  fclose(fp);
  memory_usage = (size_t)rss * (size_t)sysconf(_SC_PAGESIZE);
#endif
#endif
  return memory_usage;
}

size_t Ioss::MemoryUtils::get_hwm_memory_info()
{
  // Code from http://nadeausoftware.com/sites/NadeauSoftware.com/files/getRSS.c
  size_t memory_usage = 0;
#if defined(__IOSS_WINDOWS__)
#if 0
  /* Windows -------------------------------------------------- */
  PROCESS_MEMORY_COUNTERS info;
  GetProcessMemoryInfo(GetCurrentProcess(), &info, sizeof(info));
  memory_usage = (size_t)info.PeakWorkingSetSize;
#else
  memory_usage = 0;
#endif

#elif (defined(_AIX) || defined(__TOS__AIX__)) ||                                                  \
    (defined(__sun__) || defined(__sun) || defined(sun) && (defined(__SVR4) || defined(__svr4__)))
  /* AIX and Solaris ------------------------------------------ */
  struct psinfo psinfo;
  int           fd = -1;
  if ((fd = open("/proc/self/psinfo", O_RDONLY)) == -1)
    return (size_t)0L; /* Can't open? */
  if (read(fd, &psinfo, sizeof(psinfo)) != sizeof(psinfo)) {
    close(fd);
    return (size_t)0L; /* Can't read? */
  }
  close(fd);
  memory_usage = (size_t)(psinfo.pr_rssize * 1024L);

#elif (defined(__APPLE__) && defined(__MACH__)) || (defined(__linux__) && !defined(BGQ_LWK))
  /* BSD, Linux, and OSX -------------------------------------- */
  struct rusage rusage;
  getrusage(RUSAGE_SELF, &rusage);
#if defined(__APPLE__) && defined(__MACH__)
  memory_usage = (size_t)rusage.ru_maxrss;
#else
  memory_usage = (size_t)(rusage.ru_maxrss * 1024L);
#endif
#endif
  return memory_usage;
}
