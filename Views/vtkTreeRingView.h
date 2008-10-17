/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTreeRingView.h

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
// .NAME vtkTreeRingView - Displays a tree as a radial space filling tree.
//
// .SECTION Description
// vtkTreeRingView shows a vtkTree in a radial space filling (RSF) tree,
// where each vertex in the tree is represented by a sector of a set of
// circles.  Child sectors are connected to parent sectors, and may be
// colored and sized by various parameters.

#ifndef __vtkTreeRingView_h
#define __vtkTreeRingView_h

#include "vtkRenderView.h"

class vtkActor;
class vtkActor2D;
class vtkAlgorithmOutput;
class vtkDynamic2DLabelMapper;
class vtkLookupTable;
class vtkPolyDataMapper;
class vtkRenderWindow;
class vtkTreeFieldAggregator;
class vtkTreeLevelsFilter;
class vtkTreeRingLayout;
class vtkTreeRingDefaultLayoutStrategy;
class vtkTreeRingReversedLayoutStrategy;
class vtkTreeRingToPolyData;

class VTK_VIEWS_EXPORT vtkTreeRingView : public vtkRenderView
{
public:
  static vtkTreeRingView *New();
  vtkTypeRevisionMacro(vtkTreeRingView, vtkRenderView);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // The name of the array used to color the treemap.
  void SetColorArrayName(const char* name);
  const char* GetColorArrayName();
  
  // Description:
  // The name of the array used to size the treemap rectangles.
  void SetSizeArrayName(const char* name);
  const char* GetSizeArrayName();
  
  // Description:
  // The name of the array used to label the treemap.
  // This must be a string array.
  void SetLabelArrayName(const char* name);
  const char* GetLabelArrayName();
  
  // Description:
  // The name of the array whose value appears when the mouse hovers
  // over a rectangle in the treemap.
  // This must be a string array.
  void SetHoverArrayName(const char* name);
  const char* GetHoverArrayName();
  
  // Description:
  // Sets the treemap layout strategy
  void SetLayoutStrategy(const char* name);
  void SetLayoutStrategyToDefault();
  void SetLayoutStrategyToReversed();
 
//   // Description:
//   // The sizes of the fonts used for labeling.
//   void SetFontSizeRange(const int maxSize, const int minSize, const int delta=4);
//   void GetFontSizeRange(int range[3]);

  // Description:
  // Get/Set the shrinkage percentage for drawing each of the sectors
  void SetSectorShrinkPercentage( double shrinkFactor );
  double GetSectorShrinkPercentage();

  // Description:
  // Sets up interactor style.
  virtual void SetupRenderWindow(vtkRenderWindow* win);
  
  // Description:
  // Apply the theme to this view.
  virtual void ApplyViewTheme(vtkViewTheme* theme);

protected:
  vtkTreeRingView();
  ~vtkTreeRingView();
  
  // Description:
  // Connects the algorithm output to the internal pipeline.
  virtual void AddInputConnection( int port, int item,
    vtkAlgorithmOutput* conn,
    vtkAlgorithmOutput* selectionConn);
  
  // Description:
  // Disconnects the algorithm output from the internal pipeline.
  virtual void RemoveInputConnection( int port, int item,
    vtkAlgorithmOutput* conn,
    vtkAlgorithmOutput* selectionConn);
  
  // Description:
  // Called to process the user event from the interactor style.
  virtual void ProcessEvents(vtkObject* caller, unsigned long eventId, 
    void* callData);
  
  // Decsription:
  // Prepares the view for rendering.
  virtual void PrepareForRendering();

  // Description:
  // Gets the internal color array name.
  vtkSetStringMacro(ColorArrayNameInternal);
  vtkGetStringMacro(ColorArrayNameInternal);
    
  // Representation objects
  char* ColorArrayNameInternal;
  vtkTreeLevelsFilter*                      TreeLevelsFilter;
  vtkTreeFieldAggregator*                   TreeFieldAggregator;
  vtkTreeRingLayout*                        TreeRingLayout;
  vtkTreeRingDefaultLayoutStrategy*         TreeRingDefaultLayout;
  vtkTreeRingReversedLayoutStrategy*        TreeRingReversedLayout;
  vtkTreeRingToPolyData*                    TreeRingToPolyData;
  vtkPolyDataMapper*                        TreeRingMapper;
  vtkActor*                                 TreeRingActor;
  vtkDynamic2DLabelMapper*                  LabelMapper;
  vtkActor2D*                               LabelActor;
  vtkLookupTable*                           ColorLUT;

private:
  vtkTreeRingView(const vtkTreeRingView&);  // Not implemented.
  void operator=(const vtkTreeRingView&);  // Not implemented.
};

#endif
