/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGenericDataSetSource.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkGenericDataSetSource - Objects that generate adapted data sets
// .SECTION Description
// Typical subclasses are readers of adapted data set.

// .SECTION See Also
// vtkGenericDataSet

#ifndef __vtkGenericDataSetSource_h
#define __vtkGenericDataSetSource_h

#include "vtkSource.h"

class vtkGenericDataSet;

class VTK_FILTERING_EXPORT vtkGenericDataSetSource : public vtkSource
{
public:
  vtkTypeRevisionMacro(vtkGenericDataSetSource,vtkSource);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get the output of this source.
  virtual vtkGenericDataSet *GetOutput();
  virtual vtkGenericDataSet *GetOutput(int i);
  
protected:
  vtkGenericDataSetSource();
  ~vtkGenericDataSetSource() {}
  
private:
  vtkGenericDataSetSource(const vtkGenericDataSetSource&);  // Not implemented.
  void operator=(const vtkGenericDataSetSource&);  // Not implemented.
};

#endif


