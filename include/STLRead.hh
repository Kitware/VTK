//
// Read ASCII/Binary Stereo Lithography Files
//
#ifndef __vlSTLReader_h
#define __vlSTLReader_h

#include "PolySrc.hh"
#include <stdio.h>

class vlSTLReader : public vlPolySource 
{
public:
  vlSTLReader(char *fname=0);
  char *GetClassName() {return "vlSTLReader";};

  vlSetStringMacro(Filename);
  vlGetStringMacro(Filename);

protected:
  char *Filename;
  void Execute();
  void ReadBinarySTL(FILE *fp);
  void ReadASCIISTL(FILE *fp);
  int GetSTLFileType(FILE *fp);
};

#endif


