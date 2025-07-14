// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "GeometryViewer.h"

#include <vtkActor.h>
#include <vtkCallbackCommand.h>
#include <vtkCamera.h>
#include <vtkCameraOrientationRepresentation.h>
#include <vtkCameraOrientationWidget.h>
#include <vtkCellArray.h>
#include <vtkCellData.h>
#include <vtkCharArray.h>
#include <vtkCollectionRange.h>
#include <vtkColorSeries.h>
#include <vtkColorTransferFunction.h>
#include <vtkCompositeDataSet.h>
#include <vtkCompositePolyDataMapper.h>
#include <vtkDataObjectTreeIterator.h>
#include <vtkDataObjectTreeRange.h>
#include <vtkGLTFReader.h>
#include <vtkGeometryFilter.h>
#include <vtkHardwarePicker.h>
#include <vtkIdList.h>
#include <vtkInteractorStyleSwitch.h>
#include <vtkInteractorStyleTrackballCamera.h>
#include <vtkMath.h>
#include <vtkMemoryResourceStream.h>
#include <vtkMinimalStandardRandomSequence.h>
#include <vtkNew.h>
#include <vtkOBJReader.h>
#include <vtkOpenGLLowMemoryPolyDataMapper.h>
#include <vtkPLYReader.h>
#include <vtkPNGWriter.h>
#include <vtkPointData.h>
#include <vtkPointPicker.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkPolyDataReader.h>
#include <vtkProperty.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderedAreaPicker.h>
#include <vtkRenderer.h>
#include <vtkRendererCollection.h>
#include <vtkSTLReader.h>
#include <vtkSelectionNode.h>
#include <vtkSetGet.h>
#include <vtkShaderProgram.h>
#include <vtkSmartPointer.h>
#include <vtkSphereSource.h>
#include <vtkTubeFilter.h>
#include <vtkType.h>
#include <vtkUnsignedCharArray.h>
#include <vtkWebAssemblyOpenGLRenderWindow.h>
#include <vtkWebAssemblyRenderWindowInteractor.h>
#include <vtkWebAssemblyWebGPURenderWindow.h>
#include <vtkWindowToImageFilter.h>
#include <vtkXMLMultiBlockDataReader.h>
#include <vtkXMLPartitionedDataSetCollectionReader.h>
#include <vtkXMLPolyDataReader.h>
#include <vtkXMLUnstructuredGridReader.h>
#include <vtksys/SystemTools.hxx>

#include <emscripten.h>

#include <array>
#include <cstddef>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_set>

extern "C"
{
  void sendMessageToTooltipAt(const char* message, size_t length, int x, int y);
}

