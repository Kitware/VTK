/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPointSetSource.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPointSetSource - abstract class whose subclasses generate point data
// .SECTION Description
// vtkPointSetSource is an abstract class whose subclasses generate pointdata.

#ifndef __vtkPointSetSource_h
#define __vtkPointSetSource_h

#include "vtkSource.h"

class vtkPointSet;

class VTK_FILTERING_EXPORT vtkPointSetSource : public vtkSource
{
public:
  vtkTypeMacro(vtkPointSetSource,vtkSource);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get the output of this source.
  vtkPointSet *GetOutput();
  vtkPointSet *GetOutput(int idx);
  void SetOutput(vtkPointSet *output);
  
protected:
  vtkPointSetSource();
  ~vtkPointSetSource() {};

  virtual int FillOutputPortInformation(int, vtkInformation*);
private:
  vtkPointSetSource(const vtkPointSetSource&);  // Not implemented.
  void operator=(const vtkPointSetSource&);  // Not implemented.
};

#endif


