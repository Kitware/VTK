/*=========================================================================

  Program:   Visualization Toolkit
  Module:    SystemInformation.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME Test to print system information useful for remote debugging.
// .SECTION Description
// Remote dashboard debugging often requires access to the
// CMakeCache.txt file.  This test will display the file.

#include "vtkDebugLeaks.h"
#include <Common/Testing/Cxx/SystemInformation.h>
#include <sys/stat.h>

void vtkSystemInformationPrintFile(const char* name, ostream& os)
{
  os << "================================================================\n";
  struct stat fs;
  if(stat(name, &fs) != 0)
    {
    os << "The file \"" << name << "\" does not exist.\n";
    return;
    }

#ifdef _WIN32
  ifstream fin(name, ios::in | ios::binary);
#else
  ifstream fin(name, ios::in);
#endif

  if(fin)
    {
    os << "Contents of \"" << name << "\":\n";
    os << "----------------------------------------------------------------\n";
    const int bufferSize = 4096;
    char buffer[bufferSize];
    // This copy loop is very sensitive on certain platforms with
    // slightly broken stream libraries (like HPUX).  Normally, it is
    // incorrect to not check the error condition on the fin.read()
    // before using the data, but the fin.gcount() will be zero if an
    // error occurred.  Therefore, the loop should be safe everywhere.
    while(fin)
      {
      fin.read(buffer, bufferSize);
      if(fin.gcount())
        {
        os.write(buffer, fin.gcount());
        }
      }
    os.flush();
    }
  else
    {
    os << "Error opening \"" << name << "\" for reading.\n";
    }
}

int SystemInformation(int,char *[])
{
  const char* files[] =
    {
      CMAKE_BINARY_DIR "/CMakeCache.txt", 
      VTK_BINARY_DIR "/vtkConfigure.h",
      VTK_BINARY_DIR "/vtkToolkits.h",
      CMAKE_BINARY_DIR "/CMakeFiles/CMakeError.log",
      VTK_BINARY_DIR "/CMake/CMakeCache.txt", 
      VTK_BINARY_DIR "/VTKBuildSettings.cmake",
      VTK_BINARY_DIR "/VTKLibraryDepends.cmake",
      VTK_BINARY_DIR "/VTKConfig.cmake",
      0
    };

  cout << "CTEST_FULL_OUTPUT (Avoid ctest truncation of output)" << endl;

  const char** f;
  for(f = files; *f; ++f)
    {
    vtkSystemInformationPrintFile(*f, cout);
    }

#if defined(__sgi) && !defined(__GNUC__) && defined(_COMPILER_VERSION)
  cout << "SGI compiler version: " << int(_COMPILER_VERSION) << endl;
#endif

  return 0;
} 
