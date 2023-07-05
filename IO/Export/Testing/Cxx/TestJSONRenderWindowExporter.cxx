// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkActor.h"
#include "vtkLight.h"
#include "vtkNew.h"
#include "vtkPolyDataMapper.h"
#include "vtkRTAnalyticSource.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkSmartVolumeMapper.h"
#include "vtkSphereSource.h"
#include "vtkTestUtilities.h"
#include "vtkVolume.h"
#include "vtkWindowNode.h"
#include "vtksys/SystemTools.hxx"

#include "vtkArchiver.h"
#include "vtkJSONRenderWindowExporter.h"

int TestJSONRenderWindowExporter(int argc, char* argv[])
{
  char* tempDir =
    vtkTestUtilities::GetArgOrEnvOrDefault("-T", argc, argv, "VTK_TEMP_DIR", "Testing/Temporary");
  if (!tempDir)
  {
    std::cout << "Could not determine temporary directory.\n";
    return EXIT_FAILURE;
  }
  std::string testDirectory = tempDir;
  delete[] tempDir;

  std::string filename = testDirectory + std::string("/") + std::string("ExportVtkJS");

  vtkNew<vtkSphereSource> sphere;
  vtkNew<vtkPolyDataMapper> pmap;
  pmap->SetInputConnection(sphere->GetOutputPort());

  vtkNew<vtkRTAnalyticSource> wavelet;
  wavelet->SetWholeExtent(-10, 10, -10, 10, -10, 10);
  wavelet->SetCenter(0, 0, 0);

  vtkNew<vtkSmartVolumeMapper> volumeMapper;
  volumeMapper->SetBlendModeToComposite();
  volumeMapper->SetInputConnection(wavelet->GetOutputPort());

  vtkNew<vtkRenderWindow> rwin;

  vtkNew<vtkRenderer> ren;
  rwin->AddRenderer(ren);

  vtkNew<vtkLight> light;
  ren->AddLight(light);

  vtkNew<vtkActor> actor;
  ren->AddActor(actor);
  actor->SetMapper(pmap);

  vtkNew<vtkVolume> volume;
  ren->AddVolume(volume);
  volume->SetMapper(volumeMapper);

  vtkNew<vtkJSONRenderWindowExporter> exporter;
  exporter->GetArchiver()->SetArchiveName(filename.c_str());
  exporter->SetRenderWindow(rwin);
  exporter->Write();

  vtksys::SystemTools::RemoveADirectory(filename);

  return EXIT_SUCCESS;
}
