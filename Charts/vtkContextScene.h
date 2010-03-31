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
#include "vtkVector.h" // Needed for vtkVector2f

class vtkContext2D;
class vtkContextItem;
class vtkTransform2D;
class vtkContextMouseEvent;

class vtkInteractorStyle;
class vtkAnnotationLink;

class vtkRenderer;
class vtkAbstractContextBufferId;

class VTK_CHARTS_EXPORT vtkContextScene : public vtkObject
{
public:
  vtkTypeMacro(vtkContextScene, vtkObject);
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
  int GetNumberOfItems();

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
  // Set whether the scene should use the color buffer. Default is true.
  vtkSetMacro(UseBufferId, bool);

  // Description:
  // Get whether the scene is using the color buffer. Default is true.
  vtkGetMacro(UseBufferId, bool);

  // Description:
  // Get the width of the view
  virtual int GetViewWidth();

  // Description:
  // Get the height of the view
  virtual int GetViewHeight();

  // Description:
  // Get the width of the scene.
  int GetSceneWidth();

  // Description:
  // Get the height of the scene.
  int GetSceneHeight();

  // Description:
  // Add the scene as an observer on the supplied interactor style.
  void SetInteractorStyle(vtkInteractorStyle *interactor);

  // Description:
  // This should not be necessary as the context view should take care of
  // rendering.
  virtual void SetRenderer(vtkRenderer *renderer);

  // Description:
  // Inform the scene that something changed that requires a repaint of the
  // scene. This should only be used by the vtkContextItem derived objects in
  // a scene in their event handlers.
  void SetDirty(bool isDirty);

//BTX
  // Description:
  // Release graphics resources hold by the scene.
  void ReleaseGraphicsResources();
  
  // Description:
  // Last painter used.
  // Not part of the end-user API. Can be used by context items to
  // create their own colorbuffer id (when a context item is a container).
  vtkWeakPointer<vtkContext2D> GetLastPainter();

  // Description:
  // Return buffer id.
  // Not part of the end-user API. Can be used by context items to
  // initialize their own colorbuffer id (when a context item is a container).
  vtkAbstractContextBufferId *GetBufferId();

  // Description:
  // Set the transform for the scene.
  virtual void SetTransform(vtkTransform2D *transform);

  // Description:
  // Get the transform for the scene.
  vtkTransform2D* GetTransform();

  // Description:
  // Check whether the scene has a transform.
  bool HasTransform() { return this->Transform != 0; }

protected:
  vtkContextScene();
  ~vtkContextScene();

  // Description:
  // Protected function called after any event to check if a repaint of the
  // scene is required. Called by the Command object after interaction events.
  void CheckForRepaint();

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

  // Description:
  // Paint the scene in a special mode to build a cache for picking.
  // Use internally.
  virtual void PaintIds();

  // Description:
  // Return the item id under mouse cursor at position (x,y).
  // Return -1 if there is no item under the mouse cursor.
  // \post valid_result: result>=-1 && result<this->GetNumberOfItems()
  vtkIdType GetPickedItem(int x, int y);

  // Description:
  // Make sure the buffer id used for picking is up-to-date.
  void UpdateBufferId();

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

  vtkWeakPointer<vtkContext2D> LastPainter;

  vtkWeakPointer<vtkRenderer> Renderer;

  vtkAbstractContextBufferId *BufferId;
  bool BufferIdDirty;

  bool UseBufferId;

  // Description:
  // The scene level transform.
  vtkTransform2D* Transform;

  // Description:
  // Perform translation and fill in the vtkContextMouseEvent struct.
  void PerformTransform(vtkTransform2D *transform, vtkContextMouseEvent &mouse);

private:
  vtkContextScene(const vtkContextScene &); // Not implemented.
  void operator=(const vtkContextScene &);   // Not implemented.
//ETX
};

//BTX
// Description:
// Data structure to store context scene mouse events to be passed to items.
class vtkContextMouseEvent
{
public:
  // Description:
  // Enumeration of mouse buttons.
  enum {
    LEFT_BUTTON = 0,
    MIDDLE_BUTTON,
    RIGHT_BUTTON
  };

  // Description:
  // Position of the mouse in item coordinate system.
  vtkVector2f Pos;

  // Description:
  // Position of the mouse the scene coordinate system.
  vtkVector2f ScenePos;

  // Description:
  // Position of the mouse in screen coordinates
  vtkVector2i ScreenPos;

  // Description:
  // `Pos' at the previous mouse event.
  vtkVector2f LastPos;

  // Description:
  // `ScenePos'at the previous mouse event.
  vtkVector2f LastScenePos;

  // Description:
  // `ScreenPos' at the previous mouse event.
  vtkVector2i LastScreenPos;

  // Description:
  // Mouse button that was pressed, using the anonymous enumeration.
  int Button;
};
//ETX

#endif //__vtkContextScene_h
