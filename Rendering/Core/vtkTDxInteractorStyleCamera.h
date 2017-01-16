/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTDxInteractorStyleCamera.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkTDxInteractorStyleCamera
 * @brief   interactive manipulation of the camera with a 3DConnexion device
 *
 *
 * vtkTDxInteractorStyleCamera allows the end-user to manipulate tha camera
 * with a 3DConnexion device.
 *
 * @sa
 * vtkInteractorStyle vtkRenderWindowInteractor
 * vtkTDxInteractorStyle
*/

#ifndef vtkTDxInteractorStyleCamera_h
#define vtkTDxInteractorStyleCamera_h

#include "vtkRenderingCoreModule.h" // For export macro
#include "vtkTDxInteractorStyle.h"

class vtkTransform;

class VTKRENDERINGCORE_EXPORT vtkTDxInteractorStyleCamera : public vtkTDxInteractorStyle
{
public:
  static vtkTDxInteractorStyleCamera *New();
  vtkTypeMacro(vtkTDxInteractorStyleCamera,vtkTDxInteractorStyle);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Action on motion event.
   * \pre: motionInfo_exist: motionInfo!=0
   */
  void OnMotionEvent(vtkTDxMotionEventInfo *motionInfo) VTK_OVERRIDE;

protected:
  vtkTDxInteractorStyleCamera();
  ~vtkTDxInteractorStyleCamera() VTK_OVERRIDE;

  vtkTransform *Transform; // Used for internal intermediate calculation.

private:
  vtkTDxInteractorStyleCamera(const vtkTDxInteractorStyleCamera&) VTK_DELETE_FUNCTION;
  void operator=(const vtkTDxInteractorStyleCamera&) VTK_DELETE_FUNCTION;
};
#endif
