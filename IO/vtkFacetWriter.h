/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkFacetWriter.h

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkFacetWriter - reads a dataset in Facet format
// .SECTION Description
// vtkFacetWriter creates an unstructured grid dataset. It reads ASCII files
// stored in Facet format
//
// The facet format looks like this:
// FACET FILE ...
// nparts
// Part 1 name
// 0
// npoints 0 0
// p1x p1y p1z
// p2x p2y p2z
// ...
// 1
// Part 1 name
// ncells npointspercell
// p1c1 p2c1 p3c1 ... pnc1 materialnum partnum
// p1c2 p2c2 p3c2 ... pnc2 materialnum partnum
// ...

#ifndef __vtkFacetWriter_h
#define __vtkFacetWriter_h

#include "vtkPolyDataAlgorithm.h"

class vtkInformation;

class VTK_IO_EXPORT vtkFacetWriter : public vtkPolyDataAlgorithm
{
public:
  static vtkFacetWriter *New();
  vtkTypeMacro(vtkFacetWriter,vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Specify file name of Facet datafile to read
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);

  // Description:
  // Write data
  void Write();

  // BTX
  void WriteToStream(ostream* ost);
  // ETX
  
protected:
  vtkFacetWriter();
  ~vtkFacetWriter();

  // This is called by the superclass.
  // This is the method you should override.
  virtual int RequestData(vtkInformation *request,
                           vtkInformationVector** inputVector,
                           vtkInformationVector* outputVector);

  virtual int FillInputPortInformation(int, vtkInformation *);

  // BTX
  int WriteDataToStream(ostream* ost, vtkPolyData* data);
  // ETX

  char *FileName;
  ostream *OutputStream;

private:
  vtkFacetWriter(const vtkFacetWriter&);  // Not implemented.
  void operator=(const vtkFacetWriter&);  // Not implemented.
};

#endif

