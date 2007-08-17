/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTreeMapView.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/
// .NAME vtkTreeMapView - Displays a tree as a tree map.
//
// .SECTION Description
// vtkTreeMapView shows a vtkTree in a tree map, where each vertex in the
// tree is represented by a box.  Child boxes are contained within the
// parent box, and may be colored and sized by various parameters.

#ifndef __vtkTreeMapView_h
#define __vtkTreeMapView_h

#include "vtkRenderView.h"

class vtkActor;
class vtkActor2D;
class vtkAlgorithmOutput;
class vtkBoxLayoutStrategy;
class vtkLabeledTreeMapDataMapper;
class vtkLookupTable;
class vtkPolyDataMapper;
class vtkRenderWindow;
class vtkSliceAndDiceLayoutStrategy;
class vtkSquarifyLayoutStrategy;
class vtkTreeFieldAggregator;
class vtkTreeLevelsFilter;
class vtkTreeMapLayout;
class vtkTreeMapToPolyData;

class VTK_VIEWS_EXPORT vtkTreeMapView : public vtkRenderView
{
public:
  static vtkTreeMapView *New();
  vtkTypeRevisionMacro(vtkTreeMapView, vtkRenderView);
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
  void SetLayoutStrategyToBox();
  void SetLayoutStrategyToSliceAndDice();
  void SetLayoutStrategyToSquarify();
  
  // Description:
  // Sets the amount of border around child rectangles.
  // The percentage should be between 0 and 1.
  void SetBorderPercentage(double pcent);
  double GetBorderPercentage();
  
  // Description:
  // The sizes of the fonts used for labeling.
  void SetFontSizeRange(const int maxSize, const int minSize, const int delta=4);
  void GetFontSizeRange(int range[3]);
  
  // Description:
  // Sets up interactor style.
  virtual void SetupRenderWindow(vtkRenderWindow* win);
  
  // Description:
  // Apply the theme to this view.
  virtual void ApplyViewTheme(vtkViewTheme* theme);

protected:
  vtkTreeMapView();
  ~vtkTreeMapView();
  
  // Description:
  // Connects the algorithm output to the internal pipeline.
  virtual void AddInputConnection(vtkAlgorithmOutput* conn);
  
  // Description:
  // Disconnects the algorithm output from the internal pipeline.
  virtual void RemoveInputConnection(vtkAlgorithmOutput* conn);
  
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
  vtkTreeMapLayout*                         TreeMapLayout;
  vtkBoxLayoutStrategy*                     BoxLayout;
  vtkSliceAndDiceLayoutStrategy*            SliceAndDiceLayout;
  vtkSquarifyLayoutStrategy*                SquarifyLayout;
  vtkTreeMapToPolyData*                     TreeMapToPolyData;
  vtkPolyDataMapper*                        TreeMapMapper;
  vtkActor*                                 TreeMapActor;
  vtkLabeledTreeMapDataMapper*              LabelMapper;
  vtkActor2D*                               LabelActor;
  vtkLookupTable*                           ColorLUT;

private:
  vtkTreeMapView(const vtkTreeMapView&);  // Not implemented.
  void operator=(const vtkTreeMapView&);  // Not implemented.
};

#endif
