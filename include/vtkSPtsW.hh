/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSPtsW.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vtkStructuredPointsWriter - write vtk structured points data file
// .SECTION Description
// vtkStructuredPointsWriter is a source object that writes ASCII or binary 
// structured points data in vtk file format. See text for format details.
// .SECTION Caveats
// Binary files written on one system may not be writeable on other systems.

#ifndef __vtkStructuredPointsWriter_hh
#define __vtkStructuredPointsWriter_hh

#include "vtkDataW.hh"
#include "StrPts.hh"

class vtkStructuredPointsWriter : public vtkDataWriter
{
public:
  vtkStructuredPointsWriter() {};
  ~vtkStructuredPointsWriter() {};
  char *GetClassName() {return "vtkStructuredPointsWriter";};
  void PrintSelf(ostream& os, vtkIndent indent);

  void SetInput(vtkStructuredPoints *input);
  void SetInput(vtkStructuredPoints &input) {this->SetInput(&input);};
  vtkStructuredPoints *GetInput() {return (vtkStructuredPoints *)this->Input;};
                               
protected:
  void WriteData();

};

#endif


