/*=========================================================================

  Program:   Visualization Library
  Module:    StrPts.hh
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
// Structured points (e.g., volume, image, etc.)
//
#ifndef __vlStructuredPoints_h
#define __vlStructuredPoints_h

#include "DataSet.hh"
#include "IdList.hh"
#include "FPoints.hh"

class vlStructuredPoints : public vlDataSet {
public:
  vlStructuredPoints();
  vlStructuredPoints(const vlStructuredPoints& v);
  ~vlStructuredPoints();
  vlDataSet *MakeObject() {return new vlStructuredPoints(*this);};
  char *GetClassName() {return "vlStructuredPoints";};
  void PrintSelf(ostream& os, vlIndent indent);
  int NumberOfCells() 
    {int nCells=(Dimension[0]-1)*(Dimension[1]-1)*(Dimension[2]-1);
     return (nCells < 0 ? 0 : nCells);};
  int NumberOfPoints() 
    {int nPts=Dimension[0]*Dimension[1]*Dimension[2];
     return (nPts < 0 ? 0 : nPts);};
  int CellDimension(int cellId);
  float *GetPoint(int i);
  void GetPoints(vlIdList& ptId, vlFloatPoints& fp);
  void CellPoints(int cellId, vlIdList& ptId);
  vlMapper *MakeMapper() {return (vlMapper *)0;};

  vlSetVector3Macro(Dimension,int);
  vlGetVectorMacro(Dimension,int);

  vlSetVector3Macro(AspectRatio,float);
  vlGetVectorMacro(AspectRatio,float);

  vlSetVector3Macro(Origin,float);
  vlGetVectorMacro(Origin,float);

protected:
  int Dimension[3];
  float AspectRatio[3];
  float Origin[3];
};

#endif
