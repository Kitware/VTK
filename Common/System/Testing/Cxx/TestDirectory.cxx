// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// .NAME
// .SECTION Description
// this program tests the Directory class

#include "vtkDebugLeaks.h"
#include "vtkDirectory.h"

int TestDirectory(int, char*[])
{
  vtkDirectory* myDir = vtkDirectory::New();
  vtkDirectory* testDir = vtkDirectory::New();
  myDir->Open(".");
  char buf[1024];
  vtkDirectory::GetCurrentWorkingDirectory(buf, 1024);
  cout << "Working Directory: " << buf << endl;
  // Get each file name in the directory
  for (int i = 0; i < myDir->GetNumberOfFiles(); i++)
  {
    cout << (testDir->Open(myDir->GetFile(i)) == 0 ? "file: " : "dir:  ") << myDir->GetFile(i)
         << endl;
  }

  myDir->Delete();
  testDir->Delete();
  return 0;
}
