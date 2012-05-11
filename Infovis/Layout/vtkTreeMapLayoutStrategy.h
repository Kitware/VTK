/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTreeMapLayoutStrategy.h

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
// .NAME vtkTreeMapLayoutStrategy - abstract superclass for all tree map layout strategies
//
// .SECTION Description
// All subclasses of this class perform a tree map layout on a tree.
// This involves assigning a rectangular region to each vertex in the tree,
// and placing that information in a data array with four components per
// tuple representing (Xmin, Xmax, Ymin, Ymax).
//
// Instances of subclasses of this class may be assigned as the layout
// strategy to vtkTreeMapLayout
//
// .SECTION Thanks
// Thanks to Brian Wylie and Ken Moreland from Sandia National Laboratories
// for help developing this class.

#ifndef __vtkTreeMapLayoutStrategy_h
#define __vtkTreeMapLayoutStrategy_h


#include "vtkInfovisLayoutModule.h" // For export macro
#include "vtkAreaLayoutStrategy.h"

class vtkTree;
class vtkDataArray;

class VTKINFOVISLAYOUT_EXPORT vtkTreeMapLayoutStrategy : public vtkAreaLayoutStrategy
{
public:
  vtkTypeMacro(vtkTreeMapLayoutStrategy, vtkAreaLayoutStrategy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Find the vertex at a certain location, or -1 if none found.
  virtual vtkIdType FindVertex(
      vtkTree* tree, vtkDataArray* areaArray, float pnt[2]);

protected:
  vtkTreeMapLayoutStrategy();
  ~vtkTreeMapLayoutStrategy();
  void AddBorder( float *boxInfo);
private:
  vtkTreeMapLayoutStrategy(const vtkTreeMapLayoutStrategy&);  // Not implemented.
  void operator=(const vtkTreeMapLayoutStrategy&);  // Not implemented.
};

#endif

