/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkUGrdW.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vtkUnstructuredGridWriter - write vtk unstructured grid data file
// .SECTION Description
// vtkUnstructuredGridWriter is a source object that writes ASCII or binary 
// unstructured grid data files in vtk format. See text for format details.
// .SECTION Caveats
// Binary files written on one system may not be readable on other systems.

#ifndef __vtkUnstructuredGridWriter_hh
#define __vtkUnstructuredGridWriter_hh

#include "vtkDataW.hh"
#include "UGrid.hh"

class vtkUnstructuredGridWriter : public vtkDataWriter
{
public:
  vtkUnstructuredGridWriter() {};
  ~vtkUnstructuredGridWriter() {};
  char *GetClassName() {return "vtkUnstructuredGridWriter";};
  void PrintSelf(ostream& os, vtkIndent indent);

  void SetInput(vtkUnstructuredGrid *input);
  void SetInput(vtkUnstructuredGrid &input) {this->SetInput(&input);};
  vtkUnstructuredGrid *GetInput() {return (vtkUnstructuredGrid *)this->Input;};
                               
protected:
  void WriteData();

};

#endif