namespace
{
struct HighlighterBridge
{
  std::string ActivePointColorArray;
  std::string ActiveCellColorArray;
  vtkSmartPointer<vtkRenderer> Renderer;
};

void hideTooltip()
{
  EM_ASM({
    let el = document.getElementsByClassName("tooltip").item(0);
    el.style.visibility = 'hidden';
  });
}

void HighlightPointUnderMouse(
  vtkObject* caller, unsigned long vtkNotUsed(eventId), void* clientData, void* callData)
{
  auto interactor = vtkRenderWindowInteractor::SafeDownCast(caller);
  auto data = reinterpret_cast<HighlighterBridge*>(clientData);
  int* pos = interactor->GetEventPosition();
  vtkNew<vtkPointPicker> picker;
  const int pickedSomething = picker->Pick(pos[0], pos[1], 0, data->Renderer);
  auto* pickedDataSet = picker->GetDataSet();
  if (!pickedSomething)
  {
    ::hideTooltip();
  }
  else if (picker->GetPointId() >= 0)
  {
    std::array<double, 3> xyz;
    pickedDataSet->GetPoint(picker->GetPointId(), xyz.data());
    std::ostringstream oss;
    // point id.
    oss << "Point Id: " << picker->GetPointId() << '|';
    // position.
    oss << "x: " << std::setprecision(5) << xyz[0] << '|';
    oss << "y: " << std::setprecision(5) << xyz[1] << '|';
    oss << "z: " << std::setprecision(5) << xyz[2] << '|';
    // point data arrays.
    if (!data->ActivePointColorArray.empty())
    {
      oss << data->ActivePointColorArray << ": ";
      auto arr = pickedDataSet->GetPointData()->GetArray(data->ActivePointColorArray.c_str());
      oss << '(';
      for (int i = 0; i < arr->GetNumberOfComponents(); ++i)
      {
        oss << arr->GetComponent(picker->GetPointId(), i);
        if (i < arr->GetNumberOfComponents() - 1)
        {
          oss << ',';
        }
      }
      oss << ')';
    }
    const std::string info = oss.str();
    sendMessageToTooltipAt(info.c_str(), info.length(), pos[0], pos[1]);
  }
}

void HighlightCellUnderMouse(
  vtkObject* caller, unsigned long vtkNotUsed(eventId), void* clientData, void* callData)
{
  auto interactor = vtkRenderWindowInteractor::SafeDownCast(caller);
  auto data = reinterpret_cast<HighlighterBridge*>(clientData);
  int* pos = interactor->GetEventPosition();

  vtkNew<vtkHardwarePicker> picker;
  const int pickedSomething = picker->Pick(pos[0], pos[1], 0, data->Renderer);
  if (!pickedSomething)
  {
    ::hideTooltip();
  }
  else if (picker->GetCellId() >= 0)
  {
    // get the picked dataset, maybe null
    auto* pickedDataSet = picker->GetDataSet();
    // get the picked composite dataset and the flat index of the picked block.
    auto* pickedCompositeDataSet = picker->GetCompositeDataSet();
    auto pickedBlockIndex = picker->GetFlatBlockIndex();
    // set the picked dataset to the block at the index which was picked.
    if (pickedBlockIndex >= 0 && pickedCompositeDataSet != nullptr)
    {
      pickedDataSet =
        vtkDataSet::SafeDownCast(pickedCompositeDataSet->GetDataSet(pickedBlockIndex));
    }
    std::ostringstream oss;
    // cell id.
    oss << "Cell Id: " << picker->GetCellId() << '|';
    // cell connectivity.
    if (auto* polydata = vtkPolyData::SafeDownCast(pickedDataSet))
    {
      const vtkIdType* pts = nullptr;
      vtkIdType npts = 0;
      polydata->GetCellPoints(picker->GetCellId(), npts, pts);
      oss << "Connectivity (";
      for (vtkIdType i = 0; i < npts; ++i)
      {
        oss << pts[i];
        if (i < npts - 1)
        {
          oss << ',';
        }
      }
      oss << ")|";
    }
    // cell data arrays.
    if (!data->ActiveCellColorArray.empty())
    {
      oss << data->ActiveCellColorArray << ": ";
      auto arr = pickedDataSet->GetCellData()->GetArray(data->ActiveCellColorArray.c_str());
      oss << '(';
      for (int i = 0; i < arr->GetNumberOfComponents(); ++i)
      {
        oss << arr->GetComponent(picker->GetCellId(), i);
        if (i < arr->GetNumberOfComponents() - 1)
        {
          oss << ',';
        }
      }
      oss << ")|";
    }
    const std::string info = oss.str();
    sendMessageToTooltipAt(info.c_str(), info.length(), pos[0], pos[1]);
  }
}
} // namespace

class GeometryViewer::Internal
{
public:
  float ScrollSensitivity = 1.0;
  std::string ColorMapPreset = "Spectrum";

  std::unordered_set<std::string> PointDataArrays;
  std::unordered_set<std::string> CellDataArrays;

  void UpdateLUT();
  void FetchAvailableDataArrays(vtkDataObject* dataObject);

  HighlighterBridge HighlighterData;
  vtkNew<vtkActor> Actor;
  vtkNew<vtkRenderWindowInteractor> Interactor;
  vtkNew<vtkCameraOrientationWidget> CameraManipulator;
  vtkNew<vtkRenderWindow> Window;
  vtkNew<vtkRenderer> Renderer;

  int MouseMoveObserverTag = -1;
};

//------------------------------------------------------------------------------
GeometryViewer::GeometryViewer()
{
  this->P = std::unique_ptr<Internal>(new Internal());
  std::cout << __func__ << std::endl;
  this->P->Window->SetWindowName(__func__);
}

//------------------------------------------------------------------------------
GeometryViewer::~GeometryViewer()
{
  std::cout << __func__ << std::endl;
  this->P->Interactor->TerminateApp();
}

