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
#include "vtkJPEGWriter.h"
#include "vtkImageDifference.h"
#include "vtkImageResample.h"
#include "vtkPNGReader.h"
#include "vtkRenderWindow.h"
#include "vtkImageData.h"
#include "vtkTimerLog.h"
#include "vtkSmartPointer.h"
#include "vtkImageClip.h"
#include "vtkToolkits.h"
#include <sys/stat.h>

vtkStandardNewMacro(vtkTesting);
vtkCxxRevisionMacro(vtkTesting, "1.26");
vtkCxxSetObjectMacro(vtkTesting, RenderWindow, vtkRenderWindow);


char* vtkTestingGetArgOrEnvOrDefault(const char* arg, 
                                     int argc, char* argv[], 
                                     const char* env, 
                                     const char *def)
{
  int index = -1;
  
  for (int i = 0; i < argc; i++)
    {
    if (strcmp(arg, argv[i]) == 0 && i < argc - 1)
      {
      index = i + 1;
      }
    }

  char* value;

  if (index != -1) 
    {
    value = new char[strlen(argv[index]) + 1];
    strcpy(value, argv[index]);
    }
  else 
    {
    char *foundenv = getenv(env);
    if (foundenv)
      {
      value = new char[strlen(foundenv) + 1];
      strcpy(value, foundenv);
      }
    else
      {
      value = new char[strlen(def) + 1];
      strcpy(value, def);
      }
    }
  
  return value;
} 

vtkTesting::vtkTesting()
{
  this->FrontBuffer = 0;
  this->RenderWindow = 0;
  this->ValidImageFileName = 0;
  this->ImageDifference = 0;
  this->DataRoot = 0;
  this->TempDirectory = 0;
  this->BorderOffset = 0;
  
  // on construction we start the timer
  this->StartCPUTime = vtkTimerLog::GetCPUTime();
  this->StartWallTime = vtkTimerLog::GetUniversalTime();
}  

vtkTesting::~vtkTesting()
{
  this->SetRenderWindow(0);
  this->SetValidImageFileName(0);
  this->SetDataRoot(0);
  this->SetTempDirectory(0);
}

void vtkTesting::AddArgument(const char *arg)
{
  this->Args.push_back(arg);
}

void vtkTesting::CleanArguments()
{
  this->Args.erase( this->Args.begin(), this->Args.end() );
}

const char *vtkTesting::GetDataRoot()
{
  unsigned int i;
  char **argv = 0;
  if (this->Args.size())
    {
    argv = new char * [this->Args.size()];
    for (i = 0; i < this->Args.size(); ++i)
      {
      argv[i] = strdup(this->Args[i].c_str());
      }
    }

#ifdef VTK_DATA_ROOT 
  char *dr = vtkTestingGetArgOrEnvOrDefault(
    "-D", this->Args.size(), argv, "VTK_DATA_ROOT", 
    VTK_DATA_ROOT);
#else
  char *dr = vtkTestingGetArgOrEnvOrDefault(
    "-D", this->Args.size(), argv, "VTK_DATA_ROOT", 
    "../../../../VTKData");
#endif
  
  this->SetDataRoot(dr);
  delete [] dr;
  
  if (argv)
    {
    for (i = 0; i < this->Args.size(); ++i)
      {
      free(argv[i]);
      }
    delete [] argv;
    }
  return this->DataRoot;
}

const char *vtkTesting::GetTempDirectory()
{
  unsigned int i;
  char **argv = 0;
  if (this->Args.size())
    {
    argv = new char * [this->Args.size()];
    for (i = 0; i < this->Args.size(); ++i)
      {
      argv[i] = strdup(this->Args[i].c_str());
      }
    }
  char *td = vtkTestingGetArgOrEnvOrDefault(
      "-T", this->Args.size(), argv, "VTK_TEMP_DIR", 
      "../../../Testing/Temporary");
  this->SetTempDirectory(td);
  delete [] td;
  if (argv)
    {
    for (i = 0; i < this->Args.size(); ++i)
      {
      free(argv[i]);
      }
    delete [] argv;
    }
  return this->TempDirectory;
}

const char *vtkTesting::GetValidImageFileName()
{
  this->SetValidImageFileName(0);
  if (!this->IsValidImageSpecified())
    {
    return this->ValidImageFileName;
    }
  
  char **argv = 0;
  unsigned int i;
  if (this->Args.size())
    {
    argv = new char * [this->Args.size()];
    for (i = 0; i < this->Args.size(); ++i)
      {
      argv[i] = strdup(this->Args[i].c_str());
      }
    }
  
  char * baseline = vtkTestingGetArgOrEnvOrDefault(
    "-B", this->Args.size(), argv, 
    "VTK_BASELINE_ROOT", this->GetDataRoot());
  vtkstd::string viname = baseline;
  delete [] baseline;
  
  for (i = 0; i < (this->Args.size() - 1); ++i)
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
        viname = this->Args[i+1];
        }
      else
        {
        viname += "/";
        viname += this->Args[i+1];
        }
      break;
      }
    }

  this->SetValidImageFileName(viname.c_str());
  if (argv)
    {
    for (i = 0; i < this->Args.size(); ++i)
      {
      free(argv[i]);
      }
    delete [] argv;
    }
  return this->ValidImageFileName;
}

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

int vtkTesting::RegressionTest(double thresh, ostream &os)
{
  vtkWindowToImageFilter *rt_w2if = vtkWindowToImageFilter::New(); 
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
  rt_w2if->Delete(); 
  return res;
}

