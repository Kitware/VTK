
// Includes and a macro necessary for saving the image produced by a cxx 
// example program. This capability is critical for regression testing.

#include <stdio.h>
#include <string.h>
#include "vtkWindowToImageFilter.h"
#include "vtkTIFFWriter.h"
#define SAVEIMAGE( rw ) \
  if( (argc >= 2) && (strcmp("-S", argv[argc-1]) == 0) ) \
    { \
    char save_filename[1024]; \
    vtkWindowToImageFilter *w2if = vtkWindowToImageFilter::New(); \
    vtkTIFFWriter *rttiffw = vtkTIFFWriter::New(); \
    sprintf( save_filename, "%s.cxx.tif", argv[0] ); \
    w2if->SetInput(rw);\
    rttiffw->SetInput(w2if->GetOutput());\
    rttiffw->SetFileName(save_filename); \
    rttiffw->Write(); \
    exit( 1 ); \
    }
