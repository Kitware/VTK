/*=========================================================================

  Program:   Visualization Library
  Module:    SPt2SPtF.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vlStructuredPointsToStructuredPointsFilter - abstract filter class
// .SECTION Description
// vlStructuredPointsToStructuredPointsFilter is an abstract filter class 
// whose subclasses take on input structured points and generate
// structured points on output.

#ifndef __vlStructuredPointsToStructuredPointsFilter_h
#define __vlStructuredPointsToStructuredPointsFilter_h

#include "StrPtsF.hh"
#include "StrPts.hh"

class vlStructuredPointsToStructuredPointsFilter : public vlStructuredPoints, 
                                              public vlStructuredPointsFilter
{
public:
  char *GetClassName() {return "vlDataSetToStructuredPointsFilter";};
  void PrintSelf(ostream& os, vlIndent indent);

  // Object interface
  void Modified();
  unsigned long int GetMTime();
  void DebugOn();
  void DebugOff();

  //DataSet interface
  void Update();

protected:
  //Filter interface
  int GetDataReleased();
  void SetDataReleased(int flag);

};

#endif


