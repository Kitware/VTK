
// Includes and a macro necessary for saving the image produced by a cxx 
// example program. This capability is critical for regression testing.

#include <stdio.h>
#include <string.h>

#define SAVEVIEWERIMAGE( viewer )                                 \
  if( (argc >= 2) && (strcmp("-S", argv[argc-1]) == 0) )     \
    {                                                   \
    char save_filename[4096];                                 \
                                                        \
    sprintf( save_filename, "%s.cxx.ppm", argv[0] );             \
    vtkWindowToImageFilter *wtoif = vtkWindowToImageFilter::New(); \
    wtoif->SetInput(viewer->GetImageWindow()); \
    vtkPNMWriter *pnm = vtkPNMWriter::New(); \
    pnm->SetFileName(save_filename); \
    pnm->SetInput(wtoif->GetOutput()); \
    pnm->Write(); \
    wtoif->Delete(); \
    pnm->Delete(); \
    exit( 1 );						\
    }
