/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSequencePass.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSequencePass - Execute render passes sequentially.
// .SECTION Description
// vtkSequencePass executes a list of render passes sequentially.
// This class allows to define a sequence of render passes at run time.
// The other solution to write a sequence of render passes is to write an
// effective subclass of vtkRenderPass.
//
// As vtkSequencePass is a vtkRenderPass itself, it is possible to have a
// hierarchy of render passes built at runtime.
// .SECTION See Also
// vtkRenderPass

#ifndef vtkSequencePass_h
#define vtkSequencePass_h

#include "vtkRenderingOpenGL2Module.h" // For export macro
#include "vtkRenderPass.h"

class vtkRenderPassCollection;

class VTKRENDERINGOPENGL2_EXPORT vtkSequencePass : public vtkRenderPass
{
public:
  static vtkSequencePass *New();
  vtkTypeMacro(vtkSequencePass,vtkRenderPass);
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
  virtual void ReleaseGraphicsResources(vtkWindow *w);

  // Description:
  // The ordered list of render passes to execute sequentially.
  // If the pointer is NULL or the list is empty, it is silently ignored.
  // There is no warning.
  // Initial value is a NULL pointer.
  vtkGetObjectMacro(Passes,vtkRenderPassCollection);
  virtual void SetPasses(vtkRenderPassCollection *passes);

protected:
  vtkRenderPassCollection *Passes;

  vtkSequencePass();
  virtual ~vtkSequencePass();

private:
  vtkSequencePass(const vtkSequencePass&);  // Not implemented.
  void operator=(const vtkSequencePass&);  // Not implemented.
};

#endif
