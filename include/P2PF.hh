/*=========================================================================

  Program:   Visualization Toolkit
  Module:    P2PF.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file or its 
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vtkPolyToPolyFilter - abstract filter class
// .SECTION Description
// vtkPolyToPolyFilter is an abstract filter class whose subclasses take
// as input polygonal data and generate polygonal data on output.

#ifndef __vtkPolyToPolyFilter_h
#define __vtkPolyToPolyFilter_h

#include "PolyF.hh"
#include "PolyData.hh"

class vtkPolyToPolyFilter : public vtkPolyData, public vtkPolyFilter
{
public:
  char *GetClassName() {return "vtkPolyToPolyFilter";};
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


