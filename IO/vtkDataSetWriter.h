/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDataSetWriter.h
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
// .NAME vtkDataSetWriter - write any type of vtk dataset to file
// .SECTION Description
// vtkDataSetWriter is an abstract class for mapper objects that write their 
// data to disk (or into a communications port). The input to this object is
// a dataset of any type.

#ifndef __vtkDataSetWriter_h
#define __vtkDataSetWriter_h

#include "vtkDataWriter.h"

class VTK_IO_EXPORT vtkDataSetWriter : public vtkDataWriter
{
public:
  static vtkDataSetWriter *New();
  vtkTypeRevisionMacro(vtkDataSetWriter,vtkDataWriter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set / get the input data or filter.
  void SetInput(vtkDataSet *input);
  vtkDataSet *GetInput();

protected:
  vtkDataSetWriter() {};
  ~vtkDataSetWriter() {};

  void WriteData();

private:
  vtkDataSetWriter(const vtkDataSetWriter&);  // Not implemented.
  void operator=(const vtkDataSetWriter&);  // Not implemented.
};

#endif


