/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPPolyDataNormals.h
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
// .NAME vtkPPolyDataNormals - compute normals for polygonal mesh
// .SECTION Description

#ifndef __vtkPPolyDataNormals_h
#define __vtkPPolyDataNormals_h

#include "vtkPolyDataNormals.h"

class VTK_PARALLEL_EXPORT vtkPPolyDataNormals : public vtkPolyDataNormals
{
public:
  vtkTypeRevisionMacro(vtkPPolyDataNormals,vtkPolyDataNormals);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
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
  virtual void Execute();
  void ComputeInputUpdateExtents(vtkDataObject *output);

  int PieceInvariant;
private:
  vtkPPolyDataNormals(const vtkPPolyDataNormals&);  // Not implemented.
  void operator=(const vtkPPolyDataNormals&);  // Not implemented.
};

#endif
