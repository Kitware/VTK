/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOverlayPass.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkOverlayPass - Render the overlay geometry with property key
// filtering.
// .SECTION Description
// vtkOverlayPass renders the overlay geometry of all the props that have the
// keys contained in vtkRenderState.
//
// This pass expects an initialized depth buffer and color buffer.
// Initialized buffers means they have been cleared with farest z-value and
// background color/gradient/transparent color.
//
// .SECTION See Also
// vtkRenderPass vtkDefaultPass

#ifndef vtkOverlayPass_h
#define vtkOverlayPass_h

#include "vtkRenderingOpenGLModule.h" // For export macro
#include "vtkDefaultPass.h"

class VTKRENDERINGOPENGL_EXPORT vtkOverlayPass : public vtkDefaultPass
{
public:
  static vtkOverlayPass *New();
  vtkTypeMacro(vtkOverlayPass,vtkDefaultPass);
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
  vtkOverlayPass();

  // Description:
  // Destructor.
  virtual ~vtkOverlayPass();

 private:
  vtkOverlayPass(const vtkOverlayPass&);  // Not implemented.
  void operator=(const vtkOverlayPass&);  // Not implemented.
};

#endif
