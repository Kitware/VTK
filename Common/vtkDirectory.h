/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDirectory.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to William A. Hoffman who developed this class
  

Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

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
  vtkTypeMacro(vtkDirectory,vtkObject);

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
  char* Path;		// Path to Open'ed directory
  char** Files;			// Array of Files
  int NumberOfFiles;		// Number if files in open directory
  
private:
  vtkDirectory(const vtkDirectory&);  // Not implemented.
  void operator=(const vtkDirectory&);  // Not implemented.
};

#endif
