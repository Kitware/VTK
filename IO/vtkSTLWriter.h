/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSTLWriter.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSTLWriter - write stereo lithography files
// .SECTION Description
// vtkSTLWriter writes stereo lithography (.stl) files in either ASCII or
// binary form. Stereo lithography files only contain triangles. If polygons
// with more than 3 vertices are present, only the first 3 vertices are
// written.  Use vtkTriangleFilter to convert polygons to triangles.

// .SECTION Caveats
// Binary files written on one system may not be readable on other systems.
// vtkSTLWriter uses VAX or PC byte ordering and swaps bytes on other systems.

#ifndef __vtkSTLWriter_h
#define __vtkSTLWriter_h

#include "vtkPolyDataWriter.h"

class VTK_IO_EXPORT vtkSTLWriter : public vtkPolyDataWriter
{
public:
  static vtkSTLWriter *New();
  vtkTypeRevisionMacro(vtkSTLWriter,vtkPolyDataWriter);

protected:
  vtkSTLWriter();
  ~vtkSTLWriter() {};

  void WriteData();

  void WriteBinarySTL(vtkPoints *pts, vtkCellArray *polys);
  void WriteAsciiSTL(vtkPoints *pts, vtkCellArray *polys);
private:
  vtkSTLWriter(const vtkSTLWriter&);  // Not implemented.
  void operator=(const vtkSTLWriter&);  // Not implemented.
};

#endif

