
// Includes and a macro necessary for saving the image produced by a cxx 
// example program. This capability is critical for regression testing.
// This function returns 1 if test passed, 0 if test failed.

#include "vtkWindowToImageFilter.h"
#include "vtkPNGReader.h"
#include "vtkPNGWriter.h"
#include "vtkImageDifference.h"

int vtkRegressionTestImage2(int argc, char *argv[], vtkWindow *rw ) 
{
  if( (argc >= 3) && (strcmp("-V", argv[argc-2]) == 0) ) 
    { 
    vtkWindowToImageFilter *rt_w2if = vtkWindowToImageFilter::New(); 
    rt_w2if->SetInput(rw);
    FILE *rt_fin = fopen(argv[argc-1],"r"); 
    if (rt_fin) 
      { 
      fclose(rt_fin);
      }
    else
      {
      FILE *rt_fout = fopen(argv[argc-1],"wb"); 
      if (rt_fout) 
        { 
        fclose(rt_fout);
        vtkPNGWriter *rt_pngw = vtkPNGWriter::New();
        rt_pngw->SetFileName(argv[argc-1]);
        rt_pngw->SetInput(rt_w2if->GetOutput());
        rt_pngw->Write();
        rt_pngw->Delete();
        }
      else
        {
        fprintf(stderr,"Unable to find valid image!!!");
        return 0;
        }
      }
    vtkPNGReader *rt_png = vtkPNGReader::New(); 
    rt_png->SetFileName(argv[argc-1]); 
    vtkImageDifference *rt_id = vtkImageDifference::New(); 
    rt_id->SetInput(rt_w2if->GetOutput()); 
    rt_id->SetImage(rt_png->GetOutput()); 
    rt_id->Update(); 
    rt_w2if->Delete(); 
    rt_png->Delete(); 
    if (rt_id->GetThresholdedError() <= 10) 
      { 
      rt_id->Delete(); 
      return 1; 
      } 
    fprintf(stderr,"Failed Image Test : %f", rt_id->GetThresholdedError()); 
    rt_id->Delete(); 
    return 0;
    }
}

#define vtkRegressionTestImage(rw) \
vtkRegressionTestImage2(argc, argv, rw)
