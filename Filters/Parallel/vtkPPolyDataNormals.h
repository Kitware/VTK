/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPPolyDataNormals.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPPolyDataNormals - compute normals for polygonal mesh
// .SECTION Description

#ifndef __vtkPPolyDataNormals_h
#define __vtkPPolyDataNormals_h

#include "vtkPolyDataNormals.h"

class VTK_PARALLEL_EXPORT vtkPPolyDataNormals : public vtkPolyDataNormals
{
public:
  vtkTypeMacro(vtkPPolyDataNormals,vtkPolyDataNormals);
  void PrintSelf(ostream& os, vtkIndent indent);

  static vtkPPolyDataNormals *New();

  // Description:
  // To get piece invariance, this filter has to request an 
  // extra ghost level.  By default piece invariance is on.
  vtkSetMacro(PieceInvariant, int);
  vtkGetMacro(PieceInvariant, int);
  vtkBooleanMacro(PieceInvariant, int);

protected:
  vtkPPolyDataNormals();
  ~vtkPPolyDataNormals() {};

  // Usual data generation method
  virtual int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  virtual int RequestUpdateExtent(vtkInformation *, vtkInformationVector **, vtkInformationVector *);

  int PieceInvariant;
private:
  vtkPPolyDataNormals(const vtkPPolyDataNormals&);  // Not implemented.
  void operator=(const vtkPPolyDataNormals&);  // Not implemented.
};

#endif
