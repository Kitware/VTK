// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkAnariDeviceManager
 * @brief   base class to objects which create + manage a ANARI library + device
 *
 * Multiple VTK-ANARI objects are potentially responsible for creating and
 * managing ANARI libraries and devices, so this base class consolidates the
 * common functionality between them.
 *
 */

#ifndef vtkAnariDeviceManager_h
#define vtkAnariDeviceManager_h

#include "vtkObject.h"
#include "vtkRenderingAnariModule.h" // For export macro

#include <anari/anari_cpp.hpp> // for ANARI handles

#include <functional>

VTK_ABI_NAMESPACE_BEGIN

class vtkAnariDeviceManagerInternals;

class VTKRENDERINGANARI_EXPORT vtkAnariDeviceManager : public vtkObject
{
public:
  using OnNewDeviceCallback = std::function<void(anari::Device)>;

  static vtkAnariDeviceManager* New();
  vtkTypeMacro(vtkAnariDeviceManager, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Setup the trace directory and trace mode strings for the debug device for
   * when SetupAnariDeviceFromLibrary() is called. Once the Anari device is
   * created, this method will have no effect.
   */
  void SetAnariDebugConfig(const char* traceDir, const char* traceMode);

  /**
   * Initialize this vtkAnariDeviceManager from the name of an anari::Library
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
  anari::Device GetAnariDevice() const;

  /**
   * Get the current ANARI device extensions, which will be empty if not yet setup
   */
  const anari::Extensions& GetAnariDeviceExtensions() const;

  /**
   * Set a callback that gets called whenever a new device has been created
   */
  void SetOnNewDeviceCallback(OnNewDeviceCallback&& cb);

protected:
  /**
   * Default constructor.
   */
  vtkAnariDeviceManager();

  /**
   * Destructor.
   */
  virtual ~vtkAnariDeviceManager();

private:
  vtkAnariDeviceManager(const vtkAnariDeviceManager&) = delete;
  void operator=(const vtkAnariDeviceManager&) = delete;

  vtkAnariDeviceManagerInternals* Internal{ nullptr };
};

VTK_ABI_NAMESPACE_END
#endif
