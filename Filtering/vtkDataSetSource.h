/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDataSetSource.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkDataSetSource - abstract class whose subclasses generate datasets
// .SECTION Description
// vtkDataSetSource is an abstract class whose subclasses generate datasets.

#ifndef __vtkDataSetSource_h
#define __vtkDataSetSource_h

#include "vtkSource.h"

class vtkDataSet;

class VTK_FILTERING_EXPORT vtkDataSetSource : public vtkSource
{
public:
  vtkTypeMacro(vtkDataSetSource,vtkSource);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get the output of this source.
  vtkDataSet *GetOutput();
  vtkDataSet *GetOutput(int idx);

  void SetOutput(vtkDataSet *);
  
protected:  
  vtkDataSetSource();
  ~vtkDataSetSource() {};

  virtual int FillOutputPortInformation(int, vtkInformation*);
private:
  vtkDataSetSource(const vtkDataSetSource&);  // Not implemented.
  void operator=(const vtkDataSetSource&);  // Not implemented.
};

#endif


