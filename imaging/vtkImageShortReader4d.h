/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageShortReader4d.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to C. Charles Law who developed this class.

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
// .NAME vtkImageShortReader4d - Generic Reader Class.
// .SECTION Description
// vtkImageShortReader4d will read an image saved as unsigned shorts.
// The dimensions of the image has to be prespecified.
// The header of the file is completely ignored.  
// Images are stored in individual files: i.e. prefix.1, prefix.2 ...
// This class generate 4d regions.  It will just 
// duplicate the 3d volume for each slice of the extra dimension.


#ifndef __vtkImageShortReader4d_h
#define __vtkImageShortReader4d_h

#include <iostream.h>
#include <fstream.h>
#include "vtkImageCachedSource.h"

class vtkImageShortReader4d : public vtkImageCachedSource
{
public:
  vtkImageShortReader4d();
  ~vtkImageShortReader4d();
  char *GetClassName() {return "vtkImageShortReader4d";};
  void PrintSelf(ostream& os, vtkIndent indent);   
  
  void SetDimensions(int size0, int size1, int size2, int size3);
  void SetDimensions(int *size);
  
  // Description:
  // Set And get the aspect ratio of the data.
  vtkSetVector4Macro(AspectRatio, float);
  vtkGetVector4Macro(AspectRatio, float);
  
  // Description:
  // Set And get the origin of the data (location of point (0,0,0,0)).
  vtkSetVector4Macro(Origin, float);
  vtkGetVector4Macro(Origin, float);
  
  void SetFilePrefix(char *filePrefix);
  void SetFilePattern(char *filePattern);
  void UpdateImageInformation(vtkImageRegion *region);
  vtkImageSource *GetOutput();
  
  // Description:
  // Get the number of the first image
  vtkGetMacro(First,int);

  // Description:
  // Set/Get the pixel mask
  vtkGetMacro(PixelMask,unsigned short);
  void SetPixelMask(int val) {this->PixelMask = ((unsigned short)(val)); this->Modified();};
  
  // Description:
  // Set/Get the Signed flag
  vtkBooleanMacro(Signed,int);
  vtkSetMacro(Signed,int);
  vtkGetMacro(Signed,int);

  // Description:
  // Set/Get the byte swapping
  vtkSetMacro(SwapBytes,int);
  vtkGetMacro(SwapBytes,int);
  vtkBooleanMacro(SwapBytes,int);

  // Description:
  // Get the size of the header computed by this object.
  vtkGetMacro(HeaderSize, int);
  
  // Templated function that reads into different data types.
  friend void vtkImageShortReader4dGenerateRegion2d(
			     vtkImageShortReader4d *self,
			     vtkImageRegion *region, float *ptr);
  friend void vtkImageShortReader4dGenerateRegion2d(
			     vtkImageShortReader4d *self,
			     vtkImageRegion *region, int *ptr);
  friend void vtkImageShortReader4dGenerateRegion2d(
			     vtkImageShortReader4d *self,
			     vtkImageRegion *region, short *ptr);
  friend void vtkImageShortReader4dGenerateRegion2d(
			     vtkImageShortReader4d *self,
			     vtkImageRegion *region, unsigned short *ptr);
  friend void vtkImageShortReader4dGenerateRegion2d(
			     vtkImageShortReader4d *self,
			     vtkImageRegion *region, unsigned char *ptr);
  
protected:
  int Initialized;
  char *FilePrefix;
  char *FilePattern;
  char *FileName;
  ifstream *File;
  int FileSize;
  int HeaderSize;
  int Signed;
  int SwapBytes;
  int Dimensions[4];
  float AspectRatio[4];
  float Origin[4];
  
  // For seeking to the correct location in the files.
  int Increments[4];
  // The first image has this number
  int First;
  // Mask each pixel with
  unsigned short PixelMask;

  // Reader keeps track of the min and max for convenience.
  double PixelMin;
  double PixelMax;

  void Initialize();
  void UpdateRegion2d(vtkImageRegion *outRegion);    
};

#endif


