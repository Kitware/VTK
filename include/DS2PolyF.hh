/*=========================================================================

  Program:   Visualization Library
  Module:    DS2PolyF.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file or its 
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vlDataSetToPolyFilter - abstract filter class
// .SECTION Description
// vlDataSetToPolyFilter is an abstract filter class whose subclasses 
// take as input any dataset and generate polygonal data on output.

#ifndef __vlDataSetToPolyFilter_h
#define __vlDataSetToPolyFilter_h

#include "DataSetF.hh"
#include "PolyData.hh"

class vlDataSetToPolyFilter : public vlPolyData, public vlDataSetFilter
{
public:
  char *GetClassName() {return "vlDataSetToPolyFilter";};
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


