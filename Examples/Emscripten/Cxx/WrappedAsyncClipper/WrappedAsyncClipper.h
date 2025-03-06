#pragma once

#include <emscripten/bind.h>
#include <emscripten/proxying.h>
#include <emscripten/val.h>

#include <pthread.h>

#include "vtkCallbackCommand.h"
#include "vtkImplicitPlaneWidget2.h"
#include "vtkPlane.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSmartPointer.h"
#include "vtkTableBasedClipDataSet.h"

class WrappedAsyncClipper
{
public:
  WrappedAsyncClipper(std::string canvasId);
  ~WrappedAsyncClipper();

  // Abort the clip filter.
  void Abort();

  // Reset the abort flag for the clip filter.
  void ResetAbortFlag();

  // Update the x,y,z components of the clip plane's normal vector.
  void UpdateClipPlaneNormal(double nx, double ny, double nz);

  // Add a callback to listen to the clip plane modification event.
  // The three double values are the components of the normal vector.
  // The intention is to communicate the change in plane normal to the
  // HTML sliders. This occurs when user interacts with the 3D plane widget
  // in the vtkRenderer.
  // Note: DOM access is only allowed on main UI thread, so we will take care of
  // proxying the callback function onto the main UI thread.
  typedef void (*ClipPlaneModifiedCallbackType)(double, double, double);
  void AddClipPlaneModifiedUIObserver(ClipPlaneModifiedCallbackType callback);

  // Runs vtkRenderWindow::Render on the VTK render thread and waits for
  // completion.
  void SyncRender();
  // This is a fire-and-forget version of the above method.
  void AsyncRender();

  // Call this function from the browser's UI thread.
  // This function will create a brand new thread (WebWorker)
  // and run `StartRendering` on that thread.
  // It also transfers the canvas passed during construction
  // to offscreen so that the web worker can make OpenGL calls to the canvas.
  int Start();

private:
  // Creates a VTK pipeline for clipping some generated mesh.
  // It also constructs a plane widget for interactive clipping.
  static void* StartRendering(void* userdata);

  void OnClipPlaneInteraction(vtkObject* caller, unsigned long, void*);

  // The ID of a canvas in DOM which will be used by VTK.
  // The canvas is transferred offscreen so that the render thread can draw
  // into it without blocking main browser UI thread.
  // Note: This must be a string that begins with "#".
  // Examples: "#canvas", "#display", etc.
  std::string m_CanvasId;
  // Set to true after application event loop has started.
  // Typically happens sometime after `Start` gets invoked.
  // This flag also prevents `Start` from running ever again.
  std::atomic<bool> m_Started;
  // The queue used to send work to main thread or VTK rendering thread.
  emscripten::ProxyingQueue m_Queue;
  // Render thread id
  pthread_t m_RenderThread;
  // Main UI thread id
  pthread_t m_UIThread;
  // ClipPlane modification observer tag
  int m_ClipPlaneObserverTag = -1;
  // VTK render window interactor
  vtkSmartPointer<vtkRenderWindowInteractor> m_Interactor;
  // VTK render window
  vtkSmartPointer<vtkRenderWindow> m_RenderWindow;
  // VTK clip plane
  vtkSmartPointer<vtkPlane> m_ClipPlane;
  // VTK clip filter
  vtkSmartPointer<vtkTableBasedClipDataSet> m_Clipper;
  // VTK plane widget
  vtkSmartPointer<vtkImplicitPlaneWidget2> m_PlaneWidget;
  // VTK callback command
  vtkSmartPointer<vtkCallbackCommand> m_ClipPlaneCmd;
};

EMSCRIPTEN_BINDINGS(WrappedAsyncClipperBindings)
{
  emscripten::class_<WrappedAsyncClipper>("WrappedAsyncClipper")
    .constructor<std::string>()
    .function("Abort", &WrappedAsyncClipper::Abort)
    .function("ResetAbortFlag", &WrappedAsyncClipper::ResetAbortFlag)
    .function("UpdateClipPlaneNormal", &WrappedAsyncClipper::UpdateClipPlaneNormal)
    .function("AddClipPlaneModifiedUIObserver",
      emscripten::optional_override(
        [](WrappedAsyncClipper& self, emscripten::val jsFunc)
        {
          // the `jsFunc` must be a javascript function that takes 3 doubles and returns nothing.
          // Here, we dynamically "import" the javascript function into the wasm table by using
          // an emscripten library function called "addFunction". That must be in the list of
          // EXPORTED_RUNTIME_METHODS in the link flags. See CMakeLists.txt.
          int fp =
            emscripten::val::module_property("addFunction")(jsFunc, std::string("vddd")).as<int>();
          auto callback = reinterpret_cast<WrappedAsyncClipper::ClipPlaneModifiedCallbackType>(fp);
          self.AddClipPlaneModifiedUIObserver(callback);
        }))
    .function("SyncRender", &WrappedAsyncClipper::SyncRender)
    .function("AsyncRender", &WrappedAsyncClipper::AsyncRender)
    .function("Start", &WrappedAsyncClipper::Start);
}
