/*=========================================================================

  Program:   Visualization Library
  Module:    SGridSrc.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vlStructuredGridSource - Abstract class whose subclasses generates structured grid data
// .SECTION Description
// vlStructuredGridSource is an abstract class whose subclasses generate structured grid data.

#ifndef __vlStructuredGridSource_h
#define __vlStructuredGridSource_h

#include "Source.hh"
#include "SGrid.hh"

class vlStructuredGridSource : public vlSource, public vlStructuredGrid 
{
public:
  char *GetClassName() {return "vlStructuredGridSource";};
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
  //Source interface
  int GetDataReleased();
  void SetDataReleased(int flag);
};

#endif


