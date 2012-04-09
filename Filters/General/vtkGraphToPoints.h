/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGraphToPoints.h

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
// .NAME vtkGraphToPoints - convert a vtkGraph a set of points.
//
// .SECTION Description
// Converts a vtkGraph to a vtkPolyData containing a set of points.
// This assumes that the points
// of the graph have already been filled (perhaps by vtkGraphLayout).
// The vertex data is passed along to the point data.

#ifndef __vtkGraphToPoints_h
#define __vtkGraphToPoints_h

#include "vtkFiltersGeneralModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

class VTKFILTERSGENERAL_EXPORT vtkGraphToPoints : public vtkPolyDataAlgorithm
{
public:
  static vtkGraphToPoints *New();
  vtkTypeMacro(vtkGraphToPoints,vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

protected:
  vtkGraphToPoints();
  ~vtkGraphToPoints() {}

  // Description:
  // Convert the vtkGraph into vtkPolyData.
  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);

  // Description:
  // Set the input type of the algorithm to vtkGraph.
  int FillInputPortInformation(int port, vtkInformation* info);

private:
  vtkGraphToPoints(const vtkGraphToPoints&);  // Not implemented.
  void operator=(const vtkGraphToPoints&);  // Not implemented.
};

#endif
