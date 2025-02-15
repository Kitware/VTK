// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkTesting.h"

#include "vtkAlgorithmOutput.h"
#include "vtkDataArray.h"
#include "vtkDataArrayRange.h"
#include "vtkDataSet.h"
#include "vtkDoubleArray.h"
#include "vtkDummyController.h"
#include "vtkFloatArray.h"
#include "vtkImageClip.h"
#include "vtkImageData.h"
#include "vtkImageDifference.h"
#include "vtkImageExtractComponents.h"
#include "vtkImageRGBToXYZ.h"
#include "vtkImageSSIM.h"
#include "vtkImageShiftScale.h"
#include "vtkImageXYZToLAB.h"
#include "vtkInformation.h"
#include "vtkInteractorEventRecorder.h"
#include "vtkLogger.h"
#include "vtkMultiProcessController.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPNGReader.h"
#include "vtkPNGWriter.h"
#include "vtkPointData.h"
#include "vtkPointSet.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSmartPointer.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkTimerLog.h"
#include "vtkWindowToImageFilter.h"

#include "vtkImageRGBToHSI.h"

#include <sstream>
#include <vtksys/SystemTools.hxx>

#include <array>
#include <numeric>

#ifdef __EMSCRIPTEN__
#include "vtkTestUtilities.h"
#endif
#include "vtkXMLImageDataWriter.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkTesting);
vtkCxxSetObjectMacro(vtkTesting, RenderWindow, vtkRenderWindow);

using std::string;
using std::vector;

//------------------------------------------------------------------------------
// Find in command tail, failing that find in environment,
// failing that return a default.
// Up to caller to delete the string returned.
static string vtkTestingGetArgOrEnvOrDefault(
  const string& argName, // argument idnetifier flag. eg "-D"
  vector<string>& argv,  // command tail
  const string& env,     // environment variable name to find
  const string& def)     // default to use if "env" is not found.
{
  string argValue;

  // Search command tail.
  int argc = static_cast<int>(argv.size());
  for (int i = 0; i < argc; i++)
  {
    if ((i < (argc - 1)) && (argName == argv[i]))
    {
      argValue = argv[i + 1];
    }
  }
  // If not found search environment.
  if (argValue.empty() && !(env.empty() || def.empty()))
  {
    char* foundenv = getenv(env.c_str());
    if (foundenv)
    {
      argValue = foundenv;
    }
    else
    {
      // Not found, fall back to default.
      argValue = def;
    }
  }

  return argValue;
}

//------------------------------------------------------------------------------
// Description:
// Sum the L2 Norm point wise over all tuples. Each term
// is scaled by the magnitude of one of the inputs.
// Return sum and the number of terms.
template <class T>
vtkIdType AccumulateScaledL2Norm(T* pA, // pointer to first data array
  T* pB,                                // pointer to second data array
  vtkIdType nTups,                      // number of tuples
  int nComps,                           // number of comps
  double& SumModR)                      // result
{
  //
  SumModR = 0.0;
  for (vtkIdType i = 0; i < nTups; ++i)
  {
    double modR = 0.0;
    double modA = 0.0;
    for (int q = 0; q < nComps; ++q)
    {
      double a = pA[q];
      double b = pB[q];
      modA += a * a;
      double r = b - a;
      modR += r * r;
    }
    modA = sqrt(modA);
    modA = modA < 1.0 ? 1.0 : modA;
    SumModR += sqrt(modR) / modA;
    pA += nComps;
    pB += nComps;
  }
  return nTups;
}

//=============================================================================
vtkTesting::vtkTesting()
{
  this->FrontBuffer = 0;
  this->RenderWindow = nullptr;
  this->ValidImageFileName = nullptr;
  this->ImageDifference = 0;
  this->DataRoot = nullptr;
  this->TempDirectory = nullptr;
  this->BorderOffset = 0;
  this->Verbose = 0;
  this->Controller = vtkSmartPointer<vtkDummyController>::New();

  // on construction we start the timer
  this->StartCPUTime = vtkTimerLog::GetCPUTime();
  this->StartWallTime = vtkTimerLog::GetUniversalTime();
}

//------------------------------------------------------------------------------
vtkTesting::~vtkTesting()
{
  this->SetRenderWindow(nullptr);
  this->SetValidImageFileName(nullptr);
  this->SetDataRoot(nullptr);
  this->SetTempDirectory(nullptr);
}

//------------------------------------------------------------------------------
vtkMultiProcessController* vtkTesting::GetController() const
{
  return this->Controller;
}

//------------------------------------------------------------------------------
void vtkTesting::SetController(vtkMultiProcessController* controller)
{
  vtkSetSmartPointerBodyMacro(Controller, vtkMultiProcessController, controller);
  if (!this->Controller)
  {
    this->Controller = vtkSmartPointer<vtkDummyController>::New();
  }
}

//------------------------------------------------------------------------------
void vtkTesting::AddArgument(const char* arg)
{
  this->Args.emplace_back(arg);
}

//------------------------------------------------------------------------------
void vtkTesting::AddArguments(int argc, const char** argv)
{
  for (int i = 0; i < argc; ++i)
  {
    this->Args.emplace_back(argv[i]);
  }
}

//------------------------------------------------------------------------------
void vtkTesting::AddArguments(int argc, char** argv)
{
  for (int i = 0; i < argc; ++i)
  {
    this->Args.emplace_back(argv[i]);
  }
}

