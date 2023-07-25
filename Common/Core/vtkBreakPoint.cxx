// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkBreakPoint.h"

#ifndef _WIN32
#include <unistd.h> // gethostname(), sleep()
#endif

//------------------------------------------------------------------------------
VTK_ABI_NAMESPACE_BEGIN
void vtkBreakPoint::Break()
{
#ifndef _WIN32
  int i = 0;
  char hostname[256];
  gethostname(hostname, sizeof(hostname));
  cout << "PID " << getpid() << " on " << hostname << " ready for attach" << endl;
  while (i == 0)
  {
    sleep(5);
  }
#endif
}
VTK_ABI_NAMESPACE_END
