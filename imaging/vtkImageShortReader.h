/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageShortReader.h
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
// .NAME vtkImageShortReader - Reader that ingores headers.
// .SECTION Description
// vtkImageShortReader will read an image saved as unsigned shorts,
// but ignores all header information.  All image information needs
// to be set explicitly, especially the Dimensions.  The data is assumed to
// be stored in 2D images with identical sized headers, and a single index
// number.  The header size is computed automatically from the Dimensions
// and the file lengths.  This assumes that there is no trailer after
// the data.  In this case, the header size must be set explicitly.


#ifndef __vtkImageShortReader_h
#define __vtkImageShortReader_h

#include <iostream.h>
#include <fstream.h>
#include "vtkImageCachedSource.h"

class VTK_EXPORT vtkImageShortReader : public vtkImageCachedSource
{
public:
  vtkImageShortReader();
  ~vtkImageShortReader();
  static vtkImageShortReader *New() {return new vtkImageShortReader;};
  char *GetClassName() {return "vtkImageShortReader";};
  void PrintSelf(ostream& os, vtkIndent indent);   

  // Description:
  // Set the image dimensions.  This also computes the increments.
  void SetDimensions(int num, int *size);
  vtkImageSetMacro(Dimensions,int);

  // Description:
  // Get the image dimensions.
  void GetDimensions(int num, int *size);
  vtkImageGetMacro(Dimensions,int);
  int *GetDimensions() {return this->Dimensions;};  
  
  // Description:
  // Set/Get the spacing of the data.
  void SetSpacing(int num, float *ratio);
  vtkImageSetMacro(Spacing,float);
  void GetSpacing(int num, float *ratio);
  vtkImageGetMacro(Spacing,float);
  float *GetSpacing() {return this->Spacing;};  
  
  // Description:
  // Set/Get the origin of the data (location of point (0,0,0,...)).
  void SetOrigin(int num, float *ratio);
  vtkImageSetMacro(Origin,float);
  void GetOrigin(int num, float *ratio);
  vtkImageGetMacro(Origin,float);
  float *GetOrigin() {return this->Origin;};  
  
  void SetFilePrefix(char *filePrefix);
  void SetFilePattern(char *filePattern);
  void UpdateImageInformation(vtkImageRegion *region);
  vtkImageSource *GetOutput();
  
  // Description:
  // Get the number of the first image
  vtkSetMacro(First,int);
  vtkGetMacro(First,int);

  // Description:
  // Set/Get the pixel mask
  vtkGetMacro(PixelMask,unsigned short);
  void SetPixelMask(int val) 
  {this->PixelMask = ((unsigned short)(val)); this->Modified();};
  
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
  
  // following should only be used by methods or template helpers, not users
  ifstream *File;
  int FileSize;
  int HeaderSize;
  int Signed;
  int SwapBytes;
  unsigned short PixelMask;  // Mask each pixel with
  // Reader keeps track of the min and max for convenience.
  double PixelMin;
  double PixelMax;
  // For seeking to the correct location in the files.
  int Increments[VTK_IMAGE_DIMENSIONS];

protected:
  int Initialized;
  char *FilePrefix;
  char *FilePattern;
  char *FileName;
  int Dimensions[VTK_IMAGE_DIMENSIONS];
  float Spacing[VTK_IMAGE_DIMENSIONS];
  float Origin[VTK_IMAGE_DIMENSIONS];
  
  // The first image file has this index
  int First;

  void Initialize();
  void UpdatePointData(vtkImageRegion *outRegion);    
};

#endif


