
// Includes and a macro necessary for saving the image produced by a cxx 
// example program. This capability is critical for regression testing.

#include <stdio.h>
#include <string.h>

#define SAVEIMAGE( rw )                                 \
  if( (argc >= 2) && (strcmp("-S", argv[argc-1]) == 0) )     \
    {                                                   \
    char save_filename[100];                                 \
                                                        \
    sprintf( save_filename, "%s.cxx.ppm", argv[0] );             \
    rw->SetFileName( save_filename );                        \
    rw->SaveImageAsPPM();                               \
							\
    exit( 1 );						\
    }
