/*=========================================================================

  Program:   Visualization Library
  Module:    UGridSrc.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vlUnstructuredGridSource - abstract class whose subclasses generate unstructured grid data
// .SECTION Description
// vlUnstructuredGridSource is an abstract class whose subclasses generate unstructured grid data.

#ifndef __vlUnstructuredGridSource_h
#define __vlUnstructuredGridSource_h

#include "Source.hh"
#include "UGrid.hh"

class vlUnstructuredGridSource : public vlSource, public vlUnstructuredGrid 
{
public:
  char *GetClassName() {return "vlUnstructuredGridSource";};
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
  //Source interface
  int GetDataReleased();
  void SetDataReleased(int flag);
};

#endif