int vtkTesting::RegressionTest(vtkImageData* image, double thresh, ostream& os)
{
  // do a get to compute the real value
  this->GetValidImageFileName();
  const char * tmpDir = this->GetTempDirectory();

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
    char* vImage = new char[strlen(tmpDir) + validName.size() + 30];
    sprintf(vImage, "%s/%s", tmpDir, validName.c_str());
    vtkPNGWriter *rt_pngw = vtkPNGWriter::New();
    rt_pngw->SetFileName(vImage);
    rt_pngw->SetInput(image);
    rt_pngw->Write();
    rt_pngw->Delete();
    delete [] vImage;
    os << "<DartMeasurement name=\"ImageNotFound\" type=\"text/string\">" 
      << this->ValidImageFileName << "</DartMeasurement>" << endl;
    return FAILED;
    }

  vtkSmartPointer<vtkPNGReader> rt_png = vtkSmartPointer<vtkPNGReader>::New();
  rt_png->SetFileName(this->ValidImageFileName); 
  rt_png->Update();
  image->Update();

  vtkSmartPointer<vtkImageDifference> rt_id =
    vtkSmartPointer<vtkImageDifference>::New();

  vtkImageClip* ic1 = vtkImageClip::New();
  ic1->SetClipData(1);
  ic1->SetInput(image);

  vtkImageClip* ic2 = vtkImageClip::New();
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

  rt_id->SetInput(ic1->GetOutput()); 
  ic1->Delete();
  rt_id->SetImage(ic2->GetOutput()); 
  ic2->Delete();
  rt_id->Update(); 

  double minError = rt_id->GetThresholdedError();
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
    rt_png->Update();
    rt_id->Update();
    error = rt_id->GetThresholdedError();
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
  rt_id->Update();

  // If no image differences produced an image, do not write a
  // difference image.
  if(minError <= 0)
    {
    os << "Image differencing failed to produce an image." << endl;
    return FAILED;
    }

  // test the directory for writing
  char* diff_small = new char[strlen(tmpDir) + validName.size() + 30];
  sprintf(diff_small, "%s/%s.diff.small.jpg", tmpDir, validName.c_str());
  FILE *rt_dout = fopen(diff_small,"wb"); 
  if (rt_dout) 
    { 
    fclose(rt_dout);
    
    // write out the difference image scaled and gamma adjusted
    // for the dashboard
    int* rt_size = rt_png->GetOutput()->GetDimensions();
    double rt_magfactor=1.0;
    if ( rt_size[1] > 250.0)
      {
      rt_magfactor = 250.0 / rt_size[1];
      }
    vtkImageResample*  rt_shrink = vtkImageResample::New();
    rt_shrink->SetInput(rt_id->GetOutput());
    rt_shrink->InterpolateOn();
    rt_shrink->SetAxisMagnificationFactor(0, rt_magfactor );
    rt_shrink->SetAxisMagnificationFactor(1, rt_magfactor );
    vtkImageShiftScale* rt_gamma = vtkImageShiftScale::New();
    rt_gamma->SetInput(rt_shrink->GetOutput());
    rt_gamma->SetShift(0);
    rt_gamma->SetScale(10);

    vtkJPEGWriter* rt_jpegw_dashboard = vtkJPEGWriter::New();
    rt_jpegw_dashboard->SetFileName( diff_small );
    rt_jpegw_dashboard->SetInput(rt_gamma->GetOutput());
    rt_jpegw_dashboard->SetQuality(85);
    rt_jpegw_dashboard->Write();

    // write out the image that was generated
    rt_shrink->SetInput(ic1->GetOutput());
    rt_jpegw_dashboard->SetInput(rt_shrink-> GetOutput());
    char* valid_test_small = new char[strlen(tmpDir) + validName.size() + 30];
    sprintf(valid_test_small, "%s/%s.test.small.jpg", tmpDir, 
            validName.c_str());
    rt_jpegw_dashboard->SetFileName(valid_test_small);
    rt_jpegw_dashboard->Write();

    // write out the valid image that matched
    rt_shrink->SetInput(ic2->GetOutput());
    rt_jpegw_dashboard-> SetInput (rt_shrink->GetOutput());
    char* valid = new char[strlen(tmpDir) + validName.size() + 30];
    sprintf(valid, "%s/%s.small.jpg", tmpDir, validName.c_str());
    rt_jpegw_dashboard-> SetFileName( valid);
    rt_jpegw_dashboard->Write();
    rt_jpegw_dashboard->Delete();


    os <<  "<DartMeasurementFile name=\"TestImage\" type=\"image/jpeg\">";
    os << valid_test_small;
    delete [] valid_test_small;
    os << "</DartMeasurementFile>";
    os << "<DartMeasurementFile name=\"DifferenceImage\" type=\"image/jpeg\">";
    os << diff_small;
    os << "</DartMeasurementFile>";
    os << "<DartMeasurementFile name=\"ValidImage\" type=\"image/jpeg\">";
    os << valid;
    os <<  "</DartMeasurementFile>";

    delete [] valid;

    rt_shrink->Delete();
    rt_gamma->Delete();
    }

  delete [] diff_small;
  return FAILED;
}

int vtkTesting::Test(int argc, char *argv[], vtkRenderWindow *rw, 
                     double thresh ) 
{
  vtkTesting * testing = vtkTesting::New();
  int i;
  for (i = 0; i < argc; ++i)
    {
    testing->AddArgument(argv[i]);
    }
  
  if (testing->IsInteractiveModeSpecified())
    {
    testing->Delete();
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
    testing->Delete();
    return res;
    }

  testing->Delete();
  return NOT_RUN;
}

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
}
