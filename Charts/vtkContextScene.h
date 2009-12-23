/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkContextScene.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME vtkContextScene - Provides a 2D scene for vtkContextItem objects.
//
// .SECTION Description
// Provides a 2D scene that vtkContextItem objects can be added to. Manages the
// items, ensures that they are rendered at the right times and passes on mouse
// events.

#ifndef __vtkContextScene_h
#define __vtkContextScene_h

#include "vtkObject.h"
#include "vtkWeakPointer.h" // Needed for weak pointer to the window.

class vtkContext2D;
class vtkContextItem;
class vtkTransform2D;
struct vtkContextMouseEvent;

class vtkInteractorStyle;
class vtkAnnotationLink;

class vtkRenderWindow;

class VTK_CHARTS_EXPORT vtkContextScene : public vtkObject
{
public:
  vtkTypeRevisionMacro(vtkContextScene, vtkObject);
  virtual void PrintSelf(ostream &os, vtkIndent indent);

  // Description:
  // Creates a 2D Painter object.
  static vtkContextScene * New();

  // Description:
  // Paint event for the chart, called whenever the chart needs to be drawn
  virtual bool Paint(vtkContext2D *painter);

  // Description:
  // Add an item to the scene.
  void AddItem(vtkContextItem *item);

  // Description:
  // Get the number of items in the scene.
  int NumberOfItems();

  // Get the item at the specified index.
  vtkContextItem * GetItem(int index);

  // Description:
  // Set the vtkAnnotationLink for the chart.
  virtual void SetAnnotationLink(vtkAnnotationLink *link);

  // Description:
  // Get the vtkAnnotationLink for the chart.
  vtkGetObjectMacro(AnnotationLink, vtkAnnotationLink);

  // Description:
  // Set the width and height of the scene in pixels.
  vtkSetVector2Macro(Geometry, int);

  // Description:
  // Get the width and height of the scene in pixels.
  vtkGetVector2Macro(Geometry, int);

  // Description:
  // Get the width of the view
  virtual int GetViewWidth();

  // Description:
  // Get the height of the view
  virtual int GetViewHeight();

  // Description:
  // Add the scene as an observer on the supplied interactor style.
  void SetInteractorStyle(vtkInteractorStyle *interactor);

  // Description:
  // This should not be necessary as the context view should take care of
  // rendering.
  virtual void SetWindow(vtkRenderWindow *window);

//BTX
protected:
  vtkContextScene();
  ~vtkContextScene();

  // Description:
  // Called to process events - figure out what child(ren) to propagate events
  // to.
  virtual void ProcessEvents(vtkObject* caller, unsigned long eventId,
                             void* callData);

  // Description:
  // Process a rubber band selection event.
  virtual void ProcessSelectionEvent(vtkObject* caller, void* callData);

  // Description:
  // Process a mouse move event.
  virtual void MouseMoveEvent(int x, int y);

  // Description:
  // Process a mouse button press event.
  virtual void ButtonPressEvent(int button, int x, int y);

  // Description:
  // Process a mouse button release event.
  virtual void ButtonReleaseEvent(int button, int x, int y);

  // Description:
  // Process a mouse wheel event where delta is the movement forward or back.
  virtual void MouseWheelEvent(int delta, int x, int y);

  vtkAnnotationLink *AnnotationLink;

  // Store the chart dimensions - width, height of scene in pixels
  int Geometry[2];

  // Description:
  // The command object for the charts.
  class Command;
  friend class Command;
  Command *Observer;

  // Description:
  // Private storage object - where we hide all of our STL objects...
  class Private;
  Private *Storage;

  vtkWeakPointer<vtkRenderWindow> Window;

  // Description:
  // Perform translation and fill in the vtkContextMouseEvent struct.
  void PerformTransform(vtkTransform2D *transform, vtkContextMouseEvent &mouse);

private:
  vtkContextScene(const vtkContextScene &); // Not implemented.
  void operator=(const vtkContextScene &);   // Not implemented.
//ETX
};

// Description:
// Data structure to store context scene mouse events to be passed to items.
struct vtkContextMouseEvent
{
  float Pos[2]; // Item position
  float ScenePos[2]; // Position in scene coordinates
  int ScreenPos[2]; // Position in screen coordinates
  float LastPos[2]; // Item position
  float LastScenePos[2]; // Position in scene coordinates
  int LastScreenPos[2]; // Position in screen coordinates
  int Button; // Mouse button that was pressed (0-left, 1-middle, 2-right)
};

#endif //__vtkContextScene_h
