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
// Structured points (e.g., volume(3D), image(2D), etc.)
//
#ifndef __vlStructuredPoints_h
#define __vlStructuredPoints_h

#include "DataSet.hh"
#include "IdList.hh"
#include "FPoints.hh"

#define SINGLE_POINT 0
#define X_LINE 1
#define Y_LINE 2
#define Z_LINE 3
#define XY_PLANE 4
#define YZ_PLANE 5
#define XZ_PLANE 6
#define XYZ_GRID 7

class vlStructuredPoints : public vlDataSet {
public:
  vlStructuredPoints();
  vlStructuredPoints(const vlStructuredPoints& v);
  ~vlStructuredPoints();
  char *GetClassName() {return "vlStructuredPoints";};
  void PrintSelf(ostream& os, vlIndent indent);

  // dataset interface
  vlDataSet *MakeObject() {return new vlStructuredPoints(*this);};
  int GetNumberOfCells();
  int GetNumberOfPoints(); 
  float *GetPoint(int ptId);
  vlCell *GetCell(int cellId);
  vlMapper *MakeMapper() {return (vlMapper *)0;};

  void SetDimension(int i, int j, int k);
  void SetDimension(int dim[3]);
  vlGetVectorMacro(Dimension,int);

  vlSetVector3Macro(AspectRatio,float);
  vlGetVectorMacro(AspectRatio,float);

  vlSetVector3Macro(Origin,float);
  vlGetVectorMacro(Origin,float);

protected:
  int Dimension[3];
  float Origin[3];
  float AspectRatio[3];
  int DataDescription;
};

#endif
