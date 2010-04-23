/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkProcessStatistics.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkProcessStatistics.h"


#ifndef _WIN32
#include <sys/procfs.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkProcessStatistics);

/* This mess was copied from the GNU getpagesize.h.  */
#ifndef HAVE_GETPAGESIZE
# ifdef HAVE_UNISTD_H
#  include <unistd.h>
# endif

/* Assume that all systems that can run configure have sys/param.h.  */
# ifndef HAVE_SYS_PARAM_H
#  define HAVE_SYS_PARAM_H 1
# endif

# ifdef _SC_PAGESIZE
#  define getpagesize() sysconf(_SC_PAGESIZE)
# else /* no _SC_PAGESIZE */
#  ifdef HAVE_SYS_PARAM_H
#   include <sys/param.h>
#   ifdef EXEC_PAGESIZE
#    define getpagesize() EXEC_PAGESIZE
#   else /* no EXEC_PAGESIZE */
#    ifdef NBPG
#     define getpagesize() NBPG * CLSIZE
#     ifndef CLSIZE
#      define CLSIZE 1
#     endif /* no CLSIZE */
#    else /* no NBPG */
#     ifdef NBPC
#      define getpagesize() NBPC
#     else /* no NBPC */
#      ifdef PAGESIZE
#       define getpagesize() PAGESIZE
#      endif /* PAGESIZE */
#     endif /* no NBPC */
#    endif /* no NBPG */
#   endif /* no EXEC_PAGESIZE */
#  else /* no HAVE_SYS_PARAM_H */
#   define getpagesize() 8192   /* punt totally */
#  endif /* no HAVE_SYS_PARAM_H */
# endif /* no _SC_PAGESIZE */

#endif /* no HAVE_GETPAGESIZE */
#endif _WIN32

// Construct the ProcessStatistics with eight points.
vtkProcessStatistics::vtkProcessStatistics()
{
}


int vtkProcessStatistics::GetProcessSizeInBytes()
{

#ifndef _WIN32
  prpsinfo  psinfo;
  int       fd;
  char      pname[1024];
  int       pagesize;
  pid_t     pid;

  // Get out process id
  pid = getpid();

  // Get the size of a page in bytes
  pagesize = getpagesize();

  // Open the /proc/<pid> file and query the
  // process info
  sprintf( pname, "/proc/%d", pid );
  fd = open( pname, O_RDONLY );
  if (fd != -1)
    {
    psinfo.pr_size = 0;
    ioctl( fd, PIOCPSINFO, &psinfo );
    close( fd );
    }
  else
    {
      vtkErrorMacro(<< "Cannot get size of " << pname);
      return 0;
    }

  // The size in bytes is the page size of the process times
  // the size of a page in bytes
  return psinfo.pr_size * pagesize;
#endif

#ifdef _WIN32
  return 0;
#endif

}

double vtkProcessStatistics::GetProcessCPUTimeInMilliseconds()
{

#ifndef _WIN32
  prpsinfo  psinfo;
  int       fd;
  char      pname[1024];
  pid_t     pid;

  // Get out process id
  pid = getpid();

  // Open the /proc/<pid> file and query the
  // process info
  sprintf( pname, "/proc/%d", pid );
  fd = open( pname, O_RDONLY );
  ioctl( fd, PIOCPSINFO, &psinfo );
  close( fd );

  return 
    (double) psinfo.pr_time.tv_sec * 1000.0 + 
    (double) psinfo.pr_time.tv_nsec / 1000000.0;
#endif

#ifdef _WIN32
  return 0.0;
#endif

}

