/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPropItem.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   vtkPropItem
 * @brief   Embed a vtkProp in a vtkContextScene.
 *
 *
 * This class allows vtkProp objects to be drawn inside a vtkContextScene.
 * This is especially useful for constructing layered scenes that need to ignore
 * depth testing.
*/

#ifndef vtkPropItem_h
#define vtkPropItem_h

#include "vtkRenderingContext2DModule.h" // For export macro
#include "vtkAbstractContextItem.h"

class vtkProp;

class VTKRENDERINGCONTEXT2D_EXPORT vtkPropItem: public vtkAbstractContextItem
{
public:
  static vtkPropItem *New();
  vtkTypeMacro(vtkPropItem, vtkAbstractContextItem)
  void PrintSelf(ostream &os, vtkIndent indent) VTK_OVERRIDE;

  bool Paint(vtkContext2D *painter) VTK_OVERRIDE;
  void ReleaseGraphicsResources() VTK_OVERRIDE;

  /**
   * The actor to render.
   */
  virtual void SetPropObject(vtkProp *PropObject);
  vtkGetObjectMacro(PropObject, vtkProp)

protected:
  vtkPropItem();
  ~vtkPropItem() VTK_OVERRIDE;

  // Sync the active vtkCamera with the GL state set by the painter.
  virtual void UpdateTransforms();

  // Restore the vtkCamera state.
  virtual void ResetTransforms();

private:
  vtkProp *PropObject;

  vtkPropItem(const vtkPropItem &) VTK_DELETE_FUNCTION;
  void operator=(const vtkPropItem &) VTK_DELETE_FUNCTION;
};

#endif //vtkPropItem_h
