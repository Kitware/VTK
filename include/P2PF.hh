/*=========================================================================

  Program:   Visualization Library
  Module:    P2PF.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file or its 
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vlPolyToPolyFilter - abstract filter class
// .SECTION Description
// vlPolyToPolyFilter is an abstract filter class whose subclasses take
// as input polygonal data and generate polygonal data on output.

#ifndef __vlPolyToPolyFilter_h
#define __vlPolyToPolyFilter_h

#include "PolyF.hh"
#include "PolyData.hh"

class vlPolyToPolyFilter : public vlPolyData, public vlPolyFilter
{
public:
  char *GetClassName() {return "vlPolyToPolyFilter";};
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


