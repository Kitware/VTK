/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRenderView.h

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
// .NAME vtkRenderView - A view containing a renderer.
//
// .SECTION Description
// vtkRenderView is a view which contains a vtkRenderer.  You may add vtkActors
// directly to the renderer, or add certain vtkDataRepresentation subclasses
// to the renderer.  The render view supports drag selection with the mouse to
// select cells.
//
// This class is also the parent class for any more specialized view which uses
// a renderer.

#ifndef __vtkRenderView_h
#define __vtkRenderView_h

#include "vtkView.h"
#include "vtkSmartPointer.h" // For SP ivars

class vtkAbstractTransform;
class vtkActor2D;
class vtkAlgorithmOutput;
class vtkAppendPoints;
class vtkArrayCalculator;
class vtkBalloonRepresentation;
class vtkDynamic2DLabelMapper;
class vtkIconGlyphFilter;
class vtkInteractorStyle;
class vtkLabeledDataMapper;
class vtkLabelPlacer;
class vtkLabelSizeCalculator;
class vtkPointSetToLabelHierarchy;
class vtkPolyDataMapper2D;
class vtkRenderer;
class vtkRenderWindow;
class vtkSelection;
class vtkTextProperty;
class vtkTexture;
class vtkTexturedActor2D;
class vtkTransformCoordinateSystems;

class VTK_VIEWS_EXPORT vtkRenderView : public vtkView
{
public:
  static vtkRenderView* New();
  vtkTypeRevisionMacro(vtkRenderView, vtkView);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Gets the renderer for this view.
  vtkGetObjectMacro(Renderer, vtkRenderer);
  
  // Description:
  // Set up a render window to use this view.
  // The superclass adds the renderer to the render window.
  // Subclasses should override this to set interactor, etc.
  virtual void SetupRenderWindow(vtkRenderWindow* win);

  // Description:
  // Get a handle to the render window.
  virtual vtkRenderWindow* GetRenderWindow();
  
  //BTX
  enum
    {
    INTERACTION_MODE_2D,
    INTERACTION_MODE_3D,
    INTERACTION_MODE_UNKNOWN
    };
  //ETX

  // Description:
  // Set the interaction mode for the view. Choices are:
  // vtkRenderView::INTERACTION_MODE_2D - 2D interactor
  // vtkRenderView::INTERACTION_MODE_3D - 3D interactor
  virtual void SetInteractionMode(int mode);
  vtkGetMacro(InteractionMode, int);
  virtual void SetInteractionModeTo2D()
    { this->SetInteractionMode(INTERACTION_MODE_2D); }
  virtual void SetInteractionModeTo3D()
    { this->SetInteractionMode(INTERACTION_MODE_3D); }

  virtual void ApplyViewTheme(vtkViewTheme* theme);

  // Description:
  // Set the view's transform. All vtkRenderedRepresentations
  // added to this view should use this transform.
  virtual void SetTransform(vtkAbstractTransform* transform);
  vtkGetObjectMacro(Transform, vtkAbstractTransform);

  // Description:
  // Whether the view should display hover text.
  virtual void SetDisplayHoverText(bool b);
  vtkGetMacro(DisplayHoverText, bool);
  vtkBooleanMacro(DisplayHoverText, bool);

  //BTX
  enum {
    SURFACE = 0,
    FRUSTUM = 1
  };
  //ETX

  // Description:
  // Sets the selection mode for the render view.
  // SURFACE selection uses vtkHardwareSelector to perform a selection
  // of visible cells.
  // FRUSTUM selection just creates a view frustum selection, which will
  // select everything in the frustum.
  vtkSetClampMacro(SelectionMode, int, 0, 1);
  vtkGetMacro(SelectionMode, int);
  void SetSelectionModeToSurface() { this->SetSelectionMode(SURFACE); }
  void SetSelectionModeToFrustum() { this->SetSelectionMode(FRUSTUM); }

  // Description:
  // Calls Render() on the render window associated with this view.
  virtual void Render();
  
  // Description:
  // The interactor style associated with the render view.
  vtkGetObjectMacro(InteractorStyle, vtkInteractorStyle);
  virtual void SetInteractorStyle(vtkInteractorStyle* style);
  
  // Description:
  // Add labels from an input connection with an associated text
  // property. The output must be a vtkDataSet with a string array named
  // "LabelText" in the point data.
  virtual void AddLabels(vtkAlgorithmOutput* conn, vtkTextProperty* prop);

  // Description:
  // Remove labels from an input connection.
  virtual void RemoveLabels(vtkAlgorithmOutput* conn);

