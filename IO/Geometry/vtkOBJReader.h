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
/**
 * @class   vtkOBJReader
 * @brief   read Wavefront .obj files
 *
 * vtkOBJReader is a source object that reads Wavefront .obj
 * files. The output of this source object is polygonal data.
 * @sa
 * vtkOBJImporter
*/

#ifndef vtkOBJReader_h
#define vtkOBJReader_h

#include "vtkIOGeometryModule.h" // For export macro
#include "vtkAbstractPolyDataReader.h"

class VTKIOGEOMETRY_EXPORT vtkOBJReader : public vtkAbstractPolyDataReader
{
public:
  static vtkOBJReader *New();
  vtkTypeMacro(vtkOBJReader,vtkAbstractPolyDataReader);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

protected:
  vtkOBJReader();
  ~vtkOBJReader() VTK_OVERRIDE;

  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *) VTK_OVERRIDE;
private:
  vtkOBJReader(const vtkOBJReader&) VTK_DELETE_FUNCTION;
  void operator=(const vtkOBJReader&) VTK_DELETE_FUNCTION;
};

#endif
