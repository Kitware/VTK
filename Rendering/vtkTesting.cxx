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

#include <sys/stat.h>

vtkStandardNewMacro(vtkTesting);
vtkCxxRevisionMacro(vtkTesting, "1.10");
vtkCxxSetObjectMacro(vtkTesting, RenderWindow, vtkRenderWindow);

// Function returning either a command line argument, an environment variable
// or a default value.  The returned string has to be deleted (with delete[])
// by the user.
char* vtkTestUtilitiesGetArgOrEnvOrDefault(const char* arg, 
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

  char* value = 0;

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

// Given a file name, this function returns a new string which is (in theory)
// the full path. This path is constructed by prepending the file name with a
// command line argument, an environment variable or a default value.  If
// slash is true, appends a slash to the resulting string.  The returned
// string has to be deleted (with delete[]) by the user.
char* vtkTestUtilitiesExpandFileNameWithArgOrEnvOrDefault(const char* arg, 
                                                          int argc, 
                                                          char* argv[], 
                                                          const char* env, 
                                                          const char *def, 
                                                          const char* fname,
                                                          int slash)
{
  char* fullName = 0;

  char* value = vtkTestUtilitiesGetArgOrEnvOrDefault(arg, argc, argv, 
                                                     env, def);
  if (value)
    {
    fullName = new char[strlen(value) + strlen(fname) + 2 + (slash ? 1 : 0)];
    fullName[0] = 0;
    strcat(fullName, value);
    int len = static_cast<int>(strlen(fullName));
    fullName[len] = '/';
    fullName[len+1] = 0;
    strcat(fullName, fname);
    }
  else
    {
    fullName = new char[strlen(fname) + 1 + (slash ? 1 : 0)];
    strcpy(fullName, fname);
    }

  if (slash)
    {
    strcat(fullName, "/");
    }

  delete[] value;

  return fullName;
}

// Given a file name, this function returns a new string which is (in theory)
// the full path. This path is constructed by prepending the file name with a
// command line argument (-D path) or VTK_DATA_ROOT env. variable.  If slash
// is true, appends a slash to the resulting string.  The returned string has
// to be deleted (with delete[]) by the user.
char* vtkTestUtilities::ExpandDataFileName(int argc, char* argv[], 
                                           const char* fname,
                                           int slash)
{
  return vtkTestUtilitiesExpandFileNameWithArgOrEnvOrDefault(
    "-D", argc, argv, 
    "VTK_DATA_ROOT", 
    "../../../../VTKData",
    fname,
    slash);
}


vtkTesting::vtkTesting()
{
  this->FrontBuffer = 0;
  this->RenderWindow = 0;
  this->ValidImageFileName = 0;
  this->ImageDifference = 0;
  this->LastResultText = 0;
  this->DataRoot = 0;
  this->TempDirectory = 0;
  
  // on construction we start the timer
  this->StartCPUTime = vtkTimerLog::GetCPUTime();
  this->StartWallTime = vtkTimerLog::GetCurrentTime();
}  

vtkTesting::~vtkTesting()
{
  this->SetRenderWindow(0);
  this->SetValidImageFileName(0);
  this->SetLastResultText(0);
  this->SetDataRoot(0);
  this->SetTempDirectory(0);
}

void vtkTesting::AddArgument(const char *arg)
{
  this->Args.push_back(arg);
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

  char *dr = vtkTestUtilitiesGetArgOrEnvOrDefault(
    "-D", this->Args.size(), argv, "VTK_DATA_ROOT", 
    "../../../../VTKData");
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
  char *td = vtkTestUtilitiesGetArgOrEnvOrDefault(
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
  
  char * baseline = vtkTestUtilitiesGetArgOrEnvOrDefault(
    "-B", this->Args.size(), argv, 
    "VTK_BASELINE_ROOT", this->GetDataRoot());
  vtkstd::string viname = baseline;
  delete [] baseline;
  
  for (i = 0; i < (this->Args.size() - 1); ++i)
    {
    if ( this->Args[i] == "-V")
      {
      viname += "/";
      viname += this->Args[i+1];
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
  newFileName[marker++] = '.';
  newFileName[marker++] = 'p';
  newFileName[marker++] = 'n';
  newFileName[marker++] = 'g';
  newFileName[marker] = '\0';
  
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
  cout << vtkTimerLog::GetCurrentTime() - this->StartWallTime;
  cout << "</DartMeasurement>\n";
  cout << "<DartMeasurement name=\"CPUTime\" type=\"numeric/double\">";
  cout << vtkTimerLog::GetCPUTime() - this->StartCPUTime;
  cout << "</DartMeasurement>\n";

  return result;
}

int vtkTesting::RegressionTest(double thresh)
{
#if 0
  // clear the last result
  this->SetLastResultText(0);
  ostrstream buf_with_warning_C4701;
  int result = this->RegressionTest(thresh, buf_with_warning_C4701);

  buf_with_warning_C4701 <<  
    "<DartMeasurement name=\"WallTime\" type=\"numeric/double\">";
  buf_with_warning_C4701 << 
    vtkTimerLog::GetCurrentTime() - this->StartWallTime;
  buf_with_warning_C4701 << "</DartMeasurement>\n";
  buf_with_warning_C4701 <<  
    "<DartMeasurement name=\"CPUTime\" type=\"numeric/double\">";
  buf_with_warning_C4701 << vtkTimerLog::GetCPUTime() - this->StartCPUTime;
  buf_with_warning_C4701 << "</DartMeasurement>\n";
  
  buf_with_warning_C4701.put('\0');
  this->SetLastResultText(buf_with_warning_C4701.str());
  buf_with_warning_C4701.rdbuf()->freeze(0);
  if (this->LastResultText)
    {
    cout << this->LastResultText << "\n";
    }
#endif
  
  int result = this->RegressionTest(thresh, cout);

  cout << "<DartMeasurement name=\"WallTime\" type=\"numeric/double\">";
  cout << vtkTimerLog::GetCurrentTime() - this->StartWallTime;
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
    return FAILED;
    }

  vtkPNGReader *rt_png = vtkPNGReader::New(); 
  rt_png->SetFileName(this->ValidImageFileName); 
  vtkImageDifference *rt_id = vtkImageDifference::New(); 
  rt_id->SetInput(image); 
  rt_id->SetImage(rt_png->GetOutput()); 
  rt_id->Update(); 
  rt_png->Delete(); 

  double minError = rt_id->GetThresholdedError();
  this->ImageDifference = minError;
  int passed = 0;
  if (minError <= thresh) 
    { 
    passed = 1;
    }

  // If the test failed with the first image (foo.png) check if there are
  // images of the form foo_N.png (where N=1,2,3...) and compare against
  // them.
  double error=0;
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
      minError = error;
      passed = 1;
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
    rt_id->Delete(); 
    return PASSED; 
    }
  
  os << "Failed Image Test : " << minError << endl;
  char *rt_diffName = new char [strlen(this->ValidImageFileName) + 12];
  sprintf(rt_diffName,"%s.diff.png",this->ValidImageFileName);
  FILE *rt_dout = fopen(rt_diffName,"wb"); 
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

  if (rt_dout) 
    { 
    fclose(rt_dout);
    
    vtkPNGWriter *rt_pngw = vtkPNGWriter::New();
    rt_pngw->SetFileName(rt_diffName);
    rt_pngw->SetInput(rt_id->GetOutput());
    rt_pngw->Write();
    rt_pngw->Delete();

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
    char* diff_small = new char[strlen(tmpDir) + validName.size() + 30];
    sprintf(diff_small, "%s/%s.diff.small.jpg", tmpDir, validName.c_str());
    rt_jpegw_dashboard->SetFileName( diff_small );
    rt_jpegw_dashboard->SetInput(rt_gamma->GetOutput());
    rt_jpegw_dashboard->SetQuality(85);
    rt_jpegw_dashboard->Write();

    // write out the image that was generated
    rt_shrink->SetInput(rt_id->GetInput());
    rt_jpegw_dashboard->SetInput(rt_shrink-> GetOutput());
    char* valid_test_small = new char[strlen(tmpDir) + validName.size() + 30];
    sprintf(valid_test_small, "%s/%s.test.small.jpg", tmpDir, 
            validName.c_str());
    rt_jpegw_dashboard->SetFileName(valid_test_small);
    rt_jpegw_dashboard->Write();

    // write out the valid image that matched
    rt_shrink->SetInput(rt_id->GetImage());
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
    delete [] diff_small;

    rt_shrink->Delete();
    rt_gamma->Delete();
    }

  delete [] rt_diffName;
  rt_id->Delete(); 
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
  // dont print the this->LastResultText, it could be a bit long
  os << indent << "DataRoot: " << this->GetDataRoot() << endl;
  os << indent << "Temp Directory: " << this->GetTempDirectory() << endl;
}
