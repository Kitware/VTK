/*=========================================================================

  Program:   Visualization Toolkit
  Module:    UGridSrc.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vtkUnstructuredGridSource - abstract class whose subclasses generate unstructured grid data
// .SECTION Description
// vtkUnstructuredGridSource is an abstract class whose subclasses generate unstructured grid data.

#ifndef __vtkUnstructuredGridSource_h
#define __vtkUnstructuredGridSource_h

#include "Source.hh"
#include "UGrid.hh"

class vtkUnstructuredGridSource : public vtkSource, public vtkUnstructuredGrid 
{
public:
  char *GetClassName() {return "vtkUnstructuredGridSource";};
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


