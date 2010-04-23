/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCameraPass.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkCameraPass - Implement the camera render pass.
// .SECTION Description
// Render the camera.
//
// It setups the projection and modelview matrices and can clear the background
// It calls its delegate once.
// After its delegate returns, it restore the modelview matrix stack.
//
// Its delegate is usually set to a vtkSequencePass with a vtkLigthsPass and
// a list of passes for the geometry.
// 
// .SECTION See Also
// vtkRenderPass

#ifndef __vtkCameraPass_h
#define __vtkCameraPass_h

#include "vtkRenderPass.h"

class VTK_RENDERING_EXPORT vtkCameraPass : public vtkRenderPass
{
public:
  static vtkCameraPass *New();
  vtkTypeMacro(vtkCameraPass,vtkRenderPass);
  void PrintSelf(ostream& os, vtkIndent indent);

  //BTX
  // Description:
  // Perform rendering according to a render state \p s.
  // \pre s_exists: s!=0
  virtual void Render(const vtkRenderState *s);
  //ETX
  
  // Description:
  // Release graphics resources and ask components to release their own
  // resources.
  // \pre w_exists: w!=0
  void ReleaseGraphicsResources(vtkWindow *w);
  
  // Description:
  // Delegate for rendering the geometry.
  // If it is NULL, nothing will be rendered and a warning will be emitted.
  // It is usually set to a vtkSequencePass with a vtkLigthsPass and
  // a list of passes for the geometry.
  // Initial value is a NULL pointer.
  vtkGetObjectMacro(DelegatePass,vtkRenderPass);
  virtual void SetDelegatePass(vtkRenderPass *delegatePass);
  
 protected:
  // Description:
  // Default constructor. DelegatePass is set to NULL.
  vtkCameraPass();

  // Description:
  // Destructor.
  virtual ~vtkCameraPass();
  
  vtkRenderPass *DelegatePass;
  
 private:
  vtkCameraPass(const vtkCameraPass&);  // Not implemented.
  void operator=(const vtkCameraPass&);  // Not implemented.
};

#endif
