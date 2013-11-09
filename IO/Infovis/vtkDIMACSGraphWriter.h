/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkDIMACSGraphWriter.h,v $

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/

// .NAME vtkDIMACSGraphWriter - write vtkGraph data to a DIMACS
// formatted file

// .SECTION Description
// vtkDIMACSGraphWriter is a sink object that writes
// vtkGraph data files into a generic DIMACS (.gr) format.
//
// Output files contain a problem statement line:
//
// p graph <num_verts> <num_edges>
//
// Followed by |E| edge descriptor lines that are formatted as:
//
// e <source> <target> <weight>
//
// Vertices are numbered from 1..n in DIMACS formatted files.
//
// See webpage for format details.
// http://prolland.free.fr/works/research/dsat/dimacs.html

// .SECTION See Also
// vtkDIMACSGraphReader
//


#ifndef __vtkDIMACSGraphWriter_h
#define __vtkDIMACSGraphWriter_h

#include "vtkIOInfovisModule.h" // For export macro
#include "vtkDataWriter.h"

class vtkGraph;

class VTKIOINFOVIS_EXPORT vtkDIMACSGraphWriter : public vtkDataWriter
{
public:
  static vtkDIMACSGraphWriter *New();
  vtkTypeMacro(vtkDIMACSGraphWriter,vtkDataWriter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get the input to this writer.
  vtkGraph* GetInput();
  vtkGraph* GetInput(int port);

protected:
  vtkDIMACSGraphWriter() {}
  ~vtkDIMACSGraphWriter() {}

  void WriteData();

  virtual int FillInputPortInformation(int port, vtkInformation *info);

private:
  vtkDIMACSGraphWriter(const vtkDIMACSGraphWriter&);  // Not implemented.
  void operator=(const vtkDIMACSGraphWriter&);  // Not implemented.
};

#endif
