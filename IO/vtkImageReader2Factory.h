/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageReader2Factory.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

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
// .NAME vtkImageReader2Factory - Superclass of binary file readers.
// .SECTION Description
// vtkImageReader2Factory: This class is used to create a vtkImageReader2
// object given a path name to a file.  It calls CanReadFile on all
// availiable readers until one of them returns ture.  The availiable reader
// list comes from three places.  In the InitializeReaders function of this
// class, built-in VTK classes are added to the list, users can call
// RegisterReader, or users can create a vtkObjectFactory that has
// CreateObject method that returns a new vtkImageReader2 sub class when
// given the string "vtkImageReaderObject".  This way applications can be
// extened with new readers via a plugin dll or by calling RegisterReader.
// Of course all of the readers that are part of the vtk release are made
// automatically availiable.
//
// .SECTION See Also
// vtkImageReader2 

#ifndef __vtkImageReader2Factory_h
#define __vtkImageReader2Factory_h


#include "vtkObject.h"
class vtkImageReader2Collection;
class vtkImageReader2;

class VTK_IO_EXPORT vtkImageReader2Factory : public vtkObject
{
public:
  static vtkImageReader2Factory *New();
  vtkTypeRevisionMacro(vtkImageReader2Factory,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);   
  
  //Description: register a reader with the available readers.   
  // registered readers will be queried in CreateImageReader2 to 
  // see if they can load a given file.
  static void RegisterReader(vtkImageReader2* r);
  
  //Description: Given a path to a file find a reader that can
  // open the image file, it is the callers responsibility to call
  // Delete on the returned object.   If no reader is found, null
  // is returned.
  static vtkImageReader2* CreateImageReader2(const char* path); 

  // Description: get a list of the currently registered readers.
  // The caller must allocate the vtkImageReader2Collection and pass in the
  // pointer to this method.
  static void GetRegisteredReaders(vtkImageReader2Collection* );
protected:
  vtkImageReader2Factory();
  ~vtkImageReader2Factory();

  // Description: Initialize availiable readers list.
  static void InitializeReaders();

private:
  static vtkImageReader2Collection* AvailiableReaders;
  vtkImageReader2Factory(const vtkImageReader2Factory&);  // Not implemented.
  void operator=(const vtkImageReader2Factory&);  // Not implemented.
//BTX
  friend class vtkCleanUpImageReader2Factory;
//ETX
};

#endif
