/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkStructuredGridSource.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkStructuredGridSource - Abstract class whose subclasses generates structured grid data
// .SECTION Description
// vtkStructuredGridSource is an abstract class whose subclasses generate
// structured grid data.

// .SECTION See Also
// vtkStructuredGridReader vtkPLOT3DReader

#ifndef __vtkStructuredGridSource_h
#define __vtkStructuredGridSource_h

#include "vtkSource.h"

class vtkStructuredGrid;

class VTK_FILTERING_EXPORT vtkStructuredGridSource : public vtkSource
{
public:
  vtkTypeMacro(vtkStructuredGridSource,vtkSource);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get the output of this source.
  vtkStructuredGrid *GetOutput();
  vtkStructuredGrid *GetOutput(int idx);
  void SetOutput(vtkStructuredGrid *output);  

protected:
  vtkStructuredGridSource();
  ~vtkStructuredGridSource() {};

  virtual int FillOutputPortInformation(int, vtkInformation*);
private:
  vtkStructuredGridSource(const vtkStructuredGridSource&);  // Not implemented.
  void operator=(const vtkStructuredGridSource&);  // Not implemented.
};

#endif


