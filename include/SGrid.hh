/*=========================================================================

  Program:   Visualization Library
  Module:    SGrid.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vlStructuredGrid - topologically regular array of data
// .SECTION Description
// vlStructuredGrid is a data object that is a concrete implementation of
// vlDataSet. vlStructuredGrid represents a geometric structure that is a
// topologically regular array of points. The topology is that of a cube that
// has been subdivided into a regular array of smaller cubes. Each point/cell
// can be addressed with i-j-k indices. Examples include finite difference 
// grids.

#ifndef __vlStructuredGrid_h
#define __vlStructuredGrid_h

#include "PointSet.hh"
#include "StrData.hh"

class vlStructuredGrid : public vlPointSet, public vlStructuredData {
public:
  vlStructuredGrid();
  vlStructuredGrid(const vlStructuredGrid& sg);
  ~vlStructuredGrid();
  char *GetClassName() {return "vlStructuredGrid";};
  char *GetDataType() {return "vlStructuredGrid";};
  void PrintSelf(ostream& os, vlIndent indent);

  // dataset interface
  vlDataSet *MakeObject() {return new vlStructuredGrid(*this);};
  void Initialize();
  int GetNumberOfPoints() {vlPointSet::GetNumberOfPoints();};
  vlCell *GetCell(int cellId);
  int GetCellType(int cellId);

protected:
  // points inherited
  // point data (i.e., scalars, vectors, normals, tcoords) inherited
  // blanking information inherited
};

#endif
