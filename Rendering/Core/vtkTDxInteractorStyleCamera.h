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
// .NAME vtkTDxInteractorStyleCamera - interactive manipulation of the camera with a 3DConnexion device

// .SECTION Description
// vtkTDxInteractorStyleCamera allows the end-user to manipulate tha camera
// with a 3DConnexion device.

// .SECTION See Also
// vtkInteractorStyle vtkRenderWindowInteractor
// vtkTDxInteractorStyle

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
  void PrintSelf(ostream& os, vtkIndent indent);

  //BTX
  // Description:
  // Action on motion event.
  // \pre: motionInfo_exist: motionInfo!=0
  virtual void OnMotionEvent(vtkTDxMotionEventInfo *motionInfo);
  //ETX

protected:
  vtkTDxInteractorStyleCamera();
  virtual ~vtkTDxInteractorStyleCamera();

  vtkTransform *Transform; // Used for internal intermediate calculation.

private:
  vtkTDxInteractorStyleCamera(const vtkTDxInteractorStyleCamera&);  // Not implemented.
  void operator=(const vtkTDxInteractorStyleCamera&);  // Not implemented.
};
#endif
