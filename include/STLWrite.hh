/*=========================================================================

  Program:   Visualization Toolkit
  Module:    STLWrite.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vtkSTLWriter - write stereo lithography files
// .SECTION Description
// vtkSTLWriter writes stereo lithography (.stl) files in either ASCII or 
// binary form.
// .SECTION Caveats
// Binary files written on one system may not be readable on other systems.
// vtkSTLWriter uses VAX or PC byte ordering and swaps bytes on other systems.

#ifndef __vtkSTLWriter_h
#define __vtkSTLWriter_h

#include <stdio.h>
#include "Writer.hh"
#include "PolyData.hh"

#define STL_ASCII 0
#define STL_BINARY 1

class vtkSTLWriter : public vtkWriter
{
public:
  vtkSTLWriter();
  ~vtkSTLWriter();
  char *GetClassName() {return "vtkSTLWriter";};
  void PrintSelf(ostream& os, vtkIndent indent);

  void SetInput(vtkPolyData *input);
  void SetInput(vtkPolyData &input) {this->SetInput(&input);};
  vtkPolyData *GetInput() {return (vtkPolyData *)this->Input;};
                               
  // Description:
  // Specify the name of the file to write.
  vtkSetStringMacro(Filename);
  vtkGetStringMacro(Filename);

  // Description:
  // Specify type of file to write (ascii or binary).
  vtkSetClampMacro(WriteMode,int,STL_ASCII,STL_BINARY);
  vtkGetMacro(WriteMode,int);

protected:
  void WriteData();

  char *Filename;
  int WriteMode;

  void WriteBinarySTL(vtkPoints *pts, vtkCellArray *polys);
  void WriteAsciiSTL(vtkPoints *pts, vtkCellArray *polys);
};

#endif

