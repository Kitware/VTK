/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDirectory.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
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
#include "vtkDebugLeaks.h"

class VTK_COMMON_EXPORT vtkDirectory : public vtkObject
{
public:
  // Description:
  // Return the class name as a string.
  vtkTypeRevisionMacro(vtkDirectory,vtkObject);

  // Description:
  // Create a new vtkDirectory object.
  static vtkDirectory *New() {
#ifdef VTK_DEBUG_LEAKS
    vtkDebugLeaks::ConstructClass("vtkDirectory");
#endif    
    return new vtkDirectory;};

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
  int GetNumberOfFiles() { return this->NumberOfFiles; }
  // Description:
  // Return the file at the given index, the indexing is 0 based
  const char* GetFile(int index);
protected:
  vtkDirectory();
  ~vtkDirectory() ;
private:
  char* Path;           // Path to Open'ed directory
  char** Files;                 // Array of Files
  int NumberOfFiles;            // Number if files in open directory
  
private:
  vtkDirectory(const vtkDirectory&);  // Not implemented.
  void operator=(const vtkDirectory&);  // Not implemented.
};

#endif
