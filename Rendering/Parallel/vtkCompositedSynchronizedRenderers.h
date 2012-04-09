/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile$

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkCompositedSynchronizedRenderers
// .SECTION Description
// vtkCompositedSynchronizedRenderers is vtkSynchronizedRenderers that uses
// vtkCompositer to composite the images on  the root node.

#ifndef __vtkCompositedSynchronizedRenderers_h
#define __vtkCompositedSynchronizedRenderers_h

#include "vtkRenderingParallelModule.h" // For export macro
#include "vtkSynchronizedRenderers.h"

class vtkFloatArray;
class vtkCompositer;

class VTKRENDERINGPARALLEL_EXPORT vtkCompositedSynchronizedRenderers : public vtkSynchronizedRenderers
{
public:
  static vtkCompositedSynchronizedRenderers* New();
  vtkTypeMacro(vtkCompositedSynchronizedRenderers, vtkSynchronizedRenderers);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get/Set the composite. vtkTreeCompositer is used by default.
  void SetCompositer(vtkCompositer*);
  vtkGetObjectMacro(Compositer, vtkCompositer);

//BTX
protected:
  vtkCompositedSynchronizedRenderers();
  ~vtkCompositedSynchronizedRenderers();

  virtual void MasterEndRender();
  virtual void SlaveEndRender();
  void CaptureRenderedDepthBuffer(vtkFloatArray* depth_buffer);

  vtkCompositer* Compositer;
private:
  vtkCompositedSynchronizedRenderers(const vtkCompositedSynchronizedRenderers&); // Not implemented
  void operator=(const vtkCompositedSynchronizedRenderers&); // Not implemented
//ETX
};

#endif
