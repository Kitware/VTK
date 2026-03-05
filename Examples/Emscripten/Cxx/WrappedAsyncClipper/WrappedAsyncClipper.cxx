
#include "WrappedAsyncClipper.h"

#include "vtkCamera.h"
#include "vtkCellTypeSource.h"
#include "vtkDataSetMapper.h"
#include "vtkImplicitPlaneRepresentation.h"
#include "vtkImplicitPlaneWidget2.h"
#include "vtkInteractorStyleSwitch.h"
#include "vtkPlane.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRenderer.h"
#include "vtkSMPTools.h"

#include <array>
#include <map>

#include <emscripten/eventloop.h>
#include <emscripten/stack.h>
#include <emscripten/threading.h>

#include <iostream>

#define LOG(msg)                                                                                   \
  do                                                                                               \
  {                                                                                                \
    if (::threadNames.find(pthread_self()) != ::threadNames.end())                                 \
    {                                                                                              \
      std::cout << "[" << ::threadNames[pthread_self()] << "] " << msg << '\n';                    \
    }                                                                                              \
    else                                                                                           \
    {                                                                                              \
      std::cout << "[0x" << std::hex << pthread_self() << std::dec << "] " << msg << '\n';         \
    }                                                                                              \
  } while (0)

namespace
{
// emscripten has a function to set thread name, but does not implement
// pthread_getname_np! So we keep track of thread names in a map.
std::map<pthread_t, std::string> threadNames;
} // namespace

//---------------------------------------------------------------------
WrappedAsyncClipper::WrappedAsyncClipper(std::string canvasId)
  : m_CanvasId(canvasId)
  , m_Started(false)
{
}

//---------------------------------------------------------------------
WrappedAsyncClipper::~WrappedAsyncClipper() = default;

//---------------------------------------------------------------------
void WrappedAsyncClipper::Abort()
{
  LOG(__func__);
  if (this->m_SurfaceFilter != nullptr)
  {
    this->m_SurfaceFilter->SetAbortExecuteAndUpdateTime();
  }
  if (this->m_ClipMapper != nullptr)
  {
    // clear mapper input to skip any render requests due to the abort flag being set on the surface
    // filter.
    this->m_ClipMapper->SetInputDataObject(0, nullptr);
  }
}

//---------------------------------------------------------------------
void WrappedAsyncClipper::ResetAbortFlag()
{
  LOG(__func__);
  if (this->m_SurfaceFilter != nullptr)
  {
    if (this->m_SurfaceFilter->GetAbortExecute())
    {
      this->m_SurfaceFilter->SetAbortExecute(false);
    }
    // run the surface filter
    this->m_SurfaceFilter->Update();
    if (this->m_ClipMapper != nullptr)
    {
      // restore the mapper's input after the surface filter has been updated.
      this->m_ClipMapper->SetInputDataObject(0, this->m_SurfaceFilter->GetOutputDataObject(0));
    }
  }
}

//---------------------------------------------------------------------
void WrappedAsyncClipper::UpdateClipPlaneNormal(double nx, double ny, double nz)
{
  LOG(__func__);
  if (this->m_ClipPlane != nullptr)
  {
    this->m_ClipPlane->SetNormal(nx, ny, nz);
  }
  // Also reorient the plane widget
  if (this->m_PlaneWidget != nullptr)
  {
    auto* rep =
      reinterpret_cast<vtkImplicitPlaneRepresentation*>(this->m_PlaneWidget->GetRepresentation());
    rep->SetPlane(this->m_ClipPlane);
  }
}

//---------------------------------------------------------------------
void WrappedAsyncClipper::SyncRender()
{
  LOG(__func__);
  if (this->m_RenderWindow != nullptr)
  {
    this->m_Queue.proxySync(this->m_RenderThread,
      [this]()
      {
        LOG("WrappedAsyncClipper::RenderWindow::Render");
        this->m_RenderWindow->Render();
      });
  }
}

//---------------------------------------------------------------------
void WrappedAsyncClipper::AsyncRender()
{
  LOG(__func__);
  if (this->m_RenderWindow != nullptr)
  {
    this->m_Queue.proxyAsync(this->m_RenderThread,
      [this]()
      {
        LOG("vtkRenderWindow::Render");
        this->m_RenderWindow->Render();
      });
  }
}

