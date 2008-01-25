/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGraphLayoutViewer.h

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
// .NAME vtkGraphLayoutViewer - Layout and display of a vtkGraph.
//
// .SECTION Description
// vtkGraphLayoutViewer is a convenience class for displaying a vtkGraph.  It
// packages up the functionality found in vtkRenderWindow, vtkRenderer,
// and vtkActor into a single easy to use class.  This class also creates 
// an image interactor style(vtkInteractorStyleImage) that allows zooming 
// and panning of the laid out graph.
// 
// .SECTION Notes
// Because the labeller likes to complain quite a bit, labels are defaulted
// to OFF. Also you should set all the other parameters first and then
// call SetLabelsOn() if you want labels.
//
// .SECTION See Also
// vtkTreeMapViewer
//
// .SECTION Thanks
// Thanks to Brian Wylie from Sandia National Laboratories for conceptualizing
// and implementing this class.

#ifndef __vtkGraphLayoutViewer_h
#define __vtkGraphLayoutViewer_h

#include "vtkObject.h"
#include "vtkSmartPointer.h"    // Required for smart pointer internal ivars.

class vtkGraph;
class vtkGraphLayout;
class vtkGraphLayoutStrategy;
class vtkGraphToPolyData;
class vtkTreeLevelsFilter;
class vtkPolyDataMapper;
class vtkAlgorithmOutput;
class vtkActor;
class vtkActor2D;
class vtkRenderWindowInteractor;
class vtkInteractorStyleImage;
class vtkRenderWindow;
class vtkRenderer;
class vtkRenderWindowInteractor;
class vtkLookupTable;
class vtkLabeledDataMapper;
class vtkEventForwarderCommand;
class vtkSphereSource;
class vtkGlyph3D;

class VTK_INFOVIS_EXPORT vtkGraphLayoutViewer : public vtkObject 
{
public:
  static vtkGraphLayoutViewer *New();
  vtkTypeRevisionMacro(vtkGraphLayoutViewer,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Set the input data to the viewer.
  virtual void SetInput(vtkGraph *arg);

  // Description:
  // Set your own renderwindow
  virtual void SetRenderWindow(vtkRenderWindow *arg);
  vtkGetObjectMacro(RenderWindow, vtkRenderWindow);

  // Description:
  // Set layout strategy for the tree map
  virtual void SetLayoutStrategy(const char *strategyName);
  virtual const char* GetLayoutStrategy();
  
  // Description:
  // Is the graph layout complete? This method is useful
  // for when the strategy is iterative and the application
  // wants to show the iterative progress of the graph layout
  // See Also: UpdateLayout();
  virtual int IsLayoutComplete();
  
  // Description:
  // This method is useful for when the strategy is iterative 
  // and the application wants to show the iterative progress 
  // of the graph layout. The application would have something like
  // while(!IsLayoutComplete())
  //   {
  //   UpdateLayout();
  //   }
  // See Also: IsLayoutComplete();
  virtual void UpdateLayout();
  
  // Description:
  // The name of the vertex field used for coloring the vertices
  virtual void SetVertexColorFieldName(const char *field);
  virtual char* GetVertexColorFieldName();
  
  // Description:
  // The name of the edge field used for coloring the edges
  virtual void SetEdgeColorFieldName(const char *field);
  virtual char* GetEdgeColorFieldName();
  
  // Description:
  // The name of the field used for labeling
  virtual void SetLabelFieldName(const char *field);
  virtual char* GetLabelFieldName();
  
  // Description:
  // These methods turn labeling on or off : defaulted to off
  virtual void SetLabelsOn();
  virtual void SetLabelsOff();
  
  // Description:
  // The size of the font used for labeling
  virtual void SetFontSize(const int size);
  virtual int GetFontSize();
  
  // Description:
  // Set/Get whether the layout is shown iteratively or not
  vtkSetMacro(Iterative, bool);
  vtkGetMacro(Iterative, bool);
  
  // Description:
  // Set/Get the field to use for the edge weights.
  vtkSetStringMacro(EdgeWeightField);
  vtkGetStringMacro(EdgeWeightField);
  
  // Description:
  // Get the graph output of the layout filter
  // Note: This function may return NULL if no
  // layout strategy is registered with this class
  vtkGraph* GetGraphAfterLayout();

protected:
  vtkGraphLayoutViewer();
  ~vtkGraphLayoutViewer();
  
  vtkGraph*                         Input;
  vtkRenderWindow*                          RenderWindow;
  vtkGraphLayoutStrategy*                   GraphLayoutStrategy;
  //BTX
  vtkSmartPointer<vtkGraphLayout>           GraphLayout;
  vtkSmartPointer<vtkGraphToPolyData>       GraphToPolyData;
  vtkSmartPointer<vtkSphereSource>          SphereSource;
  vtkSmartPointer<vtkGlyph3D>               VertexGlyphs;
  vtkSmartPointer<vtkInteractorStyleImage>  InteractorStyle;
  vtkSmartPointer<vtkPolyDataMapper>        GlyphMapper;
  vtkSmartPointer<vtkPolyDataMapper>        EdgeMapper;
  vtkSmartPointer<vtkRenderer>              Renderer;
  vtkSmartPointer<vtkActor>                 VertexActor;
  vtkSmartPointer<vtkActor>                 EdgeActor;
  vtkSmartPointer<vtkActor2D>               LabelActor;
  vtkSmartPointer<vtkLookupTable>           EdgeColorLUT;
  vtkSmartPointer<vtkLookupTable>           GlyphColorLUT;
  vtkSmartPointer<vtkLabeledDataMapper>     LabeledDataMapper;
  //ETX
  
  // Description:
  // This intercepts events from the graph layout class 
  // and re-emits them as if they came from this class.
  vtkEventForwarderCommand *EventForwarder;
  unsigned long ObserverTag;
  
private:

  // Internally used methods
  
  // Description:
  // Setup the internal pipeline for the graph layout view
  virtual void SetupPipeline();
  
  // Description:
  // When the input is set with SetInput() there some
  // initialization to do for the internal pipeline
  void InputInitialize();
  
  // Description:
  // Controls whether the layout is shown iteratively or not
  bool Iterative;
  
  // Description:
  // The field to use for the edge weights
  char*  EdgeWeightField;
  
  vtkGraphLayoutViewer(const vtkGraphLayoutViewer&);  // Not implemented.
  void operator=(const vtkGraphLayoutViewer&);  // Not implemented.
};

#endif