//------------------------------------------------------------------------------
void GeometryViewer::LoadDataFileFromMemory(
  const std::string& filename, std::uintptr_t buffer, std::size_t nbytes)
{

  std::cout << __func__ << "(" << filename << ',' << buffer << ',' << nbytes << ")" << std::endl;

  vtkNew<vtkPolyDataMapper> mapper;
  vtkNew<vtkPolyData> mesh;

  auto wrappedBuffer = vtk::TakeSmartPointer(vtkBuffer<char>::New());
  wrappedBuffer->SetBuffer(reinterpret_cast<char*>(buffer), nbytes);
  wrappedBuffer->SetFreeFunction(/*noFreeFunction=*/true);

  using systools = vtksys::SystemTools;
  vtkNew<vtkMemoryResourceStream> stream;
  if (systools::StringEndsWith(filename, ".vtp"))
  {
    auto xmlreader = vtk::TakeSmartPointer(vtkXMLPolyDataReader::New());
    xmlreader->SetReadFromInputString(true);
    vtkNew<vtkCharArray> wrappedArray;
    wrappedArray->SetArray(reinterpret_cast<char*>(buffer), nbytes, 1);
    xmlreader->SetInputArray(wrappedArray);
    xmlreader->Update();
    mesh->ShallowCopy(xmlreader->GetOutput());
  }
  else if (systools::StringEndsWith(filename, ".vtu"))
  {
    auto xmlreader = vtk::TakeSmartPointer(vtkXMLUnstructuredGridReader::New());
    auto surface = vtk::TakeSmartPointer(vtkGeometryFilter::New());
    xmlreader->SetReadFromInputString(true);
    vtkNew<vtkCharArray> wrappedArray;
    wrappedArray->SetArray(reinterpret_cast<char*>(buffer), nbytes, 1);
    xmlreader->SetInputArray(wrappedArray);
    surface->SetInputConnection(xmlreader->GetOutputPort());
    surface->Update();
    mesh->ShallowCopy(surface->GetOutput());
  }
  else if (systools::StringEndsWith(filename, ".vtk"))
  {
    auto polydataReader = vtk::TakeSmartPointer(vtkPolyDataReader::New());
    polydataReader->ReadFromInputStringOn();
    vtkNew<vtkCharArray> wrappedArray;
    wrappedArray->SetArray(reinterpret_cast<char*>(buffer), nbytes, 1);
    polydataReader->SetInputArray(wrappedArray);
    polydataReader->Update();
    mesh->ShallowCopy(polydataReader->GetOutput());
  }
  else if (systools::StringEndsWith(filename, ".obj"))
  {
    auto objreader = vtk::TakeSmartPointer(vtkOBJReader::New());
    stream->SetBuffer(wrappedBuffer);
    objreader->SetStream(stream);
    objreader->Update();
    mesh->ShallowCopy(objreader->GetOutput());
  }
  else if (systools::StringEndsWith(filename, ".ply"))
  {
    auto plyreader = vtk::TakeSmartPointer(vtkPLYReader::New());
    plyreader->SetReadFromInputStream(true);
    stream->SetBuffer(wrappedBuffer);
    plyreader->SetStream(stream);
    plyreader->Update();
    mesh->ShallowCopy(plyreader->GetOutput());
  }
  else if (systools::StringEndsWith(filename, ".zip"))
  {
    auto plyreader = vtk::TakeSmartPointer(vtkPLYReader::New());
    plyreader->SetReadFromInputStream(true);
    stream->SetBuffer(wrappedBuffer);
    plyreader->SetStream(stream);
    plyreader->Update();
    mesh->ShallowCopy(plyreader->GetOutput());
  }
  mapper->SetInputData(mesh);
  this->P->Actor->SetMapper(mapper);
  this->P->Renderer->AddActor(this->P->Actor);
  this->SetColorByArray("Solid");
  this->P->UpdateLUT();
  // render once so that the pipeline is updated
  this->P->Window->Render();
  // make the mapper static so that subsequent renders do not walk up the VTK
  // pipeline anymore.
  mapper->StaticOn();
  // Fetches point and cell data arrays from reader's output.
  this->P->FetchAvailableDataArrays(mesh);
}

