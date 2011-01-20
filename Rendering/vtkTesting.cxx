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
#include "vtkPNGWriter.h"
#include "vtkImageShiftScale.h"
#include "vtkImageDifference.h"
#include "vtkPNGReader.h"
#include "vtkRenderWindow.h"
#include "vtkImageData.h"
#include "vtkTimerLog.h"
#include "vtkSmartPointer.h"
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

#include "vtkSmartPointer.h"
#define VTK_CREATE(type, name) \
  vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

#include <sys/stat.h>

#include <vtksys/SystemTools.hxx>

vtkStandardNewMacro(vtkTesting);
vtkCxxSetObjectMacro(vtkTesting, RenderWindow, vtkRenderWindow);

using vtkstd::vector;
using vtkstd::string;

//-----------------------------------------------------------------------------
// Find in command tail, failing that find in environment,
// failing that return a default.
// Up to caller to delete the string returned.
string vtkTestingGetArgOrEnvOrDefault(
          string argName,       // argument idnetifier flag. eg "-D"
          vector<string> &argv, // command tail
          string env,           // environment variable name to find
          string def)           // default to use if "env" is not found.
{
  string argValue;

  // Serach command tail.
  int argc=static_cast<int>(argv.size());
  for (int i=0; i<argc; i++)
    {
    if (argName==argv[i] && i<(argc-1))
      {
      argValue=argv[i+1];
      }
    }
  // If not found search environment.
  if (argValue.empty()
      && !(env.empty() || def.empty()))
    {
    char *foundenv=getenv(env.c_str());
    if (foundenv)
      {
      argValue=foundenv;
      }
    else
      {
      // Not found, fall back to default.
      argValue=def;
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
  SumModR=0.0;
  for (vtkIdType i=0; i<nTups; ++i)
    {
    double modR=0.0;
    double modA=0.0;
    for (int q=0; q<nComps; ++q)
      {
      double a=pA[q];
      double b=pB[q];
      modA+=a*a;
      double r=b-a;
      modR+=r*r;
      }
    modA=sqrt(modA);
    modA= modA<1.0 ? 1.0 : modA;
    SumModR+=sqrt(modR)/modA;
    pA+=nComps;
    pB+=nComps;
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
void vtkTesting::AddArguments(int argc,const char **argv)
{
  for (int i=0; i<argc; ++i)
    {
    this->Args.push_back(argv[i]);
    }
}
//-----------------------------------------------------------------------------
char *vtkTesting::GetArgument(const char *argName)
{
  string argValue
    = vtkTestingGetArgOrEnvOrDefault(argName,this->Args,"","");

  char *cArgValue=new char [argValue.size()+1];
  strcpy(cArgValue,argValue.c_str());

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

  for (size_t i=0; i<(this->Args.size()-1); ++i)
    {
    if ( this->Args[i] == "-V")
      {
      const char *ch = this->Args[i+1].c_str();
      if ( ch[0] == '/' 
#ifdef _WIN32
        || (ch[0] >= 'a' && ch[0] <= 'z' && ch[1] == ':' )
        || (ch[0] >= 'A' && ch[0] <= 'Z' && ch[1] == ':' )
#endif
        )
        {
        baseline = this->Args[i+1];
        }
      else
        {
        baseline += "/";
        baseline += this->Args[i+1];
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
  unsigned int i;
  for (i = 0; i < this->Args.size(); ++i)
    {
    if ( this->Args[i] == "-I")
      {
      return 1;
      }
    }
  return 0;
}
//-----------------------------------------------------------------------------
int vtkTesting::IsFlagSpecified(const char *flag)
{
  unsigned int i;
  for (i = 0; i < this->Args.size(); ++i)
    {
    if ( this->Args[i] == flag)
      {
      return 1;
      }
    }
  return 0;
}
//-----------------------------------------------------------------------------
int vtkTesting::IsValidImageSpecified()
{
  unsigned int i;
  for (i = 1; i < this->Args.size(); ++i)
    {
    if ( this->Args[i-1] == "-V")
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
  char* newFileName = new char[orgLen+extLen+2];
  strcpy(newFileName, fname);

  newFileName[orgLen-4] = '_';
  int i, marker;
  for(marker=orgLen-3, i=0; marker < orgLen-3+extLen; marker++, i++)
    {
    newFileName[marker] = counts[i];
    }
  strcpy( newFileName + marker, ".png" );
  
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
int vtkTesting::RegressionTest(vtkImageData* image, double thresh)
{
  int result = this->RegressionTest(image, thresh, cout);

  cout << "<DartMeasurement name=\"WallTime\" type=\"numeric/double\">";
  cout << vtkTimerLog::GetUniversalTime() - this->StartWallTime;
  cout << "</DartMeasurement>\n";
  cout << "<DartMeasurement name=\"CPUTime\" type=\"numeric/double\">";
  cout << vtkTimerLog::GetCPUTime() - this->StartCPUTime;
  cout << "</DartMeasurement>\n";

  return result;
}
//-----------------------------------------------------------------------------
int vtkTesting::RegressionTest(double thresh)
{
  int result = this->RegressionTest(thresh, cout);

  cout << "<DartMeasurement name=\"WallTime\" type=\"numeric/double\">";
  cout << vtkTimerLog::GetUniversalTime() - this->StartWallTime;
  cout << "</DartMeasurement>\n";
  cout << "<DartMeasurement name=\"CPUTime\" type=\"numeric/double\">";
  cout << vtkTimerLog::GetCPUTime() - this->StartCPUTime;
  cout << "</DartMeasurement>\n";

  return result;
}
//-----------------------------------------------------------------------------
int vtkTesting::RegressionTest(double thresh, ostream &os)
{
  VTK_CREATE(vtkWindowToImageFilter, rt_w2if);
  rt_w2if->SetInput(this->RenderWindow);

  unsigned int i;
  for (i=0; i<this->Args.size(); i++)
    {
    if ( strcmp("-FrontBuffer", this->Args[i].c_str()) == 0 )
      {
      this->FrontBufferOn();
      }
    else if ( strcmp("-NoRerender", this->Args[i].c_str()) == 0 )
      {
      rt_w2if->ShouldRerenderOff();
      }
    }

  // perform and extra render to make sure it is displayed
  if ( !this->FrontBuffer)
    {
    this->RenderWindow->Render();
    // tell it to read the back buffer
    rt_w2if->ReadFrontBufferOff();
    }
  else
    {
    // read the front buffer
    rt_w2if->ReadFrontBufferOn();
    }

  int res = this->RegressionTest(rt_w2if->GetOutput(), thresh, os);
  return res;
}
//-----------------------------------------------------------------------------
int vtkTesting::RegressionTest(vtkImageData* image, double thresh, ostream& os)
{
  // do a get to compute the real value
  this->GetValidImageFileName();
  vtkstd::string tmpDir = this->GetTempDirectory();

  // construct the names for the error images
  vtkstd::string validName = this->ValidImageFileName;
  vtkstd::string::size_type slash_pos = validName.rfind("/");
  if(slash_pos != vtkstd::string::npos)
    {
    validName = validName.substr(slash_pos + 1);
    }  

  // check the valid image
  FILE *rt_fin = fopen(this->ValidImageFileName,"r"); 
  if (rt_fin) 
    { 
    fclose(rt_fin);
    }
  else // there was no valid image, so write one to the temp dir
    {
    vtkstd::string vImage = tmpDir + "/" + validName;
    VTK_CREATE(vtkPNGWriter, rt_pngw);
    rt_pngw->SetFileName(vImage.c_str());
    rt_pngw->SetInput(image);
    rt_pngw->Write();
    os << "<DartMeasurement name=\"ImageNotFound\" type=\"text/string\">" 
      << this->ValidImageFileName << "</DartMeasurement>" << endl;
    return FAILED;
    }

  VTK_CREATE(vtkPNGReader, rt_png);
  rt_png->SetFileName(this->ValidImageFileName); 
  rt_png->Update();
  image->Update();

  VTK_CREATE(vtkImageDifference, rt_id);

  VTK_CREATE(vtkImageClip, ic1);
  ic1->SetClipData(1);
  ic1->SetInput(image);

  VTK_CREATE(vtkImageClip, ic2);
  ic2->SetClipData(1);
  ic2->SetInput(rt_png->GetOutput());

  int* wExt1 = ic1->GetInput()->GetWholeExtent();
  int* wExt2 = ic2->GetInput()->GetWholeExtent();
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
  rt_id->SetInput(ic1->GetOutput()); 
  ic1->Update();
  ic1->GetOutput()->GetExtent(ext1);
  rt_id->SetImage(ic2->GetOutput()); 
  ic2->Update();
  ic2->GetOutput()->GetExtent(ext2);

  double minError = VTK_DOUBLE_MAX;
  
  if ((ext2[1]-ext2[0]) == (ext1[1]-ext1[0]) && 
      (ext2[3]-ext2[2]) == (ext1[3]-ext1[2]) &&
      (ext2[5]-ext2[4]) == (ext1[5]-ext1[4]))
    {
    // Cannot compute difference unless image sizes are the same
    rt_id->Update(); 
    minError = rt_id->GetThresholdedError();
    }

  this->ImageDifference = minError;
  int passed = 0;
  if (minError <= thresh) 
    {
    // Make sure there was actually a difference image before
    // accepting the error measure.
    vtkImageData* output = rt_id->GetOutput();
    if(output)
      {
      int dims[3];
      output->GetDimensions(dims);
      if(dims[0]*dims[1]*dims[2] > 0)
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
  int count=1, errIndex=-1;
  char* newFileName;
  while (!passed)
    {
    newFileName = IncrementFileName(this->ValidImageFileName, count);
    if (!LookForFile(newFileName))
      {
      delete[] newFileName;
      break;
      }
    
    rt_png->SetFileName(newFileName);

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

    rt_id->GetImage()->GetExtent(ext2);
    if ((ext2[1]-ext2[0]) == (ext1[1]-ext1[0]) && 
        (ext2[3]-ext2[2]) == (ext1[3]-ext1[2]) &&
        (ext2[5]-ext2[4]) == (ext1[5]-ext1[4]))
      {
      // Cannot compute difference unless image sizes are the same
      rt_id->Update(); 
      error = rt_id->GetThresholdedError();
      }
    else
      {
      error = VTK_DOUBLE_MAX;
      }
        
    if (error <= thresh) 
      {
      // Make sure there was actually a difference image before
      // accepting the error measure.
      vtkImageData* output = rt_id->GetOutput();
      if(output)
        {
        int dims[3];
        output->GetDimensions(dims);
        if(dims[0]*dims[1]*dims[2] > 0)
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

  // output some information
  os << "<DartMeasurement name=\"ImageError\" type=\"numeric/double\">";
  os << minError;
  os << "</DartMeasurement>";
  if ( errIndex <= 0)
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
  
  os << "Failed Image Test : " << minError << endl;
  if (errIndex >= 0)
    {
    newFileName = IncrementFileName(this->ValidImageFileName, errIndex);
    rt_png->SetFileName(newFileName);
    delete[] newFileName;
    }
  else
    {
    rt_png->SetFileName(this->ValidImageFileName);
    }

  rt_png->Update();
  rt_id->GetImage()->GetExtent(ext2);

  // If no image differences produced an image, do not write a
  // difference image.
  if(minError <= 0)
    {
    os << "Image differencing failed to produce an image." << endl;
    return FAILED;
    }
  if(!(
      (ext2[1]-ext2[0]) == (ext1[1]-ext1[0]) && 
      (ext2[3]-ext2[2]) == (ext1[3]-ext1[2]) &&
      (ext2[5]-ext2[4]) == (ext1[5]-ext1[4])))
    {
    os << "Image differencing failed to produce an image because images are "
      "different size:" << endl;
    os << "Valid image: " << (ext2[1]-ext2[0]) << ", " << (ext2[3]-ext2[2])
      << ", " << (ext2[5]-ext2[4]) << endl;
    os << "Test image: " << (ext1[1]-ext1[0]) << ", " << (ext1[3]-ext1[2])
      << ", " << (ext1[5]-ext1[4]) << endl;
    return FAILED;
    }
  
  rt_id->Update();

  // test the directory for writing
  vtkstd::string diff_filename = tmpDir + "/" + validName;
  vtkstd::string::size_type dot_pos = diff_filename.rfind(".");
  if(dot_pos != vtkstd::string::npos)
    {
    diff_filename = diff_filename.substr(0, dot_pos);
    }  
  diff_filename += ".diff.png";
  FILE *rt_dout = fopen(diff_filename.c_str(), "wb"); 
  if (rt_dout) 
    { 
    fclose(rt_dout);
    
    // write out the difference image gamma adjusted for the dashboard
    VTK_CREATE(vtkImageShiftScale, rt_gamma);
    rt_gamma->SetInputConnection(rt_id->GetOutputPort());
    rt_gamma->SetShift(0);
    rt_gamma->SetScale(10);

    VTK_CREATE(vtkPNGWriter, rt_pngw);
    rt_pngw->SetFileName(diff_filename.c_str());
    rt_pngw->SetInputConnection(rt_gamma->GetOutputPort());
    rt_pngw->Write();

    // write out the image that was generated
    vtkstd::string vImage = tmpDir + "/" + validName;
    rt_pngw->SetFileName(vImage.c_str());
    rt_pngw->SetInput(image);
    rt_pngw->Write();

    os <<  "<DartMeasurementFile name=\"TestImage\" type=\"image/png\">";
    os << vImage;
    os << "</DartMeasurementFile>";
    os << "<DartMeasurementFile name=\"DifferenceImage\" type=\"image/png\">";
    os << diff_filename;
    os << "</DartMeasurementFile>";
    os << "<DartMeasurementFile name=\"ValidImage\" type=\"image/png\">";
    os << this->ValidImageFileName;
    os <<  "</DartMeasurementFile>";
    }

  return FAILED;
}
//-----------------------------------------------------------------------------
int vtkTesting::Test(int argc, char *argv[], vtkRenderWindow *rw, 
                     double thresh ) 
{
  VTK_CREATE(vtkTesting, testing);
  int i;
  for (i = 0; i < argc; ++i)
    {
    testing->AddArgument(argv[i]);
    }
  
  if (testing->IsInteractiveModeSpecified())
    {
    return DO_INTERACTOR;
    }
  
  testing->FrontBufferOff();
  for (i=0; i<argc; i++)
    {
    if ( strcmp("-FrontBuffer", argv[i]) == 0 )
      {
      testing->FrontBufferOn();
      }
    }

  if (testing->IsValidImageSpecified())
    { 
    testing->SetRenderWindow(rw);
    int res = testing->RegressionTest(thresh);
    return res;
    }

  return NOT_RUN;
}
//-----------------------------------------------------------------------------
int vtkTesting::CompareAverageOfL2Norm(
        vtkDataArray *daA,
        vtkDataArray *daB,
        double tol)
{
  int typeA=daA->GetDataType();
  int typeB=daB->GetDataType();
  if (typeA!=typeB)
    {
    vtkWarningMacro("Incompatible data types: "
                    << typeA << ","
                    << typeB << ".");
    return 0;
    }
  //
  vtkIdType nTupsA=daA->GetNumberOfTuples();
  vtkIdType nTupsB=daB->GetNumberOfTuples();
  int nCompsA=daA->GetNumberOfComponents();
  int nCompsB=daB->GetNumberOfComponents();
  //
  if ((nTupsA!=nTupsB)
     || (nCompsA!=nCompsB))
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

  double L2=0.0;
  vtkIdType N=0;
  switch (typeA)
    {
    case VTK_DOUBLE:
      {
      vtkDoubleArray *A=vtkDoubleArray::SafeDownCast(daA);
      double *pA=A->GetPointer(0);
      vtkDoubleArray *B=vtkDoubleArray::SafeDownCast(daB);
      double *pB=B->GetPointer(0);
      N=AccumulateScaledL2Norm(pA,pB,nTupsA,nCompsA,L2);
      }
      break;
    case VTK_FLOAT:
      {
      vtkFloatArray *A=vtkFloatArray::SafeDownCast(daA);
      float *pA=A->GetPointer(0);
      vtkFloatArray *B=vtkFloatArray::SafeDownCast(daB);
      float *pB=B->GetPointer(0);
      N=AccumulateScaledL2Norm(pA,pB,nTupsA,nCompsA,L2);
      }
      break;
    default:
      if (this->Verbose)
        {
        cout << "Skipping:" << daA->GetName() << endl;
        }
      return true;
      break;
    }
  //
  if (N<=0)
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
  double avgL2=L2/static_cast<double>(N);
  if (avgL2>tol)
    {
    return 0;
    }

  // Test passed
  return 1;
}
//-----------------------------------------------------------------------------
int vtkTesting::CompareAverageOfL2Norm(
        vtkDataSet *dsA,
        vtkDataSet *dsB,
        double tol)
{
  vtkDataArray *daA=0;
  vtkDataArray *daB=0;
  int status=0;

  // Compare points if the dataset derives from
  // vtkPointSet.
  vtkPointSet *ptSetA=vtkPointSet::SafeDownCast(dsA);
  vtkPointSet *ptSetB=vtkPointSet::SafeDownCast(dsB);
  if (ptSetA!=NULL && ptSetB!=NULL)
    {
    if (this->Verbose)
      {
      cout << "Comparing points:" << endl;
      }
    daA=ptSetA->GetPoints()->GetData();
    daB=ptSetB->GetPoints()->GetData();
    //
    status=CompareAverageOfL2Norm(daA,daB,tol);
    if (status==0)
      {
      return 0;
      }
    }

  // Compare point data arrays.
  if (this->Verbose)
    {
    cout << "Comparing data arrays:" << endl;
    }
  int nDaA=dsA->GetPointData()->GetNumberOfArrays();
  int nDaB=dsB->GetPointData()->GetNumberOfArrays();
  if (nDaA!=nDaB)
    {
    vtkWarningMacro("Point data, " << dsA
              <<  " and " << dsB << " differ in number of arrays"
              <<  " and cannot be compared.");
    return 0;
    }
  //
  for (int arrayId=0; arrayId<nDaA; ++arrayId)
    {
    daA=dsA->GetPointData()->GetArray(arrayId);
    daB=dsB->GetPointData()->GetArray(arrayId);
    //
    status=CompareAverageOfL2Norm(daA,daB,tol);
    if (status==0)
      {
      return 0;
      }
    }
  // All tests passed.
  return 1;
}

//-----------------------------------------------------------------------------
int vtkTesting::InteractorEventLoop( int argc, 
                                     char *argv[], 
                                     vtkRenderWindowInteractor *iren, 
                                     const char *playbackStream )
{
  bool disableReplay = false, record = false;
  for (int i = 0; i < argc; i++)
    {
    disableReplay |= (strcmp("--DisableReplay", argv[i]) == 0);
    record        |= (strcmp("--Record", argv[i]) == 0);
    }

  vtkSmartPointer<vtkInteractorEventRecorder> recorder =
      vtkSmartPointer<vtkInteractorEventRecorder>::New();
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
