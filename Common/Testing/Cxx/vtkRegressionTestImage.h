#ifndef __vtkRegressionTestImage_h
#define __vtkRegressionTestImage_h

// Includes and a macro necessary for saving the image produced by a cxx 
// example program. This capability is critical for regression testing.
// This function returns 1 if test passed, 0 if test failed.

#include <sys/stat.h>

#include "vtkWindowToImageFilter.h"
#include "vtkPNGReader.h"
#include "vtkPNGWriter.h"
#include "vtkImageDifference.h"
#include "vtkTestUtilities.h"

class vtkRegressionTester
{
public:
  static int Test(int argc, char *argv[], vtkWindow *rw, float thresh );

  enum ReturnValue {
    FAILED = 0,
    PASSED = 1,
    NOT_RUN = 2,
    DO_INTERACTOR = 3
  };
  
private:
  static char* IncrementFileName(const char* fname, int count);
  static int LookForFile(const char* newFileName);
};

#define vtkRegressionTestImage(rw) \
vtkRegressionTester::Test(argc, argv, rw, 10)

#define vtkRegressionTestImageThreshold(rw, t) \
vtkRegressionTester::Test(argc, argv, rw, t)


int vtkRegressionTester::Test(int argc, char *argv[], vtkWindow *rw, 
			      float thresh ) 
{
  int imageIndex=-1;
  int i;

  for (i=0; i<argc; i++)
    {
    if ( strcmp("-I", argv[i]) == 0 )
      {
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

  if( imageIndex != -1 ) 
    { 
    // Prepend the data root to the filename
    char* fname=vtkTestUtilities::ExpandDataFileName(argc, argv, argv[imageIndex]);

    vtkWindowToImageFilter *rt_w2if = vtkWindowToImageFilter::New(); 
    rt_w2if->SetInput(rw);
    FILE *rt_fin = fopen(fname,"r"); 
    if (rt_fin) 
      { 
      fclose(rt_fin);
      }
    else
      {
      FILE *rt_fout = fopen(fname,"wb"); 
      if (rt_fout) 
        { 
        fclose(rt_fout);
        vtkPNGWriter *rt_pngw = vtkPNGWriter::New();
        rt_pngw->SetFileName(fname);
        rt_pngw->SetInput(rt_w2if->GetOutput());
        rt_pngw->Write();
        rt_pngw->Delete();
        }
      else
        {
	cerr << "Unable to open file for writing: " << fname << endl;
	rt_w2if->Delete(); 
	delete[] fname;
        return FAILED;
        }
      }
    vtkPNGReader *rt_png = vtkPNGReader::New(); 
    rt_png->SetFileName(fname); 
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
      delete[] fname;
      return PASSED; 
      }
    // If the test failed with the first image (foo.png)
    // check if there are images of the form foo_N.png
    // (where N=1,2,3...) and compare against them.
    float error;
    int count=1, errIndex=-1;
    char* newFileName;
    while (1)
      {
      newFileName = IncrementFileName(fname, count);
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
	delete[] fname;
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
    char *rt_diffName = new char [strlen(fname) + 12];
    sprintf(rt_diffName,"%s.diff.png",fname);
    FILE *rt_dout = fopen(rt_diffName,"wb"); 
    if (errIndex >= 0)
      {
      newFileName = IncrementFileName(fname, errIndex);
      rt_png->SetFileName(newFileName);
      delete[] newFileName;
      }
    else
      {
      rt_png->SetFileName(fname);
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
      }
    delete [] rt_diffName;
    rt_id->Delete(); 
    delete[] fname;
    return FAILED;
    }
  return NOT_RUN;
}

char* vtkRegressionTester::IncrementFileName(const char* fname, 
						    int count)
{
  char counts[256];
  sprintf(counts, "%d", count);
  
  int orgLen = strlen(fname);
  if (orgLen < 5)
    {
    return 0;
    }
  int extLen = strlen(counts);
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

int vtkRegressionTester::LookForFile(const char* newFileName)
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


#endif // __vtkRegressionTestImage_h