//------------------------------------------------------------------------------
char* vtkTesting::GetArgument(const char* argName)
{
  string argValue = vtkTestingGetArgOrEnvOrDefault(argName, this->Args, "", "");

  char* cArgValue = new char[argValue.size() + 1];
  strcpy(cArgValue, argValue.c_str());

  return cArgValue;
}
//------------------------------------------------------------------------------
void vtkTesting::CleanArguments()
{
  this->Args.erase(this->Args.begin(), this->Args.end());
}
//------------------------------------------------------------------------------
const char* vtkTesting::GetDataRoot()
{
#ifdef VTK_DATA_ROOT
  string dr = vtkTestingGetArgOrEnvOrDefault("-D", this->Args, "VTK_DATA_ROOT", VTK_DATA_ROOT);
#else
  string dr =
    vtkTestingGetArgOrEnvOrDefault("-D", this->Args, "VTK_DATA_ROOT", "../../../../VTKData");
#endif
  this->SetDataRoot(vtksys::SystemTools::CollapseFullPath(dr).c_str());

  return this->DataRoot;
}
//------------------------------------------------------------------------------
const char* vtkTesting::GetTempDirectory()
{
  string td =
    vtkTestingGetArgOrEnvOrDefault("-T", this->Args, "VTK_TEMP_DIR", "../../../Testing/Temporary");
  this->SetTempDirectory(vtksys::SystemTools::CollapseFullPath(td).c_str());

  return this->TempDirectory;
}
//------------------------------------------------------------------------------
const char* vtkTesting::GetValidImageFileName()
{
  this->SetValidImageFileName(nullptr);
  if (!this->IsValidImageSpecified())
  {
    return this->ValidImageFileName;
  }

  string baseline =
    vtkTestingGetArgOrEnvOrDefault("-B", this->Args, "VTK_BASELINE_ROOT", this->GetDataRoot());

  for (size_t i = 0; i < (this->Args.size() - 1); ++i)
  {
    if (this->Args[i] == "-V")
    {
      const char* ch = this->Args[i + 1].c_str();
      if (ch[0] == '/'
#if defined(_WIN32) ||                                                                             \
  defined(__EMSCRIPTEN__) // Emscripten too, because the file could be on a windows server.
        || (ch[0] >= 'a' && ch[0] <= 'z' && ch[1] == ':') ||
        (ch[0] >= 'A' && ch[0] <= 'Z' && ch[1] == ':')
#endif
      )
      {
        baseline = this->Args[i + 1];
      }
      else
      {
        baseline += "/";
        baseline += this->Args[i + 1];
      }
      break;
    }
  }

  this->SetValidImageFileName(baseline.c_str());

  return this->ValidImageFileName;
}

//------------------------------------------------------------------------------
bool vtkTesting::GetMesaVersion(vtkRenderWindow* renderWindow, int version[3])
{
  const std::string glCaps = renderWindow->ReportCapabilities();
  bool mesaInUse = glCaps.find("OpenGL vendor string:  Mesa/X.org") != std::string::npos;
  if (!mesaInUse)
  {
    return false;
  }
  const char* versionPtr =
    vtksys::SystemTools::FindLastString(glCaps.c_str(), "OpenGL version string");
  const auto lines = vtksys::SystemTools::SplitString(std::string(versionPtr), '\n');
  const auto words = vtksys::SystemTools::SplitString(lines[0], ' ');
  auto versionIter = std::find(words.begin(), words.end(), "Mesa");
  if (versionIter != words.end())
  {
    const auto versionString = (++versionIter)->c_str();
    const auto versionNumbers = vtksys::SystemTools::SplitString(versionString, '.');
    for (int i = 0; i < 3; ++i)
    {
      version[i] = std::stoi(versionNumbers[i]);
    }
  }
  return true;
}

//------------------------------------------------------------------------------
int vtkTesting::IsInteractiveModeSpecified()
{
  for (size_t i = 0; i < this->Args.size(); ++i)
  {
    if (this->Args[i] == "-I")
    {
      return 1;
    }
  }
  return 0;
}
//------------------------------------------------------------------------------
int vtkTesting::IsFlagSpecified(const char* flag)
{
  for (size_t i = 0; i < this->Args.size(); ++i)
  {
    if (this->Args[i] == flag)
    {
      return 1;
    }
  }
  return 0;
}
//------------------------------------------------------------------------------
int vtkTesting::IsValidImageSpecified()
{
  for (size_t i = 1; i < this->Args.size(); ++i)
  {
    if (this->Args[i - 1] == "-V")
    {
      return 1;
    }
  }
  return 0;
}
//------------------------------------------------------------------------------
char* vtkTesting::IncrementFileName(const char* fname, int count)
{
  char counts[256];
  snprintf(counts, sizeof(counts), "%d", count);

  int orgLen = static_cast<int>(strlen(fname));
  if (orgLen < 5)
  {
    return nullptr;
  }
  int extLen = static_cast<int>(strlen(counts));
  char* newFileName = new char[orgLen + extLen + 2];
  strcpy(newFileName, fname);

  newFileName[orgLen - 4] = '_';
  int i, marker;
  for (marker = orgLen - 3, i = 0; marker < orgLen - 3 + extLen; marker++, i++)
  {
    newFileName[marker] = counts[i];
  }
  strcpy(newFileName + marker, ".png");

  return newFileName;
}
//------------------------------------------------------------------------------
int vtkTesting::LookForFile(const char* newFileName)
{
  if (!newFileName)
  {
    return 0;
  }
  vtksys::SystemTools::Stat_t fs;
  if (vtksys::SystemTools::Stat(newFileName, &fs) != 0)
  {
    return 0;
  }
  else
  {
    return 1;
  }
}

//------------------------------------------------------------------------------
void vtkTesting::SetFrontBuffer(vtkTypeBool frontBuffer)
{
  vtkWarningMacro("SetFrontBuffer method is deprecated and has no effect anymore.");
  this->FrontBuffer = frontBuffer;
}

