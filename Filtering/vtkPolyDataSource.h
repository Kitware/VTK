/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPolyDataSource.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPolyDataSource - abstract class whose subclasses generate polygonal data
// .SECTION Description
// vtkPolyDataSource is an abstract class whose subclasses generate polygonal
// data.

// .SECTION See Also
// vtkPolyDataReader vtkAxes vtkBYUReader vtkConeSource vtkCubeSource
// vtkCursor3D vtkCyberReader vtkCylinderSource vtkDiskSource vtkLineSource
// vtkMCubesReader vtkOutlineSource vtkPlaneSource vtkPointSource vtkSTLReader
// vtkSphereSource vtkTextSource vtkUGFacetReader vtkVectorText

#ifndef __vtkPolyDataSource_h
#define __vtkPolyDataSource_h

#include "vtkSource.h"

class vtkPolyData;

class VTK_FILTERING_EXPORT vtkPolyDataSource : public vtkSource
{
public:
  vtkTypeMacro(vtkPolyDataSource,vtkSource);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get the output of this source.
  vtkPolyData *GetOutput();
  vtkPolyData *GetOutput(int idx);
  void SetOutput(vtkPolyData *output);

protected:
  vtkPolyDataSource();
  ~vtkPolyDataSource() {};
  
  // Update extent of PolyData is specified in pieces.  
  // Since all DataObjects should be able to set UpdateExent as pieces,
  // just copy output->UpdateExtent  all Inputs.
  void ComputeInputUpdateExtents(vtkDataObject *output);
  
  int FillOutputPortInformation(int, vtkInformation*);

private:
  vtkPolyDataSource(const vtkPolyDataSource&);  // Not implemented.
  void operator=(const vtkPolyDataSource&);  // Not implemented.
};

#endif





