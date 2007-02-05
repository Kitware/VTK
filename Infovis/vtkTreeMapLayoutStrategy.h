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


#include "vtkObject.h"

class vtkTree;
class vtkDataArray;

class VTK_INFOVIS_EXPORT vtkTreeMapLayoutStrategy : public vtkObject 
{
public:
  vtkTypeRevisionMacro(vtkTreeMapLayoutStrategy,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Perform the layout of the input tree, and store the rectangular
  // bounds of each vertex as a tuple (Xmin, Xmax, Ymin, Ymax) in a
  // data array.
  virtual void Layout(vtkTree *inputTree, vtkDataArray *rectArray) = 0;

  // Description:
  // Define the percentage that children vertex regions are inset from
  // the parent vertex region.
  vtkSetMacro(BorderPercentage, double);
  vtkGetMacro(BorderPercentage, double);

protected:
  vtkTreeMapLayoutStrategy();
  ~vtkTreeMapLayoutStrategy();
  double BorderPercentage;
  void AddBorder( float *boxInfo);
private:
  vtkTreeMapLayoutStrategy(const vtkTreeMapLayoutStrategy&);  // Not implemented.
  void operator=(const vtkTreeMapLayoutStrategy&);  // Not implemented.
};

#endif

