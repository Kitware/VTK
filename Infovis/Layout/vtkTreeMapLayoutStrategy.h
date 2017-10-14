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
/**
 * @class   vtkTreeMapLayoutStrategy
 * @brief   abstract superclass for all tree map layout strategies
 *
 *
 * All subclasses of this class perform a tree map layout on a tree.
 * This involves assigning a rectangular region to each vertex in the tree,
 * and placing that information in a data array with four components per
 * tuple representing (Xmin, Xmax, Ymin, Ymax).
 *
 * Instances of subclasses of this class may be assigned as the layout
 * strategy to vtkTreeMapLayout
 *
 * @par Thanks:
 * Thanks to Brian Wylie and Ken Moreland from Sandia National Laboratories
 * for help developing this class.
*/

#ifndef vtkTreeMapLayoutStrategy_h
#define vtkTreeMapLayoutStrategy_h


#include "vtkInfovisLayoutModule.h" // For export macro
#include "vtkAreaLayoutStrategy.h"

class vtkTree;
class vtkDataArray;

class VTKINFOVISLAYOUT_EXPORT vtkTreeMapLayoutStrategy : public vtkAreaLayoutStrategy
{
public:
  vtkTypeMacro(vtkTreeMapLayoutStrategy, vtkAreaLayoutStrategy);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Find the vertex at a certain location, or -1 if none found.
   */
  vtkIdType FindVertex(
      vtkTree* tree, vtkDataArray* areaArray, float pnt[2]) override;

protected:
  vtkTreeMapLayoutStrategy();
  ~vtkTreeMapLayoutStrategy() override;
  void AddBorder( float *boxInfo);
private:
  vtkTreeMapLayoutStrategy(const vtkTreeMapLayoutStrategy&) = delete;
  void operator=(const vtkTreeMapLayoutStrategy&) = delete;
};

#endif

