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
// .NAME vtkGraphLayoutViewer - Display a 2D TreeMap.
//
// .SECTION Description
// vtkGraphLayoutViewer is a convenience class for displaying a vtkGraph.  It
// packages up the functionality found in vtkRenderWindow, vtkRenderer,
// and vtkActor into a single easy to use class.  This class also creates 
// an image interactor style(vtkInteractorStyleImage) that allows zooming 
// and panning of the laid out graph.
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

class vtkAbstractGraph;
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

class VTK_INFOVIS_EXPORT vtkGraphLayoutViewer : public vtkObject 
{
public:
  static vtkGraphLayoutViewer *New();
  vtkTypeRevisionMacro(vtkGraphLayoutViewer,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Set the input data to the viewer.
  virtual void SetInput(vtkAbstractGraph *arg);

  // Description:
  // Set your own renderwindow
  virtual void SetRenderWindow(vtkRenderWindow *arg);

  // Description:
  // Set layout strategy for the tree map
  virtual void SetLayoutStrategy(const char *strategyName);
  
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
  // The name of the field used for coloring the data
  virtual void SetColorFieldName(const char *field);
  
  // Description:
  // The name of the field used for labeling
  virtual void SetLabelFieldName(const char *field);
  
  // Description:
  // These methods turn labeling on or off
  virtual void SetLabelsOn();
  virtual void SetLabelsOff();
  
  // Description:
  // The size of the font used for labeling
  virtual void SetFontSize(const int size);

protected:
  vtkGraphLayoutViewer();
  ~vtkGraphLayoutViewer();
  
  vtkAbstractGraph*                         Input;
  vtkRenderWindow*                          RenderWindow;
  vtkGraphLayoutStrategy*                   GraphLayoutStrategy;
  //BTX
  vtkSmartPointer<vtkGraphLayout>           GraphLayout;
  vtkSmartPointer<vtkGraphToPolyData>       GraphToPolyData;
  vtkSmartPointer<vtkInteractorStyleImage>  InteractorStyle;
  vtkSmartPointer<vtkPolyDataMapper>        PolyDataMapper;
  vtkSmartPointer<vtkRenderer>              Renderer;
  vtkSmartPointer<vtkActor>                 Actor;
  vtkSmartPointer<vtkActor2D>               LabelActor;
  vtkSmartPointer<vtkLookupTable>           ColorLUT;
  vtkSmartPointer<vtkLabeledDataMapper>     LabeledDataMapper;
  //ETX
  
private:

  // Internally used methods
  
  // Description:
  // Setup the internal pipeline for the graph layout view
  virtual void SetupPipeline();

  bool PipelineInstalled;
  
  // Description:
  // When the input is set with SetInput() there some
  // initialization to do for the internal pipeline
  void InputInitialize();
  
  vtkGraphLayoutViewer(const vtkGraphLayoutViewer&);  // Not implemented.
  void operator=(const vtkGraphLayoutViewer&);  // Not implemented.
};

#endif


