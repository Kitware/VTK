/*=========================================================================

  Program:   Visualization Library
  Module:    PlaneSrc.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file or its 
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
//
// Create nxm array of quadrilaterals
//
#ifndef __vlPlaneSource_h
#define __vlPlaneSource_h

#include "PolySrc.hh"

class vlPlaneSource : public vlPolySource 
{
public:
  vlPlaneSource() : XRes(1), YRes(1) {};
  vlPlaneSource(const int xR, const int yR) {XRes=xR; YRes=yR;};
  void SetResolution(const int xR, const int yR);
  void GetResolution(int& xR,int& yR) {xR=this->XRes; yR=this->YRes;};
  char *GetClassName() {return "vlPlaneSource";};
  void PrintSelf(ostream& os, vlIndent indent);

protected:
  void Execute();
  int XRes;
  int YRes;
};

#endif


