// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkSynchronizableAvatars
 * @brief   Serialize/deserialize collection of vtkOpenGLAvatars among renderers
 *
 * vtkSynchronizableAvatars is a specialization of vtkSerializableActors
 * for synchronizing a collection of vtkOpenGLAvatars among a group of
 * cooperative renderers.
 *
 * @sa
 * vtkSynchronizedRenderers
 */

#ifndef vtkSynchronizableAvatars_h
#define vtkSynchronizableAvatars_h

#include "vtkRenderingParallelModule.h" // For export macro
#include "vtkSynchronizableActors.h"

#include <memory> // for std::unique_ptr

VTK_ABI_NAMESPACE_BEGIN
class vtkRenderer;

class VTKRENDERINGPARALLEL_EXPORT vtkSynchronizableAvatars : public vtkSynchronizableActors
{
public:
  ///@{
  /**
   * Standard new, type, and print methods.
   */
  static vtkSynchronizableAvatars* New();
  vtkTypeMacro(vtkSynchronizableAvatars, vtkSynchronizableActors);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  ///@}

  ///@{
  /**
   * In immersive environments like CAVE, all actors must be visible, so this
   * method removes cullers from the renderer.
   */
  void InitializeRenderer(vtkRenderer* ren) override;
  ///@}

  ///@{
  /**
   * Removes any OpenGLAvatar instances from the renderer.
   */
  void CleanUpRenderer(vtkRenderer* ren) override;
  ///@}

  ///@{
  /**
   * Save to the stream any vtkOpenGLAvatar instances added to the renderer.
   */
  void SaveToStream(vtkMultiProcessStream& stream, vtkRenderer* ren) override;
  ///@}

  ///@{
  /**
   * Restore from the stream a collection of vtkOpenGLAvatars and update the
   * renderer.
   */
  void RestoreFromStream(vtkMultiProcessStream& stream, vtkRenderer* ren) override;
  ///@}

protected:
  vtkSynchronizableAvatars();
  ~vtkSynchronizableAvatars() override;

private:
  vtkSynchronizableAvatars(const vtkSynchronizableAvatars&) = delete;
  void operator=(const vtkSynchronizableAvatars&) = delete;

  class vtkInternals;
  std::unique_ptr<vtkInternals> Internal;
};

VTK_ABI_NAMESPACE_END
#endif
