/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkProcessStatistics.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1998 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.


=========================================================================*/
#include "vtkProcessStatistics.h"

#include <sys/procfs.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>


// Description:
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

  // Open the /proc/pinfo/<pid> file and query the
  // process info
  sprintf( pname, "/proc/pinfo/%d", pid );
  fd = open( pname, O_RDONLY );
  ioctl( fd, PIOCPSINFO, &psinfo );
  close( fd );

  // The size in bytes is the page size of the process times
  // the size of a page in bytes
  return psinfo.pr_rssize * pagesize;
#endif

#ifdef _WIN32
  return 0;
#endif

}

float vtkProcessStatistics::GetProcessCPUTimeInMilliseconds()
{

#ifndef _WIN32
  prpsinfo  psinfo;
  int       fd;
  char      pname[1024];
  pid_t     pid;

  // Get out process id
  pid = getpid();

  // Open the /proc/pinfo/<pid> file and query the
  // process info
  sprintf( pname, "/proc/pinfo/%d", pid );
  fd = open( pname, O_RDONLY );
  ioctl( fd, PIOCPSINFO, &psinfo );
  close( fd );

  return 
    (float) psinfo.pr_time.tv_sec * 1000.0 + 
    (float) psinfo.pr_time.tv_nsec / 1000000.0 +
    (float) psinfo.pr_ctime.tv_sec * 1000.0 + 
    (float) psinfo.pr_ctime.tv_nsec / 1000000.0;
#endif

#ifdef _WIN32
  return 0.0
#endif

}

