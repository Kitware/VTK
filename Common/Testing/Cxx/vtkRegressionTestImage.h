#ifndef __vtkRegressionTestImage_h
#define __vtkRegressionTestImage_h

// Includes and a macro necessary for saving the image produced by a cxx 
// example program. This capability is critical for regression testing.
// This function returns 1 if test passed, 0 if test failed.

#include "vtkWindowToImageFilter.h"
#include "vtkPNGReader.h"
#include "vtkPNGWriter.h"
#include "vtkImageDifference.h"
#include "vtkGetDataRoot.h"

int vtkRegressionTestImage2(int argc, char *argv[], vtkWindow *rw ) 
{
  int imageIndex=-1;

  for (int i=0; i<argc; i++)
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
    char* fname=vtkExpandDataFileName(argc, argv, argv[imageIndex]);

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
        return 0;
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
    if (rt_id->GetThresholdedError() <= 10) 
      { 
      rt_id->Delete(); 
      delete[] fname;
      return 1; 
      } 
    cerr << "Failed Image Test : " << rt_id->GetThresholdedError() << endl;
    rt_id->Delete(); 
    delete[] fname;
    return 0;
    }
  return 2;
}

#define vtkRegressionTestImage(rw) \
vtkRegressionTestImage2(argc, argv, rw)

#endif // __vtkRegressionTestImage_h
