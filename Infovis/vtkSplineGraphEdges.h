/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSplineGraphEdges.h

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
// .NAME vtkSplineGraphEdges - subsample graph edges to make smooth curves
//
// .SECTION Description
// vtkSplineGraphEdges uses vtkSplineFilter to make edges into nicely sampled
// splines.

#ifndef __vtkSplineGraphEdges_h
#define __vtkSplineGraphEdges_h

#include "vtkGraphAlgorithm.h"

class vtkGraphToPolyData;
class vtkSplineFilter;

class VTK_INFOVIS_EXPORT vtkSplineGraphEdges : public vtkGraphAlgorithm 
{
public:
  static vtkSplineGraphEdges *New();
  vtkTypeRevisionMacro(vtkSplineGraphEdges,vtkGraphAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // The internal spline filter used to spline the edges. This should
  // only be used to set parameters on the filter. This was done to
  // avoid duplicating the spline filter API in this filter.
  virtual vtkSplineFilter* GetSplineFilter()
    { return this->Spline; }

protected:
  vtkSplineGraphEdges();
  ~vtkSplineGraphEdges();

  virtual int RequestData(
    vtkInformation *,
    vtkInformationVector **,
    vtkInformationVector *);

  virtual unsigned long GetMTime();

  vtkGraphToPolyData* GraphToPoly;
  vtkSplineFilter* Spline;

private:
  vtkSplineGraphEdges(const vtkSplineGraphEdges&);  // Not implemented.
  void operator=(const vtkSplineGraphEdges&);  // Not implemented.
};

#endif
