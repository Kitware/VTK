/*=========================================================================

  Program:   Visualization Toolkit
  Module:    SGridSrc.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vtkStructuredGridSource - Abstract class whose subclasses generates structured grid data
// .SECTION Description
// vtkStructuredGridSource is an abstract class whose subclasses generate structured grid data.

#ifndef __vtkStructuredGridSource_h
#define __vtkStructuredGridSource_h

#include "Source.hh"
#include "SGrid.hh"

class vtkStructuredGridSource : public vtkSource, public vtkStructuredGrid 
{
public:
  char *GetClassName() {return "vtkStructuredGridSource";};
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


