/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDSW.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vtkDataSetWriter - write any type of vtk dataset to file
// .SECTION Description
// vtkDataSetWriter is an abstract class for mapper objects that write their 
// data to disk (or into a communications port).

#ifndef __vtkDataSetWriter_hh
#define __vtkDataSetWriter_hh

#include "vtkDataW.hh"

class vtkDataSetWriter : public vtkDataWriter
{
public:
  vtkDataSetWriter() {};
  ~vtkDataSetWriter() {};
  char *GetClassName() {return "vtkDataSetWriter";};
  void PrintSelf(ostream& os, vtkIndent indent);

  void SetInput(vtkDataSet *input);
  void SetInput(vtkDataSet &input) {this->SetInput(&input);};
  vtkDataSet *GetInput() {return (vtkDataSet *)this->Input;};

protected:
  void WriteData();

};

#endif