//------------------------------------------------------------------------------
int vtkTesting::RegressionTestAndCaptureOutput(double thresh, ostream& os)
{
  const int result = this->RegressionTest(thresh, os);

  os << "<DartMeasurement name=\"WallTime\" type=\"numeric/double\">";
  os << vtkTimerLog::GetUniversalTime() - this->StartWallTime;
  os << "</DartMeasurement>\n";
  os << "<DartMeasurement name=\"CPUTime\" type=\"numeric/double\">";
  os << vtkTimerLog::GetCPUTime() - this->StartCPUTime;
  os << "</DartMeasurement>\n";

  return result;
}

//------------------------------------------------------------------------------
int vtkTesting::RegressionTest(double thresh)
{
  const int result = this->RegressionTest(thresh, cout);
  cout << "<DartMeasurement name=\"WallTime\" type=\"numeric/double\">";
  cout << vtkTimerLog::GetUniversalTime() - this->StartWallTime;
  cout << "</DartMeasurement>\n";
  cout << "<DartMeasurement name=\"CPUTime\" type=\"numeric/double\">";
  cout << vtkTimerLog::GetCPUTime() - this->StartCPUTime;
  cout << "</DartMeasurement>\n";
  return result;
}

//------------------------------------------------------------------------------
int vtkTesting::RegressionTest(double thresh, std::string& output)
{
  std::ostringstream os;
  const int result = this->RegressionTest(thresh, os);
  output = os.str();
  return result;
}

//------------------------------------------------------------------------------
int vtkTesting::RegressionTest(double thresh, ostream& os)
{
  vtkNew<vtkWindowToImageFilter> rtW2if;
  rtW2if->SetInput(this->RenderWindow);

  for (unsigned int i = 0; i < this->Args.size(); ++i)
  {
    if ("-FrontBuffer" == this->Args[i])
    {
      vtkWarningMacro("-FrontBuffer option is deprecated and has no effet anymore.");
      this->FrontBufferOn();
    }
    else if ("-NoRerender" == this->Args[i])
    {
      rtW2if->ShouldRerenderOff();
    }
  }

  std::ostringstream out1;
  // perform and extra render to make sure it is displayed
  int swapBuffers = this->RenderWindow->GetSwapBuffers();
  // since we're reading from back-buffer, it's essential that we turn off swapping
  // otherwise what remains in the back-buffer after the swap is undefined by OpenGL specs.
  this->RenderWindow->SwapBuffersOff();
  this->RenderWindow->Render();
  rtW2if->ReadFrontBufferOff();
  rtW2if->Update();
  this->RenderWindow->SetSwapBuffers(swapBuffers); // restore swap state.
  int res = this->RegressionTest(rtW2if, thresh, out1);
  int recvRes;
  this->Controller->AllReduce(&res, &recvRes, 1, vtkCommunicator::MIN_OP);
  if (recvRes == FAILED)
  {
    std::ostringstream out2;
    // tell it to read front buffer
    rtW2if->ReadFrontBufferOn();
    rtW2if->Update();
    res = this->RegressionTest(rtW2if, thresh, out2);
    this->Controller->AllReduce(&res, &recvRes, 1, vtkCommunicator::MAX_OP);
    // If both tests fail, rerun the backbuffer tests to recreate the test
    // image. Otherwise an incorrect image will be uploaded to CDash.
    if (recvRes == PASSED)
    {
      os << out2.str();
    }
    else
    {
      // we failed both back and front buffers so
      // to help us debug, write out renderwindow capabilities
      if (this->RenderWindow)
      {
        os << this->RenderWindow->ReportCapabilities();
      }
      rtW2if->ReadFrontBufferOff();
      rtW2if->Update();
      return this->RegressionTest(rtW2if, thresh, os);
    }
  }
  else
  {
    os << out1.str();
  }
  return this->Controller->GetLocalProcessId() == 0 ? res : NOT_RUN;
}

//------------------------------------------------------------------------------
int vtkTesting::RegressionTest(const string& pngFileName, double thresh)
{
  const int result = this->RegressionTest(pngFileName, thresh, cout);
  cout << "<DartMeasurement name=\"WallTime\" type=\"numeric/double\">";
  cout << vtkTimerLog::GetUniversalTime() - this->StartWallTime;
  cout << "</DartMeasurement>\n";
  cout << "<DartMeasurement name=\"CPUTime\" type=\"numeric/double\">";
  cout << vtkTimerLog::GetCPUTime() - this->StartCPUTime;
  cout << "</DartMeasurement>\n";
  return result;
}

//------------------------------------------------------------------------------
int vtkTesting::RegressionTest(const std::string& pngFileName, double thresh, std::string& output)
{
  std::ostringstream os;
  const int result = this->RegressionTest(pngFileName, thresh, os);
  output = os.str();
  return result;
}

//------------------------------------------------------------------------------
int vtkTesting::RegressionTest(const string& pngFileName, double thresh, ostream& os)
{
  vtkNew<vtkPNGReader> inputReader;

#ifdef __EMSCRIPTEN__
  std::string sandboxName = vtkEmscriptenTestUtilities::PreloadDataFile(pngFileName.c_str());
  inputReader->SetFileName(sandboxName.c_str());
#else
  inputReader->SetFileName(pngFileName.c_str());
#endif
  inputReader->Update();

  vtkAlgorithm* src = inputReader;

  vtkSmartPointer<vtkImageExtractComponents> extract;
  // Convert rgba to rgb if needed
  if (inputReader->GetOutput() && inputReader->GetOutput()->GetNumberOfScalarComponents() == 4)
  {
    extract = vtkSmartPointer<vtkImageExtractComponents>::New();
    extract->SetInputConnection(src->GetOutputPort());
    extract->SetComponents(0, 1, 2);
    extract->Update();
    src = extract;
  }

  return this->RegressionTest(src, thresh, os);
}

