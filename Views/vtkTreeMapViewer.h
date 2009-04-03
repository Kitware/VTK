/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTreeMapViewer.h

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
// .NAME vtkTreeMapViewer - Display a 2D TreeMap.
//
// .SECTION Description
// vtkTreeMapViewer is a convenience class for displaying a 2D TreeMap.  It
// packages up the functionality found in vtkRenderWindow, vtkRenderer,
// and vtkActor into a single easy to use class.  This class also creates 
// an image interactor style(vtkInteractorStyleImage) that allows zooming 
// and panning of the tree map.
//
// .SECTION See Also
// vtkGraphLayoutViewer
//
// .SECTION Thanks
// Thanks to Brian Wylie from Sandia National Laboratories for conceptualizing
// and implementing this class.

#ifndef __vtkTreeMapViewer_h
#define __vtkTreeMapViewer_h

#include "vtkObject.h"
#include "vtkSmartPointer.h"    // Required for smart pointer internal ivars.

class vtkTree;
class vtkTreeLevelsFilter;
class vtkTreeFieldAggregator;
class vtkTreeMapLayout;
class vtkTreeMapLayoutStrategy;
class vtkTreeMapToPolyData;
class vtkPolyDataMapper;
class vtkAlgorithmOutput;
class vtkActor;
class vtkActor2D;
class vtkRenderWindowInteractor;
class vtkInteractorStyleTreeMapHover;
class vtkRenderWindow;
class vtkRenderer;
class vtkRenderWindowInteractor;
class vtkLookupTable;
class vtkThreshold;
class vtkLabeledDataMapper;
class vtkLabeledTreeMapDataMapper;

class VTK_VIEWS_EXPORT vtkTreeMapViewer : public vtkObject 
{
public:
  static vtkTreeMapViewer *New();
  vtkTypeRevisionMacro(vtkTreeMapViewer,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Set the input data to the viewer.
  virtual void SetInput(vtkTree *arg);

  // Description:
  // Set your own renderwindow
  virtual void SetRenderWindow(vtkRenderWindow *arg);
  vtkGetObjectMacro(RenderWindow, vtkRenderWindow);
  
  // Description:
  // Set the aggregration field (defaults to "size")
  virtual void SetAggregationFieldName(const char *field);
  virtual char* GetAggregationFieldName();
  
  // Description:
  // Get the Interactor Style object pointer
  vtkGetObjectMacro(InteractorStyle, vtkInteractorStyleTreeMapHover);


//BTX
  enum {
    BOX_LAYOUT,
    SLICE_AND_DICE_LAYOUT,
    SQUARIFY_LAYOUT,
    NUMBER_OF_LAYOUTS
  };
//ETX

  // Description:
  // Set layout strategy for the tree map
  virtual void SetLayoutStrategy(int strategy);
  void SetLayoutStrategyToBox() { this->SetLayoutStrategy(BOX_LAYOUT); }
  void SetLayoutStrategyToSliceAndDice() {
    this->SetLayoutStrategy(SLICE_AND_DICE_LAYOUT);
  }
  void SetLayoutStrategyToSquarify() {
    this->SetLayoutStrategy(SQUARIFY_LAYOUT);
  }
  virtual int GetLayoutStrategy();

  // Description:
  // Get/Set the fraction of the treemap box to use in the border.
  void SetBorderPercentage(double pcent);
  double GetBorderPercentage();

  // Description:
  // These convenience functions use strings for use in GUIs and scripts.
  virtual void SetLayoutStrategy(const char *layoutType);
  static const char *GetLayoutStrategyName(int strategy);
  
  // Description:
  // The name of the field used for coloring the data
  virtual void SetColorFieldName(const char *field);
  virtual char* GetColorFieldName();
  
  // Description:
  // The name of the field used for labeling
  virtual void SetLabelFieldName(const char *field);
  virtual char* GetLabelFieldName();
  
  // Description:
  // The sizes of the fonts used for labeling
  virtual void SetFontSizeRange(const int maxSize, const int minSize);

  // Description:
  // Set whether the tree map uses a logarithmic scaling of sizes.
  bool GetLogScale();
  void SetLogScale(bool value);
  
  // Description:
  // Highlight the tree item that matches the pedigree id
  void HighLightItem(vtkIdType id);

  // Description:
  // Get/Set the range of levels to attempt to label.
  // The level of a vertex is the length of the path to the root
  // (the root has level 0).
  void SetLabelLevelRange(int start, int end);
  void GetLabelLevelRange(int range[2]);

  // Description:
  // Get/Set the level at which treemap labeling is dynamic.
  void SetDynamicLabelLevel(int level);
  int GetDynamicLabelLevel();

  // Description:
  // Get/Set whether the label may be moved by its ancestors.
  void SetChildLabelMotion(int mode);
  int GetChildLabelMotion();

  // Description:
  // Get/Set if the label can be displayed clipped by the window.
  // 0 - ok to clip labels
  // 1 - auto center labels w/r to the area of the vertex's clipped
  // region
  void SetLabelClipMode(int mode);
  int GetLabelClipMode();

protected:
  vtkTreeMapViewer();
  ~vtkTreeMapViewer();
  
  // Protected methods that may be called by inherited classes
  
  // Description:
  // Setup the internal pipeline for the tree map view
  virtual void SetupPipeline();
  
  vtkTree*                                  Input;
  vtkRenderWindow*                          RenderWindow;
  vtkInteractorStyleTreeMapHover*           InteractorStyle;
  //BTX
  vtkSmartPointer<vtkTreeLevelsFilter>      TreeLevelsFilter;
  vtkSmartPointer<vtkTreeFieldAggregator>   TreeFieldAggregator;
  vtkSmartPointer<vtkTreeMapLayout>         TreeMapLayout;
  vtkSmartPointer<vtkTreeMapToPolyData>     TreeMapToPolyData;
  vtkSmartPointer<vtkPolyDataMapper>        PolyDataMapper;
  vtkSmartPointer<vtkRenderer>              Renderer;
  vtkSmartPointer<vtkActor>                 Actor;
  vtkSmartPointer<vtkActor2D>               LabelActor;
  vtkSmartPointer<vtkLookupTable>           ColorLUT;
  vtkSmartPointer<vtkLabeledTreeMapDataMapper>     LabeledDataMapper;
  //ETX
  
private:

  // Internally used methods
  
  // Description:
  // When the input is set with SetInput() there some
  // initialization to do for the internal pipeline
  void InputInitialize();

  vtkTreeMapViewer(const vtkTreeMapViewer&);  // Not implemented.
  void operator=(const vtkTreeMapViewer&);  // Not implemented.
};

#endif


