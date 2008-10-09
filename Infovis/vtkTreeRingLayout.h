/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTreeRingLayout.h

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
// .NAME vtkTreeRingLayout - layout a vtkTree into a tree map
//
// .SECTION Description
// vtkTreeRingLayout assigns sector regions to each vertex in the tree,
// creating a tree ring.  The data is added as a data array with four
// components per tuple representing the location and size of the
// sector using the format (innerRadius, outerRadius, StartAngle, EndAngle).
//
// This algorithm relies on a helper class to perform the actual layout.
// This helper class is a subclass of vtkTreeRingLayoutStrategy.
//
// .SECTION Thanks
// Thanks to Jason Shepherd from Sandia National Laboratories
// for help developing this class.

#ifndef __vtkTreeRingLayout_h
#define __vtkTreeRingLayout_h


#include "vtkTreeAlgorithm.h"

class vtkTreeRingLayoutStrategy;

class VTK_INFOVIS_EXPORT vtkTreeRingLayout : public vtkTreeAlgorithm 
{
public:
  static vtkTreeRingLayout *New();

  vtkTypeRevisionMacro(vtkTreeRingLayout,vtkTreeAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // The field name to use for storing the sector for each vertex.
  // The rectangles are stored in a quadruple float array 
  // (innerRadius, outerRadius, startAngle, endAngle).
  vtkGetStringMacro(SectorsFieldName);
  vtkSetStringMacro(SectorsFieldName);

  // Description:
  // The strategy to use when laying out the tree map.
  vtkGetObjectMacro(LayoutStrategy, vtkTreeRingLayoutStrategy);
  void SetLayoutStrategy(vtkTreeRingLayoutStrategy * strategy);

  // Description:
  // Returns the vertex id that contains pnt (or -1 if no one contains it)
  vtkIdType FindVertex(float pnt[2]);
  
  // Description:
  // Return the bounding sector information of the 
  // vertex's sector
 void GetBoundingSector(vtkIdType id, float *sinfo);

  // Description:
  // Get the modification time of the layout algorithm.
  virtual unsigned long GetMTime();

protected:
  vtkTreeRingLayout();
  ~vtkTreeRingLayout();

  char* SectorsFieldName;
  vtkTreeRingLayoutStrategy* LayoutStrategy;

  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  
private:

  vtkTreeRingLayout(const vtkTreeRingLayout&);  // Not implemented.
  void operator=(const vtkTreeRingLayout&);  // Not implemented.
};

#endif
