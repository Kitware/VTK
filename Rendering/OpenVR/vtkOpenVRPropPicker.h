/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkOpenVRPropPicker.h

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
* @class   vtkOpenVRPropPicker
* @brief   pick an actor/prop given a controller position and orientation
*
* vtkOpenVRPropPicker is used to pick an actor/prop along a ray.
* The ray position and orientation are defined by the event position and
* orientation in world coordinate.
* This class stores the picked actor/prop and the picked position in world
* coordinates; point and cell ids are not determined.
* This is useful for VRE devices that provide 3D positions and orientation.
*
* @sa
* vtkProp3DPicker vtkOpenVRInteractorStylePointer
*/

#ifndef vtkOpenVRPropPicker_h
#define vtkOpenVRPropPicker_h

#include "vtkRenderingOpenVRModule.h" // For export macro
#include "vtkPropPicker3D.h"

class vtkProp;

class VTKRENDERINGOPENVR_EXPORT vtkOpenVRPropPicker : public vtkPropPicker3D
{
public:
  static vtkOpenVRPropPicker *New();

  vtkTypeMacro(vtkOpenVRPropPicker, vtkPropPicker3D);

  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
  * Perform a pick from the user-provided list of vtkProps.
  */
  int PickProp(double selectionPt[3], double eventWorldOrientation[4],
    vtkRenderer *renderer, vtkPropCollection* pickfrom);

  /**
  * Overide superclasses' Pick() method.
  */
  int Pick(double selectionX, double selectionY, double selectionZ,
    vtkRenderer *renderer) VTK_OVERRIDE;


protected:
  vtkOpenVRPropPicker();
  ~vtkOpenVRPropPicker() VTK_OVERRIDE;

  void Initialize() VTK_OVERRIDE;


private:
  vtkOpenVRPropPicker(const vtkOpenVRPropPicker&) VTK_DELETE_FUNCTION;// Not implemented.
  void operator=(const vtkOpenVRPropPicker&) VTK_DELETE_FUNCTION;// Not implemented.
};

#endif
