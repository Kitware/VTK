/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTreeRingToPolyData.h

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
// .NAME vtkTreeRingToPolyData - converts a tree to a polygonal data representing 
// radial space filling tree.
//
// .SECTION Description
// This algorithm requires that the vtkTreeRingLayout filter has already been applied to the
// data in order to create the quadruple array (inner radius, outer radius, start angle, 
// end angle) of bounds for each vertex of the tree.

#ifndef __vtkTreeRingToPolyData_h
#define __vtkTreeRingToPolyData_h

#include "vtkPolyDataAlgorithm.h"

class VTK_INFOVIS_EXPORT vtkTreeRingToPolyData : public vtkPolyDataAlgorithm 
{
public:
  static vtkTreeRingToPolyData *New();

  vtkTypeRevisionMacro(vtkTreeRingToPolyData,vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // The field containing quadruples of the form (min x, max x, min y, max y)
  // representing the bounds of the rectangles for each vertex.
  // This field may be added to the tree using vtkTreeRingLayout.
  // This array must be set.
  // TODO: This should be removed, and instead use ArrayToProcess from vtkAlgorithm.
  vtkGetStringMacro(SectorsFieldName);
  vtkSetStringMacro(SectorsFieldName);

//FIXME-jfsheph - Levels should be moved to the strategy and used to designate ring thickness...
  // The field containing the levels of each vertex in the tree
  // This array may be added to the tree using vtkTreeLevelsFilter.
  // The z-coordinate for vertex i is computed by 0.001 * level[i].
  // If this array is not set, the GetLevel() method of vtkTree is used
  // to determine the level.
  // TODO: This should be removed, and instead use ArrayToProcess from vtkAlgorithm.
  // TODO: Since we have access to the tree structure, this requirement could be
  // removed altogether.
//  vtkGetStringMacro(LevelsFieldName);
//  vtkSetStringMacro(LevelsFieldName);

//   // Description:
//   // The spacing along the z-axis between tree map levels.
//   vtkGetMacro(LevelDeltaZ, double);
//   vtkSetMacro(LevelDeltaZ, double);

  int FillInputPortInformation(int port, vtkInformation* info);

protected:
  vtkTreeRingToPolyData();
  ~vtkTreeRingToPolyData();

//  char * LevelsFieldName;
  char * SectorsFieldName;
//  double LevelDeltaZ;

  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
private:
  vtkTreeRingToPolyData(const vtkTreeRingToPolyData&);  // Not implemented.
  void operator=(const vtkTreeRingToPolyData&);  // Not implemented.
};

#endif
