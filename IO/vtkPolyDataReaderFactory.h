/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPolyDataReaderFactory.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPolyDataReaderFactory - Factory of polydata file readers.
// .SECTION Description
// vtkPolyDataReaderFactory: This class is used to create a
// vtkPolyData reader object of corresponding file type,
// given a path name to a file. It calls CanReadFile on all
// available readers until one of them returns true.  The available reader
// list comes from three places.  In the InitializeReaders function of this
// class, built-in VTK classes are added to the list, users can call
// RegisterReader, or users can create a vtkObjectFactory that has
// CreateObject method that returns a new vtkAbstractPolyDataReader 
// sub class.  This way applications can be extended with new readers
// via a plugin dll or by calling RegisterReader. Of course all of
// the readers that are part of the vtk release are made automatically
// available.
//
// .SECTION See Also
// vtkAbstractPolyDataReader 

#ifndef __vtkPolyDataReaderFactory_h
#define __vtkPolyDataReaderFactory_h


#include "vtkObject.h"

class vtkAbstractPolyDataReader;
class vtkPolyDataReaderCollection;
class vtkPolyDataReaderFactoryCleanup;

class VTK_IO_EXPORT vtkPolyDataReaderFactory : public vtkObject
{
public:
  static vtkPolyDataReaderFactory *New();
  vtkTypeRevisionMacro(vtkPolyDataReaderFactory,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);   
  
  //Description: register a reader with the available readers.   
  // registered readers will be queried in CreatePolyDataReader to 
  // see if they can load a given file.
  static void RegisterReader(vtkAbstractPolyDataReader* r);
  
  //Description: Given a path to a file find a reader that can
  // open the image file, it is the callers responsibility to call
  // Delete on the returned object.   If no reader is found, null
  // is returned.
  static vtkAbstractPolyDataReader* CreatePolyDataReader(const char* path); 

  // Description: get a list of the currently registered readers.
  // The caller must allocate the vtkPolyDataReaderCollection and pass in the
  // pointer to this method.
  static void GetRegisteredReaders(vtkPolyDataReaderCollection* );
protected:
  vtkPolyDataReaderFactory();
  ~vtkPolyDataReaderFactory();

  // Description: Initialize availiable readers list.
  static void InitializeReaders();

private:
  static vtkPolyDataReaderCollection* AvailableReaders;
  vtkPolyDataReaderFactory(const vtkPolyDataReaderFactory&);  // Not implemented.
  void operator=(const vtkPolyDataReaderFactory&);  // Not implemented.
//BTX
  friend class vtkPolyDataReaderFactoryCleanup;
//ETX
};

#endif
