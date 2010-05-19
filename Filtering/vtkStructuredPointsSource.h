/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkStructuredPointsSource.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkStructuredPointsSource - Abstract class whose subclasses generates structured Points data
// .SECTION Description
// vtkStructuredPointsSource is an abstract class whose subclasses generate
// structured Points data.

// .SECTION See Also
// vtkStructuredPointsReader vtkPLOT3DReader

#ifndef __vtkStructuredPointsSource_h
#define __vtkStructuredPointsSource_h

#include "vtkSource.h"

class vtkStructuredPoints;

class VTK_FILTERING_EXPORT vtkStructuredPointsSource : public vtkSource
{
public:
  vtkTypeMacro(vtkStructuredPointsSource,vtkSource);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set/Get the output of this source.
  void SetOutput(vtkStructuredPoints *output);
  vtkStructuredPoints *GetOutput();
  vtkStructuredPoints *GetOutput(int idx);
  
protected:
  vtkStructuredPointsSource();
  ~vtkStructuredPointsSource() {};

  // Default method performs Update to get information.  Not all the old
  // structured points sources compute information
  void ExecuteInformation();

  virtual int FillOutputPortInformation(int, vtkInformation*);
private:
  vtkStructuredPointsSource(const vtkStructuredPointsSource&);  // Not implemented.
  void operator=(const vtkStructuredPointsSource&);  // Not implemented.
};

#endif


