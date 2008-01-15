/*=========================================================================

Program:   Visualization Toolkit
Module:    TestFBO.cxx

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// This code test to make sure vtkOpenGLExtensionManager can properly get
// extension functions that can be used.  To do this, we convolve an image
// with a kernel for a Laplacian filter.  This requires the use of functions
// defined in OpenGL 1.2, which should be available pretty much everywhere
// but still has functions that can be loaded as extensions.

#include "vtkObject.h"
#include <vtksys/Process.h>
#include <vtksys/SystemTools.hxx>
#include <vtkstd/string>
#include "TestFBOInclude.h"

// We spawn a new process so that no exceptions or segfaults can ever result in
// this test failing.
int TestFBO(int vtkNotUsed(argc), char *vtkNotUsed(argv)[])
{
  vtkstd::string executable = EXECUTABLE_OUTPUT_PATH;
#ifdef CMAKE_INTDIR
  executable += "/" CMAKE_INTDIR;
#endif
  executable += "/" TEST_FBO_IMPLEMENTATION_EXE;

  cout << "Executable: " << executable.c_str() << endl;
  vtksysProcess* process = vtksysProcess_New();
  vtkstd::vector<const char*> commandLine;
  commandLine.push_back(executable.c_str());
  commandLine.push_back(0);
  vtksysProcess_SetCommand(process, &commandLine[0]);
  vtksysProcess_Execute(process);

  char* data;
  int length;
  int pipe;
  do
    {
    pipe =  vtksysProcess_WaitForData(process, &data, &length, NULL);
    switch (pipe)
      {
    case vtksysProcess_Pipe_STDOUT:
      cout.write(data, length);
      break;

    case vtksysProcess_Pipe_STDERR:
      cerr.write(data, length);
      break;
      }
    } while (pipe != vtksysProcess_Pipe_None);

  vtksysProcess_Delete(process);
  return 0; // 0==passed, always pass.
}
