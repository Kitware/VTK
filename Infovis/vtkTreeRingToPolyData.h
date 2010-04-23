/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTreeRingToPolyData.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/
// .NAME vtkTreeRingToPolyData - converts a tree to a polygonal data
// representing radial space filling tree.
//
// .SECTION Description
// This algorithm requires that the vtkTreeRingLayout filter has already
// been applied to the data in order to create the quadruple array 
// (start angle, end angle, inner radius, outer radius) of bounds 
// for each vertex of the tree.

#ifndef __vtkTreeRingToPolyData_h
#define __vtkTreeRingToPolyData_h

#include "vtkPolyDataAlgorithm.h"

class VTK_INFOVIS_EXPORT vtkTreeRingToPolyData : public vtkPolyDataAlgorithm 
{
public:
  static vtkTreeRingToPolyData *New();
  
  vtkTypeMacro(vtkTreeRingToPolyData,vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // The field containing quadruples of the form (start angle, end angle, 
  // inner radius, outer radius)
  // representing the bounds of the rectangles for each vertex.
  // This field may be added to the tree using vtkTreeRingLayout.
  // This array must be set.
  virtual void SetSectorsArrayName(const char* name)
    { this->SetInputArrayToProcess(0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_VERTICES, name); }
  
  // Description:
  // Define a shrink percentage for each of the sectors.
  vtkSetMacro(ShrinkPercentage, double);
  vtkGetMacro(ShrinkPercentage, double);
  
  int FillInputPortInformation(int port, vtkInformation* info);
  
protected:
  vtkTreeRingToPolyData();
  ~vtkTreeRingToPolyData();
  
  double ShrinkPercentage;
  
  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
private:
  vtkTreeRingToPolyData(const vtkTreeRingToPolyData&);  // Not implemented.
  void operator=(const vtkTreeRingToPolyData&);  // Not implemented.
};

#endif
