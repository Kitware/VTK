// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkAnariDeviceManager
 * @brief   base class to objects which create + manage ANARI libraries + devices
 *
 * Multiple VTK-ANARI objects are potentially are responsible for creating and
 * managing ANARI libraries and devices, so this base class consolidates the
 * common functionality between them.
 *
 */

#ifndef vtkAnariDeviceManager_h
#define vtkAnariDeviceManager_h

#include "vtkRenderingAnariModule.h" // For export macro

#include <anari/anari_cpp.hpp> // for external getter/setters

VTK_ABI_NAMESPACE_BEGIN

class vtkAnariDeviceManagerInternals;

class VTKRENDERINGANARI_EXPORT vtkAnariDeviceManager
{
public:
  /**
   * Setup the trace directory and trace mode strings for the debug device for
   * when SetupAnariDeviceFromLibrary() is called. Once the Anari device is
   * created, this method will have no effect.
   */
  void SetAnariDebugConfig(const char* traceDir, const char* traceMode);

  /**
   * Initialize this vtkAnariDeviceManager from the name of an anari::Library and anari::Device
   * to be loaded. This initialization will use whatever debug configuration set
   * by SetupAnariDebugConfig() prior to this function when 'enableDebugLayer' is true.
   */
  void SetupAnariDeviceFromLibrary(
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

  friend class vtkAnariDeviceManagerInternals;
  vtkAnariDeviceManagerInternals* Internal{ nullptr };
};

VTK_ABI_NAMESPACE_END
#endif
