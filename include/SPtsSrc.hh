/*=========================================================================

  Program:   Visualization Library
  Module:    SPtsSrc.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vlStructuredPointsSource - abstract class whose subclasses generate structured points data
// .SECTION Description
// vlStructuredPointsSource is an abstract class whose subclasses
// generate vlStructuredPoints data.

#ifndef __vlStructuredPointsSource_h
#define __vlStructuredPointsSource_h

#include "Source.hh"
#include "StrPts.hh"

class vlStructuredPointsSource : public vlSource, public vlStructuredPoints
{
public:
  char *GetClassName() {return "vlStructuredPointSource";};
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


