// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkSynchronizableActors
 * @brief   abstract base class for synchronizing a collection of actors
 *
 * vtkSynchronizableActors is an abstract base class for communicating
 * details about a collection of actors among a set of vtkRenderer
 * instances doing cooperative rendering in a tile-display or CAVE
 * environment.
 *
 * @sa
 * vtkSynchronizedRenderers vtkSynchronizableOpenGLAvatars
 */

#ifndef vtkSynchronizableActors_h
#define vtkSynchronizableActors_h

#include "vtkMultiProcessStream.h" // For vtkMultiProcessStream param by ref
#include "vtkObject.h"
#include "vtkRenderingParallelModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class vtkRenderer;

class VTKRENDERINGPARALLEL_EXPORT vtkSynchronizableActors : public vtkObject
{
public:
  ///@{
  /**
   * Standard type and print methods
   */
  vtkTypeMacro(vtkSynchronizableActors, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  ///@}

  ///@{
  /**
   * Perform any necessary initialization tasks with the vtkRenderer.
   */
  virtual void InitializeRenderer(vtkRenderer* ren) = 0;
  ///@}

  ///@{
  /**
   * Perform any necessary cleanup tasks with the vtkRenderer.
   */
  virtual void CleanUpRenderer(vtkRenderer* ren) = 0;
  ///@}

  ///@{
  /**
   * Identify target actors added to the vtkRenderer, save them to the stream.
   */
  virtual void SaveToStream(vtkMultiProcessStream& stream, vtkRenderer* ren) = 0;
  ///@}

  ///@{
  /**
   * Read actor information from the stream, update actors already added to
   * the vtkRenderer.  Possibly create actors and add them to the renderer,
   * or remove actors that are no longer needed.
   */
  virtual void RestoreFromStream(vtkMultiProcessStream& stream, vtkRenderer* ren) = 0;
  ///@}

protected:
  vtkSynchronizableActors();
  ~vtkSynchronizableActors() override;

private:
  vtkSynchronizableActors(const vtkSynchronizableActors&) = delete;
  void operator=(const vtkSynchronizableActors&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
