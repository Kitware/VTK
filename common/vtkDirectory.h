/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDirectory.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to William A. Hoffman who developed this class
  

Copyright (c) 1993-1998 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.


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

class VTK_EXPORT vtkDirectory : public vtkObject
{
public:
  // Description:
  // Return the class name as a string.
  virtual const char *GetClassName() {return "vtkDirectory";};

  // Description:
  // Create a new vtkDirectory object.
  static vtkDirectory *New() {return new vtkDirectory;};

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
  vtkDirectory(const vtkDirectory&) {};
  void operator=(const vtkDirectory&) {};
private:
  const char* Path;		// Path to Open'ed directory
  char** Files;			// Array of Files
  int NumberOfFiles;		// Number if files in open directory
  
};

#endif
