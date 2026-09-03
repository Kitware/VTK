// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#define VTK_DEPRECATION_LEVEL 0
#include "vtkTestingInteractor.h"

#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkSmartPointer.h"
#include "vtkTesting.h"

#if VTK_MODULE_ENABLE_VTK_ParallelMPI
#include "vtkMPIController.h"
#endif

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkTestingInteractor);

vtkCxxSetSmartPointerMacro(vtkTestingInteractor, Controller, vtkMultiProcessController);

namespace
{
struct vtkTestingInteractorData
{
  int TestReturnStatus = -1;
  double ErrorThreshold = vtkRegressionTester::ErrorThreshold;
  std::string ValidBaseline;
  std::string TempDirectory;
  std::string DataDirectory;
};

vtkTestingInteractorData& GetStaticData()
{
  static vtkTestingInteractorData data;
  return data;
}
}

int vtkTestingInteractor::TestReturnStatus = -1;
double vtkTestingInteractor::ErrorThreshold = vtkRegressionTester::ErrorThreshold;
std::string vtkTestingInteractor::ValidBaseline;
std::string vtkTestingInteractor::TempDirectory;
std::string vtkTestingInteractor::DataDirectory;

void vtkTestingInteractor::SetTestReturnStatus(int status)
{
  auto& Data = GetStaticData();
  vtkTestingInteractor::TestReturnStatus = status;
  Data.TestReturnStatus = status;
}

int vtkTestingInteractor::GetTestReturnStatus()
{
  auto& Data = GetStaticData();
  if (vtkTestingInteractor::TestReturnStatus != Data.TestReturnStatus)
  {
    Data.TestReturnStatus = vtkTestingInteractor::TestReturnStatus;
  }
  return Data.TestReturnStatus;
}

void vtkTestingInteractor::SetErrorThreshold(double threshold)
{
  auto& Data = GetStaticData();
  vtkTestingInteractor::ErrorThreshold = threshold;
  Data.ErrorThreshold = threshold;
}

double vtkTestingInteractor::GetErrorThreshold()
{
  auto& Data = GetStaticData();
  if (vtkTestingInteractor::ErrorThreshold != Data.ErrorThreshold)
  {
    Data.ErrorThreshold = vtkTestingInteractor::ErrorThreshold;
  }
  return Data.ErrorThreshold;
}

void vtkTestingInteractor::SetValidBaseline(std::string baseline)
{
  auto& Data = GetStaticData();
  vtkTestingInteractor::ValidBaseline = baseline;
  Data.ValidBaseline = std::move(baseline);
}

std::string const& vtkTestingInteractor::GetValidBaseline()
{
  auto& Data = GetStaticData();
  if (vtkTestingInteractor::ValidBaseline != Data.ValidBaseline)
  {
    Data.ValidBaseline = vtkTestingInteractor::ValidBaseline;
  }
  return Data.ValidBaseline;
}

void vtkTestingInteractor::SetTempDirectory(std::string dir)
{
  auto& Data = GetStaticData();
  vtkTestingInteractor::TempDirectory = dir;
  Data.TempDirectory = std::move(dir);
}

std::string const& vtkTestingInteractor::GetTempDirectory()
{
  auto& Data = GetStaticData();
  if (vtkTestingInteractor::TempDirectory != Data.TempDirectory)
  {
    Data.TempDirectory = vtkTestingInteractor::TempDirectory;
  }
  return Data.TempDirectory;
}

void vtkTestingInteractor::SetDataDirectory(std::string dir)
{
  auto& Data = GetStaticData();
  vtkTestingInteractor::DataDirectory = dir;
  Data.DataDirectory = std::move(dir);
}

std::string const& vtkTestingInteractor::GetDataDirectory()
{
  auto& Data = GetStaticData();
  if (vtkTestingInteractor::DataDirectory != Data.DataDirectory)
  {
    Data.DataDirectory = vtkTestingInteractor::DataDirectory;
  }
  return Data.DataDirectory;
}

//------------------------------------------------------------------------------
vtkTestingInteractor::vtkTestingInteractor()
{
#if VTK_MODULE_ENABLE_VTK_ParallelMPI
  this->Controller = vtkSmartPointer<vtkMPIController>::New();
  // IF MPI is not set up, set to nullptr
  if (!this->Controller->GetCommunicator())
  {
    this->Controller = nullptr;
  }
#else
  this->Controller = nullptr;
#endif
}

//------------------------------------------------------------------------------
vtkMultiProcessController* vtkTestingInteractor::GetController() const
{
  return this->Controller;
}

//------------------------------------------------------------------------------
// Start normally starts an event loop. This iterator uses vtkTesting
// to grab the render window and compare the results to a baseline image
void vtkTestingInteractor::Start()
{
  vtkSmartPointer<vtkTesting> testing = vtkSmartPointer<vtkTesting>::New();
  testing->SetRenderWindow(this->GetRenderWindow());
  testing->SetController(this->Controller);

  // Location of the temp directory for testing
  testing->AddArgument("-T");
  testing->AddArgument(vtkTestingInteractor::TempDirectory.c_str());

  // Location of the Data directory. If NOTFOUND, suppress regression
  // testing
  if (vtkTestingInteractor::DataDirectory != "VTK_DATA_ROOT-NOTFOUND")
  {
    testing->AddArgument("-D");
    testing->AddArgument(vtkTestingInteractor::DataDirectory.c_str());

    // The name of the valid baseline image
    testing->AddArgument("-V");
    std::string valid = vtkTestingInteractor::ValidBaseline;
    testing->AddArgument(valid.c_str());

    // Regression test the image
    vtkTestingInteractor::TestReturnStatus =
      testing->RegressionTest(vtkTestingInteractor::ErrorThreshold);
  }
}

//------------------------------------------------------------------------------
void vtkTestingInteractor::PrintSelf(ostream& os, vtkIndent indent)
{
  // Superclass typedef defined in vtkTypeMacro() found in vtkSetGet.h
  this->Superclass::PrintSelf(os, indent);
}
VTK_ABI_NAMESPACE_END
