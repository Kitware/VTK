/*=========================================================================

  Program:   Visualization Toolkit
  Module:    DS2PolyF.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file or its 
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vtkDataSetToPolyFilter - abstract filter class
// .SECTION Description
// vtkDataSetToPolyFilter is an abstract filter class whose subclasses 
// take as input any dataset and generate polygonal data on output.

#ifndef __vtkDataSetToPolyFilter_h
#define __vtkDataSetToPolyFilter_h

#include "DataSetF.hh"
#include "PolyData.hh"

class vtkDataSetToPolyFilter : public vtkPolyData, public vtkDataSetFilter
{
public:
  char *GetClassName() {return "vtkDataSetToPolyFilter";};
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