//------------------------------------------------------------------------------
int vtkTesting::RegressionTest(vtkAlgorithm* imageSource, double thresh)
{
  const int result = this->RegressionTest(imageSource, thresh, cout);
  cout << "<DartMeasurement name=\"WallTime\" type=\"numeric/double\">";
  cout << vtkTimerLog::GetUniversalTime() - this->StartWallTime;
  cout << "</DartMeasurement>\n";
  cout << "<DartMeasurement name=\"CPUTime\" type=\"numeric/double\">";
  cout << vtkTimerLog::GetCPUTime() - this->StartCPUTime;
  cout << "</DartMeasurement>\n";

  return result;
}

int vtkTesting::RegressionTest(vtkAlgorithm* imageSource, double thresh, std::string& output)
{
  std::ostringstream os;
  const int result = this->RegressionTest(imageSource, thresh, os);
  output = os.str();
  return result;
}

//------------------------------------------------------------------------------
int vtkTesting::RegressionTest(vtkAlgorithm* imageSource, double thresh, ostream& os)
{
  // do a get to compute the real value
  this->GetValidImageFileName();
  string tmpDir = this->GetTempDirectory();

  // Make sure the tmpDir actual exists
  if (!vtksys::SystemTools::MakeDirectory(tmpDir))
  {
    vtkWarningMacro("Could not create a temporary directory to write images to:'"
      << tmpDir << "'. Output images may be missing.");
  }

  // construct the names for the error images
  string validName = this->ValidImageFileName;
  string::size_type slashPos = validName.rfind('/');
  if (slashPos != string::npos)
  {
    validName = validName.substr(slashPos + 1);
  }

  // We want to print the filename of the best matching image for better
  // comparisons in CDash:
  string bestImageFileName = this->ValidImageFileName;

  // check the valid image
#ifdef __EMSCRIPTEN__
  vtkEmscriptenTestUtilities::PreloadDataFile(this->ValidImageFileName, validName);
  FILE* rtFin = vtksys::SystemTools::Fopen(validName, "r");
#else
  FILE* rtFin = vtksys::SystemTools::Fopen(this->ValidImageFileName, "r");
#endif
  if (rtFin)
  {
    fclose(rtFin);
  }
  else if (!tmpDir.empty()) // there was no valid image, so write one to the temp dir
  {
    string vImage = tmpDir + "/" + validName;
#ifdef __EMSCRIPTEN__
    vtkNew<vtkPNGWriter> rtPngw;
    rtPngw->SetWriteToMemory(true);
    rtPngw->SetInputConnection(imageSource->GetOutputPort());
    rtPngw->Write();
    auto* result = rtPngw->GetResult();
    vtkEmscriptenTestUtilities::DumpFile(
      vImage, result->GetPointer(0), result->GetDataTypeSize() * result->GetDataSize());
    os << "<DartMeasurement name=\"ImageNotFound\" type=\"text/string\">"
       << this->ValidImageFileName << "</DartMeasurement>" << endl;
    // Write out the image upload tag for the test image.
    os << "<DartMeasurementFile name=\"TestImage\" type=\"image/png\">";
    os << vImage;
    os << "</DartMeasurementFile>";
#else
    rtFin = vtksys::SystemTools::Fopen(vImage, "wb");
    if (rtFin)
    {
      fclose(rtFin);
      vtkNew<vtkPNGWriter> rtPngw;
      rtPngw->SetFileName(vImage.c_str());
      rtPngw->SetInputConnection(imageSource->GetOutputPort());
      rtPngw->Write();
      os << "<DartMeasurement name=\"ImageNotFound\" type=\"text/string\">"
         << this->ValidImageFileName << "</DartMeasurement>" << endl;
      // Write out the image upload tag for the test image.
      os << "<DartMeasurementFile name=\"TestImage\" type=\"image/png\">";
      os << vImage;
      os << "</DartMeasurementFile>";
    }
    else
    {
      vtkErrorMacro("Could not open file '" << vImage << "' for writing.");
    }
#endif
    return FAILED;
  }

  imageSource->Update();

  vtkNew<vtkPNGReader> rtPng;
#ifdef __EMSCRIPTEN__
  rtPng->SetFileName(validName.c_str());
#else
  rtPng->SetFileName(this->ValidImageFileName);
#endif
  rtPng->Update();

  vtkNew<vtkImageExtractComponents> rtExtract;
  rtExtract->SetInputConnection(rtPng->GetOutputPort());
  rtExtract->SetComponents(0, 1, 2);
  rtExtract->Update();

  auto createLegacyDiffFilter = [](vtkAlgorithm* source, vtkAlgorithm* extract)
  {
    auto alg = vtkSmartPointer<vtkAlgorithm>::Take(vtkImageDifference::New());
    alg->SetInputConnection(source->GetOutputPort());
    alg->SetInputConnection(1, extract->GetOutputPort());
    return alg;
  };

  auto createSSIMFilter = [](vtkAlgorithm* source, vtkAlgorithm* extract)
  {
    auto createPipeline = [](vtkAlgorithm* alg)
    {
      vtkNew<vtkImageShiftScale> normalizer;
      vtkNew<vtkImageRGBToXYZ> rgb2xyz;
      vtkNew<vtkImageXYZToLAB> xyz2lab;

      normalizer->SetScale(1.0 / 255);
      normalizer->SetOutputScalarTypeToDouble();
      normalizer->SetInputConnection(alg->GetOutputPort());
      rgb2xyz->SetInputConnection(normalizer->GetOutputPort());
      xyz2lab->SetInputConnection(rgb2xyz->GetOutputPort());

      return xyz2lab;
    };

    auto pipeline1 = createPipeline(source);
    auto pipeline2 = createPipeline(extract);

    auto ssim = vtkImageSSIM::New();
    ssim->SetInputToLab();
    ssim->ClampNegativeValuesOn();
    auto alg = vtkSmartPointer<vtkAlgorithm>::Take(ssim);
    alg->SetInputConnection(pipeline1->GetOutputPort());
    alg->SetInputConnection(1, pipeline2->GetOutputPort());
    return alg;
  };

  vtkNew<vtkImageClip> ic1;
  ic1->SetClipData(1);
  ic1->SetInputConnection(imageSource->GetOutputPort());

  vtkNew<vtkImageClip> ic2;
  ic2->SetClipData(1);
  ic2->SetInputConnection(rtExtract->GetOutputPort());

  int* wExt1 = ic1->GetInputInformation()->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT());
  int* wExt2 = ic2->GetInputInformation()->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT());
  ic1->SetOutputWholeExtent(wExt1[0] + this->BorderOffset, wExt1[1] - this->BorderOffset,
    wExt1[2] + this->BorderOffset, wExt1[3] - this->BorderOffset, wExt1[4], wExt1[5]);

  ic2->SetOutputWholeExtent(wExt2[0] + this->BorderOffset, wExt2[1] - this->BorderOffset,
    wExt2[2] + this->BorderOffset, wExt2[3] - this->BorderOffset, wExt2[4], wExt2[5]);

  int ext1[6], ext2[6];
  ic1->Update();
  ic1->GetOutput()->GetExtent(ext1);
  ic2->Update();
  ic2->GetOutput()->GetExtent(ext2);

  double minError = VTK_DOUBLE_MAX;

  enum
  {
    LEGACY,
    LOOSE,
    TIGHT,
    NONE
  };

  int imageCompareMethod = []
  {
    auto imageCompareString = []
    {
      if (!vtksys::SystemTools::HasEnv("VTK_TESTING_IMAGE_COMPARE_METHOD"))
      {
        vtkLog(WARNING, "Environment variable VTK_TESTING_IMAGE_COMPARE_METHOD is not set.");
        return std::string("LEGACY_VALID");
      }

      return std::string(vtksys::SystemTools::GetEnv("VTK_TESTING_IMAGE_COMPARE_METHOD"));
    }();

    vtkLog(INFO, "Using " << imageCompareString << " image comparison method.");
    if (imageCompareString == "LEGACY_VALID")
    {
      return LEGACY;
    }
    else if (imageCompareString == "TIGHT_VALID")
    {
      return TIGHT;
    }
    else if (imageCompareString == "LOOSE_VALID")
    {
      return LOOSE;
    }
    return NONE;
  }();

  auto rtId =
    imageCompareMethod == LEGACY ? createLegacyDiffFilter(ic1, ic2) : createSSIMFilter(ic1, ic2);

  auto executeComparison = [&](double& err)
  {
    rtId->Update();

    vtkDoubleArray* scalars = vtkArrayDownCast<vtkDoubleArray>(
      vtkDataSet::SafeDownCast(rtId->GetOutputDataObject(0))->GetPointData()->GetScalars());

    if (imageCompareMethod == LEGACY)
    {
      err = vtkImageDifference::SafeDownCast(rtId)->GetThresholdedError();
    }
    else
    {
      assert(scalars);
      double tight, loose;
      vtkImageSSIM::ComputeErrorMetrics(scalars, tight, loose);

      vtkLog(INFO,
        "When comparing images, error is defined as the maximum of all individual"
          << " values within the used method (TIGHT or LOOSE) using the threshold " << thresh);
      vtkLog(
        INFO, "Error computations on Lab channels using Minkownski and Wasserstein distances:");
      vtkLog(INFO, "TIGHT_VALID metric (euclidean): " << tight);
      vtkLog(INFO, "LOOSE_VALID metric (manhattan / earth's mover): " << loose);
      vtkLog(INFO,
        "Note: if the test fails but is visually acceptable, one can make the test pass"
          << " by changing the method (TIGHT_VALID vs LOOSE_VALID) and the threshold in CMake.");

      switch (imageCompareMethod)
      {
        case TIGHT:
        {
          err = tight;
          break;
        }
        case LOOSE:
          err = loose;
          break;
        default:
          vtkLog(ERROR,
            "Image comparison method not set correctly."
              << " If not using the \"LEGACY_VALID\" method, it should be \"TIGHT_VALID\" or "
                 "\"LOOSE_VALID\");");
      }
    }
  };

  if ((ext2[1] - ext2[0]) == (ext1[1] - ext1[0]) && (ext2[3] - ext2[2]) == (ext1[3] - ext1[2]) &&
    (ext2[5] - ext2[4]) == (ext1[5] - ext1[4]))
  {
    vtkLog(INFO, "Comparing baselines using the default image baseline.");

    executeComparison(minError);
  }

  this->ImageDifference = minError;
  int passed = 0;
  if (minError <= thresh)
  {
    // Make sure there was actually a difference image before
    // accepting the error measure.
    vtkImageData* output = vtkImageData::SafeDownCast(rtId->GetOutputDataObject(0));
    if (output)
    {
      int dims[3];
      output->GetDimensions(dims);
      if (dims[0] * dims[1] * dims[2] > 0)
      {
        passed = 1;
      }
      else
      {
        vtkErrorMacro("ImageDifference produced output with no data.");
      }
    }
    else
    {
      vtkErrorMacro("ImageDifference did not produce output.");
    }
  }

  // If the test failed with the first image (foo.png) check if there are
  // images of the form foo_N.png (where N=1,2,3...) and compare against
  // them.
  double error;
  int count = 1, errIndex = -1;
  char* newFileName;
  while (!passed)
  {
    newFileName = IncrementFileName(this->ValidImageFileName, count);
#ifdef __EMSCRIPTEN__
    std::string hostFileName = std::string(newFileName);
    // sandboxes the host file using the stem
    std::string sandboxedFileName = vtksys::SystemTools::GetFilenameName(hostFileName);
    vtkEmscriptenTestUtilities::PreloadDataFile(hostFileName.c_str(), sandboxedFileName);
    // so that subsequent code uses the sandboxed file name instead of host file name.
    delete[] newFileName;
    newFileName = new char[sandboxedFileName.size() + 1];
    strcpy(newFileName, sandboxedFileName.c_str());
#endif
    if (!LookForFile(newFileName))
    {
      delete[] newFileName;
      break;
    }

    rtPng->SetFileName(newFileName);

    // Need to reset the output whole extent cause we may have baselines
    // of differing sizes. (Yes, we have such cases !)
    ic2->ResetOutputWholeExtent();
    ic2->SetOutputWholeExtent(wExt2[0] + this->BorderOffset, wExt2[1] - this->BorderOffset,
      wExt2[2] + this->BorderOffset, wExt2[3] - this->BorderOffset, wExt2[4], wExt2[5]);
    ic2->UpdateWholeExtent();

    vtkImageData::SafeDownCast(ic2->GetOutputDataObject(0))->GetExtent(ext2);
    if ((ext2[1] - ext2[0]) == (ext1[1] - ext1[0]) && (ext2[3] - ext2[2]) == (ext1[3] - ext1[2]) &&
      (ext2[5] - ext2[4]) == (ext1[5] - ext1[4]))
    {
      vtkLog(INFO, "Trying another baseline.");
      // Cannot compute difference unless image sizes are the same
      executeComparison(error);
    }
    else
    {
      error = VTK_DOUBLE_MAX;
    }

    if (error <= thresh)
    {
      // Make sure there was actually a difference image before
      // accepting the error measure.
      vtkImageData* output = vtkImageData::SafeDownCast(rtId->GetOutputDataObject(0));
      if (output)
      {
        int dims[3];
        output->GetDimensions(dims);
        if (dims[0] * dims[1] * dims[2] > 0)
        {
          minError = error;
          passed = 1;
        }
      }
    }
    else
    {
      if (error < minError)
      {
        errIndex = count;
        minError = error;
        bestImageFileName = newFileName;
      }
    }
    ++count;
    delete[] newFileName;
  }

  this->ImageDifference = minError;

  // output some information
  os << "<DartMeasurement name=\"ImageError\" type=\"numeric/double\">";
  os << minError;
  os << "</DartMeasurement>";
  if (errIndex <= 0)
  {
    os << "<DartMeasurement name=\"BaselineImage\" type=\"text/string\">Standard</DartMeasurement>";
  }
  else
  {
    os << "<DartMeasurement name=\"BaselineImage\" type=\"numeric/integer\">";
    os << errIndex;
    os << "</DartMeasurement>";
  }

  if (passed)
  {
    return PASSED;
  }

  // write out the image that was generated
  string testImageFileName = tmpDir + "/" + validName;
