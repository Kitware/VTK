// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// Checks that vtkProp::GENERAL_TEXTURE_TRANSFORM() reaches the backend and is applied with the
// right orientation and a homogeneous divide. Each transform maps every coordinate onto the center
// of one texel, so the expected color does not depend on rasterization or on texture filtering.

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkFloatArray.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkLookupTable.h"
#include "vtkMatrix4x4.h"
#include "vtkNew.h"
#include "vtkPlaneSource.h"
#include "vtkPointData.h"
#include "vtkPolyDataMapper.h"
#include "vtkProp.h"
#include "vtkProperty.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkTexture.h"
#include "vtkTransform.h"
#include "vtkUnsignedCharArray.h"
#include "vtkWindowToImageFilter.h"

#include <iostream>
#include <set>
#include <tuple>

namespace
{
using Color = std::tuple<int, int, int>;

// The OpenGL mappers declare tcMatrix only when they build the shader, so a scene that uses a
// texture transform has one before its first render.
vtkSmartPointer<vtkRenderWindow> Show(vtkActor* actor)
{
  vtkNew<vtkRenderer> renderer;
  renderer->AddActor(actor);
  renderer->SetBackground(0.0, 0.0, 0.0);

  vtkSmartPointer<vtkRenderWindow> window = vtkSmartPointer<vtkRenderWindow>::New();
  window->SetSize(300, 300);
  window->ShowWindowOff();
  window->AddRenderer(renderer);
  renderer->ResetCamera();
  // The plane overfills the view, so the sampled center stays on it even when the window does not
  // get the requested aspect ratio.
  renderer->GetActiveCamera()->Zoom(2.0);
  return window;
}

// Colors in the central half of the window.
std::set<Color> CenterColors(vtkRenderWindow* window)
{
  window->Render();
  vtkNew<vtkWindowToImageFilter> capture;
  capture->SetInput(window);
  capture->Update();
  vtkImageData* image = capture->GetOutput();
  int dimensions[3];
  image->GetDimensions(dimensions);
  std::set<Color> colors;
  for (int y = dimensions[1] / 4; y < 3 * dimensions[1] / 4; ++y)
  {
    for (int x = dimensions[0] / 4; x < 3 * dimensions[0] / 4; ++x)
    {
      auto* pixel = static_cast<unsigned char*>(image->GetScalarPointer(x, y, 0));
      colors.insert(Color(pixel[0], pixel[1], pixel[2]));
    }
  }
  return colors;
}

void Report(const char* label, const std::set<Color>& colors)
{
  std::cerr << label << " found " << colors.size() << " color(s):";
  for (const auto& color : colors)
  {
    std::cerr << " (" << std::get<0>(color) << ", " << std::get<1>(color) << ", "
              << std::get<2>(color) << ")";
  }
  std::cerr << std::endl;
}

bool ExpectOnly(const char* label, const std::set<Color>& colors, const Color& expected)
{
  if (colors.size() == 1 && *colors.begin() == expected)
  {
    return true;
  }
  std::cerr << "The texture transform was not applied as expected: every sample should be ("
            << std::get<0>(expected) << ", " << std::get<1>(expected) << ", "
            << std::get<2>(expected) << ")." << std::endl;
  Report(label, colors);
  return false;
}

// Sends every (u, v) to (x / w, y / w).
vtkSmartPointer<vtkTransform> ConstantCoordinate(double x, double y, double w)
{
  vtkNew<vtkMatrix4x4> matrix;
  matrix->Zero();
  matrix->SetElement(0, 3, x);
  matrix->SetElement(1, 3, y);
  matrix->SetElement(2, 2, 1.0);
  matrix->SetElement(3, 3, w);
  vtkSmartPointer<vtkTransform> transform = vtkSmartPointer<vtkTransform>::New();
  transform->SetMatrix(matrix);
  return transform;
}

// A plane with a 2x2 texture whose texels all differ. Their centers are at 0.25 and 0.75.
vtkSmartPointer<vtkActor> TexturedPlane(vtkTransform* textureTransform)
{
  vtkNew<vtkImageData> image;
  image->SetDimensions(2, 2, 1);
  vtkNew<vtkUnsignedCharArray> texels;
  texels->SetNumberOfComponents(3);
  texels->SetNumberOfTuples(4);
  texels->SetTuple3(0, 255, 0, 0);
  texels->SetTuple3(1, 0, 255, 0);
  texels->SetTuple3(2, 0, 0, 255);
  texels->SetTuple3(3, 255, 255, 255);
  image->GetPointData()->SetScalars(texels);

  vtkNew<vtkTexture> texture;
  texture->SetInputData(image);
  texture->InterpolateOff();
  texture->MipmapOff();
  texture->SetTransform(textureTransform);

  vtkNew<vtkPlaneSource> plane;
  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputConnection(plane->GetOutputPort());

  vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
  actor->SetMapper(mapper);
  actor->SetTexture(texture);
  actor->GetProperty()->LightingOff();
  return actor;
}

bool CheckSurfaceTexture()
{
  bool success = true;

  vtkSmartPointer<vtkActor> plain = TexturedPlane(nullptr);
  const std::set<Color> plainColors = CenterColors(Show(plain));
  if (plainColors.size() < 2)
  {
    std::cerr << "The texture is not mapped across the plane: the sampled region should show "
                 "several texels without a texture transform."
              << std::endl;
    Report("without a transform,", plainColors);
    success = false;
  }

  // (1.125, 0.375) over w = 1.5 is (0.75, 0.25), the green texel. Keep it asymmetric with w != 1:
  // a wrong transpose, a dropped translation and a skipped divide all render the (0, 0) texel, so
  // a matrix aimed there would pass under each of those mistakes.
  vtkSmartPointer<vtkActor> actor = TexturedPlane(ConstantCoordinate(1.125, 0.375, 1.5));
  vtkSmartPointer<vtkRenderWindow> window = Show(actor);
  success &= ExpectOnly("with a transform,", CenterColors(window), Color(0, 255, 0));

  // A change after the first render must reach the next frame: (0.25, 0.75) is the blue texel.
  actor->GetTexture()->SetTransform(ConstantCoordinate(0.375, 1.125, 1.5));
  success &= ExpectOnly("after changing the transform,", CenterColors(window), Color(0, 0, 255));

  return success;
}

bool CheckLookupTableCoordinate()
{
  auto render = [](vtkTransform* textureTransform)
  {
    vtkNew<vtkPlaneSource> plane;
    plane->SetResolution(16, 16);
    plane->Update();

    // Scalars varying across the plane, so the lookup table spreads several colors over it.
    vtkPolyData* polyData = plane->GetOutput();
    vtkNew<vtkFloatArray> scalars;
    scalars->SetName("scalars");
    scalars->SetNumberOfTuples(polyData->GetNumberOfPoints());
    for (vtkIdType i = 0; i < polyData->GetNumberOfPoints(); ++i)
    {
      double point[3];
      polyData->GetPoint(i, point);
      scalars->SetValue(i, static_cast<float>(point[0] + 0.5));
    }
    polyData->GetPointData()->SetScalars(scalars);

    vtkNew<vtkLookupTable> lut;
    lut->SetNumberOfTableValues(4);
    lut->SetTableValue(0, 1.0, 0.0, 0.0);
    lut->SetTableValue(1, 0.0, 1.0, 0.0);
    lut->SetTableValue(2, 0.0, 0.0, 1.0);
    lut->SetTableValue(3, 1.0, 1.0, 0.0);
    lut->SetTableRange(0.0, 1.0);
    lut->Build();

    vtkNew<vtkPolyDataMapper> mapper;
    mapper->SetInputData(polyData);
    mapper->SetLookupTable(lut);
    mapper->SetScalarRange(0.0, 1.0);
    mapper->ScalarVisibilityOn();
    mapper->SetInterpolateScalarsBeforeMapping(1);

    vtkNew<vtkActor> actor;
    actor->SetMapper(mapper);
    actor->GetProperty()->LightingOff();
    if (textureTransform)
    {
      // No texture of its own here, so publish the key the way vtkGLTFImporter does.
      vtkNew<vtkInformation> keys;
      keys->Set(vtkProp::GENERAL_TEXTURE_TRANSFORM(), textureTransform->GetMatrix()->GetData(), 16);
      actor->SetPropertyKeys(keys);
    }
    return CenterColors(Show(actor));
  };

  bool success = true;

  const std::set<Color> plain = render(nullptr);
  if (plain.size() < 2)
  {
    std::cerr << "The scalars are not spread over the plane: the sampled region should show "
                 "several lookup table colors without a texture transform."
              << std::endl;
    Report("lookup table without a transform,", plain);
    success = false;
  }

  // vtkMapper's color texture is 6x2 here: the four entries between a below and an above range
  // entry, over a row for NaN. (0.625, 0.375) over w = 1.5 is (5/12, 1/4), the center of the green
  // entry, rather than u = 0.5, a texel boundary that backends round either way.
  success &= ExpectOnly("lookup table with a transform,",
    render(ConstantCoordinate(0.625, 0.375, 1.5)), Color(0, 255, 0));

  return success;
}
}

int TestTextureTransform(int, char*[])
{
  bool success = CheckSurfaceTexture();
  success &= CheckLookupTableCoordinate();
  return success ? EXIT_SUCCESS : EXIT_FAILURE;
}
