/*=========================================================================

  Program:   Visualization Library
  Module:    SGrid.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Description:
---------------------------------------------------------------------------
This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
//
// Structured Grid (i.e., topologically regular arrangement of points)
//
#ifndef __vlStructuredGrid_h
#define __vlStructuredGrid_h

#include "PointSet.hh"
#include "StrData.hh"
#include "CArray.hh"

class vlStructuredGrid : public vlPointSet, public vlStructuredDataSet {
public:
  vlStructuredGrid();
  ~vlStructuredGrid();
  char *GetClassName() {return "vlStructuredGrid";};
  void PrintSelf(ostream& os, vlIndent indent);

  // dataset interface
  vlDataSet *MakeObject() {return new vlStructuredGrid(*this);};
  vlMapper *MakeMapper() {return (vlMapper *)0;};
  void Initialize();
  int GetNumberOfPoints() {vlPointSet::GetNumberOfPoints();};
  vlCell *GetCell(int cellId);

protected:
  // points inherited
  // point data (i.e., scalars, vectors, normals, tcoords) inherited
  // blanking information inherited
};

#endif