#ifdef __EMSCRIPTEN__
  {
    vtkNew<vtkPNGWriter> rtPngw;
    rtPngw->SetWriteToMemory(true);
    rtPngw->SetInputConnection(imageSource->GetOutputPort());
    rtPngw->Write();
    auto* result = rtPngw->GetResult();
    vtkEmscriptenTestUtilities::DumpFile(
      testImageFileName, result->GetPointer(0), result->GetDataTypeSize() * result->GetDataSize());
    // Write out the image upload tag for the test image.
    os << "<DartMeasurementFile name=\"TestImage\" type=\"image/png\">";
    os << testImageFileName;
    os << "</DartMeasurementFile>\n";
  }
#else
  FILE* testImageFile = vtksys::SystemTools::Fopen(testImageFileName, "wb");
  if (testImageFile)
  {
    fclose(testImageFile);
    vtkNew<vtkPNGWriter> rtPngw;
    rtPngw->SetFileName(testImageFileName.c_str());
    rtPngw->SetInputConnection(imageSource->GetOutputPort());
    rtPngw->Write();

    // Write out the image upload tag for the test image.
    os << "<DartMeasurementFile name=\"TestImage\" type=\"image/png\">";
    os << testImageFileName;
    os << "</DartMeasurementFile>\n";
  }
  else
  {
    vtkErrorMacro("Could not open file '" << testImageFileName
                                          << "' for "
                                             "writing.");
  }
