/*=========================================================================

  Program:   Visualization Library
  Module:    PNMSrc.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vlPNMSource - read pnm (i.e., portable anymap) files
// .SECTION Description
// vlPNMSource is a source object that reads pnm (portable anymap) files.
// This includes .pbm (bitmap), .pgm (grayscale), and .ppm (pixmap) files.
// (Currently this object only reads binary versions of these files).
//    PNMSource creates structured point datasets. The dimension of the 
// dataset depends upon the number of files read. Reading a single file 
// results in a 2D image, while reading more than one file results in a 
// 3D volume.
//    To read a volume, files must be of the form "filename.<number>"
// (e.g., foo.ppm.0, foo.ppm.1, ...). You must also specify the image 
// range. This range specifies the beginning and ending files to read (range
// can be any pair of non-negative numbers). 
//    The default behavior is to read a single file. In this case, the form
// of the file is simply "filename" (e.g., foo.bar, foo.ppm, foo.pnm). To 
// differentiate between reading images and volumes, the image range is set
// to  (-1,-1) to read a single image file.

#ifndef __vlPNMSource_h
#define __vlPNMSource_h

#include <stdio.h>
#include "SPtsSrc.hh"
#include "Pixmap.hh"
#include "Graymap.hh"
#include "Bitmap.hh"

class vlPNMSource : public vlStructuredPointsSource
{
public:
  vlPNMSource();
  ~vlPNMSource() {};
  char *GetClassName() {return "vlPNMSource";};
  void PrintSelf(ostream& os, vlIndent indent);

  // Description:
  // Specify file name of pnm file(s).
  vlSetStringMacro(Filename);
  vlGetStringMacro(Filename);

  // Description:
  // Set the range of files to read.
  vlSetVector2Macro(ImageRange,int);
  vlGetVectorMacro(ImageRange,int,2);

  // Description:
  // Specify an aspect ratio for the data.
  vlSetVector3Macro(DataAspectRatio,float);
  vlGetVectorMacro(DataAspectRatio,float,3);

  // Description:
  // Specify the origin for the data.
  vlSetVector3Macro(DataOrigin,float);
  vlGetVectorMacro(DataOrigin,float,3);

protected:
  void Execute();
  char *Filename;
  int ImageRange[2];
  float DataAspectRatio[3];
  float DataOrigin[3];

  vlColorScalars *ReadImage(int dim[3]);
  vlColorScalars *ReadVolume(int dim[3]);

  int ReadBinaryPBM(FILE *fp, vlBitmap *s, int n, int xsize, int ysize);
  int ReadBinaryPGM(FILE *fp, vlGraymap *s, int n, int xsize, int ysize);
  int ReadBinaryPPM(FILE *fp, vlPixmap *s, int n, int xsize, int ysize);
};

#endif


