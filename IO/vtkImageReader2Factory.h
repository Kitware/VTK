/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageReader2Factory.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkImageReader2Factory - Superclass of binary file readers.
// .SECTION Description
// vtkImageReader2Factory: This class is used to create a vtkImageReader2
// object given a path name to a file.  It calls CanReadFile on all
// available readers until one of them returns true.  The available reader
// list comes from three places.  In the InitializeReaders function of this
// class, built-in VTK classes are added to the list, users can call
// RegisterReader, or users can create a vtkObjectFactory that has
// CreateObject method that returns a new vtkImageReader2 sub class when
// given the string "vtkImageReaderObject".  This way applications can be
// extended with new readers via a plugin dll or by calling RegisterReader.
// Of course all of the readers that are part of the vtk release are made
// automatically available.
//
// .SECTION See Also
// vtkImageReader2 

#ifndef __vtkImageReader2Factory_h
#define __vtkImageReader2Factory_h


#include "vtkObject.h"

class vtkImageReader2;
class vtkImageReader2Collection;
class vtkImageReader2FactoryCleanup;

class VTK_IO_EXPORT vtkImageReader2Factory : public vtkObject
{
public:
  static vtkImageReader2Factory *New();
  vtkTypeMacro(vtkImageReader2Factory,vtkObject);
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
  static vtkImageReader2Collection* AvailableReaders;
  vtkImageReader2Factory(const vtkImageReader2Factory&);  // Not implemented.
  void operator=(const vtkImageReader2Factory&);  // Not implemented.
//BTX
  friend class vtkImageReader2FactoryCleanup;
//ETX
};

#endif
