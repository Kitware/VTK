/*=========================================================================

  Program:   Visualization Toolkit
  Module:    SG2PolyF.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vtkStructuredGridToPolyFilter - abstract filter class
// .SECTION Description
// vtkStructuredGridToPolyFilter are filters whose subclasses take as input
// structured data (e.g., structured points, structured grid) and generate
// polygonal data on output.

#ifndef __vtkStructuredGridToPolyFilter_h
#define __vtkStructuredGridToPolyFilter_h

#include "SGridF.hh"
#include "PolyData.hh"

class vtkStructuredGridToPolyFilter : public vtkPolyData, public vtkStructuredGridFilter
{
public:
  char *GetClassName() {return "vtkStructuredGridToPolyFilter";};
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
  //Filter interface
  int GetDataReleased();
  void SetDataReleased(int flag);

};

#endif