//------------------------------------------------------------------------------
void GeometryViewer::LoadDataFile(const std::string& filename)
{

  std::cout << __func__ << "(" << filename << ")" << std::endl;

  auto mapper = vtk::TakeSmartPointer(vtkPolyDataMapper::New());
  vtkSmartPointer<vtkDataObject> mesh;

  using systools = vtksys::SystemTools;
  if (systools::StringEndsWith(filename, ".vtp"))
  {
    auto xmlreader = vtk::TakeSmartPointer(vtkXMLPolyDataReader::New());
    xmlreader->SetFileName(filename.c_str());
    xmlreader->Update();
    mesh = xmlreader->GetOutput();
  }
  else if (systools::StringEndsWith(filename, ".vtu"))
  {
    auto xmlreader = vtk::TakeSmartPointer(vtkXMLUnstructuredGridReader::New());
    xmlreader->SetFileName(filename.c_str());
    auto surface = vtk::TakeSmartPointer(vtkGeometryFilter::New());
    surface->SetInputConnection(xmlreader->GetOutputPort());
    surface->Update();
    mesh = surface->GetOutput();
  }
  else if (systools::StringEndsWith(filename, ".vtk"))
  {
    auto polydataReader = vtk::TakeSmartPointer(vtkPolyDataReader::New());
    polydataReader->SetFileName(filename.c_str());
    polydataReader->Update();
    mesh = polydataReader->GetOutput();
  }
  else if (systools::StringEndsWith(filename, ".glb") ||
    systools::StringEndsWith(filename, ".gltf"))
  {
    // TODO: Hangs.
    // auto gltfreader = vtk::TakeSmartPointer(vtkGLTFReader::New());
    // mapper = vtk::TakeSmartPointer(vtkPolyDataMapper::New());
    // gltfreader->SetFileName(filename.c_str());
    // reader = gltfreader;
  }
  else if (systools::StringEndsWith(filename, ".obj"))
  {
    auto objreader = vtk::TakeSmartPointer(vtkOBJReader::New());
    objreader->SetFileName(filename.c_str());
    objreader->Update();
    mesh = objreader->GetOutput();
  }
  else if (systools::StringEndsWith(filename, ".ply"))
  {
    auto plyreader = vtk::TakeSmartPointer(vtkPLYReader::New());
    plyreader->SetFileName(filename.c_str());
    plyreader->Update();
    mesh = plyreader->GetOutput();
  }
  else if (systools::StringEndsWith(filename, ".vtpc"))
  {
    mapper = vtk::TakeSmartPointer(vtkCompositePolyDataMapper::New());
    auto vtpcreader = vtk::TakeSmartPointer(vtkXMLPartitionedDataSetCollectionReader::New());
    vtpcreader->SetFileName(filename.c_str());
    vtpcreader->Update();
    mesh = vtpcreader->GetOutput();
  }
  else if (systools::StringEndsWith(filename, ".vtm"))
  {
    mapper = vtk::TakeSmartPointer(vtkCompositePolyDataMapper::New());
    auto vtmreader = vtk::TakeSmartPointer(vtkXMLMultiBlockDataReader::New());
    vtmreader->SetFileName(filename.c_str());
    vtmreader->Update();
    mesh = vtmreader->GetOutput();
  }
  mapper->SetInputDataObject(mesh);
  this->P->Actor->SetMapper(mapper);
  this->P->Renderer->AddActor(this->P->Actor);
  this->SetColorByArray("Solid");
  this->P->UpdateLUT();
  // render once so that the pipeline is updated
  this->P->Window->Render();
  // make the mapper static so that subsequent renders do not walk up the VTK
  // pipeline anymore.
  mapper->StaticOn();
  // Fetches point and cell data arrays from reader's output.
  this->P->FetchAvailableDataArrays(mesh);
}

//------------------------------------------------------------------------------
void GeometryViewer::WriteDataFileToVirtualFS(
  const std::string& filename, std::uintptr_t buffer, std::size_t nbytes)
{
  std::cout << __func__ << std::endl;
  using systools = vtksys::SystemTools;
  const auto parentDir = systools::GetParentDirectory(filename);
  if (!parentDir.empty())
  {
    if (!systools::MakeDirectory(parentDir).IsSuccess())
    {
      std::cerr << "ERROR: Failed to create parent directory for file = " << filename << '\n';
    }
  }
  std::ofstream ofs(filename, std::ios::binary);
  ofs.write(reinterpret_cast<char*>(buffer), nbytes);
  std::cout << "Wrote " << nbytes << " bytes into " << filename << '\n';
}

//------------------------------------------------------------------------------
void GeometryViewer::Azimuth(float value)
{
  this->P->Renderer->GetActiveCamera()->Azimuth(value);
  this->P->Renderer->ResetCameraClippingRange();
}

//------------------------------------------------------------------------------
void GeometryViewer::Initialize()
{
  std::cout << __func__ << std::endl;
  this->P->Renderer->GradientBackgroundOn();
  this->P->Renderer->SetGradientMode(
    vtkRenderer::GradientModes::VTK_GRADIENT_RADIAL_VIEWPORT_FARTHEST_CORNER);
  // initialize picker data
  this->P->HighlighterData.Renderer = this->P->Renderer;
  // create the default renderer
  this->P->Window->AddRenderer(this->P->Renderer);
  this->P->Window->SetInteractor(this->P->Interactor);
  auto iren = vtkWebAssemblyRenderWindowInteractor::SafeDownCast(this->P->Interactor);
  iren->SetCanvasSelector("#vtk-3d-canvas");
  if (auto wasmWebGPURenderWindow = vtkWebAssemblyWebGPURenderWindow::SafeDownCast(this->P->Window))
  {
    wasmWebGPURenderWindow->SetCanvasSelector("#vtk-3d-canvas");
  }
  if (auto wasmOpenGLRenderWindow = vtkWebAssemblyOpenGLRenderWindow::SafeDownCast(this->P->Window))
  {
    wasmOpenGLRenderWindow->SetCanvasSelector("#vtk-3d-canvas");
  }
  // turn on camera manipulator
  // this->P->CameraManipulator->SetParentRenderer(this->P->Renderer);
  // this->P->CameraManipulator->SetAnimate(false);
  // vtkCameraOrientationRepresentation::SafeDownCast(
  //     this->P->CameraManipulator->GetRepresentation())
  //     ->AnchorToLowerLeft();
  // this->P->CameraManipulator->On();
  // do not simulate infinite loop.
  vtkRenderWindowInteractor::InteractorManagesTheEventLoop = false;
  // set the current style to TrackBallCamera. Default is joystick
  if (auto iStyle = vtkInteractorStyle::SafeDownCast(this->P->Interactor->GetInteractorStyle()))
  {
    if (auto switchStyle = vtkInteractorStyleSwitch::SafeDownCast(iStyle))
    {
      switchStyle->SetCurrentStyleToTrackballCamera();
    }
  }
  this->SetMouseWheelMotionFactor(0.15);
}

