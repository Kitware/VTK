/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDirectory.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkDirectory - OS independent class for access to system directories
// .SECTION Description
// vtkDirectory provides a portable way of finding the names of the files
// in a system directory.

// .SECTION Caveats
// vtkDirectory works with windows and unix only.



#ifndef __vtkDirectory_h
#define __vtkDirectory_h

#include "vtkObject.h"

class vtkStringArray;

class VTK_COMMON_EXPORT vtkDirectory : public vtkObject
{
public:
  // Description:
  // Return the class name as a string.
  vtkTypeRevisionMacro(vtkDirectory,vtkObject);

  // Description:
  // Create a new vtkDirectory object.
  static vtkDirectory *New();

  // Description:
  // Print directory to stream.
  virtual void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Open the specified directory and load the names of the files
  // in that directory. 0 is returned if the directory can not be 
  // opened, 1 if it is opened.   
  int Open(const char* dir);

  // Description:
  // Return the number of files in the current directory.
  int GetNumberOfFiles();

  // Description:
  // Return the file at the given index, the indexing is 0 based
  const char* GetFile(int index);

  // Description:
  // Get an array that contains all the file names.
  vtkGetObjectMacro(Files, vtkStringArray);

  // Description:
  // Get the current working directory.
  static const char* GetCurrentWorkingDirectory(char* buf, unsigned int len);

  // Description:
  // Create directory. Needs rework to do all the testing and to work
  // on all platforms.
  static int MakeDirectory(const char* dir);

#ifdef VTK_WORKAROUND_WINDOWS_MANGLE
# define CreateDirectoryA CreateDirectory
# define CreateDirectoryW CreateDirectory
#endif

  // Description:
  // @deprecated Replaced by vtkDirectory::MakeDirectory() as of VTK 5.0.
  VTK_LEGACY(static int CreateDirectory(const char* dir));

#ifdef VTK_WORKAROUND_WINDOWS_MANGLE
# undef CreateDirectoryW
# undef CreateDirectoryA
  //BTX
  VTK_LEGACY(static int CreateDirectoryA(const char* dir));
  VTK_LEGACY(static int CreateDirectoryW(const char* dir));
  //ETX
#endif

protected:
  // delete the Files and Path ivars and set
  // NumberOfFiles to 0
  void CleanUpFilesAndPath();
  vtkDirectory();
  ~vtkDirectory() ;

private:
  char* Path;           // Path to Open'ed directory
  vtkStringArray *Files;    // VTK array of files

  static int CreateDirectoryInternal(const char* dir);

private:
  vtkDirectory(const vtkDirectory&);  // Not implemented.
  void operator=(const vtkDirectory&);  // Not implemented.
};

#endif
