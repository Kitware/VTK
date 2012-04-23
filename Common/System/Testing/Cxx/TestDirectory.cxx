/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestDirectory.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME
// .SECTION Description
// this program tests the Directory class

#include "vtkDirectory.h"
#include "vtkDebugLeaks.h"

int TestDirectory(int,char *[])
{
  vtkDirectory *myDir = vtkDirectory::New();
  vtkDirectory *testDir = vtkDirectory::New();
  myDir->Open (".");
  char buf[1024];
  myDir->GetCurrentWorkingDirectory(buf, 1024);
  cout << "Working Directory: " << buf << endl;
  // Get each file name in the directory
  for (int i = 0; i < myDir->GetNumberOfFiles(); i++)
    {
    cout << (testDir->Open(myDir->GetFile(i)) == 0 ? "file: " : "dir:  ") <<  myDir->GetFile(i) << endl;
    }

  myDir->Delete();
  testDir->Delete();
  return 0;
}
