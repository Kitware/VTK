/*=========================================================================

  Program:   Visualization Library
  Module:    SG2PolyF.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vlStructuredGridToPolyFilter - abstract filter class
// .SECTION Description
// vlStructuredGridToPolyFilter are filters whose subclasses take as input
// structured data (e.g., structured points, structured grid) and generate
// polygonal data on output.

#ifndef __vlStructuredGridToPolyFilter_h
#define __vlStructuredGridToPolyFilter_h

#include "SGridF.hh"
#include "PolyData.hh"

class vlStructuredGridToPolyFilter : public vlPolyData, public vlStructuredGridFilter
{
public:
  char *GetClassName() {return "vlStructuredGridToPolyFilter";};
  void PrintSelf(ostream& os, vlIndent indent);

  // Object interface
  void Modified();
  unsigned long int GetMTime();
  unsigned long int _GetMTime() {return this->GetMTime();};
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


