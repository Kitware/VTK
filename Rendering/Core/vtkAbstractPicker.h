/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAbstractPicker.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkAbstractPicker - define API for picking subclasses
// .SECTION Description
// vtkAbstractPicker is an abstract superclass that defines a minimal API
// for its concrete subclasses. The minimum functionality of a picker is
// to return the x-y-z global coordinate position of a pick (the pick
// itself is defined in display coordinates).
//
// The API to this class is to invoke the Pick() method with a selection
// point (in display coordinates - pixels) and a renderer. Then get the
// resulting pick position in global coordinates with the GetPickPosition()
// method.
//
// vtkPicker fires events during the picking process.  These
// events are StartPickEvent, PickEvent, and EndPickEvent which are
// invoked prior to picking, when something is picked, and after all picking
// candidates have been tested. Note that during the pick process the
// PickEvent of vtkProp (and its subclasses such as vtkActor) is fired
// prior to the PickEvent of vtkPicker.

// .SECTION Caveats
// vtkAbstractPicker and its subclasses will not pick props that are
// "unpickable" (see vtkProp) or are fully transparent (if transparency
// is a property of the vtkProp).
//
// There are two classes of pickers: those that pick using geometric methods
// (typically a ray cast); and those that use rendering hardware. Geometric
// methods return more information but are slower. Hardware methods are much
// faster and return minimal information. Examples of geometric pickers
// include vtkPicker, vtkCellPicker, and vtkPointPicker. Examples of hardware
// pickers include vtkWorldPointPicker and vtkPropPicker.

// .SECTION See Also
// vtkPropPicker uses hardware acceleration to pick an instance of vtkProp.
// (This means that 2D and 3D props can be picked, and it's relatively fast.)
// If you need to pick cells or points, you might wish to use vtkCellPicker
// or vtkPointPicker. vtkWorldPointPicker is the fastest picker, returning
// an x-y-z coordinate value using the hardware z-buffer. vtkPicker can be
// used to pick the bounding box of 3D props.

#ifndef vtkAbstractPicker_h
#define vtkAbstractPicker_h

#include "vtkRenderingCoreModule.h" // For export macro
#include "vtkObject.h"

class vtkRenderer;
class vtkProp;
class vtkPropCollection;

class VTKRENDERINGCORE_EXPORT vtkAbstractPicker : public vtkObject
{
public:
  vtkTypeMacro(vtkAbstractPicker,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get the renderer in which pick event occurred.
  vtkGetObjectMacro(Renderer,vtkRenderer);

  // Description:
  // Get the selection point in screen (pixel) coordinates. The third
  // value is related to z-buffer depth. (Normally should be =0.)
  vtkGetVectorMacro(SelectionPoint,double,3);

  // Description:
  // Return position in global coordinates of pick point.
  vtkGetVectorMacro(PickPosition,double,3);

  // Description:
  // Perform pick operation with selection point provided. Normally the
  // first two values for the selection point are x-y pixel coordinate, and
  // the third value is =0. Return non-zero if something was successfully
  // picked.
  virtual int Pick(double selectionX, double selectionY, double selectionZ,
                   vtkRenderer *renderer) = 0;

  // Description: Perform pick operation with selection point
  // provided. Normally the first two values for the selection point
  // are x-y pixel coordinate, and the third value is =0. Return
  // non-zero if something was successfully picked.
  int Pick(double selectionPt[3], vtkRenderer *ren)
    {return this->Pick(selectionPt[0],selectionPt[1],selectionPt[2],ren);};

  // Description:
  // Use these methods to control whether to limit the picking to this list
  // (rather than renderer's actors). Make sure that the pick list contains
  // actors that referred to by the picker's renderer.
  vtkSetMacro(PickFromList,int);
  vtkGetMacro(PickFromList,int);
  vtkBooleanMacro(PickFromList,int);

  // Description:
  // Initialize list of actors in pick list.
  void InitializePickList();

  // Description:
  // Add an actor to the pick list.
  void AddPickList(vtkProp *);

  // Description:
  // Delete an actor from the pick list.
  void DeletePickList(vtkProp *);

  // Description:
  // Return the list of actors in the PickList.
  vtkPropCollection *GetPickList() {return this->PickList;}

protected:
  vtkAbstractPicker();
  ~vtkAbstractPicker();

  virtual void Initialize();

  vtkRenderer *Renderer; //pick occurred in this renderer's viewport
  double SelectionPoint[3]; //selection point in window (pixel) coordinates
  double PickPosition[3]; //selection point in world coordinates

  // use the following to control picking from a list
  int PickFromList;
  vtkPropCollection *PickList;
private:
  vtkAbstractPicker(const vtkAbstractPicker&);  // Not implemented.
  void operator=(const vtkAbstractPicker&);  // Not implemented.
};

#endif


