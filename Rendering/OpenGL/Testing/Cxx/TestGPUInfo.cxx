/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestGPUInfo.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkTestUtilities.h"
#include "vtkGPUInfoList.h"
#include "vtkGPUInfo.h"

int TestGPUInfo(int vtkNotUsed(argc), char *vtkNotUsed(argv)[])
{
  vtkGPUInfoList *l=vtkGPUInfoList::New();
  l->Probe();
  int c=l->GetNumberOfGPUs();

  cout << "There is " << c << " GPU(s)."<< endl;
  int i=0;
  while(i<c)
    {
    cout << " GPU " << i << ": " << endl;
    vtkGPUInfo *info=l->GetGPUInfo(i);
    vtkTypeUInt64 value;
    value=info->GetDedicatedVideoMemory();
    cout << "  dedicated VRAM=" << value/(1024*1024) << " MiB" << endl;
    value=info->GetDedicatedSystemMemory();
    cout << "  dedicated RAM=" << value/(1024*1024) << " MiB" << endl;
    value=info->GetSharedSystemMemory();
    cout << "  shared RAM=" << value/(1024*1024) << " MiB" << endl;
    ++i;
    }
  l->Delete();

  return 0; // 0==never fails.
}
