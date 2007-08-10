/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSquarifyLayoutStrategy.h
  
  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/
// .NAME vtkSquarifyLayoutStrategy - uses the squarify tree map layout algorithm
//
// .SECTION Description
// vtkSquarigyLayoutStrategy partitions the space for child vertices into regions
// that use all avaliable space and are as close to squares as possible.
// The algorithm also takes into account the relative vertex size.
//
// .SECTION Thanks
// The squarified tree map algorithm comes from:
// Bruls, D.M., C. Huizing, J.J. van Wijk. Squarified Treemaps.
// In: W. de Leeuw, R. van Liere (eds.), Data Visualization 2000, 
// Proceedings of the joint Eurographics and IEEE TCVG Symposium on Visualization, 
// 2000, Springer, Vienna, p. 33-42.

#ifndef __vtkSquarifyLayoutStrategy_h
#define __vtkSquarifyLayoutStrategy_h

#include "vtkTreeMapLayoutStrategy.h"

class vtkIdList;

class VTK_INFOVIS_EXPORT vtkSquarifyLayoutStrategy : public vtkTreeMapLayoutStrategy 
{
public:
  static vtkSquarifyLayoutStrategy *New();

  vtkTypeRevisionMacro(vtkSquarifyLayoutStrategy,vtkTreeMapLayoutStrategy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // The field name associated with the size of the vertex.
  vtkGetStringMacro(SizeFieldName);
  vtkSetStringMacro(SizeFieldName);

  // Description:
  // Perform the layout of a tree and place the results as 4-tuples in
  // coordsArray (Xmin, Xmax, Ymin, Ymax).
  void Layout(vtkTree *inputTree, vtkDataArray *coordsArray);

protected:
  vtkSquarifyLayoutStrategy();
  ~vtkSquarifyLayoutStrategy();

private:

  char * SizeFieldName;

  void LayoutChildren(
    vtkTree *tree, 
    vtkDataArray *coordsArray,
    vtkDataArray *sizeArray,
    vtkIdType nchildren,
    const vtkIdType* children,
    vtkIdType begin, 
    float minX, float maxX, 
    float minY, float maxY);

  vtkSquarifyLayoutStrategy(const vtkSquarifyLayoutStrategy&);  // Not implemented.
  void operator=(const vtkSquarifyLayoutStrategy&);  // Not implemented.
};

#endif

