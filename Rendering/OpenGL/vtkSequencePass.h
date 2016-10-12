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
/**
 * @class   vtkSequencePass
 * @brief   Execute render passes sequentially.
 *
 * vtkSequencePass executes a list of render passes sequentially.
 * This class allows to define a sequence of render passes at run time.
 * The other solution to write a sequence of render passes is to write an
 * effective subclass of vtkRenderPass.
 *
 * As vtkSequencePass is a vtkRenderPass itself, it is possible to have a
 * hierarchy of render passes built at runtime.
 * @sa
 * vtkRenderPass
*/

#ifndef vtkSequencePass_h
#define vtkSequencePass_h

#include "vtkRenderingOpenGLModule.h" // For export macro
#include "vtkRenderPass.h"

class vtkRenderPassCollection;

class VTKRENDERINGOPENGL_EXPORT vtkSequencePass : public vtkRenderPass
{
public:
  static vtkSequencePass *New();
  vtkTypeMacro(vtkSequencePass,vtkRenderPass);
  void PrintSelf(ostream& os, vtkIndent indent);

  /**
   * Perform rendering according to a render state \p s.
   * \pre s_exists: s!=0
   */
  virtual void Render(const vtkRenderState *s);

  /**
   * Release graphics resources and ask components to release their own
   * resources.
   * \pre w_exists: w!=0
   */
  virtual void ReleaseGraphicsResources(vtkWindow *w);

  //@{
  /**
   * The ordered list of render passes to execute sequentially.
   * If the pointer is NULL or the list is empty, it is silently ignored.
   * There is no warning.
   * Initial value is a NULL pointer.
   */
  vtkGetObjectMacro(Passes,vtkRenderPassCollection);
  virtual void SetPasses(vtkRenderPassCollection *passes);
  //@}

protected:
  vtkRenderPassCollection *Passes;

  vtkSequencePass();
  virtual ~vtkSequencePass();

private:
  vtkSequencePass(const vtkSequencePass&) VTK_DELETE_FUNCTION;
  void operator=(const vtkSequencePass&) VTK_DELETE_FUNCTION;
};

#endif
