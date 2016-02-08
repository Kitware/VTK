/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOsprayPass.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkOsprayPass - a render pass that uses OSPRay instead of GL
// .SECTION Description
// This is a render pass that can be put into a vtkRenderWindow which makes
// it use OSPRay instead of OpenGL to render. Adding/Removing the pass
// will swap back and forth between the two.

#ifndef vtkOsprayPass_h
#define vtkOsprayPass_h

#include "vtkRenderingOsprayModule.h" // For export macro
#include "vtkRenderPass.h"

class vtkOsprayWindowNode;

class VTKRENDERINGOSPRAY_EXPORT vtkOsprayPass : public vtkRenderPass
{
public:
  static vtkOsprayPass *New();
  vtkTypeMacro(vtkOsprayPass,vtkRenderPass);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Perform rendering according to a render state s.
  virtual void Render(const vtkRenderState *s);

  // Description:
  // Tells the pass what it will render.
  void SetSceneGraph(vtkOsprayWindowNode *);
  vtkGetObjectMacro(SceneGraph, vtkOsprayWindowNode);

 protected:
  // Description:
  // Default constructor.
  vtkOsprayPass();

  // Description:
  // Destructor.
  virtual ~vtkOsprayPass();

  vtkOsprayWindowNode *SceneGraph;

 private:
  vtkOsprayPass(const vtkOsprayPass&);  // Not implemented.
  void operator=(const vtkOsprayPass&);  // Not implemented.
};

#endif