#endif

  os << "Failed Image Test ( " << validName << " ) : " << minError << endl;
  if (errIndex >= 0)
  {
    newFileName = IncrementFileName(this->ValidImageFileName, errIndex);
#ifdef __EMSCRIPTEN__
    std::string sandboxedFileName = vtkEmscriptenTestUtilities::PreloadDataFile(newFileName);
    delete[] newFileName;
    newFileName = new char[sandboxedFileName.size() + 1];
    strcpy(newFileName, sandboxedFileName.c_str());
#endif
    rtPng->SetFileName(newFileName);
    delete[] newFileName;
  }
  else
  {
#ifdef __EMSCRIPTEN__
    rtPng->SetFileName(validName.c_str());
#else
    rtPng->SetFileName(this->ValidImageFileName);
#endif
  }

  rtPng->Update();
  vtkImageData::SafeDownCast(ic2->GetOutputDataObject(0))->GetExtent(ext2);

  // If no image differences produced an image, do not write a
  // difference image.
  bool hasDiff = minError > 0;
  if (!hasDiff)
  {
    os << "Image differencing failed to produce an image." << endl;
  }
  if (!((ext2[1] - ext2[0]) == (ext1[1] - ext1[0]) && (ext2[3] - ext2[2]) == (ext1[3] - ext1[2]) &&
        (ext2[5] - ext2[4]) == (ext1[5] - ext1[4])))
  {
    os << "Image differencing failed to produce an image because images are "
          "different size:"
       << endl;
    os << "Valid image: " << (ext2[1] - ext2[0] + 1) << ", " << (ext2[3] - ext2[2] + 1) << ", "
       << (ext2[5] - ext2[4] + 1) << endl;
    os << "Test image: " << (ext1[1] - ext1[0] + 1) << ", " << (ext1[3] - ext1[2] + 1) << ", "
       << (ext1[5] - ext1[4] + 1) << endl;
    return FAILED;
  }

  rtId->Update();

  // test the directory for writing
  if (hasDiff && !tmpDir.empty())
  {
    string diffFilename = tmpDir + "/" + validName;
    string::size_type dotPos = diffFilename.rfind('.');
    if (dotPos != string::npos)
    {
      diffFilename = diffFilename.substr(0, dotPos);
    }

    if (imageCompareMethod != LEGACY)
    {
      auto ssim = vtkImageData::SafeDownCast(rtId->GetOutputDataObject(0));
      vtkDataSet* current = vtkDataSet::SafeDownCast(rtId->GetExecutive()->GetInputData(0, 0));
      vtkDataSet* baseline = vtkDataSet::SafeDownCast(rtId->GetExecutive()->GetInputData(1, 0));
      auto addOriginalArray = [&ssim](vtkDataSet* ds, std::string&& name)
      {
        vtkDataArray* scalars = ds->GetPointData()->GetScalars();
        auto array = vtkSmartPointer<vtkDataArray>::Take(scalars->NewInstance());
        array->ShallowCopy(scalars);
        array->SetName(name.c_str());
        ssim->GetPointData()->AddArray(array);
      };
      addOriginalArray(baseline, "Baseline");
      addOriginalArray(current, "Current");

      std::string vtiName = diffFilename + ".vti";

#ifdef __EMSCRIPTEN__
      {
        vtkNew<vtkXMLImageDataWriter> vtiWriter;
        vtiWriter->WriteToOutputStringOn();
        vtiWriter->SetInputData(ssim);
        vtiWriter->Write();
        const auto result = vtiWriter->GetOutputString();
        vtkEmscriptenTestUtilities::DumpFile(vtiName, result.data(), result.size());
      }
#else
      vtkNew<vtkXMLImageDataWriter> vtiWriter;
      vtiWriter->SetFileName(vtiName.c_str());
      vtiWriter->SetInputData(ssim);
      vtiWriter->Write();
#endif
    }

    diffFilename += ".diff.png";

    // write out the difference image gamma adjusted for the dashboard
    vtkNew<vtkImageShiftScale> rtGamma;
    rtGamma->SetInputConnection(rtId->GetOutputPort());
    rtGamma->SetShift(0);
    rtGamma->SetScale(imageCompareMethod == LEGACY ? 10 : 255);
    rtGamma->SetOutputScalarTypeToUnsignedChar();
    rtGamma->ClampOverflowOn();

#ifdef __EMSCRIPTEN__
    {
      vtkNew<vtkPNGWriter> rtPngw;
      rtPngw->SetWriteToMemory(true);
      rtPngw->SetInputConnection(rtGamma->GetOutputPort());
      rtPngw->Write();
      const auto result = rtPngw->GetResult();
      vtkEmscriptenTestUtilities::DumpFile(
        diffFilename, result->GetPointer(0), result->GetDataTypeSize() * result->GetDataSize());
      os << "<DartMeasurementFile name=\"DifferenceImage\" type=\"image/png\">";
      os << diffFilename;
      os << "</DartMeasurementFile>";
    }
#else
    FILE* rtDout = vtksys::SystemTools::Fopen(diffFilename, "wb");
    if (rtDout)
    {
      fclose(rtDout);

      vtkNew<vtkPNGWriter> rtPngw;
      rtPngw->SetFileName(diffFilename.c_str());
      rtPngw->SetInputConnection(rtGamma->GetOutputPort());
      rtPngw->Write();

      os << "<DartMeasurementFile name=\"DifferenceImage\" type=\"image/png\">";
      os << diffFilename;
      os << "</DartMeasurementFile>";
    }
    else
    {
      vtkErrorMacro("Could not open file '" << diffFilename << "' for writing.");
    }
#endif
  }

  os << "<DartMeasurementFile name=\"ValidImage\" type=\"image/png\">";
  os << bestImageFileName;
  os << "</DartMeasurementFile>";

  return FAILED;
}

