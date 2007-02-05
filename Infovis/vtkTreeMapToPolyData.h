/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTreeMapToPolyData.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkTreeMapToPolyData - converts a tree to a polygonal data representing a tree map
//
// .SECTION Description
// This algorithm requires that the vtkTreeMapLayout filter has already applied to the
// data in order to create the quadruple array (min x, max x, min y, max y) of
// bounds for each vertex of the tree.

#ifndef __vtkTreeMapToPolyData_h
#define __vtkTreeMapToPolyData_h

#include "vtkPolyDataAlgorithm.h"

class VTK_INFOVIS_EXPORT vtkTreeMapToPolyData : public vtkPolyDataAlgorithm 
{
public:
  static vtkTreeMapToPolyData *New();

  vtkTypeRevisionMacro(vtkTreeMapToPolyData,vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // The field containing quadruples of the form (min x, max x, min y, max y)
  // representing the bounds of the rectangles for each vertex.
  // This field may be added to the tree using vtkTreeMapLayout.
  // This array must be set.
  // TODO: This should be removed, and instead use ArrayToProcess from vtkAlgorithm.
  vtkGetStringMacro(RectanglesFieldName);
  vtkSetStringMacro(RectanglesFieldName);

  // The field containing the levels of each vertex in the tree
  // This array may be added to the tree using vtkTreeLevelsFilter.
  // The z-coordinate for vertex i is computed by 0.001 * level[i].
  // If this array is not set, the GetLevel() method of vtkTree is used
  // to determine the level.
  // TODO: This should be removed, and instead use ArrayToProcess from vtkAlgorithm.
  // TODO: Since we have access to the tree structure, this requirement could be
  // removed altogether.
  vtkGetStringMacro(LevelsFieldName);
  vtkSetStringMacro(LevelsFieldName);

  // Description:
  // The spacing along the z-axis between tree map levels.
  vtkGetMacro(LevelDeltaZ, double);
  vtkSetMacro(LevelDeltaZ, double);

  int FillInputPortInformation(int port, vtkInformation* info);

protected:
  vtkTreeMapToPolyData();
  ~vtkTreeMapToPolyData();

  char * LevelsFieldName;
  char * RectanglesFieldName;
  double LevelDeltaZ;

  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
private:
  vtkTreeMapToPolyData(const vtkTreeMapToPolyData&);  // Not implemented.
  void operator=(const vtkTreeMapToPolyData&);  // Not implemented.
};

#endif