//---------------------------------------------------------------------
int WrappedAsyncClipper::Start()
{
  LOG(__func__);
  if (this->m_Started)
  {
    return 0;
  }
  // This is the main thread for the application.
  this->m_UIThread = pthread_self();
  ::threadNames[this->m_UIThread] = "ui-thread";
  // For thread profiler
  emscripten_set_thread_name(pthread_self(), "ui-thread");

  // Enable SMP after https://gitlab.kitware.com/vtk/vtk/-/issues/19424 is resolved
  // const int numThreadsAvailable = vtkSMPTools::GetEstimatedDefaultNumberOfThreads();
  // vtkSMPTools::Initialize(numThreadsAvailable - 1); // leave one for rendering.
  // std::cout << "VTK SMP Backend: " << vtkSMPTools::GetBackend() << " with "
  //           << vtkSMPTools::GetEstimatedNumberOfThreads() << " thread (s)\n";

  pthread_attr_t attr;
  pthread_attr_init(&attr);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
  // Use the size of the current stack, which is the normal size of the stack
  // that main thread gets.
  pthread_attr_setstacksize(&attr, emscripten_stack_get_base() - emscripten_stack_get_end());
  if (!this->m_CanvasId.empty())
  {
    LOG("Transferring canvas " << this->m_CanvasId << " offscreen");
    emscripten_pthread_attr_settransferredcanvases(&attr, this->m_CanvasId.c_str());
  }
  else
  {
    // Pass special ID -1 to the list of transferred canvases to denote that the
    // thread creation should instead take a list of canvases that are specified
    // from the command line with -sOFFSCREENCANVASES_TO_PTHREAD linker flag.
    emscripten_pthread_attr_settransferredcanvases(&attr, (const char*)-1);
  }
  int rc = pthread_create(&this->m_RenderThread, &attr, WrappedAsyncClipper::StartRendering, this);
  pthread_attr_destroy(&attr);

  this->m_Started = true;
  return rc;
}

//---------------------------------------------------------------------
void* WrappedAsyncClipper::StartRendering(void* userdata)
{
  LOG(__func__);
  // This is the rendering thread for the application.
  threadNames[pthread_self()] = "render-thread";
  // For thread profiler
  emscripten_set_thread_name(pthread_self(), "render-thread");
  vtkRenderWindowInteractor::InteractorManagesTheEventLoop = false;

  auto* self = reinterpret_cast<WrappedAsyncClipper*>(userdata);

  LOG("Starting event loop");
  self->m_RenderWindow = vtk::TakeSmartPointer(vtkRenderWindow::New());
  self->m_Interactor = vtk::TakeSmartPointer(vtkRenderWindowInteractor::New());
  self->m_Interactor->SetRenderWindow(self->m_RenderWindow);
  auto* istyleSwitch =
    vtkInteractorStyleSwitch::SafeDownCast(self->m_Interactor->GetInteractorStyle());
  istyleSwitch->SetCurrentStyleToTrackballCamera();
  self->m_Interactor->Start();

  LOG("Resizing to window");
  self->m_Queue.proxyAsync(
    self->m_UIThread, []() { EM_ASM(window.dispatchEvent(new Event('resize'))); });

  LOG("Setting up pipeline");
  // Create pipeline
  std::array<double, 6> bounds = { 0, 200, 0, 200, 0, 200 };
  std::array<double, 3> origin;
  for (int i = 0; i < 3; ++i)
  {
    origin[i] = 0.5 * (bounds[i * 2] + bounds[i * 2 + 1]);
  }
  vtkNew<vtkCellTypeSource> ugridSource;
  ugridSource->SetCellType(VTK_HEXAHEDRON);
  ugridSource->SetBlocksDimensions(bounds[1], bounds[3], bounds[5]);
  vtkNew<vtkDataSetMapper> ugridMapper;
  ugridMapper->SetInputConnection(ugridSource->GetOutputPort());
  vtkNew<vtkActor> ugridActor;
  ugridActor->SetMapper(ugridMapper);
  ugridActor->GetProperty()->SetOpacity(0.3);

  self->m_Clipper = vtk::TakeSmartPointer(vtkTableBasedClipDataSet::New());
  self->m_ClipPlane = vtk::TakeSmartPointer(vtkPlane::New());
  self->m_ClipPlane->SetOrigin(origin.data());
  self->m_Clipper->SetClipFunction(self->m_ClipPlane);
  self->m_Clipper->SetInputConnection(ugridSource->GetOutputPort());

  self->m_SurfaceFilter = vtk::TakeSmartPointer(vtkDataSetSurfaceFilter::New());
  self->m_SurfaceFilter->SetInputConnection(self->m_Clipper->GetOutputPort());

  vtkMapper::SetResolveCoincidentTopologyToPolygonOffset();
  self->m_ClipMapper = vtk::TakeSmartPointer(vtkPolyDataMapper::New());
  self->m_ClipMapper->SetRelativeCoincidentTopologyPolygonOffsetParameters(1, 1);
  self->m_ClipMapper->SetInputConnection(self->m_SurfaceFilter->GetOutputPort());

  vtkNew<vtkActor> clippedActor;
  clippedActor->SetMapper(self->m_ClipMapper);
  clippedActor->GetProperty()->SetEdgeVisibility(true);
  clippedActor->GetProperty()->SetEdgeColor(0, 0, 1);

  vtkNew<vtkRenderer> renderer;
  renderer->AddActor(clippedActor);
  renderer->AddActor(ugridActor);
  self->m_RenderWindow->AddRenderer(renderer);

  vtkNew<vtkImplicitPlaneRepresentation> planeWidgetRep;
  planeWidgetRep->SetPlaceFactor(1.25);
  planeWidgetRep->PlaceWidget(bounds.data());
  planeWidgetRep->SetPlane(self->m_ClipPlane);
  planeWidgetRep->SetDrawOutline(false);

  self->m_PlaneWidget = vtk::TakeSmartPointer(vtkImplicitPlaneWidget2::New());
  self->m_PlaneWidget->SetInteractor(self->m_Interactor);
  self->m_PlaneWidget->SetRepresentation(planeWidgetRep);
  self->m_PlaneWidget->AddObserver(
    vtkCommand::StartInteractionEvent, self, &WrappedAsyncClipper::OnClipPlaneInteractionStarted);
  self->m_PlaneWidget->AddObserver(
    vtkCommand::EndInteractionEvent, self, &WrappedAsyncClipper::OnClipPlaneInteractionEnded);

  if (self->m_ClipPlaneCmd)
  {
    self->m_ClipPlaneObserverTag =
      self->m_ClipPlane->AddObserver(vtkCommand::ModifiedEvent, self->m_ClipPlaneCmd);
  }

  LOG("Rendering for the first time, please wait...");
  self->m_PlaneWidget->On();
  renderer->GetActiveCamera()->Azimuth(-60);
  renderer->GetActiveCamera()->Elevation(30);
  renderer->ResetCamera();
  renderer->GetActiveCamera()->Zoom(0.75);
  self->m_RenderWindow->Render();

  return nullptr;
}

