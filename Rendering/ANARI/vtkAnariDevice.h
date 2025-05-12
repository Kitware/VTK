// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkAnariDevice
 * @brief   base class to objects which create + manage a ANARI library + device
 *
 * Multiple VTK-ANARI objects are potentially responsible for creating and
 * managing ANARI libraries and devices, so this base class consolidates the
 * common functionality between them.
 *
 */

#ifndef vtkAnariDevice_h
#define vtkAnariDevice_h

#include "vtkObject.h"
#include "vtkRenderingAnariModule.h" // For export macro

#include <anari/anari_cpp.hpp> // for ANARI handles

#include <functional> // for std::function

VTK_ABI_NAMESPACE_BEGIN

class vtkAnariDeviceInternals;

class VTKRENDERINGANARI_EXPORT vtkAnariDevice : public vtkObject
{
public:
  using OnNewDeviceCallback = std::function<void(anari::Device)>;

  static vtkAnariDevice* New();
  vtkTypeMacro(vtkAnariDevice, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Setup the trace directory and trace mode strings for the debug device for
   * when SetupAnariDeviceFromLibrary() is called. Once the Anari device is
   * created, this method will have no effect.
   */
  void SetAnariDebugConfig(const char* traceDir, const char* traceMode);

  /**
   * Initialize this vtkAnariDevice from the name of an anari::Library
   * and anari::Device to be loaded. This initialization will use whatever debug
   * configuration set by SetupAnariDebugConfig() prior to this function when
   * 'enableDebugLayer' is true. Returns success of getting everything setup.
   */
  bool SetupAnariDeviceFromLibrary(
    const char* libraryName, const char* deviceName, bool enableDebugLayer = false);

  /**
   * Check if ANARI has been initialized with SetupAnariDeviceFromLibrary
   */
  bool AnariInitialized() const;

  /**
   * Get the current ANARI device, which will be NULL if not yet setup
   */
  anari::Device GetHandle() const;

  /**
   * Get the current ANARI device extensions, which will be empty if not yet setup
   */
  const anari::Extensions& GetAnariDeviceExtensions() const;

  /**
   * Get the current ANARI device extensions as list of strings
   */
  const char* const* GetAnariDeviceExtensionStrings() const;

  /**
   * Set a callback that gets called whenever a new device has been created
   */
  void SetOnNewDeviceCallback(OnNewDeviceCallback&& cb);

  //@{
  /**
   * Methods to set/commit generic parameteters on the underlying
   * anari::Renderer object.  These are primarily to support setting parameters
   * from Python -- C++ users can also use the ANARI API directly by using
   * anari::setParameter() and anari::commitParameters() directly as it is
   * equivalent.
   */
  void SetParameterc(const char* param, char*);
  void SetParameterb(const char* param, bool);
  void SetParameteri(const char* param, int);
  void SetParameter2i(const char* param, int, int);
  void SetParameter3i(const char* param, int, int, int);
  void SetParameter4i(const char* param, int, int, int, int);
  void SetParameterf(const char* param, float);
  void SetParameter2f(const char* param, float, float);
  void SetParameter3f(const char* param, float, float, float);
  void SetParameter4f(const char* param, float, float, float, float);
  void SetParameterd(const char* param, double);
  void CommitParameters();
  //@}

  /**
   * Get the anari library name
   */
  std::string& GetAnariLibraryName() const;

  /**
   * Get the anari device name
   */
  std::string& GetAnariDeviceName() const;

protected:
  /**
   * Default constructor.
   */
  vtkAnariDevice();

  /**
   * Destructor.
   */
  ~vtkAnariDevice() override;

private:
  vtkAnariDevice(const vtkAnariDevice&) = delete;
  void operator=(const vtkAnariDevice&) = delete;

  vtkAnariDeviceInternals* Internal{ nullptr };
};

VTK_ABI_NAMESPACE_END
#endif