//------------------------------------------------------------------------------
void GeometryViewer::Render()
{
  this->P->Window->Render();
}

//------------------------------------------------------------------------------
void GeometryViewer::ResetView()
{
  std::cout << __func__ << std::endl;
  auto ren = this->P->Window->GetRenderers()->GetFirstRenderer();
  if (ren != nullptr)
  {
    ren->ResetCamera();
  }
}

//------------------------------------------------------------------------------
void GeometryViewer::SetSize(int width, int height)
{
  std::cout << __func__ << width << ',' << height << std::endl;
  this->P->Interactor->UpdateSize(width, height);
}

//------------------------------------------------------------------------------
void GeometryViewer::RemoveAllActors()
{
  std::cout << __func__ << std::endl;
  auto ren = this->P->Window->GetRenderers()->GetFirstRenderer();
  ren->RemoveAllViewProps();
  // frees up memory by deleting the mapper and it's input data object.
  this->P->Actor->SetMapper(nullptr);
  this->P->Window->Render();
}

//------------------------------------------------------------------------------
void GeometryViewer::Start()
{
  std::cout << __func__ << std::endl;
  this->P->Renderer->ResetCamera();
  this->P->Window->Render();
  this->P->Interactor->Start();
}

//------------------------------------------------------------------------------
void GeometryViewer::Halt()
{
  emscripten_pause_main_loop();
}

//------------------------------------------------------------------------------
void GeometryViewer::Resume()
{
  emscripten_resume_main_loop();
}

//------------------------------------------------------------------------------
void GeometryViewer::SetBackgroundColor1(int r, int g, int b)
{
  std::cout << __func__ << '(' << r / 255. << ',' << g / 255. << ',' << b / 255. << ')'
            << std::endl;
  this->P->Renderer->SetBackground(r / 255., g / 255., b / 255.);
}

//------------------------------------------------------------------------------
void GeometryViewer::SetBackgroundColor2(int r, int g, int b)
{
  std::cout << __func__ << '(' << r / 255. << ',' << g / 255. << ',' << b / 255. << ')'
            << std::endl;
  this->P->Renderer->SetBackground2(r / 255., g / 255., b / 255.);
}

//------------------------------------------------------------------------------
void GeometryViewer::SetMouseWheelMotionFactor(float sensitivity)
{
  std::cout << __func__ << "(" << sensitivity << ")" << std::endl;
  if (auto iStyle = vtkInteractorStyle::SafeDownCast(this->P->Interactor->GetInteractorStyle()))
  {
    if (auto switchStyle = vtkInteractorStyleSwitch::SafeDownCast(iStyle))
    {
      switchStyle->GetCurrentStyle()->SetMouseWheelMotionFactor(sensitivity);
    }
    else
    {
      iStyle->SetMouseWheelMotionFactor(sensitivity);
    }
  }
  this->P->ScrollSensitivity = sensitivity;
}

//------------------------------------------------------------------------------
void GeometryViewer::SetUseOrthographicProjection(bool value)
{
  std::cout << __func__ << "(" << value << ")" << std::endl;
  this->P->Renderer->GetActiveCamera()->SetParallelProjection(value);
}

//------------------------------------------------------------------------------
void GeometryViewer::SetDitherGradient(bool value)
{
  std::cout << __func__ << "(" << value << ")" << std::endl;
  this->P->Renderer->SetDitherGradient(value);
}

