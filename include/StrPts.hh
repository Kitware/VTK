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

#include "StrData.hh"

class vlStructuredPoints : public vlStructuredDataSet {
public:
  vlStructuredPoints();
  vlStructuredPoints(const vlStructuredPoints& v);
  ~vlStructuredPoints();
  char *GetClassName() {return "vlStructuredPoints";};
  void PrintSelf(ostream& os, vlIndent indent);

  // dataset interface
  vlDataSet *MakeObject() {return new vlStructuredPoints(*this);};
  float *GetPoint(int ptId);
  vlCell *GetCell(int cellId);
  vlMapper *MakeMapper() {return (vlMapper *)0;};
  void Initialize();
  int FindCell(float x[3], float dist2);

  vlSetVector3Macro(AspectRatio,float);
  vlGetVectorMacro(AspectRatio,float);

  vlSetVector3Macro(Origin,float);
  vlGetVectorMacro(Origin,float);

protected:
  float Origin[3];
  float AspectRatio[3];
};

#endif
