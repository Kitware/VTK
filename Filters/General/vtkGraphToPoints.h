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
/**
 * @class   vtkGraphToPoints
 * @brief   convert a vtkGraph a set of points.
 *
 *
 * Converts a vtkGraph to a vtkPolyData containing a set of points.
 * This assumes that the points
 * of the graph have already been filled (perhaps by vtkGraphLayout).
 * The vertex data is passed along to the point data.
*/

#ifndef vtkGraphToPoints_h
#define vtkGraphToPoints_h

#include "vtkFiltersGeneralModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

class VTKFILTERSGENERAL_EXPORT vtkGraphToPoints : public vtkPolyDataAlgorithm
{
public:
  static vtkGraphToPoints *New();
  vtkTypeMacro(vtkGraphToPoints,vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkGraphToPoints();
  ~vtkGraphToPoints() override {}

  /**
   * Convert the vtkGraph into vtkPolyData.
   */
  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *) override;

  /**
   * Set the input type of the algorithm to vtkGraph.
   */
  int FillInputPortInformation(int port, vtkInformation* info) override;

private:
  vtkGraphToPoints(const vtkGraphToPoints&) = delete;
  void operator=(const vtkGraphToPoints&) = delete;
};

#endif
