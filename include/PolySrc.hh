/*=========================================================================

  Program:   Visualization Toolkit
  Module:    PolySrc.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file or its 
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vtkPolySource - abstract class whose subclasses generate polygonal data
// .SECTION Description
// vtkPolySource is an abstract class whose subclasses generate polygonal data.

#ifndef __vtkPolySource_h
#define __vtkPolySource_h

#include "Source.hh"
#include "PolyData.hh"

class vtkPolySource : public vtkSource, public vtkPolyData 
{
public:
  char *GetClassName() {return "vtkPolySource";};
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


