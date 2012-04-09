/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDataSetMapper.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkDataSetMapper - map vtkDataSet and derived classes to graphics primitives
// .SECTION Description
// vtkDataSetMapper is a mapper to map data sets (i.e., vtkDataSet and
// all derived classes) to graphics primitives. The mapping procedure
// is as follows: all 0D, 1D, and 2D cells are converted into points,
// lines, and polygons/triangle strips and then mapped to the graphics
// system. The 2D faces of 3D cells are mapped only if they are used by
// only one cell, i.e., on the boundary of the data set.

#ifndef __vtkDataSetMapper_h
#define __vtkDataSetMapper_h

#include "vtkRenderingCoreModule.h" // For export macro
#include "vtkMapper.h"

class vtkPolyDataMapper;
class vtkDataSetSurfaceFilter;

class VTKRENDERINGCORE_EXPORT vtkDataSetMapper : public vtkMapper
{
public:
  static vtkDataSetMapper *New();
  vtkTypeMacro(vtkDataSetMapper,vtkMapper);
  void PrintSelf(ostream& os, vtkIndent indent);
  void Render(vtkRenderer *ren, vtkActor *act);

  // Description:
  // Get the internal poly data mapper used to map data set to graphics system.
  vtkGetObjectMacro(PolyDataMapper, vtkPolyDataMapper);

  // Description:
  // Release any graphics resources that are being consumed by this mapper.
  // The parameter window could be used to determine which graphic
  // resources to release.
  void ReleaseGraphicsResources(vtkWindow *);

  // Description:
  // Get the mtime also considering the lookup table.
  unsigned long GetMTime();

  // Description:
  // Set the Input of this mapper.
  void SetInputData(vtkDataSet *input);
  vtkDataSet *GetInput();

protected:
  vtkDataSetMapper();
  ~vtkDataSetMapper();

  vtkDataSetSurfaceFilter *GeometryExtractor;
  vtkPolyDataMapper *PolyDataMapper;

  virtual void ReportReferences(vtkGarbageCollector*);

  // see algorithm for more info
  virtual int FillInputPortInformation(int port, vtkInformation* info);

private:
  vtkDataSetMapper(const vtkDataSetMapper&);  // Not implemented.
  void operator=(const vtkDataSetMapper&);  // Not implemented.
};

#endif


