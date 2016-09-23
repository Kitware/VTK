/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTreeMapLayout.h

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
 * @class   vtkTreeMapLayout
 * @brief   layout a vtkTree into a tree map
 *
 *
 * vtkTreeMapLayout assigns rectangular regions to each vertex in the tree,
 * creating a tree map.  The data is added as a data array with four
 * components per tuple representing the location and size of the
 * rectangle using the format (Xmin, Xmax, Ymin, Ymax).
 *
 * This algorithm relies on a helper class to perform the actual layout.
 * This helper class is a subclass of vtkTreeMapLayoutStrategy.
 *
 * @par Thanks:
 * Thanks to Brian Wylie and Ken Moreland from Sandia National Laboratories
 * for help developing this class.
 *
 * @par Thanks:
 * Tree map concept comes from:
 * Shneiderman, B. 1992. Tree visualization with tree-maps: 2-d space-filling approach.
 * ACM Trans. Graph. 11, 1 (Jan. 1992), 92-99.
*/

#ifndef vtkTreeMapLayout_h
#define vtkTreeMapLayout_h

#include "vtkInfovisLayoutModule.h" // For export macro
#include "vtkTreeAlgorithm.h"

class vtkTreeMapLayoutStrategy;

class VTKINFOVISLAYOUT_EXPORT vtkTreeMapLayout : public vtkTreeAlgorithm
{
public:
  static vtkTreeMapLayout *New();

  vtkTypeMacro(vtkTreeMapLayout,vtkTreeAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  //@{
  /**
   * The field name to use for storing the rectangles for each vertex.
   * The rectangles are stored in a quadruple float array
   * (minX, maxX, minY, maxY).
   */
  vtkGetStringMacro(RectanglesFieldName);
  vtkSetStringMacro(RectanglesFieldName);
  //@}

  /**
   * The array to use for the size of each vertex.
   */
  virtual void SetSizeArrayName(const char* name)
    { this->SetInputArrayToProcess(0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_VERTICES, name); }

  //@{
  /**
   * The strategy to use when laying out the tree map.
   */
  vtkGetObjectMacro(LayoutStrategy, vtkTreeMapLayoutStrategy);
  void SetLayoutStrategy(vtkTreeMapLayoutStrategy * strategy);
  //@}

  /**
   * Returns the vertex id that contains pnt (or -1 if no one contains it)
   */
  vtkIdType FindVertex(float pnt[2], float *binfo=0);

  /**
   * Return the min and max 2D points of the
   * vertex's bounding box
   */
  void GetBoundingBox(vtkIdType id, float *binfo);

  /**
   * Get the modification time of the layout algorithm.
   */
  virtual vtkMTimeType GetMTime();

protected:
  vtkTreeMapLayout();
  ~vtkTreeMapLayout();

  char * RectanglesFieldName;
  vtkTreeMapLayoutStrategy* LayoutStrategy;

  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);

private:

  vtkTreeMapLayout(const vtkTreeMapLayout&) VTK_DELETE_FUNCTION;
  void operator=(const vtkTreeMapLayout&) VTK_DELETE_FUNCTION;
};

#endif
