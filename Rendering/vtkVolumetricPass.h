/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVolumetricPass.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkVolumetricPass - Render the volumetric geometry with property key
// filtering.
// .SECTION Description
// vtkVolumetricPass renders the volumetric geometry of all the props that
// have the keys contained in vtkRenderState.
//
// This pass expects an initialized depth buffer and color buffer.
// Initialized buffers means they have been cleared with farest z-value and
// background color/gradient/transparent color.
// 
// .SECTION See Also
// vtkRenderPass vtkDefaultPass

#ifndef __vtkVolumetricPass_h
#define __vtkVolumetricPass_h

#include "vtkDefaultPass.h"

class VTK_RENDERING_EXPORT vtkVolumetricPass : public vtkDefaultPass
{
public:
  static vtkVolumetricPass *New();
  vtkTypeMacro(vtkVolumetricPass,vtkDefaultPass);
  void PrintSelf(ostream& os, vtkIndent indent);

  //BTX
  // Description:
  // Perform rendering according to a render state \p s.
  // \pre s_exists: s!=0
  virtual void Render(const vtkRenderState *s);
  //ETX
  
 protected:
  // Description:
  // Default constructor.
  vtkVolumetricPass();

  // Description:
  // Destructor.
  virtual ~vtkVolumetricPass();
  
 private:
  vtkVolumetricPass(const vtkVolumetricPass&);  // Not implemented.
  void operator=(const vtkVolumetricPass&);  // Not implemented.
};

#endif
