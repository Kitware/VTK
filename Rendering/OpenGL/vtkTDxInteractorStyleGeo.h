/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTDxInteractorStyleGeo.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkTDxInteractorStyleGeo
 * @brief   interactive manipulation of the camera with a 3DConnexion device, similar to google earth
 *
 *
 * vtkTDxInteractorStyleGeo allows the end-user to manipulate tha camera
 * with a 3DConnexion device similar to google earth interaction.
 *
 * @sa
 * vtkInteractorStyle vtkRenderWindowInteractor
 * vtkTDxInteractorStyle
*/

#ifndef vtkTDxInteractorStyleGeo_h
#define vtkTDxInteractorStyleGeo_h

#include "vtkRenderingOpenGLModule.h" // For export macro
#include "vtkTDxInteractorStyle.h"

class vtkTransform;

class VTKRENDERINGOPENGL_EXPORT vtkTDxInteractorStyleGeo : public vtkTDxInteractorStyle
{
public:
  static vtkTDxInteractorStyleGeo *New();
  vtkTypeMacro(vtkTDxInteractorStyleGeo,vtkTDxInteractorStyle);
  void PrintSelf(ostream& os, vtkIndent indent);

  /**
   * Action on motion event.
   * \pre: motionInfo_exist: motionInfo!=0
   */
  virtual void OnMotionEvent(vtkTDxMotionEventInfo *motionInfo);

protected:
  vtkTDxInteractorStyleGeo();
  virtual ~vtkTDxInteractorStyleGeo();

  vtkTransform *Transform; // Used for internal intermediate calculation.

private:
  vtkTDxInteractorStyleGeo(const vtkTDxInteractorStyleGeo&) VTK_DELETE_FUNCTION;
  void operator=(const vtkTDxInteractorStyleGeo&) VTK_DELETE_FUNCTION;
};
#endif
