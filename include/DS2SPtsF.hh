/*=========================================================================

  Program:   Visualization Library
  Module:    DS2SPtsF.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vlDataSetToStructuredPointsFilter - abstract filter class
// .SECTION Description
// vlDataSetToStructuredPointsFilter is an abstract filter class whose
// subclasses take as input any dataset and generate structured points 
// data on output.

#ifndef __vlDataSetToStructuredPointsFilter_h
#define __vlDataSetToStructuredPointsFilter_h

#include "DataSetF.hh"
#include "StrPts.hh"

class vlDataSetToStructuredPointsFilter : public vlStructuredPoints, public vlDataSetFilter
{
public:
  char *GetClassName() {return "vlDataSetToStructuredPointsFilter";};
  void PrintSelf(ostream& os, vlIndent indent);

  // Object interface
  void Modified();
  unsigned long int GetMTime();
  unsigned long int _GetMTime() {this->GetMTime();};
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


