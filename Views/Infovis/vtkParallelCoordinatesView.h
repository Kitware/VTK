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
/**
 * @class   vtkParallelCoordinatesView
 * @brief   view to be used with vtkParallelCoordinatesRepresentation
 *
 *
 *
 * This class manages interaction with the vtkParallelCoordinatesRepresentation.  There are
 * two inspection modes: axis manipulation and line selection.  In axis manipulation mode,
 * PC axes can be dragged and reordered with the LMB, axis ranges can be increased/decreased
 * by dragging up/down with the LMB, and RMB controls zoom and pan.
 *
 * In line selection mode, there are three subclasses of selections: lasso, angle, and
 * function selection.  Lasso selection lets the user brush a line and select all PC lines
 * that pass nearby.  Angle selection lets the user draw a representative line between axes
 * and select all lines that have similar orientation.  Function selection lets the user
 * draw two  representative lines between a pair of axes and select all lines that match
 * the linear interpolation of those lines.
 *
 * There are several self-explanatory operators for combining selections: ADD, SUBTRACT
 * REPLACE, and INTERSECT.
*/

#ifndef vtkParallelCoordinatesView_h
#define vtkParallelCoordinatesView_h

#include "vtkViewsInfovisModule.h" // For export macro
#include "vtkRenderView.h"

class vtkActor2D;
class vtkOutlineSource;
class vtkParallelCoordinatesRepresentation;
class vtkPolyData;
class vtkPolyDataMapper2D;

class VTKVIEWSINFOVIS_EXPORT vtkParallelCoordinatesView : public vtkRenderView
{
public:
  vtkTypeMacro(vtkParallelCoordinatesView, vtkRenderView);
  static vtkParallelCoordinatesView *New();
  void PrintSelf(ostream &os, vtkIndent indent);

  enum {VTK_BRUSH_LASSO=0,VTK_BRUSH_ANGLE,VTK_BRUSH_FUNCTION,VTK_BRUSH_AXISTHRESHOLD,VTK_BRUSH_MODECOUNT};
  enum {VTK_BRUSHOPERATOR_ADD=0,VTK_BRUSHOPERATOR_SUBTRACT,VTK_BRUSHOPERATOR_INTERSECT,VTK_BRUSHOPERATOR_REPLACE,VTK_BRUSHOPERATOR_MODECOUNT};
  enum {VTK_INSPECT_MANIPULATE_AXES=0, VTK_INSPECT_SELECT_DATA, VTK_INSPECT_MODECOUNT};

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

  enum {VTK_HIGHLIGHT_CENTER=0,VTK_HIGHLIGHT_MIN,VTK_HIGHLIGHT_MAX};
  vtkSmartPointer<vtkOutlineSource> HighlightSource;
  vtkSmartPointer<vtkPolyDataMapper2D> HighlightMapper;
  vtkSmartPointer<vtkActor2D> HighlightActor;

  int InspectMode;
  int BrushMode;
  int BrushOperator;
  int MaximumNumberOfBrushPoints;
  int NumberOfBrushPoints;
  int CurrentBrushClass;

  vtkSmartPointer<vtkPolyData> BrushData;
  vtkSmartPointer<vtkPolyDataMapper2D> BrushMapper;
  vtkSmartPointer<vtkActor2D> BrushActor;

  int FirstFunctionBrushLineDrawn;
  int AxisHighlightPosition;

  vtkTimeStamp WorldBuildTime;
  bool RebuildNeeded;

  virtual void ProcessEvents(vtkObject *caller, unsigned long event, void *callData);
  virtual vtkDataRepresentation* CreateDefaultRepresentation(vtkAlgorithmOutput* conn);

  void PrepareForRendering();

  //@{
  /**
   * Handle axis manipulation
   */
  void Hover(unsigned long event);
  void ManipulateAxes(unsigned long event);
  void SelectData(unsigned long event);
  void Zoom(unsigned long event);
  void Pan(unsigned long event);
  //@}

  /**
   * Set/Get the position of axis highlights
   */
  int SetAxisHighlightPosition(vtkParallelCoordinatesRepresentation* rep, int position);

  /**
   * Set the highlight position using normalized viewport coordinates
   */
  int SetAxisHighlightPosition(vtkParallelCoordinatesRepresentation* rep, double position);

  int AddLassoBrushPoint(double *p);
  int SetBrushLine(int line, double *p1, double *p2);
  void GetBrushLine(int line, vtkIdType &npts, vtkIdType* &ptids);
  int SetAngleBrushLine(double *p1, double *p2);
  int SetFunctionBrushLine1(double *p1, double *p2);
  int SetFunctionBrushLine2(double *p1, double *p2);
  void ClearBrushPoints();


private:
  vtkParallelCoordinatesView(const vtkParallelCoordinatesView&) VTK_DELETE_FUNCTION;
  void operator=(const vtkParallelCoordinatesView&) VTK_DELETE_FUNCTION;

};

#endif
