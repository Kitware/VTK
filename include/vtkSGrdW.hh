/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSGrdW.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vtkStructuredGridWriter - write vtk structured grid data file
// .SECTION Description
// vtkStructuredGridWriter is a source object that writes ASCII or binary 
// structured grid data files in vtk format. See text for format details.
// .SECTION Caveats
// Binary files written on one system may not be readable on other systems.

#ifndef __vtkStructuredGridWriter_hh
#define __vtkStructuredGridWriter_hh

#include "vtkDataW.hh"
#include "SGrid.hh"

class vtkStructuredGridWriter : public vtkDataWriter
{
public:
  vtkStructuredGridWriter() {};
  ~vtkStructuredGridWriter() {};
  char *GetClassName() {return "vtkStructuredGridWriter";};
  void PrintSelf(ostream& os, vtkIndent indent);

  void SetInput(vtkStructuredGrid *input);
  void SetInput(vtkStructuredGrid &input) {this->SetInput(&input);};
  vtkStructuredGrid *GetInput() {return (vtkStructuredGrid *)this->Input;};
                               
protected:
  void WriteData();

};

#endif


