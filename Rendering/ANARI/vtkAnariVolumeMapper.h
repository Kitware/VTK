// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkAnariVolumeMapper
 * @brief   Standalone ANARI VolumeMapper.
 *
 * This is a standalone interface for ANARI volume rendering to be used
 * within otherwise OpenGL rendering contexts such as within the
 * SmartVolumeMapper.
 *
 * @par Thanks:
 * Kevin Griffin kgriffin@nvidia.com for creating and contributing the class
 * and NVIDIA for supporting this work.
 */

#ifndef vtkAnariVolumeMapper_h
#define vtkAnariVolumeMapper_h

#include "vtkAnariVolumeInterface.h"
#include "vtkRenderingAnariModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN

class vtkAnariPass;
class vtkRenderer;
class vtkWindow;

class VTKRENDERINGANARI_EXPORT vtkAnariVolumeMapper : public vtkAnariVolumeInterface
{
public:
  static vtkAnariVolumeMapper* New();
  vtkTypeMacro(vtkAnariVolumeMapper, vtkAnariVolumeInterface);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Release any graphics resources that are being consumed by this mapper.
   * The parameter window could be used to determine which graphic
   * resources to release.
   */
  virtual void ReleaseGraphicsResources(vtkWindow*) override;

  // Initialize internal constructs
  virtual void Init();

  /**
   * Render the volume onto the screen.
   * Overridden to use ANARI to do the work.
   */
  virtual void Render(vtkRenderer*, vtkVolume*) override;

  /**
   * Allow vtkAnariRendererNode properties to be set on the internal vtkRenderer.
   */
  vtkRenderer* GetInternalRenderer() const { return this->InternalRenderer; }

  //@{
  /**
   * Set/Get whether ANARI has been initialized.
   * By default, Initialized is false.
   */
  vtkSetMacro(Initialized, bool);
  vtkGetMacro(Initialized, bool);
  vtkBooleanMacro(Initialized, bool);
  //@}

protected:
  vtkAnariVolumeMapper();
  ~vtkAnariVolumeMapper();

  vtkRenderer* InternalRenderer;
  vtkAnariPass* InternalAnariPass;
  bool Initialized;

private:
  vtkAnariVolumeMapper(const vtkAnariVolumeMapper&) = delete;
  void operator=(const vtkAnariVolumeMapper&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
