/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTesting.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkTesting.h"

#include "vtkObjectFactory.h"
#include "vtkWindowToImageFilter.h"
#include "vtkPNGReader.h"
#include "vtkPNGWriter.h"
#include "vtkImageShiftScale.h"
#include "vtkImageDifference.h"
#include "vtkRenderWindow.h"
#include "vtkImageData.h"
#include "vtkTimerLog.h"
#include "vtkNew.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkInteractorEventRecorder.h"
#include "vtkImageClip.h"
#include "vtkToolkits.h"
#include "vtkDataSet.h"
#include "vtkPointSet.h"
#include "vtkPointData.h"
#include "vtkDataArray.h"
#include "vtkDoubleArray.h"
#include "vtkFloatArray.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkInformation.h"

#include <sys/stat.h>

#include <vtksys/ios/sstream>
#include <vtksys/SystemTools.hxx>

vtkStandardNewMacro(vtkTesting);
vtkCxxSetObjectMacro(vtkTesting, RenderWindow, vtkRenderWindow);

using std::vector;
using std::string;

//-----------------------------------------------------------------------------
// Find in command tail, failing that find in environment,
// failing that return a default.
// Up to caller to delete the string returned.
static string vtkTestingGetArgOrEnvOrDefault(
          string argName,       // argument idnetifier flag. eg "-D"
          vector<string> &argv, // command tail
          string env,           // environment variable name to find
          string def)           // default to use if "env" is not found.
{
  string argValue;

  // Serach command tail.
  int argc = static_cast<int>(argv.size());
  for (int i = 0; i < argc; i++)
    {
    if (argName == argv[i] && i < (argc - 1))
      {
      argValue = argv[i + 1];
      }
    }
  // If not found search environment.
  if (argValue.empty()
      && !(env.empty() || def.empty()))
    {
    char *foundenv=getenv(env.c_str());
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


//-----------------------------------------------------------------------------
// Description:
// Sum the L2 Norm point wise over all tuples. Each term
// is scaled by the magnitude of one of the inputs.
// Return sum and the number of terms.
template <class T>
vtkIdType AccumulateScaledL2Norm(
        T *pA,           // pointer to first data array
        T *pB,           // pointer to second data array
        vtkIdType nTups, // number of tuples
        int nComps,      // number of comps
        double &SumModR) // result
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
    modA = modA<1.0 ? 1.0 : modA;
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
  this->RenderWindow = 0;
  this->ValidImageFileName = 0;
  this->ImageDifference = 0;
  this->DataRoot = 0;
  this->TempDirectory = 0;
  this->BorderOffset = 0;
  this->Verbose = 0;

  // on construction we start the timer
  this->StartCPUTime = vtkTimerLog::GetCPUTime();
  this->StartWallTime = vtkTimerLog::GetUniversalTime();
}

//-----------------------------------------------------------------------------
vtkTesting::~vtkTesting()
{
  this->SetRenderWindow(0);
  this->SetValidImageFileName(0);
  this->SetDataRoot(0);
  this->SetTempDirectory(0);
}

//-----------------------------------------------------------------------------
void vtkTesting::AddArgument(const char *arg)
{
  this->Args.push_back(arg);
}

//-----------------------------------------------------------------------------
void vtkTesting::AddArguments(int argc, const char **argv)
{
  for (int i = 0; i < argc; ++i)
    {
    this->Args.push_back(argv[i]);
    }
}

//-----------------------------------------------------------------------------
void vtkTesting::AddArguments(int argc, char **argv)
{
  for (int i = 0; i < argc; ++i)
    {
    this->Args.push_back(argv[i]);
    }
}

//-----------------------------------------------------------------------------
char *vtkTesting::GetArgument(const char *argName)
{
  string argValue
    = vtkTestingGetArgOrEnvOrDefault(argName, this->Args, "", "");

  char *cArgValue = new char[argValue.size() + 1];
  strcpy(cArgValue, argValue.c_str());

  return cArgValue;
}
//-----------------------------------------------------------------------------
void vtkTesting::CleanArguments()
{
  this->Args.erase( this->Args.begin(), this->Args.end() );
}
//-----------------------------------------------------------------------------
const char *vtkTesting::GetDataRoot()
{
#ifdef VTK_DATA_ROOT
  string dr=vtkTestingGetArgOrEnvOrDefault(
                "-D",this->Args,"VTK_DATA_ROOT",VTK_DATA_ROOT);
#else
  string dr=vtkTestingGetArgOrEnvOrDefault(
                "-D",this->Args, "VTK_DATA_ROOT","../../../../VTKData");
#endif
  this->SetDataRoot(
     vtksys::SystemTools::CollapseFullPath(dr.c_str()).c_str());

  return this->DataRoot;
}
//-----------------------------------------------------------------------------
const char *vtkTesting::GetTempDirectory()
{
  string td=vtkTestingGetArgOrEnvOrDefault(
                "-T",this->Args, "VTK_TEMP_DIR","../../../Testing/Temporary");
  this->SetTempDirectory(
    vtksys::SystemTools::CollapseFullPath(td.c_str()).c_str());

  return this->TempDirectory;
}
//-----------------------------------------------------------------------------
const char *vtkTesting::GetValidImageFileName()
{
  this->SetValidImageFileName(0);
  if (!this->IsValidImageSpecified())
    {
    return this->ValidImageFileName;
    }

  string baseline=vtkTestingGetArgOrEnvOrDefault(
                "-B", this->Args,"VTK_BASELINE_ROOT", this->GetDataRoot());

  for (size_t i = 0; i < (this->Args.size() - 1); ++i)
    {
    if (this->Args[i] == "-V")
      {
      const char *ch = this->Args[i + 1].c_str();
      if (ch[0] == '/'
#ifdef _WIN32
        || (ch[0] >= 'a' && ch[0] <= 'z' && ch[1] == ':')
        || (ch[0] >= 'A' && ch[0] <= 'Z' && ch[1] == ':')
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
//-----------------------------------------------------------------------------
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
//-----------------------------------------------------------------------------
int vtkTesting::IsFlagSpecified(const char *flag)
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
//-----------------------------------------------------------------------------
int vtkTesting::IsValidImageSpecified()
{
  for (size_t i = 1; i < this->Args.size(); ++i)
    {
    if (this->Args[i-1] == "-V")
      {
      return 1;
      }
    }
  return 0;
}
//-----------------------------------------------------------------------------
char* vtkTesting::IncrementFileName(const char* fname, int count)
{
  char counts[256];
  sprintf(counts, "%d", count);

  int orgLen = static_cast<int>(strlen(fname));
  if (orgLen < 5)
    {
    return 0;
    }
  int extLen = static_cast<int>(strlen(counts));
  char* newFileName = new char[orgLen + extLen + 2];
  strcpy(newFileName, fname);

  newFileName[orgLen - 4] = '_';
  int i, marker;
  for (marker = orgLen - 3, i = 0; marker < orgLen - 3 + extLen;
       marker++, i++)
    {
    newFileName[marker] = counts[i];
    }
  strcpy(newFileName + marker, ".png");

  return newFileName;
}
//-----------------------------------------------------------------------------
int vtkTesting::LookForFile(const char* newFileName)
{
  if (!newFileName)
    {
    return 0;
    }
  struct stat fs;
  if (stat(newFileName, &fs) != 0)
    {
    return 0;
    }
  else
    {
    return 1;
    }
}
//-----------------------------------------------------------------------------
int vtkTesting::RegressionTest(vtkAlgorithm* imageSource, double thresh)
{
  int result = this->RegressionTest(imageSource, thresh, cout);

  cout << "<DartMeasurement name=\"WallTime\" type=\"numeric/double\">";
  cout << vtkTimerLog::GetUniversalTime() - this->StartWallTime;
  cout << "</DartMeasurement>\n";
  cout << "<DartMeasurement name=\"CPUTime\" type=\"numeric/double\">";
  cout << vtkTimerLog::GetCPUTime() - this->StartCPUTime;
  cout << "</DartMeasurement>\n";

  return result;
}
//-----------------------------------------------------------------------------
int vtkTesting::RegressionTestAndCaptureOutput(double thresh, ostream &os)
{
  int result = this->RegressionTest(thresh, os);

  os << "<DartMeasurement name=\"WallTime\" type=\"numeric/double\">";
  os << vtkTimerLog::GetUniversalTime() - this->StartWallTime;
  os << "</DartMeasurement>\n";
  os << "<DartMeasurement name=\"CPUTime\" type=\"numeric/double\">";
  os << vtkTimerLog::GetCPUTime() - this->StartCPUTime;
  os << "</DartMeasurement>\n";

  return result;
}
//-----------------------------------------------------------------------------
int vtkTesting::RegressionTest(double thresh)
{
  int result = this->RegressionTestAndCaptureOutput(thresh, cout);
  return result;
}
//-----------------------------------------------------------------------------
int vtkTesting::RegressionTest(double thresh, ostream &os)
{
  vtkNew<vtkWindowToImageFilter> rtW2if;
  rtW2if->SetInput(this->RenderWindow);

  for (unsigned int i = 0; i < this->Args.size(); ++i)
    {
    if ("-FrontBuffer" == this->Args[i])
      {
      this->FrontBufferOn();
      }
    else if ("-NoRerender" == this->Args[i])
      {
      rtW2if->ShouldRerenderOff();
      }
    }

  // perform and extra render to make sure it is displayed
  if (!this->FrontBuffer)
    {
    this->RenderWindow->Render();
    // tell it to read the back buffer
    rtW2if->ReadFrontBufferOff();
    }
  else
    {
    // read the front buffer
    rtW2if->ReadFrontBufferOn();
    }

  rtW2if->Update();
  int res = this->RegressionTest(rtW2if.Get(), thresh, os);
  return res;
}
//-----------------------------------------------------------------------------
int vtkTesting::RegressionTest(const string &pngFileName, double thresh)
{
  return this->RegressionTest(pngFileName, thresh, cout);
}
//-----------------------------------------------------------------------------
int vtkTesting::RegressionTest(const string &pngFileName, double thresh,
                               ostream &os)
{
  vtkNew<vtkPNGReader> inputReader;
  inputReader->SetFileName(pngFileName.c_str());
  inputReader->Update();
  return this->RegressionTest(inputReader.GetPointer(), thresh, os);
}
//-----------------------------------------------------------------------------
int vtkTesting::RegressionTest(vtkAlgorithm* imageSource,
                               double thresh,
                               ostream& os)
{
  // do a get to compute the real value
  this->GetValidImageFileName();
  string tmpDir = this->GetTempDirectory();

  // construct the names for the error images
  string validName = this->ValidImageFileName;
  string::size_type slashPos = validName.rfind("/");
  if (slashPos != string::npos)
    {
    validName = validName.substr(slashPos + 1);
    }

  // check the valid image
  FILE *rtFin = fopen(this->ValidImageFileName, "r");
  if (rtFin)
    {
    fclose(rtFin);
    }
  else // there was no valid image, so write one to the temp dir
    {
    string vImage = tmpDir + "/" + validName;
    rtFin = fopen(vImage.c_str(), "wb");
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
      os <<  "<DartMeasurementFile name=\"TestImage\" type=\"image/png\">";
      os << vImage;
      os << "</DartMeasurementFile>";
      }
    else
      {
      vtkErrorMacro("Could not open file '" << vImage << "' for writing.");
      }
    return FAILED;
    }

  vtkNew<vtkPNGReader> rtPng;
  rtPng->SetFileName(this->ValidImageFileName);
  rtPng->Update();
  imageSource->Update();

  vtkNew<vtkImageDifference> rtId;

  vtkNew<vtkImageClip> ic1;
  ic1->SetClipData(1);
  ic1->SetInputConnection(imageSource->GetOutputPort());

  vtkNew<vtkImageClip> ic2;
  ic2->SetClipData(1);
  ic2->SetInputConnection(rtPng->GetOutputPort());

  int* wExt1 = ic1->GetInputInformation()->Get(
    vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT());
  int* wExt2 = ic2->GetInputInformation()->Get(
    vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT());
  ic1->SetOutputWholeExtent(wExt1[0] + this->BorderOffset,
                            wExt1[1] - this->BorderOffset,
                            wExt1[2] + this->BorderOffset,
                            wExt1[3] - this->BorderOffset,
                            wExt1[4],
                            wExt1[5]);

  ic2->SetOutputWholeExtent(wExt2[0] + this->BorderOffset,
                            wExt2[1] - this->BorderOffset,
                            wExt2[2] + this->BorderOffset,
                            wExt2[3] - this->BorderOffset,
                            wExt2[4],
                            wExt2[5]);

  int ext1[6], ext2[6];
  rtId->SetInputConnection(ic1->GetOutputPort());
  ic1->Update();
  ic1->GetOutput()->GetExtent(ext1);
  rtId->SetImageConnection(ic2->GetOutputPort());
  ic2->Update();
  ic2->GetOutput()->GetExtent(ext2);

  double minError = VTK_DOUBLE_MAX;

  if ((ext2[1]-ext2[0]) == (ext1[1]-ext1[0]) &&
      (ext2[3]-ext2[2]) == (ext1[3]-ext1[2]) &&
      (ext2[5]-ext2[4]) == (ext1[5]-ext1[4]))
    {
    // Cannot compute difference unless image sizes are the same
    rtId->Update();
    minError = rtId->GetThresholdedError();
    }

  this->ImageDifference = minError;
  int passed = 0;
  if (minError <= thresh)
    {
    // Make sure there was actually a difference image before
    // accepting the error measure.
    vtkImageData* output = rtId->GetOutput();
    if (output)
      {
      int dims[3];
      output->GetDimensions(dims);
      if(dims[0] * dims[1] * dims[2] > 0)
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
    if (!LookForFile(newFileName))
      {
      delete[] newFileName;
      break;
      }

    rtPng->SetFileName(newFileName);

    // Need to reset the output whole extent cause we may have baselines
    // of differing sizes. (Yes, we have such cases !)
    ic2->ResetOutputWholeExtent();
    ic2->SetOutputWholeExtent(wExt2[0] + this->BorderOffset,
                              wExt2[1] - this->BorderOffset,
                              wExt2[2] + this->BorderOffset,
                              wExt2[3] - this->BorderOffset,
                              wExt2[4],
                              wExt2[5]);
    ic2->UpdateWholeExtent();

    rtId->GetImage()->GetExtent(ext2);
    if ((ext2[1] - ext2[0]) == (ext1[1] - ext1[0]) &&
        (ext2[3] - ext2[2]) == (ext1[3] - ext1[2]) &&
        (ext2[5] - ext2[4]) == (ext1[5] - ext1[4]))
      {
      // Cannot compute difference unless image sizes are the same
      rtId->Update();
      error = rtId->GetThresholdedError();
      }
    else
      {
      error = VTK_DOUBLE_MAX;
      }

    if (error <= thresh)
      {
      // Make sure there was actually a difference image before
      // accepting the error measure.
      vtkImageData* output = rtId->GetOutput();
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
    os <<  "<DartMeasurement name=\"BaselineImage\" type=\"numeric/integer\">";
    os << errIndex;
    os << "</DartMeasurement>";
    }

  if (passed)
    {
    return PASSED;
    }

  // write out the image that was generated
  string testImageFileName = tmpDir + "/" + validName;
  FILE *testImageFile = fopen(testImageFileName.c_str(), "wb");
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
    vtkErrorMacro("Could not open file '" << testImageFileName << "' for "
                  "writing.");
    }


  os << "Failed Image Test : " << minError << endl;
  if (errIndex >= 0)
    {
    newFileName = IncrementFileName(this->ValidImageFileName, errIndex);
    rtPng->SetFileName(newFileName);
    delete[] newFileName;
    }
  else
    {
    rtPng->SetFileName(this->ValidImageFileName);
    }

  rtPng->Update();
  rtId->GetImage()->GetExtent(ext2);

  // If no image differences produced an image, do not write a
  // difference image.
  bool hasDiff = minError > 0;
  if (!hasDiff)
    {
    os << "Image differencing failed to produce an image." << endl;
    }
  if (!(
       (ext2[1] - ext2[0]) == (ext1[1] - ext1[0]) &&
       (ext2[3] - ext2[2]) == (ext1[3] - ext1[2]) &&
       (ext2[5] - ext2[4]) == (ext1[5] - ext1[4])))
    {
    os << "Image differencing failed to produce an image because images are "
      "different size:" << endl;
    os << "Valid image: " << (ext2[1] - ext2[0]) << ", " << (ext2[3] - ext2[2])
      << ", " << (ext2[5] - ext2[4]) << endl;
    os << "Test image: " << (ext1[1] - ext1[0]) << ", " << (ext1[3] - ext1[2])
      << ", " << (ext1[5] - ext1[4]) << endl;
    return FAILED;
    }

  rtId->Update();

  // test the directory for writing
  if (hasDiff)
    {
    string diffFilename = tmpDir + "/" + validName;
    string::size_type dotPos = diffFilename.rfind(".");
    if (dotPos != string::npos)
      {
      diffFilename = diffFilename.substr(0, dotPos);
      }
    diffFilename += ".diff.png";
    FILE *rtDout = fopen(diffFilename.c_str(), "wb");
    if (rtDout)
      {
      fclose(rtDout);

      // write out the difference image gamma adjusted for the dashboard
      vtkNew<vtkImageShiftScale> rtGamma;
      rtGamma->SetInputConnection(rtId->GetOutputPort());
      rtGamma->SetShift(0);
      rtGamma->SetScale(10);

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
      vtkErrorMacro("Could not open file '" << diffFilename
                    << "' for writing.");
      }
    }

  os << "<DartMeasurementFile name=\"ValidImage\" type=\"image/png\">";
  os << this->ValidImageFileName;
  os <<  "</DartMeasurementFile>";

  return FAILED;
}
//-----------------------------------------------------------------------------
int vtkTesting::Test(int argc, char *argv[], vtkRenderWindow *rw,
                     double thresh )
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

  testing->FrontBufferOff();
  for (int i = 0; i < argc; ++i)
    {
    if (strcmp("-FrontBuffer", argv[i]) == 0)
      {
      testing->FrontBufferOn();
      }
    }

  if (testing->IsValidImageSpecified())
    {
    testing->SetRenderWindow(rw);

    std::ostringstream out1;
    int res = testing->RegressionTestAndCaptureOutput(thresh, out1);
    double diff1 = testing->GetImageDifference();
    bool write_out1 = true;

    // Typically, the image testing is done using the back buffer
    // to avoid accidentally capturing overlapping window artifacts
    // in the image when using the front buffer. However, some graphics
    // drivers do not have up to date contents in the back buffer,
    // causing "failed" tests even though, upon visual inspection, the
    // front buffer looks perfectly valid... So:
    //
    // If the test failed using the back buffer, re-test using the
    // front buffer. This way, more tests pass on dashboards run with
    // the Intel HD built-in graphics drivers.
    //
    if (res == vtkTesting::FAILED && testing->GetFrontBuffer() == 0)
      {
      testing->FrontBufferOn();

      std::ostringstream out2;
      res = testing->RegressionTestAndCaptureOutput(thresh, out2);
      double diff2 = testing->GetImageDifference();

      if (diff2 < diff1)
        {
        cout << out2.str();
        write_out1 = false;
        }
      }

    if (write_out1)
      {
      cout << out1.str();
      }

    return res;
    }

  return NOT_RUN;
}
//-----------------------------------------------------------------------------
int vtkTesting::CompareAverageOfL2Norm(vtkDataArray *daA,
                                       vtkDataArray *daB,
                                       double tol)
{
  int typeA = daA->GetDataType();
  int typeB = daB->GetDataType();
  if (typeA != typeB)
    {
    vtkWarningMacro("Incompatible data types: "
                    << typeA << ","
                    << typeB << ".");
    return 0;
    }
  //
  vtkIdType nTupsA = daA->GetNumberOfTuples();
  vtkIdType nTupsB = daB->GetNumberOfTuples();
  int nCompsA = daA->GetNumberOfComponents();
  int nCompsB = daB->GetNumberOfComponents();
  //
  if ((nTupsA != nTupsB)
     || (nCompsA != nCompsB))
    {
    vtkWarningMacro(
              "Arrays: " << daA->GetName()
              << " (nC=" << nCompsA
              << " nT= "<< nTupsA << ")"
              << " and " << daB->GetName()
              << " (nC=" << nCompsB
              << " nT= "<< nTupsB << ")"
              << " do not have the same structure.");
    return 0;
    }

  double L2 = 0.0;
  vtkIdType N = 0;
  switch (typeA)
    {
    case VTK_DOUBLE:
      {
      vtkDoubleArray *A = vtkDoubleArray::SafeDownCast(daA);
      double *pA = A->GetPointer(0);
      vtkDoubleArray *B = vtkDoubleArray::SafeDownCast(daB);
      double *pB = B->GetPointer(0);
      N = AccumulateScaledL2Norm(pA, pB, nTupsA, nCompsA, L2);
      }
      break;
    case VTK_FLOAT:
      {
      vtkFloatArray *A = vtkFloatArray::SafeDownCast(daA);
      float *pA = A->GetPointer(0);
      vtkFloatArray *B = vtkFloatArray::SafeDownCast(daB);
      float *pB = B->GetPointer(0);
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
    cout << "Sum(L2)/N of "
         << daA->GetName()
         << " < " << tol
         << "? = " << L2
         << "/" << N
         << "."  << endl;
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
//-----------------------------------------------------------------------------
int vtkTesting::CompareAverageOfL2Norm(vtkDataSet *dsA, vtkDataSet *dsB,
                                       double tol)
{
  vtkDataArray *daA = 0;
  vtkDataArray *daB = 0;
  int status = 0;

  // Compare points if the dataset derives from
  // vtkPointSet.
  vtkPointSet *ptSetA = vtkPointSet::SafeDownCast(dsA);
  vtkPointSet *ptSetB = vtkPointSet::SafeDownCast(dsB);
  if (ptSetA != NULL && ptSetB != NULL)
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
    vtkWarningMacro("Point data, " << dsA
              <<  " and " << dsB << " differ in number of arrays"
              <<  " and cannot be compared.");
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

//-----------------------------------------------------------------------------
int vtkTesting::InteractorEventLoop(int argc,
                                    char *argv[],
                                    vtkRenderWindowInteractor *iren,
                                    const char *playbackStream)
{
  bool disableReplay = false, record = false, playbackFile = false;;
  std::string playbackFileName;
  for (int i = 0; i < argc; i++)
    {
    disableReplay |= (strcmp("--DisableReplay", argv[i]) == 0);
    record        |= (strcmp("--Record", argv[i]) == 0);
    playbackFile  |= (strcmp("--PlaybackFile", argv[i]) == 0);
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

//-----------------------------------------------------------------------------
void vtkTesting::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "RenderWindow: " << this->RenderWindow << endl;
  os << indent << "ValidImageFileName: " << (this->ValidImageFileName?this->ValidImageFileName:"(none)") << endl;
  os << indent << "FrontBuffer: " << (this->FrontBuffer?"On":"Off") << endl;
  os << indent << "ImageDifference: " << this->ImageDifference << endl;
  os << indent << "DataRoot: " << this->GetDataRoot() << endl;
  os << indent << "Temp Directory: " << this->GetTempDirectory() << endl;
  os << indent << "BorderOffset: " << this->GetBorderOffset() << endl;
  os << indent << "Verbose: " << this->GetVerbose() << endl;
}
