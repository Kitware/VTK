/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPropPicker3D.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPropPicker3D
 * @brief   pick an actor/prop given XYZ coordinates
 *
 * vtkPropPicker3D is used to pick an actor/prop given a selection
 * point in world coordinates.
 * This class determines the actor/prop and pick position in world
 * coordinates; point and cell ids are not determined.
 * This is useful for VRE devices that provide 3D positions
 * directly via the vtkRenderWindowInteractor3D. It is the default
 * picker for the vtkInteractorStyle3D
 *
 * @sa
 * vtkPicker vtkRenderWindowInteractor3D vtkInteractorStyle3D
*/

#ifndef vtkPropPicker3D_h
#define vtkPropPicker3D_h

#include "vtkRenderingCoreModule.h" // For export macro
#include "vtkAbstractPropPicker.h"

class vtkProp;

class VTKRENDERINGCORE_EXPORT vtkPropPicker3D : public vtkAbstractPropPicker
{
public:
  static vtkPropPicker3D *New();

  vtkTypeMacro(vtkPropPicker3D, vtkAbstractPropPicker);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Perform the pick and set the PickedProp ivar. If something is picked, a
   * 1 is returned, otherwise 0 is returned.  Use the GetViewProp() method
   * to get the instance of vtkProp that was picked.  Props are picked from
   * the renderers list of pickable Props.
   */
  int PickProp(double selectionX, double selectionY,
    double selectionZ, vtkRenderer *renderer);

  /**
   * Perform a pick from the user-provided list of vtkProps and not from the
   * list of vtkProps that the render maintains.
   */
  int PickProp(double selectionX, double selectionY,
    double selectionZ, vtkRenderer *renderer,
    vtkPropCollection* pickfrom);

  /**
   * Override superclasses' Pick() method.
   */
  int Pick(double selectionX, double selectionY, double selectionZ,
           vtkRenderer *renderer) VTK_OVERRIDE;
  int Pick(double selectionPt[3], vtkRenderer *renderer)
    { return this->Pick( selectionPt[0],
                         selectionPt[1], selectionPt[2], renderer); }

protected:
  vtkPropPicker3D();
  ~vtkPropPicker3D() VTK_OVERRIDE;

  void Initialize() VTK_OVERRIDE;

  vtkPropCollection* PickFromProps;

 private:
  vtkPropPicker3D(const vtkPropPicker3D&) VTK_DELETE_FUNCTION;  // Not implemented.
  void operator=(const vtkPropPicker3D&) VTK_DELETE_FUNCTION;  // Not implemented.
};

#endif
