/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPNMReader.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1995 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.


=========================================================================*/
// .NAME vtkPNMReader - read pnm (i.e., portable anymap) files
// .SECTION Description
// vtkPNMReader is a source object that reads pnm (portable anymap) files.
// This includes .pbm (bitmap), .pgm (grayscale), and .ppm (pixmap) files.
// (Currently this object only reads binary versions of these files.)
//
// PNMReader creates structured point datasets. The dimension of the 
// dataset depends upon the number of files read. Reading a single file 
// results in a 2D image, while reading more than one file results in a 
// 3D volume.
//
// To read a volume, files must be of the form "filename.<number>"
// (e.g., foo.ppm.0, foo.ppm.1, ...). You must also specify the image 
// range. This range specifies the beginning and ending files to read (range
// can be any pair of non-negative numbers). 
//
// The default behavior is to read a single file. In this case, the form
// of the file is simply "filename" (e.g., foo.bar, foo.ppm, foo.pnm). To 
// differentiate between reading images and volumes, the image range is set
// to  (-1,-1) to read a single image file.

#ifndef __vtkPNMReader_h
#define __vtkPNMReader_h

#include <stdio.h>
#include "vtkVolumeReader.hh"
#include "vtkPixmap.hh"
#include "vtkGraymap.hh"
#include "vtkBitmap.hh"

class vtkPNMReader : public vtkVolumeReader
{
public:
  vtkPNMReader();
  char *GetClassName() {return "vtkPNMReader";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Specify file name of pnm file(s).
  vtkSetStringMacro(Filename);
  vtkGetStringMacro(Filename);

  vtkStructuredPoints *GetImage(int ImageNum);
  
protected:
  void Execute();
  char *Filename;

  vtkColorScalars *ReadImage(int dim[3]);
  vtkColorScalars *ReadVolume(int dim[3]);

  vtkColorScalars *ReadBinaryPNM(FILE *fp, vtkColorScalars *s, int &type, 
                                 int offset, int &xsize, int &ysize);
  int ReadBinaryPBM(FILE *fp, vtkBitmap *s, int offset, int xsize, int ysize);
  int ReadBinaryPGM(FILE *fp, vtkGraymap *s, int offset, int xsize, int ysize);
  int ReadBinaryPPM(FILE *fp, vtkPixmap *s, int offset, int xsize, int ysize);
};

#endif


