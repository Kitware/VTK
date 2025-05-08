// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkWasmSceneManager
 * @brief   vtkWasmSceneManager provides additional functionality that relates to a vtkRenderWindow
 *          and user interaction.
 *
 * `vtkWasmSceneManager` is a javascript wrapper of `vtkSceneManager` for managing VTK
 * objects, specifically designed for webassembly (wasm). It extends
 * functionality of `vtkObjectManager` for managing objects such as `vtkRenderWindow`,
 * `vtkRenderWindowInteractor` and enables event-observers in webassembly
 * visualization applications.
 *
 * @sa vtkObjectManager
 */
#ifndef vtkWasmSceneManager_h
#define vtkWasmSceneManager_h

#include "vtkObjectManager.h"

#include "vtkSerializationManagerModule.h" // for export macro

VTK_ABI_NAMESPACE_BEGIN

class VTKSERIALIZATIONMANAGER_EXPORT vtkWasmSceneManager : public vtkObjectManager
{
public:
  static vtkWasmSceneManager* New();
  vtkTypeMacro(vtkWasmSceneManager, vtkObjectManager);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  bool Initialize() override;

  /**
   * Set the size of the `vtkRenderWindow` object at `identifier` to
   * the supplied dimensions.
   *
   * Returns `true` if the object at `identifier` is a `vtkRenderWindow`
   * with a `vtkRenderWindowInteractor` attached to it,
   * `false` otherwise.
   */
  bool SetSize(vtkTypeUInt32 identifier, int width, int height);

  /**
   * Render the `vtkRenderWindow` object at `identifier`.
   *
   * Returns `true` if the object at `identifier` is a `vtkRenderWindow`
   * `false` otherwise.
   */
  bool Render(vtkTypeUInt32 identifier);

  /**
   * Reset the active camera of the `vtkRenderer` object at `identifier`.
   *
   * Returns `true` if the object at `identifier` is a `vtkRenderer`
   * `false` otherwise.
   */
  bool ResetCamera(vtkTypeUInt32 identifier);

  /**
   * Start event loop of the `vtkRenderWindowInteractor` object at `identifier`.
   *
   * Returns `true` if the object at `identifier` is a `vtkRenderWindowInteractor`
   * `false` otherwise.
   */
  bool StartEventLoop(vtkTypeUInt32 identifier);

  /**
   * Stop event loop of the `vtkRenderWindowInteractor` object at `identifier`.
   *
   * Returns `true` if the object at `identifier` is a `vtkRenderWindowInteractor`
   * `false` otherwise.
   */
  bool StopEventLoop(vtkTypeUInt32 identifier);

  typedef void (*ObserverCallbackF)(vtkTypeUInt32, const char*);

  /**
   * Observes `eventName` event emitted by an object registered at `identifier`
   * and invokes `callback` with the `identifier` and `eventName` for every such emission.
   *
   * Returns the tag of an observer for `eventName`. You can use the tag in `RemoveObserver`
   * to stop observing `eventName` event from the object at `identifier`
   */
  unsigned long AddObserver(
    vtkTypeUInt32 identifier, std::string eventName, ObserverCallbackF callback);

  /**
   * Stop observing the object at `identifier`.
   * Returns `true` if an object exists at `identifier`,
   * `false` otherwise.
   */
  bool RemoveObserver(vtkTypeUInt32 identifier, unsigned long tag);

  /**
   * Bind a `vtkRenderWindow` object at `renderWindowIdentifier` to a canvas element with the
   * specified `canvasSelector`. This allows the `vtkRenderWindow` to render its content onto the
   * specified HTML canvas element in a web application.
   *
   * @param renderWindowIdentifier The identifier of the `vtkRenderWindow` object to bind.
   * @param canvasSelector The ID of the HTML canvas element to bind the `vtkRenderWindow` to.
   */
  bool BindRenderWindow(vtkTypeUInt32 renderWindowIdentifier, const char* canvasSelector);

protected:
  vtkWasmSceneManager();
  ~vtkWasmSceneManager() override;

private:
  vtkWasmSceneManager(const vtkWasmSceneManager&) = delete;
  void operator=(const vtkWasmSceneManager&) = delete;
};
VTK_ABI_NAMESPACE_END
#endif
