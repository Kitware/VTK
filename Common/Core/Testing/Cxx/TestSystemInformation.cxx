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
#include <vtksys/SystemTools.hxx>
#include <string>

void vtkSystemInformationPrintFile(const char* name, ostream& os)
{
  os << "================================================================\n";
  vtksys::SystemTools::Stat_t fs;
  if(vtksys::SystemTools::Stat(name, &fs) != 0)
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

int TestSystemInformation(int argc, char* argv[])
{
  if(argc != 2)
  {
    cerr << "Usage: TestSystemInformation <top-of-build-tree>\n";
    return 1;
  }
  std::string build_dir = argv[1];
  build_dir += "/";

  const char* files[] =
    {
    "CMakeCache.txt",
    "CMakeFiles/CMakeError.log",
    "Common/Core/vtkConfigure.h",
    "Common/Core/vtkToolkits.h",
    "VTKConfig.cmake",
    "Testing/Temporary/ConfigSummary.txt",
    0
    };

  cout << "CTEST_FULL_OUTPUT (Avoid ctest truncation of output)" << endl;

  for(const char** f = files; *f; ++f)
  {
    std::string fname = build_dir + *f;
    vtkSystemInformationPrintFile(fname.c_str(), cout);
  }

  return 0;
}
