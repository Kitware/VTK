
// Includes and a macro necessary for saving the image produced by a cxx 
// example program. This capability is critical for regression testing.

#include <stdio.h>
#include <string.h>
#include "vtkWindowToImageFilter.h"
#include "vtkTIFFWriter.h"

#define SAVEVIEWERIMAGE( viewer )                                 \
  if( (argc >= 2) && (strcmp("-S", argv[argc-1]) == 0) )     \
    {                                                   \
    char save_filename[4096];                                 \
                                                        \
    sprintf( save_filename, "%s.cxx.tif", argv[0] );             \
    vtkWindowToImageFilter *wtoif = vtkWindowToImageFilter::New(); \
    wtoif->SetInput(viewer->GetImageWindow()); \
    vtkTIFFWriter *rttiffw = vtkTIFFWriter::New(); \
    rttiffw->SetInput(wtoif->GetOutput());\
    rttiffw->SetFileName(save_filename); \
    rttiffw->SetInput(wtoif->GetOutput()); \
    rttiffw->Write(); \
    rttiffw->SetFileName(save_filename); \
    rttiffw->Write(); \
    wtoif->Delete(); \
    rttiffw->Delete(); \
    exit( 1 );						\
    }
