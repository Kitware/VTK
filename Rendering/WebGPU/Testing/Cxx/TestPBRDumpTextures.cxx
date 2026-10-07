// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkActor.h"
#include "vtkFloatArray.h"
#include "vtkImageData.h"
#include "vtkNew.h"
#include "vtkPointData.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkSphereSource.h"
#include "vtkStringFormatter.h"
#include "vtkTestUtilities.h"
#include "vtkTexture.h"

#include <vtksys/SystemTools.hxx>

#include <iostream>
#include <string>

int TestPBRDumpTextures(int argc, char* argv[])
{
  char* tempDirectory =
    vtkTestUtilities::GetArgOrEnvOrDefault("-T", argc, argv, "VTK_TEMP_DIR", "Testing/Temporary");
  const std::string directory = std::string(tempDirectory) + "/TestPBRDumpTextures";
  delete[] tempDirectory;
  vtksys::SystemTools::RemoveADirectory(directory);
  vtksys::SystemTools::MakeDirectory(directory);
  vtksys::SystemTools::PutEnv("VTK_WEBGPU_PBR_DUMP_DIR=" + directory);

  vtkNew<vtkImageData> environment;
  environment->SetDimensions(64, 32, 1);
  vtkNew<vtkFloatArray> colors;
  colors->SetNumberOfComponents(3);
  colors->SetNumberOfTuples(64 * 32);
  for (int y = 0; y < 32; ++y)
  {
    for (int x = 0; x < 64; ++x)
    {
      const float sky = static_cast<float>(y) / 31.0f;
      colors->SetTuple3(y * 64 + x, 0.2 + 0.8 * sky, 0.3 + 0.7 * sky, 1.0 + 3.0 * sky);
    }
  }
  environment->GetPointData()->SetScalars(colors);

  vtkNew<vtkTexture> texture;
  texture->SetInputData(environment);
  texture->SetColorModeToDirectScalars();
  texture->InterpolateOn();

  vtkNew<vtkSphereSource> sphere;
  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputConnection(sphere->GetOutputPort());
  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper);
  actor->GetProperty()->SetInterpolationToPBR();

  vtkNew<vtkRenderer> renderer;
  renderer->UseImageBasedLightingOn();
  renderer->SetEnvironmentTexture(texture);
  renderer->AddActor(actor);
  vtkNew<vtkRenderWindow> window;
  window->SetSize(64, 64);
  window->AddRenderer(renderer);
  window->Render();

  vtksys::SystemTools::UnPutEnv("VTK_WEBGPU_PBR_DUMP_DIR");

  int missing = 0;
  const auto check = [&](const std::string& name)
  {
    if (!vtksys::SystemTools::FileExists(directory + "/" + name))
    {
      std::cerr << "Missing " << name << std::endl;
      ++missing;
    }
  };
  check("brdf_lookup_table.pfm");
  check("spherical_harmonics.txt");
  for (int face = 0; face < 6; ++face)
  {
    check("irradiance_face" + vtk::to_string(face) + ".pfm");
    for (int level = 0; level < 5; ++level)
    {
      check("prefilter_level" + vtk::to_string(level) + "_face" + vtk::to_string(face) + ".pfm");
    }
  }
  return missing == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
