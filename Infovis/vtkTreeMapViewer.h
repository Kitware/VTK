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
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/
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

class VTK_INFOVIS_EXPORT vtkTreeMapViewer : public vtkObject 
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
  
  // Description:
  // Set the aggregration field (defaults to "size")
  virtual void SetAggregationFieldName(const char *field);
  
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

  // Description:
  // These convenience functions use strings for use in GUIs and scripts.
  virtual void SetLayoutStrategy(const char *layoutType);
  static const char *GetLayoutStrategyName(int strategy);
  
  // Description:
  // The name of the field used for coloring the data
  virtual void SetColorFieldName(const char *field);
  
  // Description:
  // The name of the field used for labeling
  virtual void SetLabelFieldName(const char *field);
  
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

  void SetLabelLevelRange(int start, int end);
  void SetDynamicLabelLevel(int level);
  void SetChildLabelMotion(int mode);
  void SetLabelClipMode(int mode);

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


