// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkActor.h"
#include "vtkAppendPolyData.h"
#include "vtkCellData.h"
#include "vtkCommand.h"
#include "vtkElevationFilter.h"
#include "vtkFloatArray.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkNew.h"
#include "vtkPlane.h"
#include "vtkPlaneSource.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkPolyDataNormals.h"
#include "vtkPolyLineSource.h"
#include "vtkProperty.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkSlangShaderManager.h"
#include "vtkSmartPointer.h"
#include "vtkSphereSource.h"
#include "vtkTestErrorObserver.h"
#include "vtkTesting.h"
#include "vtkTexture.h"
#include "vtkTexturedSphereSource.h"
#include "vtkUnsignedCharArray.h"
#include "vtkWebGPURenderWindow.h"
#include "vtkWebGPUShaderProperty.h"

#include <cstdlib>
#include <functional>
#include <iostream>

namespace
{
constexpr int Size = 200;

const char* InvertModule = R"(
module parity_probe;
import slangkit_shading;
public struct Invert : ISurfaceOutput {
  public bool keep(SurfaceSample s) { return true; }
  public float4 finish(float4 c, MaterialSample m, SurfaceSample s) { return float4(1.f - c.rgb, c.a); }
  public float depth(float d, SurfaceSample s) { return d; }
};
)";

void Grab(vtkRenderWindow* renWin, vtkUnsignedCharArray* pixels)
{
  renWin->Render();
  renWin->GetPixelData(0, 0, Size - 1, Size - 1, 1, pixels);
}

double DifferingFraction(vtkUnsignedCharArray* a, vtkUnsignedCharArray* b, int threshold)
{
  const vtkIdType n = a->GetNumberOfTuples();
  const int comps = a->GetNumberOfComponents();
  vtkIdType differing = 0;
  for (vtkIdType i = 0; i < n; ++i)
  {
    int maxDiff = 0;
    for (int c = 0; c < 3; ++c)
    {
      const int d = std::abs(static_cast<int>(a->GetValue(i * comps + c)) -
        static_cast<int>(b->GetValue(i * comps + c)));
      maxDiff = d > maxDiff ? d : maxDiff;
    }
    differing += maxDiff > threshold ? 1 : 0;
  }
  return static_cast<double>(differing) / static_cast<double>(n);
}

vtkSmartPointer<vtkPolyData> Sphere(bool normals)
{
  vtkNew<vtkSphereSource> sphere;
  sphere->SetThetaResolution(24);
  sphere->SetPhiResolution(16);
  sphere->Update();
  auto output = vtkSmartPointer<vtkPolyData>::New();
  output->ShallowCopy(sphere->GetOutput());
  if (!normals)
  {
    output->GetPointData()->SetNormals(nullptr);
  }
  return output;
}

vtkSmartPointer<vtkPolyData> ZigZag()
{
  vtkNew<vtkPolyLineSource> line;
  line->SetNumberOfPoints(5);
  line->SetPoint(0, -1.0, -0.4, 0.0);
  line->SetPoint(1, -0.5, 0.4, 0.1);
  line->SetPoint(2, 0.0, -0.4, 0.0);
  line->SetPoint(3, 0.5, 0.4, -0.1);
  line->SetPoint(4, 1.0, -0.4, 0.0);
  vtkNew<vtkElevationFilter> elevation;
  elevation->SetInputConnection(line->GetOutputPort());
  elevation->SetLowPoint(-1, 0, 0);
  elevation->SetHighPoint(1, 0, 0);
  elevation->Update();
  auto output = vtkSmartPointer<vtkPolyData>::New();
  output->ShallowCopy(elevation->GetOutput());
  return output;
}

vtkSmartPointer<vtkTexture> Checker()
{
  vtkNew<vtkImageData> image;
  image->SetDimensions(8, 8, 1);
  image->AllocateScalars(VTK_UNSIGNED_CHAR, 3);
  auto* texels = static_cast<unsigned char*>(image->GetScalarPointer());
  for (int j = 0; j < 8; ++j)
  {
    for (int i = 0; i < 8; ++i)
    {
      const unsigned char v = ((i + j) % 2) ? 230 : 40;
      unsigned char* t = texels + 3 * (j * 8 + i);
      t[0] = v;
      t[1] = static_cast<unsigned char>(255 - v);
      t[2] = 128;
    }
  }
  auto texture = vtkSmartPointer<vtkTexture>::New();
  texture->SetInputData(image);
  texture->InterpolateOff();
  return texture;
}

