/*=========================================================================

  Program:   Visualization Library
  Module:    StrData.hh
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
// Structured data (described by dimensions)
//
#ifndef __vlStructuredDataSet_h
#define __vlStructuredDataSet_h

#include "DataSet.hh"
#include "CArray.hh"

#define SINGLE_POINT 0
#define X_LINE 1
#define Y_LINE 2
#define Z_LINE 3
#define XY_PLANE 4
#define YZ_PLANE 5
#define XZ_PLANE 6
#define XYZ_GRID 7

class vlStructuredDataSet : public vlDataSet {
public:
  vlStructuredDataSet();
  vlStructuredDataSet(const vlStructuredDataSet& sds);
  ~vlStructuredDataSet();
  char *GetClassName() {return "vlStructuredDataSet";};
  void PrintSelf(ostream& os, vlIndent indent);

  // dataset interface
  int GetNumberOfCells();
  int GetNumberOfPoints(); 
  void Initialize();

  void SetDimensions(int i, int j, int k);
  void SetDimensions(int dim[3]);
  vlGetVectorMacro(Dimensions,int);

  void BlankingOn();
  void BlankingOff();
  int GetBlanking() {return this->Blanking;};
  void BlankPoint(int ptId);
  void UnBlankPoint(int ptId);
  int IsPointVisible(int ptId) {if (!this->Blanking) return 1; 
                                else return this->PointVisibility->GetValue(ptId);} 

protected:
  int Dimensions[3];
  int DataDescription;
  int Blanking;
  vlCharArray *PointVisibility;
};

#endif
