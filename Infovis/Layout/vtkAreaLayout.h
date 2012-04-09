/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAreaLayout.h

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
// .NAME vtkAreaLayout - layout a vtkTree into a tree map
//
// .SECTION Description
// vtkAreaLayout assigns sector regions to each vertex in the tree,
// creating a tree ring.  The data is added as a data array with four
// components per tuple representing the location and size of the
// sector using the format (StartAngle, EndAngle, innerRadius, outerRadius).
//
// This algorithm relies on a helper class to perform the actual layout.
// This helper class is a subclass of vtkAreaLayoutStrategy.
//
// .SECTION Thanks
// Thanks to Jason Shepherd from Sandia National Laboratories
// for help developing this class.

#ifndef __vtkAreaLayout_h
#define __vtkAreaLayout_h

#include "vtkTreeAlgorithm.h"

class vtkAreaLayoutStrategy;

class VTK_INFOVIS_EXPORT vtkAreaLayout : public vtkTreeAlgorithm
{
public:
  static vtkAreaLayout *New();
  vtkTypeMacro(vtkAreaLayout,vtkTreeAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // The array name to use for retrieving the relative size of each vertex.
  // If this array is not found, use constant size for each vertex.
  virtual void SetSizeArrayName(const char* name)
    { this->SetInputArrayToProcess(0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_VERTICES, name); }

  // Description:
  // The name for the array created for the area for each vertex.
  // The rectangles are stored in a quadruple float array
  // (startAngle, endAngle, innerRadius, outerRadius).
  // For rectangular layouts, this is (minx, maxx, miny, maxy).
  vtkGetStringMacro(AreaArrayName);
  vtkSetStringMacro(AreaArrayName);

  // Description:
  // Whether to output a second output tree with vertex locations
  // appropriate for routing bundled edges. Default is on.
  vtkGetMacro(EdgeRoutingPoints, bool);
  vtkSetMacro(EdgeRoutingPoints, bool);
  vtkBooleanMacro(EdgeRoutingPoints, bool);

  // Description:
  // The strategy to use when laying out the tree map.
  vtkGetObjectMacro(LayoutStrategy, vtkAreaLayoutStrategy);
  void SetLayoutStrategy(vtkAreaLayoutStrategy * strategy);

  // Description:
  // Get the modification time of the layout algorithm.
  virtual unsigned long GetMTime();

  // Description:
  // Get the vertex whose area contains the point, or return -1
  // if no vertex area covers the point.
  vtkIdType FindVertex(float pnt[2]);

  // Description:
  // The bounding area information for a certain vertex id.
  void GetBoundingArea(vtkIdType id, float *sinfo);

protected:
  vtkAreaLayout();
  ~vtkAreaLayout();

  char* AreaArrayName;
  bool  EdgeRoutingPoints;
  char* EdgeRoutingPointsArrayName;
  vtkAreaLayoutStrategy* LayoutStrategy;

  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);

private:

  vtkAreaLayout(const vtkAreaLayout&);  // Not implemented.
  void operator=(const vtkAreaLayout&);  // Not implemented.
};

#endif
