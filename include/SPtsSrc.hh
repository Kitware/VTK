/*=========================================================================

  Program:   Visualization Toolkit
  Module:    SPtsSrc.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vtkStructuredPointsSource - abstract class whose subclasses generate structured points data
// .SECTION Description
// vtkStructuredPointsSource is an abstract class whose subclasses
// generate vtkStructuredPoints data.

#ifndef __vtkStructuredPointsSource_h
#define __vtkStructuredPointsSource_h

#include "Source.hh"
#include "StrPts.hh"

class vtkStructuredPointsSource : public vtkSource, public vtkStructuredPoints
{
public:
  char *GetClassName() {return "vtkStructuredPointSource";};
  void PrintSelf(ostream& os, vtkIndent indent);

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