namespace
{
struct CallbackOnThreadBridge
{
  emscripten::ProxyingQueue* Queue;
  pthread_t* Target;
  WrappedAsyncClipper::ClipPlaneModifiedCallbackType Call;
};
} // namespace

//---------------------------------------------------------------------
void WrappedAsyncClipper::AddClipPlaneModifiedUIObserver(ClipPlaneModifiedCallbackType callback)
{
  LOG(__func__);

  auto* bridge = new CallbackOnThreadBridge();
  bridge->Call = callback;
  bridge->Target = &(this->m_UIThread);
  bridge->Queue = &(this->m_Queue);

  this->m_ClipPlaneCmd = vtk::TakeSmartPointer(vtkCallbackCommand::New());
  this->m_ClipPlaneCmd->SetClientData(bridge);
  this->m_ClipPlaneCmd->SetClientDataDeleteCallback(
    [](void* clientdata)
    {
      if (auto* _bridge = reinterpret_cast<CallbackOnThreadBridge*>(clientdata))
      {
        delete _bridge;
      }
    });
  this->m_ClipPlaneCmd->SetCallback(
    [](vtkObject* caller, unsigned long, void* clientdata, void*)
    {
      if (auto* _bridge = reinterpret_cast<CallbackOnThreadBridge*>(clientdata))
      {
        auto* plane = reinterpret_cast<vtkPlane*>(caller);
        _bridge->Queue->proxyAsync(*(_bridge->Target),
          [plane, f = _bridge->Call]()
          {
            double n[3] = {};
            plane->GetNormal(n);
            f(n[0], n[1], n[2]);
          });
      }
    });
}

//---------------------------------------------------------------------
void WrappedAsyncClipper::OnClipPlaneInteractionStarted(vtkObject* caller, unsigned long, void*)
{
  LOG(__func__);
  if (this->m_ClipMapper != nullptr)
  {
    // remove the input geometry for the old clipped actor
    this->m_ClipMapper->SetInputDataObject(0, nullptr);
  }
}

//---------------------------------------------------------------------
void WrappedAsyncClipper::OnClipPlaneInteractionEnded(vtkObject* caller, unsigned long, void*)
{
  LOG(__func__);
  auto* planeWidget = reinterpret_cast<vtkImplicitPlaneWidget2*>(caller);
  auto* rep = reinterpret_cast<vtkImplicitPlaneRepresentation*>(planeWidget->GetRepresentation());
  rep->GetPlane(vtkPlane::SafeDownCast(this->m_Clipper->GetClipFunction()));
  this->ResetAbortFlag();
}