struct Variant
{
  const char* Name;
  std::function<void(vtkPolyDataMapper*, vtkActor*)> Setup;
};
}

int TestSlangSurfaceShadingParity(int, char*[])
{
  {
    vtkNew<vtkRenderWindow> probe;
    if (vtkWebGPURenderWindow::SafeDownCast(probe) == nullptr)
    {
      std::cerr << "This test needs the WebGPU rendering backend.\n";
      return EXIT_FAILURE;
    }
  }
  if (!vtkSlangShaderManager::IsSlangAvailable())
  {
    std::cout << "Slang is unavailable; skipping.\n";
    return VTK_SKIP_RETURN_CODE;
  }

  const Variant variants[] = {
    { "point normals", [](vtkPolyDataMapper* m, vtkActor*) { m->SetInputData(Sphere(true)); } },
    { "no normals (face normals)",
      [](vtkPolyDataMapper* m, vtkActor*) { m->SetInputData(Sphere(false)); } },
    { "cell normals",
      [](vtkPolyDataMapper* m, vtkActor*)
      {
        vtkNew<vtkPolyDataNormals> normals;
        normals->SetInputData(Sphere(false));
        normals->ComputePointNormalsOff();
        normals->ComputeCellNormalsOn();
        normals->Update();
        auto output = vtkSmartPointer<vtkPolyData>::New();
        output->ShallowCopy(normals->GetOutput());
        m->SetInputData(output);
      } },
    { "point colors",
      [](vtkPolyDataMapper* m, vtkActor*)
      {
        vtkNew<vtkElevationFilter> elevation;
        elevation->SetInputData(Sphere(true));
        elevation->SetLowPoint(0, -0.5, 0);
        elevation->SetHighPoint(0, 0.5, 0);
        elevation->Update();
        auto output = vtkSmartPointer<vtkPolyData>::New();
        output->ShallowCopy(elevation->GetOutput());
        m->SetInputData(output);
        m->SetScalarRange(0, 1);
      } },
    { "cell colors",
      [](vtkPolyDataMapper* m, vtkActor*)
      {
        auto sphere = Sphere(true);
        vtkNew<vtkFloatArray> scalars;
        scalars->SetName("cellScalars");
        for (vtkIdType i = 0; i < sphere->GetNumberOfCells(); ++i)
        {
          scalars->InsertNextValue(static_cast<float>(i % 7) / 6.f);
        }
        sphere->GetCellData()->SetScalars(scalars);
        m->SetInputData(sphere);
        m->SetScalarModeToUseCellData();
        m->SetScalarRange(0, 1);
      } },
    { "lookup table texture colors",
      [](vtkPolyDataMapper* m, vtkActor*)
      {
        vtkNew<vtkElevationFilter> elevation;
        elevation->SetInputData(Sphere(true));
        elevation->Update();
        auto output = vtkSmartPointer<vtkPolyData>::New();
        output->ShallowCopy(elevation->GetOutput());
        m->SetInputData(output);
        m->SetScalarRange(0, 1);
        m->InterpolateScalarsBeforeMappingOn();
      } },
    { "actor texture",
      [](vtkPolyDataMapper* m, vtkActor* a)
      {
        vtkNew<vtkTexturedSphereSource> sphere;
        sphere->SetThetaResolution(24);
        sphere->SetPhiResolution(16);
        sphere->Update();
        auto output = vtkSmartPointer<vtkPolyData>::New();
        output->ShallowCopy(sphere->GetOutput());
        m->SetInputData(output);
        a->SetTexture(Checker());
      } },
    { "texture transform",
      [](vtkPolyDataMapper* m, vtkActor* a)
      {
        vtkNew<vtkTexturedSphereSource> sphere;
        sphere->SetThetaResolution(24);
        sphere->SetPhiResolution(16);
        sphere->Update();
        auto output = vtkSmartPointer<vtkPolyData>::New();
        output->ShallowCopy(sphere->GetOutput());
        m->SetInputData(output);
        a->SetTexture(Checker());
        // Row major, with a shear and a translation.
        const double matrix[16] = { 2, 0.5, 0, 0.25, 0, 1.5, 0, 0.1, 0, 0, 1, 0, 0, 0, 0, 1 };
        vtkNew<vtkInformation> keys;
        keys->Set(vtkProp::GENERAL_TEXTURE_TRANSFORM(), matrix, 16);
        a->SetPropertyKeys(keys);
      } },
    { "edges",
      [](vtkPolyDataMapper* m, vtkActor* a)
      {
        m->SetInputData(Sphere(true));
        a->GetProperty()->EdgeVisibilityOn();
        a->GetProperty()->SetEdgeColor(0.1, 0.9, 0.1);
      } },
    { "clipping planes",
      [](vtkPolyDataMapper* m, vtkActor*)
      {
        m->SetInputData(Sphere(true));
        vtkNew<vtkPlane> plane;
        plane->SetOrigin(0.1, 0, 0);
        plane->SetNormal(-1, 0.3, 0);
        m->AddClippingPlane(plane);
      } },
    { "backface property",
      [](vtkPolyDataMapper* m, vtkActor* a)
      {
        // Clip away the cap facing the camera so the inside of the sphere shows.
        m->SetInputData(Sphere(true));
        vtkNew<vtkPlane> plane;
        plane->SetOrigin(0, 0, 0.1);
        plane->SetNormal(0, 0, -1);
        m->AddClippingPlane(plane);
        vtkNew<vtkProperty> backface;
        backface->SetDiffuseColor(0.9, 0.3, 0.2);
        backface->SetSpecular(0.4);
        a->SetBackfaceProperty(backface);
      } },
    { "actor texture hidden on back faces",
      [](vtkPolyDataMapper* m, vtkActor* a)
      {
        // Clip away the cap facing the camera so the inside of the textured sphere shows.
        vtkNew<vtkTexturedSphereSource> sphere;
        sphere->SetThetaResolution(24);
        sphere->SetPhiResolution(16);
        sphere->Update();
        auto output = vtkSmartPointer<vtkPolyData>::New();
        output->ShallowCopy(sphere->GetOutput());
        m->SetInputData(output);
        vtkNew<vtkPlane> plane;
        plane->SetOrigin(0, 0, 0.1);
        plane->SetNormal(0, 0, -1);
        m->AddClippingPlane(plane);
        a->SetTexture(Checker());
        a->GetProperty()->ShowTexturesOnBackfaceOff();
      } },
    { "lighting off",
      [](vtkPolyDataMapper* m, vtkActor* a)
      {
        m->SetInputData(Sphere(true));
        a->GetProperty()->LightingOff();
      } },
    { "mixed cell sizes",
      [](vtkPolyDataMapper* m, vtkActor*)
      {
        vtkNew<vtkPlaneSource> plane;
        plane->SetResolution(4, 4);
        plane->SetCenter(0, 0, -0.6);
        vtkNew<vtkAppendPolyData> append;
        append->AddInputData(Sphere(true));
        append->AddInputConnection(plane->GetOutputPort());
        append->Update();
        auto output = vtkSmartPointer<vtkPolyData>::New();
        output->ShallowCopy(append->GetOutput());
        m->SetInputData(output);
      } },
    { "wireframe",
      [](vtkPolyDataMapper* m, vtkActor* a)
      {
        m->SetInputData(Sphere(true));
        a->GetProperty()->SetRepresentationToWireframe();
      } },
    { "thick wireframe",
      [](vtkPolyDataMapper* m, vtkActor* a)
      {
        m->SetInputData(Sphere(true));
        a->GetProperty()->SetRepresentationToWireframe();
        a->GetProperty()->SetLineWidth(4);
      } },
    { "wireframe as tubes",
      [](vtkPolyDataMapper* m, vtkActor* a)
      {
        m->SetInputData(Sphere(true));
        a->GetProperty()->SetRepresentationToWireframe();
        a->GetProperty()->SetLineWidth(5);
        a->GetProperty()->RenderLinesAsTubesOn();
      } },
    { "polyline, miter joins, colors",
      [](vtkPolyDataMapper* m, vtkActor* a)
      {
        m->SetInputData(ZigZag());
        m->SetScalarRange(0, 1);
        a->GetProperty()->SetLineWidth(8);
        a->GetProperty()->SetLineJoin(vtkProperty::LineJoinType::MiterJoin);
      } },
    { "polyline, round caps and joins",
      [](vtkPolyDataMapper* m, vtkActor* a)
      {
        m->SetInputData(ZigZag());
        m->ScalarVisibilityOff();
        a->GetProperty()->SetLineWidth(8);
        a->GetProperty()->SetLineJoin(vtkProperty::LineJoinType::RoundCapRoundJoin);
      } },
    { "points",
      [](vtkPolyDataMapper* m, vtkActor* a)
      {
        m->SetInputData(Sphere(true));
        a->GetProperty()->SetRepresentationToPoints();
      } },
    { "square points",
      [](vtkPolyDataMapper* m, vtkActor* a)
      {
        m->SetInputData(Sphere(true));
        a->GetProperty()->SetRepresentationToPoints();
        a->GetProperty()->SetPointSize(6);
      } },
    { "round points",
      [](vtkPolyDataMapper* m, vtkActor* a)
      {
        m->SetInputData(Sphere(true));
        a->GetProperty()->SetRepresentationToPoints();
        a->GetProperty()->SetPointSize(7);
        a->GetProperty()->SetPoint2DShape(vtkProperty::Point2DShapeType::Round);
      } },
    { "points as spheres",
      [](vtkPolyDataMapper* m, vtkActor* a)
      {
        m->SetInputData(Sphere(true));
        a->GetProperty()->SetRepresentationToPoints();
        a->GetProperty()->SetPointSize(10);
        a->GetProperty()->RenderPointsAsSpheresOn();
      } },
    { "vertex visibility",
      [](vtkPolyDataMapper* m, vtkActor* a)
      {
        m->SetInputData(Sphere(true));
        a->GetProperty()->VertexVisibilityOn();
        a->GetProperty()->SetPointSize(5);
        a->GetProperty()->SetVertexColor(0.2, 0.9, 0.3);
      } },
    { "coincident topology offset",
      [](vtkPolyDataMapper* m, vtkActor*)
      {
        m->SetInputData(Sphere(true));
        m->SetResolveCoincidentTopologyToPolygonOffset();
        m->SetRelativeCoincidentTopologyPolygonOffsetParameters(2, 2);
      } },
  };

  int failures = 0;
  for (const Variant& variant : variants)
  {
    vtkNew<vtkRenderWindow> renWin;
    renWin->SetMultiSamples(0);
    renWin->SetSize(Size, Size);
    renWin->SetShowWindow(false);
    renWin->SetUseOffScreenBuffers(true);
    vtkNew<vtkRenderer> renderer;
    renderer->SetBackground(0.1, 0.1, 0.15);
    renWin->AddRenderer(renderer);
    vtkNew<vtkPolyDataMapper> mapper;
    vtkNew<vtkActor> actor;
    actor->SetMapper(mapper);
    actor->GetProperty()->SetColor(0.9, 0.6, 0.3);
    actor->GetProperty()->SetSpecular(0.4);
    actor->GetProperty()->SetSpecularPower(25);
    renderer->AddActor(actor);
    vtkMapper::SetResolveCoincidentTopologyToDefault();
    variant.Setup(mapper, actor);
    renderer->ResetCamera();

    vtkNew<vtkTest::ErrorObserver> observer;
    mapper->AddObserver(vtkCommand::ErrorEvent, observer);
    mapper->AddObserver(vtkCommand::WarningEvent, observer);
    actor->AddObserver(vtkCommand::WarningEvent, observer);

    auto* shaderProperty = vtkWebGPUShaderProperty::SafeDownCast(actor->GetShaderProperty());
    vtkNew<vtkUnsignedCharArray> templ, slang, probe;
    ::Grab(renWin, templ);
    shaderProperty->UseSlangOn();
    ::Grab(renWin, slang);
    shaderProperty->SetSlangModule(InvertModule);
    shaderProperty->SetSlangSpecialization("ISurfaceOutput", "Invert");
    ::Grab(renWin, probe);

    const double parity = ::DifferingFraction(templ, slang, 1);
    const double probed = ::DifferingFraction(slang, probe, 20);
    const bool ok =
      parity <= 0.001 && probed >= 0.005 && !observer->GetError() && !observer->GetWarning();
    std::cout << (ok ? "PASS " : "FAIL ") << variant.Name << ": " << 100 * parity
              << "% differ from the template, the probe changed " << 100 * probed << "%\n";
    if (observer->GetError() || observer->GetWarning())
    {
      std::cout << "  " << observer->GetErrorMessage() << observer->GetWarningMessage() << "\n";
    }
    failures += ok ? 0 : 1;
  }
  return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
