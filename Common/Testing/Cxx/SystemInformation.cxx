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
#include <vtkstd/string>
#include <sys/stat.h>
#include <time.h>

// Construct the name of the notes file.
#define VTK_SYSTEM_INFORMATION_NOTES \
  VTK_BINARY_DIR "/Testing/HTML/TestingResults/Sites/" \
  VTKTesting_SITE "/" VTKTesting_BUILD_NAME "/BuildNameNotes.xml"

vtkstd::string vtkGetCurrentDateTime(const char* format)
{
  char buf[1024];
  time_t t;
  time(&t);
  strftime(buf, sizeof(buf), format, localtime(&t));
  return buf;
}

void vtkSystemInformationPrintFile(const char* name, ostream& os,
                                   char note=0)
{
  if (!note)
    {
    os << "================================================================\n";
    }
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
    if (!note)
      {
      os << "Contents of \"" << name << "\":\n";
      os << "----------------------------------------------------------------\n";
      }
    const int bufferSize = 4096;
    char bufferIn[bufferSize];
    char bufferOut[6*bufferSize]; // worst case scenario
    // This copy loop is very sensitive on certain platforms with
    // slightly broken stream libraries (like HPUX).  Normally, it is
    // incorrect to not check the error condition on the fin.read()
    // before using the data, but the fin.gcount() will be zero if an
    // error occurred.  Therefore, the loop should be safe everywhere.
    while(fin)
      {
      fin.read(bufferIn, bufferSize);
      if(fin.gcount())
        {
        // convert buffer to an XML safe form
        const char *s = bufferIn;
        char *x = bufferOut;
        *x = '\0';
        for (int i = 0; i < static_cast<int>(fin.gcount()); i++)
          {
          // replace all special characters
          switch (*s)
            {
            case '&':
              strcat(x, "&amp;"); x += 5;
              break;
            case '"':
              strcat(x, "&quot;"); x += 6;
              break;
            case '\'':
              strcat(x, "&apos;"); x += 6;
              break;
            case '<':
              strcat(x, "&lt;"); x += 4;
              break;
            case '>':
              strcat(x, "&gt;"); x += 4;
              break;
            default:
              *x = *s; x++;
              *x = '\0'; // explicitly terminate the new string
            }
          s++;
          }
        os.write(bufferOut, x - bufferOut);
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
  vtkDebugLeaks::PromptUserOff();
  const char* files[] =
    {
      VTK_BINARY_DIR "/CMakeCache.txt", 
      VTK_BINARY_DIR "/vtkConfigure.h",
      VTK_BINARY_DIR "/vtkToolkits.h",
      VTK_BINARY_DIR "/CMakeError.log",
      VTK_BINARY_DIR "/CMake/CMakeCache.txt", 
      VTK_BINARY_DIR "/VTKBuildSettings.cmake",
      VTK_BINARY_DIR "/VTKLibraryDepends.cmake",
      VTK_BINARY_DIR "/VTKConfig.cmake",
      0
    };

  const char** f;
  for(f = files; *f; ++f)
    {
    vtkSystemInformationPrintFile(*f, cout);
    }

  ofstream outf(VTK_SYSTEM_INFORMATION_NOTES, ios::out);
  if(outf)
    {
    cout << "Also writing this information to file " << VTK_SYSTEM_INFORMATION_NOTES << "\n";
  
    outf << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" << endl;
    outf << "<Site BuildName=\"" << VTKTesting_BUILD_NAME << "\"  Name=\""
         << VTKTesting_SITE << "\">" << endl;
    outf << "<BuildNameNotes>" << endl;
    for(f = files; *f; ++f)
      {
      vtkstd::string currentDateTime =  vtkGetCurrentDateTime("%a %b %d %Y %H:%M:%S %Z");
      outf << "<Note Name=\"" << *f << "\">" << endl;
      outf << "<DateTime>"
           << currentDateTime.c_str()
           << "</DateTime>" << endl;
      outf << "<Text>" << endl;
    
      vtkSystemInformationPrintFile(*f, outf, 1);

      outf << "</Text>" << endl;
      outf << "</Note>" << endl;
      }
    
    outf << "</BuildNameNotes>" << endl;
    outf << "</Site>" << endl;
    outf.close();
    }
  else
    {
    cerr << "Error writing this information to file " << VTK_SYSTEM_INFORMATION_NOTES << "\n";
    return 1;
    }
  
#if defined(__sgi) && !defined(__GNUC__) && defined(_COMPILER_VERSION)
  cout << "SGI compiler version: " << int(_COMPILER_VERSION) << endl;
#endif

  return 0;
} 