//------------------------------------------------------------------------------
void GeometryViewer::SetHighlightOnHover(bool value, bool snapToPoint /*=false*/)
{
  std::cout << __func__ << "(" << value << ',' << snapToPoint << ")" << std::endl;
  this->P->Interactor->RemoveObserver(this->P->MouseMoveObserverTag);
  ::hideTooltip();
  if (value)
  {
    vtkNew<vtkCallbackCommand> cbc;
    if (snapToPoint)
    {
      cbc->SetCallback(::HighlightPointUnderMouse);
    }
    else
    {
      cbc->SetCallback(::HighlightCellUnderMouse);
    }
    cbc->SetClientData(&(this->P->HighlighterData));
    this->P->MouseMoveObserverTag =
      this->P->Interactor->AddObserver(vtkCommand::MouseMoveEvent, cbc);
  }
}

//------------------------------------------------------------------------------
void GeometryViewer::SetRepresentation(int rep)
{
  std::cout << __func__ << '(' << rep << ')' << std::endl;
  this->P->Actor->GetProperty()->SetRepresentation(rep);
}

//------------------------------------------------------------------------------
void GeometryViewer::SetVertexVisibility(bool visible)
{
  std::cout << __func__ << '(' << visible << ')' << std::endl;
  this->P->Actor->GetProperty()->SetVertexVisibility(visible);
}

//------------------------------------------------------------------------------
void GeometryViewer::SetRenderPointsAsSpheres(bool value)
{
  std::cout << __func__ << '(' << value << ')' << std::endl;
  this->P->Actor->GetProperty()->SetRenderPointsAsSpheres(value);
}

//------------------------------------------------------------------------------
void GeometryViewer::SetPointSize(float value)
{
  std::cout << __func__ << '(' << value << ')' << std::endl;
  this->P->Actor->GetProperty()->SetPointSize(value);
}

//------------------------------------------------------------------------------
void GeometryViewer::SetEdgeVisibility(bool visible)
{
  std::cout << __func__ << '(' << visible << ')' << std::endl;
  this->P->Actor->GetProperty()->SetEdgeVisibility(visible);
}

//------------------------------------------------------------------------------
void GeometryViewer::SetRenderLinesAsTubes(bool value)
{
  std::cout << __func__ << '(' << value << ')' << std::endl;
  this->P->Actor->GetProperty()->SetRenderLinesAsTubes(value);
}

//------------------------------------------------------------------------------
void GeometryViewer::SetLineWidth(float value)
{
  std::cout << __func__ << '(' << value << ')' << std::endl;
  this->P->Actor->GetProperty()->SetLineWidth(value);
}

//------------------------------------------------------------------------------
void GeometryViewer::SetColorByArray(const std::string& arrayName)
{
  auto mapper = this->P->Actor->GetMapper();
  this->P->HighlighterData.ActivePointColorArray = "";
  this->P->HighlighterData.ActiveCellColorArray = "";
  if (!mapper)
  {
    return;
  }
  std::cout << __func__ << '(' << arrayName << ')' << std::endl;
  if (arrayName == "Solid")
  {
    mapper->ScalarVisibilityOff();
    return;
  }
  mapper->ScalarVisibilityOn();
  vtkDataArray* scalarArray = nullptr;
  if (this->P->PointDataArrays.count(arrayName))
  {
    this->P->HighlighterData.ActivePointColorArray = arrayName;
    mapper->SetScalarModeToUsePointFieldData();
    scalarArray = mapper->GetDataSetInput()->GetPointData()->GetArray(arrayName.c_str());
  }
  else if (this->P->CellDataArrays.count(arrayName))
  {
    this->P->HighlighterData.ActiveCellColorArray = arrayName;
    mapper->SetScalarModeToUseCellFieldData();
    scalarArray = mapper->GetDataSetInput()->GetCellData()->GetArray(arrayName.c_str());
  }
  else
  {
    return;
  }
  // TODO: handle multi-component arrays with magnitude, x, y, z ..
  mapper->ColorByArrayComponent(arrayName.c_str(), 0);
  if (scalarArray)
  {
    double range[2];
    scalarArray->GetRange(range);
    mapper->SetScalarRange(range);
  }
  this->P->UpdateLUT();
}

//------------------------------------------------------------------------------
void GeometryViewer::SetInterpolateScalarsBeforeMapping(bool value)
{
  std::cout << __func__ << '(' << value << ')' << std::endl;
  if (auto mapper = this->P->Actor->GetMapper())
  {
    mapper->SetInterpolateScalarsBeforeMapping(value);
  }
}

//------------------------------------------------------------------------------
void GeometryViewer::SetColor(int r, int g, int b)
{
  std::cout << __func__ << '(' << r << ',' << g << ',' << b << ')' << std::endl;
  this->P->Actor->GetProperty()->SetColor(r / 255., g / 255., b / 255.);
}

