/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCollectGraph.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkBreakPoint.h"

#ifndef _WIN32
#include <unistd.h> // gethostname(), sleep()
#endif

// ----------------------------------------------------------------------------
void vtkBreakPoint::Break()
{
#ifndef _WIN32
  int i=0;
  char hostname[256];
  gethostname(hostname,sizeof(hostname));
  cout << "PID " << getpid() << " on " << hostname << " ready for attach"
       << endl;
  while(i==0)
    {
    sleep(5);
    }
#endif
}
