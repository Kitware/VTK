/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtLabelMapper.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/
// .NAME vtkQtLabelMapper - draw text labels at 2D dataset points
// .SECTION Description
// vtkQtLabelMapper is a mapper that renders text at dataset
// points such that the labels do not overlap.
// Various items can be labeled including point ids, scalars,
// vectors, normals, texture coordinates, tensors, and field data components.
// This mapper assumes that the points are located on the x-y plane
// and that the camera remains perpendicular to that plane with a y-up
// axis (this can be constrained using vtkImageInteractor).
// On the first render, the mapper computes the visiblility of all labels
// at all scales, and queries this information on successive renders.
// This causes the first render to be much slower. The visibility algorithm
// is a greedy approach based on the point id, so the label for a point
// will be drawn unless the label for a point with lower id overlaps it.

// .SECTION Caveats
// Use this filter in combination with vtkSelectVisiblePoints if you want
// to label only points that are visible. If you want to label cells rather
// than points, use the filter vtkCellCenters to generate points at the
// center of the cells. Also, you can use the class vtkIdFilter to
// generate ids as scalars or field data, which can then be labeled.

// .SECTION See Also
// vtkLabeledDataMapper

// .SECTION Thanks

#ifndef __vtkQtLabelMapper_h
#define __vtkQtLabelMapper_h

#include "vtkLabeledDataMapper.h"
#include "QVTKWin32Header.h"

class vtkQtLabelSizeCalculator;
class vtkLabelPlacer;
class vtkPointSetToLabelHierarchy;
class vtkQtLabelSurface;
class vtkPolyDataMapper2D;

#include "vtkSmartPointer.h" //include to avoid compiler error

class QVTK_EXPORT vtkQtLabelMapper : public vtkLabeledDataMapper
{
public:
  static vtkQtLabelMapper *New();
  vtkTypeRevisionMacro(vtkQtLabelMapper, vtkLabeledDataMapper);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Draw non-overlapping labels to the screen.
  void RenderOpaqueGeometry(vtkViewport* viewport, vtkActor2D* actor);
  void RenderOverlay(vtkViewport *viewport, vtkActor2D *actor);

protected:
  vtkQtLabelMapper();
  ~vtkQtLabelMapper();

//BTX
  vtkSmartPointer<vtkQtLabelSizeCalculator> pcLabelSizer;
  vtkSmartPointer<vtkLabelPlacer> labelPlacer;
  vtkSmartPointer<vtkPointSetToLabelHierarchy> pointSetToLabelHierarchy;
  vtkSmartPointer<vtkQtLabelSurface> QtLabelSurface;
  vtkSmartPointer<vtkPolyDataMapper2D> polyDataMapper2; 
//ETX

private: 
  vtkQtLabelMapper(const vtkQtLabelMapper&);  // Not implemented.
  void operator=(const vtkQtLabelMapper&);  // Not implemented.
};

#endif

