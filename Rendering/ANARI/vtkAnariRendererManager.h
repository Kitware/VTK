// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkAnariRendererManager
 * @brief   base class to objects which create + manage an ANARI library, device, and renderer
 *
 * This class extends vtkAnariDeviceManager to also manage an instance of an
 * ANARI renderer object, as well as being able to query what renderer subtypes
 * are available and setting parameters on the object. Note that applications which
 * set any 'background' parameters on the handle directly will conflict with
 * vtkAnariRendererNode setting it to whatever the vtkRenderer values are.
 *
 */

#ifndef vtkAnariRendererManager_h
#define vtkAnariRendererManager_h

#include "vtkAnariDeviceManager.h"
#include "vtkRenderingAnariModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN

class vtkAnariRendererManagerInternals;

class VTKRENDERINGANARI_EXPORT vtkAnariRendererManager : public vtkAnariDeviceManager
{
public:
  /**
   * Set the underlying subtype of the anari::Renderer. When a different subtype
   * is passed from what was already in-use, a new anari::Renderer handle will
   * be created and will not keep any parameter values set on the previous
   * anari::Renderer. Calling this without having a setup anari::Device will
   * cause a default setup of the anari::Device to be done using the
   * 'environment' device.
   */
  void SetAnariRendererSubtype(const char* subtype = "default");

  /**
   * Get the subtype of the current underlying anari::Renderer. Empty if not
   * yet set.
   */
  const char* GetAnariRendererSubtype() const;

  //@{
  /**
   * Methods to set/commit generic parameteters on the underlying
   * anari::Renderer object.  These are primarily to support setting parameters
   * from Python -- C++ users can also use the ANARI API directly by using
   * anari::setParameter() and anari::commitParameters() directly as it is
   * equivalent.
   */
  void SetAnariRendererParameter(const char* param, bool);
  void SetAnariRendererParameter(const char* param, int);
  void SetAnariRendererParameter(const char* param, int, int);
  void SetAnariRendererParameter(const char* param, int, int, int);
  void SetAnariRendererParameter(const char* param, int, int, int, int);
  void SetAnariRendererParameter(const char* param, float);
  void SetAnariRendererParameter(const char* param, float, float);
  void SetAnariRendererParameter(const char* param, float, float, float);
  void SetAnariRendererParameter(const char* param, float, float, float, float);
  //@}

  /**
   * Get the current ANARI renderer, which will be NULL if not yet setup
   */
  anari::Renderer GetAnariRenderer() const;

protected:
  /**
   * Default constructor.
   */
  vtkAnariRendererManager();

  /**
   * Destructor.
   */
  virtual ~vtkAnariRendererManager();

  /**
   * Respond to a new device being set as the device in-use.
   */
  void OnNewDevice() override;

  /**
   * Signal child classes that a new renderer was created so they can respond
   * accordingly (e.g. release old handles). This only gets called when
   * SetAnariRendererSubtype() causes a new renderer to get created.
   */
  virtual void OnNewRenderer();

private:
  void CheckAnariDeviceInitialized();

  vtkAnariRendererManager(const vtkAnariRendererManager&) = delete;
  void operator=(const vtkAnariRendererManager&) = delete;

  friend class vtkAnariRendererManagerInternals;
  vtkAnariRendererManagerInternals* Internal{ nullptr };
};

VTK_ABI_NAMESPACE_END
#endif