  // Description:
  // Add icons from an input connection. The connection must be a
  // vtkDataSet with an integer array named "IconType" in the point data.
  virtual void AddIcons(vtkAlgorithmOutput* conn);

  // Description:
  // Remove icons from an input connection.
  virtual void RemoveIcons(vtkAlgorithmOutput* conn);

  // Description:
  // Set the icon sheet to use for rendering icons.
  virtual void SetIconTexture(vtkTexture* texture);
  virtual vtkTexture* GetIconTexture();

  // Description:
  // Set the size of each icon in the icon texture.
  virtual void SetIconSize(int* size)
    { this->SetIconSize(size[0], size[1]); }
  virtual void SetIconSize(int x, int y);
  virtual int* GetIconSize();

  //BTX
  enum
    {
    DYNAMIC_2D,
    LABEL_PLACER,
    ALL
    };
  //ETX

  // Description:
  // Label placement mode.
  // DYNAMIC_2D uses vtkDynamic2DLabelMapper, which has slower startup time,
  // and only works in 2D, but may be more stable.
  // LABEL_PLACER uses vtkLabelPlacer, which has a faster startup time and
  // works with 2D or 3D labels.
  // ALL displays all labels (Warning: This may cause incredibly slow render
  // times on datasets with more than a few hundred labels).
  virtual void SetLabelPlacementMode(int mode);
  virtual int GetLabelPlacementMode();
  virtual void SetLabelPlacementModeToDynamic2D()
    { this->SetLabelPlacementMode(DYNAMIC_2D); }
  virtual void SetLabelPlacementModeToLabelPlacer()
    { this->SetLabelPlacementMode(LABEL_PLACER); }
  virtual void SetLabelPlacementModeToAll()
    { this->SetLabelPlacementMode(ALL); }

  //BTX
  enum
    {
    FREETYPE,
    QT
    };
  //ETX

  // Description:
  // Label render mode.
  // FREETYPE uses the freetype label rendering.
  // QT uses more advanced Qt-based label rendering.
  virtual void SetLabelRenderMode(int mode);
  vtkGetMacro(LabelRenderMode, int);
  virtual void SetLabelRenderModeToFreetype()
    { this->SetLabelRenderMode(FREETYPE); }
  virtual void SetLabelRenderModeToQt()
    { this->SetLabelRenderMode(QT); }

protected:
  vtkRenderView();
  ~vtkRenderView();
  
  // Description:
  // Called to process events.
  // Captures StartEvent events from the renderer and calls Update().
  // This may be overridden by subclasses to process additional events.
  virtual void ProcessEvents(vtkObject* caller, unsigned long eventId, 
    void* callData);

  // Description:
  // Generates the selection based on the view event and the selection mode.
  virtual void GenerateSelection(
    void* callData, vtkSelection* selection);

  // Description:
  // Called by the view when the renderer is about to render.
  virtual void PrepareForRendering();
  
  // Description:
  // Called when a representation's selection changed.
  virtual void RepresentationSelectionChanged(
    vtkDataRepresentation* rep,
    vtkSelection* selection);

  // Description:
  // Called in PrepareForRendering to update the hover text.
  virtual void UpdateHoverText();

  vtkRenderer* Renderer;
  int SelectionMode;
  int InteractionMode;
  int LabelRenderMode;
  bool DisplayHoverText;

  vtkAbstractTransform* Transform;
  vtkInteractorStyle* InteractorStyle;

  //BTX
  vtkSmartPointer<vtkBalloonRepresentation>    Balloon;

  vtkSmartPointer<vtkAppendPoints>             LabelAppend;
  vtkSmartPointer<vtkLabelSizeCalculator>      LabelSize;
  vtkSmartPointer<vtkPointSetToLabelHierarchy> LabelHierarchy;
  vtkSmartPointer<vtkLabelPlacer>              LabelPlacer;
  vtkSmartPointer<vtkLabeledDataMapper>        LabelMapper;
  vtkSmartPointer<vtkActor2D>                  LabelActor;
  vtkSmartPointer<vtkDynamic2DLabelMapper>     LabelMapper2D;

  vtkSmartPointer<vtkAppendPoints>               IconAppend;
  vtkSmartPointer<vtkArrayCalculator>            IconSize;
  vtkSmartPointer<vtkIconGlyphFilter>            IconGlyph;
  vtkSmartPointer<vtkTransformCoordinateSystems> IconTransform;
  vtkSmartPointer<vtkPolyDataMapper2D>           IconMapper;
  vtkSmartPointer<vtkTexturedActor2D>            IconActor;
  //ETX

private:
  vtkRenderView(const vtkRenderView&);  // Not implemented.
  void operator=(const vtkRenderView&);  // Not implemented.
};

#endif
