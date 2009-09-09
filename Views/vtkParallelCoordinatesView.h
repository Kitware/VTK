/*=========================================================================
  
Program:   Visualization Toolkit
Module:    vtkParallelCoordinatesView.h

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2009 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/
// .NAME vtkParallelCoordinatesView - Self-contained view for MPDE height-field data
//
// .SECTION Description
//
// MPDE data gets represented as a vtkRectilinearGrid when it comes
// out of the reader.  This class encapsulates the process of splitting
// that out into selections, converting to poly data, and rendering it
// with appropriate decorations.

#ifndef __vtkParallelCoordinatesView_h
#define __vtkParallelCoordinatesView_h

#include <vtkRenderView.h>

class vtkActor2D;
class vtkOutlineSource;
class vtkParallelCoordinatesRepresentation;
class vtkPolyData;
class vtkPolyDataMapper2D;

class VTK_VIEWS_EXPORT vtkParallelCoordinatesView : public vtkRenderView
{
public:
  vtkTypeRevisionMacro(vtkParallelCoordinatesView, vtkRenderView);
  static vtkParallelCoordinatesView *New();
  void PrintSelf(ostream &os, vtkIndent indent);

  //BTX
  enum {VTK_BRUSH_LASSO=0,VTK_BRUSH_ANGLE,VTK_BRUSH_FUNCTION,VTK_BRUSH_AXISTHRESHOLD,VTK_BRUSH_MODECOUNT};
  enum {VTK_BRUSHOPERATOR_ADD=0,VTK_BRUSHOPERATOR_SUBTRACT,VTK_BRUSHOPERATOR_INTERSECT,VTK_BRUSHOPERATOR_REPLACE,VTK_BRUSHOPERATOR_MODECOUNT};
  enum {VTK_INSPECT_MANIPULATE_AXES=0, VTK_INSPECT_SELECT_DATA, VTK_INSPECT_MODECOUNT};
  //ETX

  void SetBrushMode(int);
  void SetBrushModeToLasso() { this->SetBrushMode(VTK_BRUSH_LASSO); }
  void SetBrushModeToAngle() { this->SetBrushMode(VTK_BRUSH_ANGLE); }
  void SetBrushModeToFunction() { this->SetBrushMode(VTK_BRUSH_FUNCTION); }
  void SetBrushModeToAxisThreshold() { this->SetBrushMode(VTK_BRUSH_AXISTHRESHOLD); }
  vtkGetMacro(BrushMode,int);
  
  void SetBrushOperator(int);
  void SetBrushOperatorToAdd() { this->SetBrushOperator(VTK_BRUSHOPERATOR_ADD); }
  void SetBrushOperatorToSubtract() { this->SetBrushOperator(VTK_BRUSHOPERATOR_SUBTRACT); }
  void SetBrushOperatorToIntersect() { this->SetBrushOperator(VTK_BRUSHOPERATOR_INTERSECT); }
  void SetBrushOperatorToReplace() { this->SetBrushOperator(VTK_BRUSHOPERATOR_REPLACE); }
  vtkGetMacro(BrushOperator,int);

  void SetInspectMode(int);
  void SetInspectModeToManipulateAxes() { this->SetInspectMode(VTK_INSPECT_MANIPULATE_AXES); }
  void SetInpsectModeToSelectData() { this->SetInspectMode(VTK_INSPECT_SELECT_DATA); }
  vtkGetMacro(InspectMode,int);
  
  void SetMaximumNumberOfBrushPoints(int);
  vtkGetMacro(MaximumNumberOfBrushPoints,int);

  vtkSetMacro(CurrentBrushClass,int);
  vtkGetMacro(CurrentBrushClass,int);

  virtual void ApplyViewTheme(vtkViewTheme* theme);

protected:
  vtkParallelCoordinatesView();
  virtual ~vtkParallelCoordinatesView();

  int SelectedAxisPosition;
  //BTX
  enum {VTK_HIGHLIGHT_CENTER=0,VTK_HIGHLIGHT_MIN,VTK_HIGHLIGHT_MAX};
  vtkSmartPointer<vtkOutlineSource> HighlightSource;
  vtkSmartPointer<vtkPolyDataMapper2D> HighlightMapper;
  vtkSmartPointer<vtkActor2D> HighlightActor;
  //ETX

  int InspectMode;
  int BrushMode;
  int BrushOperator;
  int MaximumNumberOfBrushPoints;
  int NumberOfBrushPoints;
  int CurrentBrushClass;
  //BTX
  vtkSmartPointer<vtkPolyData> BrushData;
  vtkSmartPointer<vtkPolyDataMapper2D> BrushMapper;
  vtkSmartPointer<vtkActor2D> BrushActor;
  //ETX

  int FirstFunctionBrushLineDrawn;
  int AxisHighlightPosition;

  vtkTimeStamp WorldBuildTime;
  bool RebuildNeeded;

  virtual void ProcessEvents(vtkObject *caller, unsigned long event, void *callData);
  virtual vtkDataRepresentation* CreateDefaultRepresentation(vtkAlgorithmOutput* conn);

  void PrepareForRendering();

  // Description: 
  // Handle axis manipulation
  void Hover(unsigned long event);
  void ManipulateAxes(unsigned long event);
  void SelectData(unsigned long event);
  void Zoom(unsigned long event);
  void Pan(unsigned long event);

  // Description:
  // Set/Get the position of axis highlights
  int SetAxisHighlightPosition(vtkParallelCoordinatesRepresentation* rep, int position);

  // Description:
  // Set the highlight position using normalized viewport coordinates
  int SetAxisHighlightPosition(vtkParallelCoordinatesRepresentation* rep, double position);

  int AddLassoBrushPoint(double *p);
  int SetBrushLine(int line, double *p1, double *p2);
  void GetBrushLine(int line, vtkIdType &npts, vtkIdType* &ptids);
  int SetAngleBrushLine(double *p1, double *p2);
  int SetFunctionBrushLine1(double *p1, double *p2);
  int SetFunctionBrushLine2(double *p1, double *p2);
  void ClearBrushPoints();


private:
  vtkParallelCoordinatesView(const vtkParallelCoordinatesView&); // Not implemented
  void operator=(const vtkParallelCoordinatesView&); // Not implemented

};

#endif
