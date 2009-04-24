/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRenderedSurfaceRepresentation.h

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
// .NAME vtkRenderedSurfaceRepresentation - Displays a geometric dataset as a surface.
//
// .SECTION Description
// vtkRenderedSurfaceRepresentation is used to show a geometric dataset in a view.
// The representation uses a vtkGeometryFilter to convert the dataset to
// polygonal data (e.g. volumetric data is converted to its external surface).
// The representation may then be added to vtkRenderView.

#ifndef __vtkRenderedSurfaceRepresentation_h
#define __vtkRenderedSurfaceRepresentation_h

#include "vtkDataRepresentation.h"

class vtkActor;
class vtkAlgorithmOutput;
class vtkDataObject;
class vtkExtractSelection;
class vtkGeometryFilter;
class vtkPolyDataMapper;
class vtkScalarsToColors;
class vtkSelection;
class vtkView;

class VTK_VIEWS_EXPORT vtkRenderedSurfaceRepresentation : public vtkDataRepresentation
{
public:
  static vtkRenderedSurfaceRepresentation *New();
  vtkTypeRevisionMacro(vtkRenderedSurfaceRepresentation, vtkDataRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Sets the input pipeline connection to this representation.
  virtual void SetInputConnection(vtkAlgorithmOutput* conn);

  //Description:
  //Sets the color array name
  virtual void SetCellColorArrayName(const char* arrayName);

  // Description:
  //Sets the lookup table
  virtual void SetCellColorLookupTable(vtkScalarsToColors* lut);
  virtual vtkScalarsToColors* GetCellColorLookupTable();
  
  // Description:
  // Specify range in terms of scalar minimum and maximum. 
  // These values are used to map scalars into lookup table.
  void SetCellColorScalarRange(double _arg1, double _arg2);

protected:
  vtkRenderedSurfaceRepresentation();
  ~vtkRenderedSurfaceRepresentation();
  
  // Decription:
  // Adds the representation to the view.  This is called from
  // vtkView::AddRepresentation().
  virtual bool AddToView(vtkView* view);
  
  // Decription:
  // Removes the representation to the view.  This is called from
  // vtkView::RemoveRepresentation().
  virtual bool RemoveFromView(vtkView* view);
  
  // Description:
  // Convert the selection to a type appropriate for sharing with other
  // representations through vtkSelectionLink.
  // If the selection cannot be applied to this representation, returns NULL.
  virtual vtkSelection* ConvertSelection(vtkView* view, vtkSelection* selection);
  
  // Description:
  // Internal pipeline objects.
  vtkGeometryFilter*    GeometryFilter;
  vtkPolyDataMapper*    Mapper;
  vtkActor*             Actor;
  vtkExtractSelection*  ExtractSelection;
  vtkGeometryFilter*    SelectionGeometryFilter;
  vtkPolyDataMapper*    SelectionMapper;
  vtkActor*             SelectionActor;

private:
  vtkRenderedSurfaceRepresentation(const vtkRenderedSurfaceRepresentation&);  // Not implemented.
  void operator=(const vtkRenderedSurfaceRepresentation&);  // Not implemented.
};

#endif
