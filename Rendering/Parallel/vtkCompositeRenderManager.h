// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkCompositeRenderManager
 * @brief   An object to control sort-last parallel rendering.
 *
 *
 * vtkCompositeRenderManager is a subclass of vtkParallelRenderManager that
 * uses compositing to do parallel rendering.  This class has
 * replaced vtkCompositeManager.
 *
 */

#ifndef vtkCompositeRenderManager_h
#define vtkCompositeRenderManager_h

#include "vtkParallelRenderManager.h"
#include "vtkRenderingParallelModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class vtkCompositer;
class vtkFloatArray;

class VTKRENDERINGPARALLEL_EXPORT vtkCompositeRenderManager : public vtkParallelRenderManager
{
public:
  vtkTypeMacro(vtkCompositeRenderManager, vtkParallelRenderManager);
  static vtkCompositeRenderManager* New();
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Set/Get the composite algorithm.
   */
  void SetCompositer(vtkCompositer* c);
  vtkGetObjectMacro(Compositer, vtkCompositer);
  ///@}

protected:
  vtkCompositeRenderManager();
  ~vtkCompositeRenderManager() override;

  vtkCompositer* Compositer;

  void PreRenderProcessing() override;
  void PostRenderProcessing() override;

  vtkFloatArray* DepthData;
  vtkUnsignedCharArray* TmpPixelData;
  vtkFloatArray* TmpDepthData;

  int SavedMultiSamplesSetting;

private:
  vtkCompositeRenderManager(const vtkCompositeRenderManager&) = delete;
  void operator=(const vtkCompositeRenderManager&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif // vtkCompositeRenderManager_h
