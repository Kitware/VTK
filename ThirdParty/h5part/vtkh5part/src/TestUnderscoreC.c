#include <stdio.h>

// XXX(kitware) include mangling header
#include "vtk_h5part_mangle.h"

void findunderscores(void){
  printf("#ifndef F77_NO_UNDERSCORE\n");
  printf("#define F77_NO_UNDERSCORE\n");
  printf("#endif\n");
  printf("#ifndef F77_NO_CAPS\n");
  printf("#define F77_NO_CAPS\n");
  printf("#endif\n");
}

void FindUnderscores(void){
  printf("#ifndef F77_NO_UNDERSCORE\n");
  printf("#define F77_NO_UNDERSCORE\n");
  printf("#endif\n");
}

void FindUnderscores_(void){
  printf("#ifndef F77_SINGLE_UNDERSCORE\n");
  printf("#define F77_SINGLE_UNDERSCORE\n");
  printf("#endif\n");
}

void findunderscores_(void){
  printf("#ifndef F77_SINGLE_UNDERSCORE\n");
  printf("#define F77_SINGLE_UNDERSCORE\n");
  printf("#endif\n");
  printf("#ifndef F77_NO_CAPS\n");
  printf("#define F77_NO_CAPS\n");
  printf("#endif\n");
}
void FINDUNDERSCORES(void){
  printf("#ifndef F77_CRAY_UNDERSCORE\n");
  printf("#define F77_CRAY_UNDERSCORE\n");
  printf("#endif\n");
}