//------------------------------------------------------------------------------
int vtkTesting::Test(int argc, char* argv[], vtkRenderWindow* rw, double thresh)
{
  vtkNew<vtkTesting> testing;
  for (int i = 0; i < argc; ++i)
  {
    testing->AddArgument(argv[i]);
  }

  if (testing->IsInteractiveModeSpecified())
  {
    return DO_INTERACTOR;
  }

  if (testing->IsValidImageSpecified())
  {
    testing->SetRenderWindow(rw);

    return testing->RegressionTest(thresh, cout);
  }
  return NOT_RUN;
}
//------------------------------------------------------------------------------
int vtkTesting::CompareAverageOfL2Norm(vtkDataArray* daA, vtkDataArray* daB, double tol)
{
  int typeA = daA->GetDataType();
  int typeB = daB->GetDataType();
  if (typeA != typeB)
  {
    vtkWarningMacro("Incompatible data types: " << typeA << "," << typeB << ".");
    return 0;
  }
  //
  vtkIdType nTupsA = daA->GetNumberOfTuples();
  vtkIdType nTupsB = daB->GetNumberOfTuples();
  int nCompsA = daA->GetNumberOfComponents();
  int nCompsB = daB->GetNumberOfComponents();
  //
  if ((nTupsA != nTupsB) || (nCompsA != nCompsB))
  {
    vtkWarningMacro("Arrays: " << daA->GetName() << " (nC=" << nCompsA << " nT= " << nTupsA << ")"
                               << " and " << daB->GetName() << " (nC=" << nCompsB
                               << " nT= " << nTupsB << ")"
                               << " do not have the same structure.");
    return 0;
  }

  double L2 = 0.0;
  vtkIdType N = 0;
  switch (typeA)
  {
    case VTK_DOUBLE:
    {
      vtkDoubleArray* A = vtkArrayDownCast<vtkDoubleArray>(daA);
      double* pA = A->GetPointer(0);
      vtkDoubleArray* B = vtkArrayDownCast<vtkDoubleArray>(daB);
      double* pB = B->GetPointer(0);
      N = AccumulateScaledL2Norm(pA, pB, nTupsA, nCompsA, L2);
    }
    break;
    case VTK_FLOAT:
    {
      vtkFloatArray* A = vtkArrayDownCast<vtkFloatArray>(daA);
      float* pA = A->GetPointer(0);
      vtkFloatArray* B = vtkArrayDownCast<vtkFloatArray>(daB);
      float* pB = B->GetPointer(0);
      N = AccumulateScaledL2Norm(pA, pB, nTupsA, nCompsA, L2);
    }
    break;
    default:
      if (this->Verbose)
      {
        cout << "Skipping:" << daA->GetName() << endl;
      }
      return true;
  }
  //
  if (N <= 0)
  {
    return 0;
  }
  //
  if (this->Verbose)
  {
    cout << "Sum(L2)/N of " << daA->GetName() << " < " << tol << "? = " << L2 << "/" << N << "."
         << endl;
  }
  //
  double avgL2 = L2 / static_cast<double>(N);
  if (avgL2 > tol)
  {
    return 0;
  }

  // Test passed
  return 1;
}
//------------------------------------------------------------------------------
int vtkTesting::CompareAverageOfL2Norm(vtkDataSet* dsA, vtkDataSet* dsB, double tol)
{
  vtkDataArray* daA = nullptr;
  vtkDataArray* daB = nullptr;
  int status = 0;

  // Compare points if the dataset derives from
  // vtkPointSet.
  vtkPointSet* ptSetA = vtkPointSet::SafeDownCast(dsA);
  vtkPointSet* ptSetB = vtkPointSet::SafeDownCast(dsB);
  if (ptSetA != nullptr && ptSetB != nullptr)
  {
    if (this->Verbose)
    {
      cout << "Comparing points:" << endl;
    }
    daA = ptSetA->GetPoints()->GetData();
    daB = ptSetB->GetPoints()->GetData();
    //
    status = CompareAverageOfL2Norm(daA, daB, tol);
    if (status == 0)
    {
      return 0;
    }
  }

  // Compare point data arrays.
  if (this->Verbose)
  {
    cout << "Comparing data arrays:" << endl;
  }
  int nDaA = dsA->GetPointData()->GetNumberOfArrays();
  int nDaB = dsB->GetPointData()->GetNumberOfArrays();
  if (nDaA != nDaB)
  {
    vtkWarningMacro("Point data, " << dsA << " and " << dsB << " differ in number of arrays"
                                   << " and cannot be compared.");
    return 0;
  }
  //
  for (int arrayId = 0; arrayId < nDaA; ++arrayId)
  {
    daA = dsA->GetPointData()->GetArray(arrayId);
    daB = dsB->GetPointData()->GetArray(arrayId);
    //
    status = CompareAverageOfL2Norm(daA, daB, tol);
    if (status == 0)
    {
      return 0;
    }
  }
  // All tests passed.
  return 1;
}