//------------------------------------------------------------------------------
void GeometryViewer::SetColorMapPreset(const std::string& presetName)
{
  std::cout << __func__ << '(' << presetName << ')' << std::endl;
  this->P->ColorMapPreset = presetName;
  this->P->UpdateLUT();
}

//------------------------------------------------------------------------------
void GeometryViewer::SetEdgeColor(int r, int g, int b)
{
  std::cout << __func__ << '(' << r << ',' << g << ',' << b << ')' << std::endl;
  this->P->Actor->GetProperty()->SetEdgeColor(r / 255., g / 255., b / 255.);
}

//------------------------------------------------------------------------------
void GeometryViewer::SetVertexColor(int r, int g, int b)
{
  std::cout << __func__ << '(' << r << ',' << g << ',' << b << ')' << std::endl;
  this->P->Actor->GetProperty()->SetVertexColor(r / 255., g / 255., b / 255.);
}

//------------------------------------------------------------------------------
void GeometryViewer::SetOpacity(float value)
{
  std::cout << __func__ << '(' << value << ')' << std::endl;
  this->P->Actor->GetProperty()->SetOpacity(value);
}

//------------------------------------------------------------------------------
void GeometryViewer::SetEdgeOpacity(float value)
{
  std::cout << __func__ << '(' << value << ')' << std::endl;
  this->P->Actor->GetProperty()->SetEdgeOpacity(value);
}

//------------------------------------------------------------------------------
std::string GeometryViewer::GetPointDataArrays()
{
  std::cout << __func__ << std::endl;
  std::string result;
  for (auto& arrayName : this->P->PointDataArrays)
  {
    result = result + arrayName + ';';
  }
  // remove that last ';'
  return result.substr(0, result.length() - 1);
}

//------------------------------------------------------------------------------
std::string GeometryViewer::GetCellDataArrays()
{
  std::cout << __func__ << std::endl;
  std::string result;
  for (auto& arrayName : this->P->CellDataArrays)
  {
    result = result + arrayName + ';';
  }
  // remove that last ';'
  return result.substr(0, result.length() - 1);
}

namespace
{
std::map<std::string, int> PresetNames = { { "Spectrum", vtkColorSeries::SPECTRUM },
  { "Warm", vtkColorSeries::WARM }, { "Cool", vtkColorSeries::COOL },
  { "Blues", vtkColorSeries::BLUES }, { "WildFlower", vtkColorSeries::WILD_FLOWER },
  { "Citrus", vtkColorSeries::CITRUS } };
} // namespace

//------------------------------------------------------------------------------
std::string GeometryViewer::GetColorMapPresets()
{
  return "Spectrum;Warm;Cool;Blues;WildFlower;Citrus";
}

//------------------------------------------------------------------------------
void GeometryViewer::Internal::UpdateLUT()
{
  auto mapper = this->Actor->GetMapper();
  if (!mapper)
  {
    return;
  }
  std::array<double, 2> scalarRange;
  mapper->GetScalarRange(scalarRange.data());
  vtkNew<vtkColorTransferFunction> lut;
  lut->SetColorSpaceToHSV();
  vtkNew<vtkColorSeries> colorSeries;
  colorSeries->SetColorScheme(PresetNames.at(this->ColorMapPreset));
  auto numColors = colorSeries->GetNumberOfColors();
  for (int i = 0; i < numColors; ++i)
  {
    vtkColor3ub color = colorSeries->GetColor(i);
    double dColor[3];
    dColor[0] = static_cast<double>(color[0]) / 255.0;
    dColor[1] = static_cast<double>(color[1]) / 255.0;
    dColor[2] = static_cast<double>(color[2]) / 255.0;
    double t =
      scalarRange[0] + (scalarRange[1] - scalarRange[0]) / (static_cast<double>(numColors) - 1) * i;
    lut->AddRGBPoint(t, dColor[0], dColor[1], dColor[2]);
  }
  mapper->SetLookupTable(lut);
}

//------------------------------------------------------------------------------
void GeometryViewer::Internal::FetchAvailableDataArrays(vtkDataObject* dataObject)
{
  if (auto* dataObjectTree = vtkDataObjectTree::SafeDownCast(dataObject))
  {
    using Opts = vtk::DataObjectTreeOptions;
    for (vtkDataObject* child : vtk::Range(dataObjectTree, Opts::None))
    {
      if (child)
      {
        this->FetchAvailableDataArrays(child);
      }
    }
  }
  else if (auto* dataSet = vtkDataSet::SafeDownCast(dataObject))
  {
    this->PointDataArrays.clear();
    if (auto pointData = dataSet->GetPointData())
    {
      for (int i = 0; i < pointData->GetNumberOfArrays(); ++i)
      {
        this->PointDataArrays.insert(pointData->GetArrayName(i));
      }
    }
    this->CellDataArrays.clear();
    if (auto cellData = dataSet->GetCellData())
    {
      for (int i = 0; i < cellData->GetNumberOfArrays(); ++i)
      {
        this->CellDataArrays.insert(cellData->GetArrayName(i));
      }
    }
  }
}

