/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRectilinearGridSource.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkRectilinearGridSource - Abstract class whose subclasses generates rectilinear grid data
// .SECTION Description
// vtkRectilinearGridSource is an abstract class whose subclasses generate
// rectilinear grid data.

// .SECTION See Also
// vtkRectilinearGridReader

#ifndef __vtkRectilinearGridSource_h
#define __vtkRectilinearGridSource_h

#include "vtkSource.h"

class vtkRectilinearGrid;

class VTK_FILTERING_EXPORT vtkRectilinearGridSource : public vtkSource
{
public:
  vtkTypeMacro(vtkRectilinearGridSource,vtkSource);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get the output of this source.
  vtkRectilinearGrid *GetOutput();
  vtkRectilinearGrid *GetOutput(int idx);
  void SetOutput(vtkRectilinearGrid *output);

protected:
  vtkRectilinearGridSource();
  ~vtkRectilinearGridSource() {};

  virtual int FillOutputPortInformation(int, vtkInformation*);
private:
  vtkRectilinearGridSource(const vtkRectilinearGridSource&);  // Not implemented.
  void operator=(const vtkRectilinearGridSource&);  // Not implemented.
};

#endif


