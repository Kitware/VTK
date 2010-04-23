/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkScenePicker.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkScenePicker - Picks an entire viewport at one shot. 
// .SECTION Description
// The Scene picker, unline conventional pickers picks an entire viewport at
// one shot and caches the result, which can be retrieved later. 
//    The utility of the class arises during <b>Actor Selection</b>. Let's 
// say you have a couple of polygonal objects in your scene and you wish to 
// have a status bar that indicates the object your mouse is over. Picking
// repeatedly every time your mouse moves would be very slow. The 
// scene picker automatically picks your viewport every time the camera is
// changed and caches the information. Additionally, it observes the 
// vtkRenderWindowInteractor to avoid picking during interaction, so that
// you still maintain your interactivity. In effect, the picker does an
// additional pick-render of your scene every time you stop interacting with
// your scene. As an example, see Rendering/TestScenePicker.
// 
// .SECTION Caveats
// - Unlike a vtkHoverWidget, this class is not timer based. The hover widget
//   picks a scene when the mouse is over an actor for a specified duration.
// - This class uses a vtkHardwareSelector under the hood. Hence, it will
//   work only for actors that have opaque geomerty and are rendered by a 
//   vtkPolyDataMapper.
// 
// .SECTION See Also
// vtkHoverWidget vtkHardwareSelector

#ifndef __vtkScenePicker_h
#define __vtkScenePicker_h

#include "vtkObject.h"

class vtkRenderer;
class vtkProp;
class vtkHardwareSelector;
class vtkRenderWindowInteractor;
class vtkScenePickerSelectionRenderCommand;

class VTK_RENDERING_EXPORT vtkScenePicker : public vtkObject
{
  //BTX
  friend class vtkRenderer;
  friend class vtkScenePickerSelectionRenderCommand;
  //ETX
public:
  static vtkScenePicker * New();
  vtkTypeMacro(vtkScenePicker,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the renderer. Scene picks are restricted to the viewport.
  virtual void SetRenderer( vtkRenderer* );
  vtkGetObjectMacro(Renderer,vtkRenderer);

  // Description:
  // Get cell id at the pick position. 
  // Returns -1 if no cell was picked. 
  // Makes sense only after Pick has been called.
  vtkIdType GetCellId( int displayPos[2] );

  // Description:
  // Get cell id at the pick position. 
  // Returns -1 if no cell was picked. 
  // Makes sense only after Pick has been called.
  vtkIdType GetVertexId( int displayPos[2] );

  // Description:
  // Get actor at the pick position. 
  // Returns NULL if none. 
  // Makes sense only after Pick has been called.
  vtkProp * GetViewProp( int displayPos[2] );
  
  // Description:
  // Vertex picking (using the method GetVertexId()), required 
  // additional resources and can slow down still render time by
  // 5-10%. Enabled by default.
  vtkSetMacro( EnableVertexPicking, int );
  vtkGetMacro( EnableVertexPicking, int );
  vtkBooleanMacro( EnableVertexPicking, int );

protected:
  vtkScenePicker();
  ~vtkScenePicker();

  // Pick render entire viewport
  // Automatically invoked from vtkRenderer at the end of a still render.
  void PickRender();

  // Pick render a region of the renderwindow
  void PickRender( int x0, int y0, int x1, int y1 );

  // Internal update method retrieves info from the Selector
  void Update( int displayPos[2] );

  // The RenderWindowInteractor must be set, so that avoid scene picks (which
  // involve extra renders) during interaction. This is done by observing the
  // RenderWindowInteractor for start and end interaction events. 
  void SetInteractor( vtkRenderWindowInteractor * );
  
  int                                     EnableVertexPicking;
  vtkHardwareSelector                   * Selector;
  vtkRenderer                           * Renderer;
  vtkRenderWindowInteractor             * Interactor;
  vtkIdType                               VertId;
  vtkIdType                               CellId;
  vtkProp                               * Prop;
  bool                                    NeedToUpdate;
  int                                     LastQueriedDisplayPos[2];
  vtkScenePickerSelectionRenderCommand  * SelectionRenderCommand;

  vtkTimeStamp PickRenderTime;
private:
  vtkScenePicker(
                 const vtkScenePicker&); // Not implemented.
  void operator=(const vtkScenePicker&); // Not implemented.
};

#endif

