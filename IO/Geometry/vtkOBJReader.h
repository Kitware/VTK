/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOBJReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkOBJReader - read Wavefront .obj files
// .SECTION Description
// vtkOBJReader is a source object that reads Wavefront .obj
// files. The output of this source object is polygonal data.
// .SECTION See Also
// vtkOBJImporter

#ifndef vtkOBJReader_h
#define vtkOBJReader_h

#include "vtkIOGeometryModule.h" // For export macro
#include "vtkAbstractPolyDataReader.h"

class VTKIOGEOMETRY_EXPORT vtkOBJReader : public vtkAbstractPolyDataReader
{
public:
  static vtkOBJReader *New();
  vtkTypeMacro(vtkOBJReader,vtkAbstractPolyDataReader);
  void PrintSelf(ostream& os, vtkIndent indent);

protected:
  vtkOBJReader();
  ~vtkOBJReader();

  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
private:
  vtkOBJReader(const vtkOBJReader&);  // Not implemented.
  void operator=(const vtkOBJReader&);  // Not implemented.
};

#endif
