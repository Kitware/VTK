/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPolyW.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vtkPolyWriter - write vtk polygonal data
// .SECTION Description
// vtkPolyWriter is a source object that writes ASCII or binary 
// polygonal data files in vtk format. See text for format details.
// .SECTION Caveats
// Binary files written on one system may not be readable on other systems.

#ifndef __vtkPolyWriter_hh
#define __vtkPolyWriter_hh

#include "vtkDataW.hh"
#include "PolyData.hh"

class vtkPolyWriter : public vtkDataWriter
{
public:
  vtkPolyWriter() {};
  ~vtkPolyWriter() {};
  char *GetClassName() {return "vtkPolyWriter";};
  void PrintSelf(ostream& os, vtkIndent indent);

  void SetInput(vtkPolyData *input);
  void SetInput(vtkPolyData &input) {this->SetInput(&input);};
  vtkPolyData *GetInput() {return (vtkPolyData *)this->Input;};
                               
protected:
  void WriteData();

};

#endif