//------------------------------------------------------------------------------
int vtkTesting::InteractorEventLoop(
  int argc, char* argv[], vtkRenderWindowInteractor* iren, const char* playbackStream)
{
  bool disableReplay = false, record = false, playbackFile = false;
  std::string playbackFileName;
  for (int i = 0; i < argc; i++)
  {
    disableReplay |= (strcmp("--DisableReplay", argv[i]) == 0);
    record |= (strcmp("--Record", argv[i]) == 0);
    playbackFile |= (strcmp("--PlaybackFile", argv[i]) == 0);
    if (playbackFile && playbackFileName.empty())
    {
      if (i + 1 < argc)
      {
        playbackFileName = std::string(argv[i + 1]);
        ++i;
      }
    }
  }

  vtkNew<vtkInteractorEventRecorder> recorder;
  recorder->SetInteractor(iren);

  if (!disableReplay)
  {

    if (record)
    {
      recorder->SetFileName("vtkInteractorEventRecorder.log");
      recorder->On();
      recorder->Record();
    }
    else
    {
      if (playbackStream)
      {
        recorder->ReadFromInputStringOn();
        recorder->SetInputString(playbackStream);
        recorder->Play();

        // Without this, the "-I" option if specified will fail
        recorder->Off();
      }
      else if (playbackFile)
      {
        recorder->SetFileName(playbackFileName.c_str());
        recorder->Play();

        // Without this, the "-I" option if specified will fail
        recorder->Off();
      }
    }
  }

  // iren will be either the object factory instantiation (vtkTestingInteractor)
  // or vtkRenderWindowInteractor depending on whether or not "-I" is specified.
  iren->Start();

  recorder->Off();

  return EXIT_SUCCESS;
}

//------------------------------------------------------------------------------
void vtkTesting::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "RenderWindow: " << this->RenderWindow << endl;
  os << indent
     << "ValidImageFileName: " << (this->ValidImageFileName ? this->ValidImageFileName : "(none)")
     << endl;
  os << indent << "FrontBuffer: " << (this->FrontBuffer ? "On" : "Off") << endl;
  os << indent << "ImageDifference: " << this->ImageDifference << endl;
  os << indent << "DataRoot: " << this->GetDataRoot() << endl;
  os << indent << "Temp Directory: " << this->GetTempDirectory() << endl;
  os << indent << "BorderOffset: " << this->GetBorderOffset() << endl;
  os << indent << "Verbose: " << this->GetVerbose() << endl;
}
VTK_ABI_NAMESPACE_END
