/*=========================================================================

  Program:   Visualization Library
  Module:    DS2UGrid.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vlDataSetToUnstructuredGridFilter - abstract filter class
// .SECTION Description
// vlDataSetToUnstructuredGridFilter is an abstract filter class whose 
// subclasses take as input any dataset and generate an unstructured
// grid on output.

#ifndef __vlDataSetToUnstructuredGridFilter_h
#define __vlDataSetToUnstructuredGridFilter_h

#include "DataSetF.hh"
#include "UGrid.hh"

class vlDataSetToUnstructuredGridFilter : public vlUnstructuredGrid, public vlDataSetFilter
{
public:
  char *GetClassName() {return "vlDataSetToUnstructuredGridFilter";};
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


