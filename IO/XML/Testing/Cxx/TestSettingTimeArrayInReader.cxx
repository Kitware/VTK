/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestXML.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME Test of time array selection in vtkXMLReader
// .SECTION This test reads a multi block data set featuring
// a time array named "T". If tries to load it, then disable it.
//

#include "vtkInformation.h"
#include "vtkNew.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkStringArray.h"
#include "vtkXMLMultiBlockDataReader.h"
#include "vtkXMLReader.h"

#include <cmath>
#include <string>

int TestSettingTimeArrayInReader(int argc, char* argv[])
{
  // Need to get the data root.
  const char* data_root = nullptr;
  for (int i = 0; i < argc - 1; i++)
  {
    if (strcmp("-D", argv[i]) == 0)
    {
      data_root = argv[i + 1];
      break;
    }
  }
  if (!data_root)
  {
    vtkGenericWarningMacro("Need to specify the directory to VTK_DATA_ROOT with -D <dir>.");
    return EXIT_FAILURE;
  }

  vtkNew<vtkXMLMultiBlockDataReader> reader;
  reader->SetFileName(
    (std::string(data_root) + std::string("/Data/mg_diff/mg_diff_0062.vtm")).c_str());
  reader->Update();

  reader->SetActiveTimeDataArrayName("T");
  if (strcmp(reader->GetActiveTimeDataArrayName(), "T") != 0)
  {
    vtkGenericWarningMacro("Time data selection does not work");
    return EXIT_FAILURE;
  }

  reader->Update();

  vtkInformation* info = reader->GetOutputInformation(0);

  double* timeSteps = info->Get(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
  double* timeRange = info->Get(vtkStreamingDemandDrivenPipeline::TIME_RANGE());
  if (!timeSteps || !timeRange || std::fabs(*timeSteps - 0.000107247) > 0.000000001 ||
    std::fabs(timeRange[0] - 0.000107247) > 0.000000001 ||
    std::fabs(timeRange[1] - 0.000107247) > 0.000000001)
  {
    vtkGenericWarningMacro("Time data not set properly");
    return EXIT_FAILURE;
  }

  reader->SetActiveTimeDataArrayName(nullptr);
  reader->Update();

  if (info->Get(vtkStreamingDemandDrivenPipeline::TIME_STEPS()) ||
    info->Get(vtkStreamingDemandDrivenPipeline::TIME_RANGE()))
  {
    vtkGenericWarningMacro("Time data not set properly. It should be set to default behavior.");
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
