// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkAnariRenderer
 * @brief   base class to objects which create + manage an ANARI library, device, and renderer
 *
 * This class extends vtkAnariDevice to also manage an instance of an
 * ANARI renderer object, as well as being able to query what renderer subtypes
 * are available and setting parameters on the object. Note that applications which
 * set any 'background' parameters on the handle directly will conflict with
 * vtkAnariRendererNode setting it to whatever the vtkRenderer values are.
 *
 */

#ifndef vtkAnariRenderer_h
#define vtkAnariRenderer_h

#include "vtkObject.h"
#include "vtkRenderingAnariModule.h" // For export macro

#include <anari/anari_cpp.hpp> // for ANARI handles

VTK_ABI_NAMESPACE_BEGIN

class vtkAnariRendererInternals;

class VTKRENDERINGANARI_EXPORT vtkAnariRenderer : public vtkObject
{
public:
  static vtkAnariRenderer* New();
  vtkTypeMacro(vtkAnariRenderer, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  void SetAnariDevice(anari::Device d);
  anari::Device GetAnariDevice() const;

  /**
   * Set the underlying subtype of the anari::Renderer. When a different subtype
   * is passed from what was already in-use, a new anari::Renderer handle will
   * be created and will not keep any parameter values set on the previous
   * anari::Renderer. Calling this without having a setup anari::Device will
   * cause a default setup of the anari::Device to be done using the
   * 'environment' device.
   */
  void SetSubtype(const char* subtype = "default");

  /**
   * Get the subtype of the current underlying anari::Renderer. Empty if not
   * yet set.
   */
  const char* GetSubtype() const;

  //@{
  /**
   * Methods to set/commit generic parameteters on the underlying
   * anari::Renderer object.  These are primarily to support setting parameters
   * from Python -- C++ users can also use the ANARI API directly by using
   * anari::setParameter() and anari::commitParameters() directly as it is
   * equivalent.
   */
  void SetParameterb(const char* param, bool);
  void SetParameteri(const char* param, int);
  void SetParameter2i(const char* param, int, int);
  void SetParameter3i(const char* param, int, int, int);
  void SetParameter4i(const char* param, int, int, int, int);
  void SetParameterf(const char* param, float);
  void SetParameter2f(const char* param, float, float);
  void SetParameter3f(const char* param, float, float, float);
  void SetParameter4f(const char* param, float, float, float, float);
  //@}

  /**
   * Get the current ANARI renderer, which will be NULL if not yet setup
   */
  anari::Renderer GetHandle() const;

protected:
  /**
   * Default constructor.
   */
  vtkAnariRenderer();

  /**
   * Destructor.
   */
  ~vtkAnariRenderer() override;

private:
  void CheckAnariDeviceInitialized();

  vtkAnariRenderer(const vtkAnariRenderer&) = delete;
  void operator=(const vtkAnariRenderer&) = delete;

  vtkAnariRendererInternals* Internal{ nullptr };
};

VTK_ABI_NAMESPACE_END
#endif