//------------------------------------------------------------------------------
void GeometryViewer::SaveScreenshotAsPNG()
{
  vtkNew<vtkWindowToImageFilter> windowToImageFilter;
  windowToImageFilter->SetInput(this->P->Window);
  windowToImageFilter->ReadFrontBufferOff(); // read from the back buffer
  vtkNew<vtkPNGWriter> pngWriter;
  pngWriter->SetInputConnection(windowToImageFilter->GetOutputPort());
  pngWriter->SetWriteToMemory(true);
  pngWriter->Write();
  auto arr = pngWriter->GetResult();
  EM_ASM(
    {
      function bytesToBase64(bytes)
      {
        var binary = String();
        var len = bytes.byteLength;
        for (var i = 0; i < len; i++)
        {
          binary += String.fromCharCode(bytes[i]);
        }
        return window.btoa(binary);
      }
      document.getElementById("ScreenShot").src =
        "data:image/png;base64," + bytesToBase64(HEAPU8.subarray($0, $1));
    },
    arr->Begin(), arr->End());
}

#ifdef __EMSCRIPTEN__
#include <emscripten/bind.h>
EMSCRIPTEN_BINDINGS(GeometryViewerJSBindings)
{
  emscripten::class_<GeometryViewer>("GeometryViewer")
    .constructor<>()
    .function(
      "loadDataFileFromMemory", &GeometryViewer::LoadDataFileFromMemory, emscripten::async())
    .function("loadDataFile", &GeometryViewer::LoadDataFile, emscripten::async())
    .function("writeDataFileToVirtualFS", &GeometryViewer::WriteDataFileToVirtualFS)
    .function("initialize", &GeometryViewer::Initialize)
    .function("render", &GeometryViewer::Render, emscripten::async())
    .function("resetView", &GeometryViewer::ResetView)
    .function("setSize", &GeometryViewer::SetSize)
    .function("removeAllActors", &GeometryViewer::RemoveAllActors, emscripten::async())
    .function("start", &GeometryViewer::Start, emscripten::async())
    .function("halt", &GeometryViewer::Halt)
    .function("resume", &GeometryViewer::Resume)
    .function("setBackgroundColor1", &GeometryViewer::SetBackgroundColor1)
    .function("setBackgroundColor2", &GeometryViewer::SetBackgroundColor2)
    .function("setMouseWheelMotionFactor", &GeometryViewer::SetMouseWheelMotionFactor)
    .function("setUseOrthographicProjection", &GeometryViewer::SetUseOrthographicProjection)
    .function("setDitherGradient", &GeometryViewer::SetDitherGradient)
    .function("setHighlightOnHover", &GeometryViewer::SetHighlightOnHover)
    .function("azimuth", &GeometryViewer::Azimuth)
    .function("setRepresentation", &GeometryViewer::SetRepresentation)
    .function("setVertexVisibility", &GeometryViewer::SetVertexVisibility)
    .function("setRenderPointsAsSpheres", &GeometryViewer::SetRenderPointsAsSpheres)
    .function("setPointSize", &GeometryViewer::SetPointSize)
    .function("setEdgeVisibility", &GeometryViewer::SetEdgeVisibility)
    .function("setRenderLinesAsTubes", &GeometryViewer::SetRenderLinesAsTubes)
    .function("setLineWidth", &GeometryViewer::SetLineWidth)
    .function("setColorByArray", &GeometryViewer::SetColorByArray)
    .function(
      "setInterpolateScalarsBeforeMapping", &GeometryViewer::SetInterpolateScalarsBeforeMapping)
    .function("setColor", &GeometryViewer::SetColor)
    .function("setColorMapPreset", &GeometryViewer::SetColorMapPreset)
    .function("setVertexColor", &GeometryViewer::SetVertexColor)
    .function("setEdgeColor", &GeometryViewer::SetEdgeColor)
    .function("setOpacity", &GeometryViewer::SetOpacity)
    .function("setEdgeOpacity", &GeometryViewer::SetEdgeOpacity)
    .function("getPointDataArrays", &GeometryViewer::GetPointDataArrays)
    .function("getCellDataArrays", &GeometryViewer::GetCellDataArrays)
    .function("getColorMapPresets", &GeometryViewer::GetColorMapPresets)
    .function("saveScreenshotAsPNG", &GeometryViewer::SaveScreenshotAsPNG);
}
#endif
