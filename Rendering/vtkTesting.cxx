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

#include <sys/stat.h>

vtkStandardNewMacro(vtkTesting);
vtkCxxRevisionMacro(vtkTesting, "1.1");
vtkCxxSetObjectMacro(vtkTesting, RenderWindow, vtkRenderWindow);


char* vtkTestUtilities::GetDataRoot(int argc, char* argv[])
{
  return vtkTestUtilities::GetArgOrEnvOrDefault(
    "-D", argc, argv, 
    "VTK_DATA_ROOT", 
    "../../../../VTKData");
}


char* vtkTestUtilities::ExpandDataFileName(int argc, char* argv[], 
                                           const char* fname,
                                           int slash)
{
  return vtkTestUtilities::ExpandFileNameWithArgOrEnvOrDefault(
    "-D", argc, argv, 
    "VTK_DATA_ROOT", 
    "../../../../VTKData",
    fname,
    slash);
}


char* vtkTestUtilities::GetArgOrEnvOrDefault(const char* arg, 
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


char* vtkTestUtilities::ExpandFileNameWithArgOrEnvOrDefault(const char* arg, 
                                                            int argc, 
                                                            char* argv[], 
                                                            const char* env, 
                                                            const char *def, 
                                                            const char* fname,
                                                            int slash)
{
  char* fullName = 0;

  char* value = vtkTestUtilities::GetArgOrEnvOrDefault(arg, argc, argv, 
                                                       env,
                                                       def);
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

vtkTesting::vtkTesting()
{
  this->FrontBuffer = 0;
  this->RenderWindow = 0;
  this->DataFileName = 0;
}  

vtkTesting::~vtkTesting()
{
  this->SetRenderWindow(0);
  this->SetDataFileName(0);
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

int vtkTesting::RegressionTest(float thresh)
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

  FILE *rt_fin = fopen(this->DataFileName,"r"); 
  if (rt_fin) 
    { 
    fclose(rt_fin);
    }
  else
    {
    FILE *rt_fout = fopen(this->DataFileName,"wb"); 
    if (rt_fout) 
      { 
      fclose(rt_fout);
      vtkPNGWriter *rt_pngw = vtkPNGWriter::New();
      rt_pngw->SetFileName(this->DataFileName);
      rt_pngw->SetInput(rt_w2if->GetOutput());
      rt_pngw->Write();
      rt_pngw->Delete();
      }
    else
      {
      cerr << "Unable to open file for writing: " << this->DataFileName << endl;
      rt_w2if->Delete(); 
      return FAILED;
      }
    }

  vtkPNGReader *rt_png = vtkPNGReader::New(); 
  rt_png->SetFileName(this->DataFileName); 
  vtkImageDifference *rt_id = vtkImageDifference::New(); 
  rt_id->SetInput(rt_w2if->GetOutput()); 
  rt_id->SetImage(rt_png->GetOutput()); 
  rt_id->Update(); 
  rt_w2if->Delete(); 
  rt_png->Delete(); 

  float minError = rt_id->GetThresholdedError();
  if (minError <= thresh) 
    { 
    rt_id->Delete(); 
    return PASSED; 
    }

  // If the test failed with the first image (foo.png)
  // check if there are images of the form foo_N.png
  // (where N=1,2,3...) and compare against them.
  float error=0;
  int count=1, errIndex=-1;
  char* newFileName;
  while (1)
    {
    newFileName = IncrementFileName(this->DataFileName, count);
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
      rt_id->Delete(); 
      delete[] newFileName;
      return PASSED; 
      }
    else
      {
      if (error > minError)
        {
        errIndex = count;
        minError = error;
        }
      }
    ++count;
    delete[] newFileName;
    }

  cerr << "Failed Image Test : " << minError << endl;
  char *rt_diffName = new char [strlen(this->DataFileName) + 12];
  sprintf(rt_diffName,"%s.diff.png",this->DataFileName);
  FILE *rt_dout = fopen(rt_diffName,"wb"); 
  if (errIndex >= 0)
    {
    newFileName = IncrementFileName(this->DataFileName, errIndex);
    rt_png->SetFileName(newFileName);
    delete[] newFileName;
    }
  else
    {
    rt_png->SetFileName(this->DataFileName);
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
    float rt_magfactor=1.0;
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
    char* diff_small = new char[strlen(this->DataFileName) + 30];
    sprintf(diff_small, "%s.diff.small.jpg", this->DataFileName);
    rt_jpegw_dashboard->SetFileName( diff_small );
    rt_jpegw_dashboard->SetInput(rt_gamma->GetOutput());
    rt_jpegw_dashboard->SetQuality(85);
    rt_jpegw_dashboard->Write();

    // write out the image that was generated
    rt_shrink->SetInput(rt_id->GetInput());
    rt_jpegw_dashboard->SetInput(rt_shrink-> GetOutput());
    char* valid_test_small = new char[strlen(this->DataFileName) + 30];
    sprintf(valid_test_small, "%s.test.small.jpg", this->DataFileName);
    rt_jpegw_dashboard->SetFileName(valid_test_small);
    rt_jpegw_dashboard->Write();

    // write out the valid image that matched
    rt_shrink->SetInput(rt_id->GetImage());
    rt_jpegw_dashboard-> SetInput (rt_shrink->GetOutput());
    char* valid = new char[strlen(this->DataFileName) + 30];
    sprintf(valid, "%s.small.jpg", this->DataFileName);
    rt_jpegw_dashboard-> SetFileName( valid);
    rt_jpegw_dashboard->Write();
    rt_jpegw_dashboard->Delete();


    cout << "<DartMeasurement name=\"ImageError\" type=\"numeric/double\">";
    cout << error;
    cout << "</DartMeasurement>";
    if ( errIndex <= 0)
      {
      cout << "<DartMeasurement name=\"BaselineImage\" type=\"text/string\">Standard</DartMeasurement>";
      } 
    else 
      {
      cout <<  "<DartMeasurement name=\"BaselineImage\" type=\"numeric/integer\">";
      cout << errIndex;
      cout << "</DartMeasurement>";
      }

    cout <<  "<DartMeasurementFile name=\"TestImage\" type=\"image/jpeg\">";
    cout << valid_test_small;
    delete [] valid_test_small;
    cout << "</DartMeasurementFile>";
    cout << "<DartMeasurementFile name=\"DifferenceImage\" type=\"image/jpeg\">";
    cout << diff_small;
    cout << "</DartMeasurementFile>";
    cout << "<DartMeasurementFile name=\"ValidImage\" type=\"image/jpeg\">";
    cout << valid;
    cout <<  "</DartMeasurementFile>";

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
                              float thresh ) 
{
  vtkTesting * testing = vtkTesting::New();
  int imageIndex=-1;
  int i;

  for (i=0; i<argc; i++)
    {
    if ( strcmp("-I", argv[i]) == 0 )
      {
      testing->Delete();
      return DO_INTERACTOR;
      }
    }

  for (i=0; i<argc; i++)
    {
    if ( strcmp("-V", argv[i]) == 0 )
      {
      if ( i < argc-1 )
        {
        imageIndex = i+1;
        }
      }
    }

  int frontBuffer = 0;
  for (i=0; i<argc; i++)
    {
    if ( strcmp("-FrontBuffer", argv[i]) == 0 )
      {
      frontBuffer = 1;
      testing->FrontBufferOn();
      }
    }

  if( imageIndex != -1 ) 
    { 
    // Prepend the data root to the filename
    char* fname=vtkTestUtilities::ExpandDataFileName(argc, argv, argv[imageIndex]);
    testing->SetDataFileName(fname);
    testing->SetRenderWindow(rw);
    int res = testing->RegressionTest(thresh);
    delete[] fname;
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
  os << indent << "DataFileName: " << (this->DataFileName?this->DataFileName:"(none)") << endl;
  os << indent << "FrontBuffer: " << (this->FrontBuffer?"On":"Off") << endl;
}
