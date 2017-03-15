/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPropPicker.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPropPicker
 * @brief   pick an actor/prop using graphics hardware
 *
 * vtkPropPicker is used to pick an actor/prop given a selection
 * point (in display coordinates) and a renderer. This class uses
 * graphics hardware/rendering system to pick rapidly (as compared
 * to using ray casting as does vtkCellPicker and vtkPointPicker).
 * This class determines the actor/prop and pick position in world
 * coordinates; point and cell ids are not determined.
 *
 * @sa
 * vtkPicker vtkWorldPointPicker vtkCellPicker vtkPointPicker
*/

#ifndef vtkPropPicker_h
#define vtkPropPicker_h

#include "vtkRenderingCoreModule.h" // For export macro
#include "vtkAbstractPropPicker.h"

class vtkProp;
class vtkWorldPointPicker;

class VTKRENDERINGCORE_EXPORT vtkPropPicker : public vtkAbstractPropPicker
{
public:
  static vtkPropPicker *New();

  vtkTypeMacro(vtkPropPicker, vtkAbstractPropPicker);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Perform the pick and set the PickedProp ivar. If something is picked, a
   * 1 is returned, otherwise 0 is returned.  Use the GetViewProp() method
   * to get the instance of vtkProp that was picked.  Props are picked from
   * the renderers list of pickable Props.
   */
  int PickProp(double selectionX, double selectionY, vtkRenderer *renderer);

  /**
   * Perform a pick from the user-provided list of vtkProps and not from the
   * list of vtkProps that the render maintains.
   */
  int PickProp(double selectionX, double selectionY, vtkRenderer *renderer,
               vtkPropCollection* pickfrom);

  /**
   * override superclasses' Pick() method.
   */
  int Pick(double selectionX, double selectionY, double selectionZ,
           vtkRenderer *renderer) VTK_OVERRIDE;
  int Pick(double selectionPt[3], vtkRenderer *renderer)
    { return this->Pick( selectionPt[0],
                         selectionPt[1], selectionPt[2], renderer); }

protected:
  vtkPropPicker();
  ~vtkPropPicker() VTK_OVERRIDE;

  void Initialize() VTK_OVERRIDE;

  vtkPropCollection* PickFromProps;

  // Used to get x-y-z pick position
  vtkWorldPointPicker *WorldPointPicker;
private:
  vtkPropPicker(const vtkPropPicker&) VTK_DELETE_FUNCTION;
  void operator=(const vtkPropPicker&) VTK_DELETE_FUNCTION;
};

#endif
