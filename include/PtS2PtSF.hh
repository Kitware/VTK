/*=========================================================================

  Program:   Visualization Library
  Module:    PtS2PtSF.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vlPointSetToPointSetFilter - abstract filter class 
// .SECTION Description
// vlPointSetToPointSetFilter is an abstract filter class whose subclasses
// take as input a point set and generates a point set on output.
// At a minimum the concrete subclasses of vlPointSetToPointSetFilter
// modify their point coordinates. They never modify their topological 
// form, however.

#ifndef __vlPointSetToPointSetFilter_h
#define __vlPointSetToPointSetFilter_h

#include "PtSetF.hh"
#include "PointSet.hh"

class vlPointSetToPointSetFilter : public vlPointSet, public vlPointSetFilter
{
public:
  vlPointSetToPointSetFilter();
  ~vlPointSetToPointSetFilter();
  char *GetClassName() {return "vlPointSetToPointSetFilter";};
  char *GetDataType() {return this->PointSet->GetDataType();};
  void PrintSelf(ostream& os, vlIndent indent);

  // dataset interface
  vlDataSet *MakeObject();
  int GetNumberOfCells() {return this->PointSet->GetNumberOfCells();};
  vlCell *GetCell(int cellId) {return this->PointSet->GetCell(cellId);};
  int GetCellType(int cellId) {return this->PointSet->GetCellType(cellId);};
  void GetCellPoints(int cellId, vlIdList& ptIds)
    {this->PointSet->GetCellPoints(cellId, ptIds);};
  void GetPointCells(int ptId, vlIdList& cellIds)
    {this->PointSet->GetPointCells(ptId, cellIds);};
  void Initialize();

  void ComputeBounds();
  void Update();

protected:
  vlDataSet *PointSet;

};

#endif


