/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTreeRingViewer.h

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
// .NAME vtkTreeRingViewer - Display a 2D TreeRing.
//
// .SECTION Description
// vtkTreeRingViewer is a convenience class for displaying a 2D TreeRing.  It
// packages up the functionality found in vtkRenderWindow, vtkRenderer,
// and vtkActor into a single easy to use class.  This class also creates 
// an image interactor style(vtkInteractorStyleImage) that allows zooming 
// and panning of the tree map.
//
// .SECTION See Also
// vtkGraphLayoutViewer
//
// .SECTION Thanks
// Thanks to Jason Shepherd from Sandia National Laboratories for 
// implementing this class.

#ifndef __vtkTreeRingViewer_h
#define __vtkTreeRingViewer_h

#include "vtkObject.h"
#include "vtkSmartPointer.h"    // Required for smart pointer internal ivars.

class vtkTree;
class vtkTreeFieldAggregator;
class vtkTreeRingLayout;
class vtkTreeRingLayoutStrategy;
class vtkTreeRingToPolyData;
class vtkPolyDataMapper;
class vtkAlgorithmOutput;
class vtkActor;
class vtkActor2D;
class vtkRenderWindowInteractor;
class vtkInteractorStyleTreeRingHover;
class vtkRenderWindow;
class vtkRenderer;
class vtkRenderWindowInteractor;
class vtkLookupTable;
class vtkThreshold;
//class vtkLabeledDataMapper;
//class vtkLabeledTreeRingDataMapper;
class vtkDynamic2DLabelMapper;

class VTK_INFOVIS_EXPORT vtkTreeRingViewer : public vtkObject 
{
public:
  static vtkTreeRingViewer *New();
  vtkTypeRevisionMacro(vtkTreeRingViewer,vtkObject);
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
  vtkGetObjectMacro(InteractorStyle, vtkInteractorStyleTreeRingHover);

//BTX
  enum {
    TREE_RING_DEFAULT_LAYOUT,
    TREE_RING_REVERSED_LAYOUT,
    NUMBER_OF_LAYOUTS
  };
//ETX

  // Description:
  // Set layout strategy for the tree map
  virtual void SetLayoutStrategy(int strategy);
  void SetLayoutStrategyToDefault() { 
    this->SetLayoutStrategy(TREE_RING_DEFAULT_LAYOUT); }
  void SetLayoutStrategyToReversed() {
    this->SetLayoutStrategy(TREE_RING_REVERSED_LAYOUT); }
  virtual int GetLayoutStrategy();

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
//  virtual void SetFontSizeRange(const int maxSize, const int minSize);

  // Description:
  // Set whether the tree uses a logarithmic scaling of sizes.
  bool GetLogScale();
  void SetLogScale(bool value);
  
  // Description:
  // Highlight the tree item that matches the pedigree id
  void HighLightItem(vtkIdType id);

  // Description:
  // Get/Set whether the label may be moved by its ancestors.
//   void SetChildLabelMotion(int mode);
//   int GetChildLabelMotion();

  // Description:
  // Get/Set if the label can be displayed clipped by the window.
  // 0 - ok to clip labels
  // 1 - auto center labels w/r to the area of the vertex's clipped
  // region
//   void SetLabelClipMode(int mode);
//   int GetLabelClipMode();

  // Description:
  // Get/Set the shrinkage percentage for drawing each of the sectors
  void SetSectorShrinkPercentage( double shrinkFactor );
  double GetSectorShrinkPercentage();
  
protected:
  vtkTreeRingViewer();
  ~vtkTreeRingViewer();
  
  // Protected methods that may be called by inherited classes
  
  // Description:
  // Setup the internal pipeline for the tree map view
  virtual void SetupPipeline();
  
  vtkTree*                                  Input;
  vtkRenderWindow*                          RenderWindow;
  vtkInteractorStyleTreeRingHover*          InteractorStyle;
  //BTX
  vtkSmartPointer<vtkTreeFieldAggregator>   TreeFieldAggregator;
  vtkSmartPointer<vtkTreeRingLayout>        TreeRingLayout;
  vtkSmartPointer<vtkTreeRingToPolyData>    TreeRingToPolyData;
  vtkSmartPointer<vtkPolyDataMapper>        PolyDataMapper;
  vtkSmartPointer<vtkRenderer>              Renderer;
  vtkSmartPointer<vtkActor>                 Actor;
  vtkSmartPointer<vtkActor2D>               LabelActor;
  vtkSmartPointer<vtkLookupTable>           ColorLUT;
  vtkSmartPointer<vtkDynamic2DLabelMapper>  LabelMapper;
  //ETX
  
private:

  // Internally used methods
  
  // Description:
  // When the input is set with SetInput() there some
  // initialization to do for the internal pipeline
  void InputInitialize();

  vtkTreeRingViewer(const vtkTreeRingViewer&);  // Not implemented.
  void operator=(const vtkTreeRingViewer&);  // Not implemented.
};

#endif


