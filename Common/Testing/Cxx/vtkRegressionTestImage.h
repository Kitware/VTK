
// Includes and a macro necessary for saving the image produced by a cxx 
// example program. This capability is critical for regression testing.
// This function returns 1 if test passed, 0 if test failed.

#include "vtkWindowToImageFilter.h"
#include "vtkPNGReader.h"
#include "vtkPNGWriter.h"
#include "vtkImageDifference.h"

int vtkRegressionTestImage2(int argc, char *argv[], vtkWindow *rw ) 
{
  int dataIndex=-1, imageIndex=-1;

  for (int i=0; i<argc; i++)
    {
    if ( strcmp("-V", argv[i]) == 0 )
      {
      if ( i < argc-1 )
	{
	imageIndex = i+1;
	}
      }
    else if ( strcmp("-D", argv[i]) == 0 )
      {
      if ( i < argc-1 )
	{
	dataIndex = i+1;
	}
      }
    }

  if( imageIndex != -1 ) 
    { 
    char* dataRoot=0;
    if ( dataIndex != -1 ) 
      {
      dataRoot = argv[dataIndex];
      }
    else 
      {
      dataRoot = getenv("VTK_DATA_ROOT");
      }

    char* fname;
    if (dataRoot)
      {
      fname = new char[strlen(dataRoot)+strlen(argv[imageIndex])+2];
      fname[0] = 0;
      strcat(fname, dataRoot);
      int len = strlen(fname);
      fname[len] = '/';
      fname[len+1] = 0;
      strcat(fname, argv[imageIndex]);
      }
    else
      {
      fname = argv[imageIndex];
      }

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
      return 1; 
      } 
    cerr << "Failed Image Test : " << rt_id->GetThresholdedError() << endl;
    rt_id->Delete(); 
    return 0;
    }
  return 2;
}

#define vtkRegressionTestImage(rw) \
vtkRegressionTestImage2(argc, argv, rw)
