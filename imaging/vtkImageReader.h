/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageReader.h
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
// .NAME vtkImageReader - Superclass of binary file readers.
// .SECTION Description
// vtkImageReader provides methods needed to read a region from a file.


#ifndef __vtkImageReader_h
#define __vtkImageReader_h

#include <iostream.h>
#include <fstream.h>
#include "vtkImageCachedSource.h"

#define VTK_FILE_BYTE_ORDER_BIG_ENDIAN 0
#define VTK_FILE_BYTE_ORDER_LITTLE_ENDIAN 1

class VTK_EXPORT vtkImageReader : public vtkImageCachedSource
{
public:
  vtkImageReader();
  ~vtkImageReader();
  char *GetClassName() {return "vtkImageReader";};
  void PrintSelf(ostream& os, vtkIndent indent);   

  void SetDataScalarTypeToFloat(){this->SetDataScalarType(VTK_FLOAT);}
  void SetDataScalarTypeToInt(){this->SetDataScalarType(VTK_INT);}
  void SetDataScalarTypeToShort(){this->SetDataScalarType(VTK_SHORT);}
  void SetDataScalarTypeToUnsignedShort()
    {this->SetDataScalarType(VTK_UNSIGNED_SHORT);}
  void SetDataScalarTypeToUnsignedChar()
    {this->SetDataScalarType(VTK_UNSIGNED_CHAR);}
  void SetDataScalarType(int type);
  // Description:
  // Get the file format.  Pixels are this type in the file.
  vtkGetMacro(DataScalarType, int);
  
  // Description:
  // Get/Set the image extent.  This is an alternative way of setting
  // the dimensions.  It is more compatable with the imaging pipelines.
  void SetDataExtent(int num, int *extent);
  vtkImageSetExtentMacro(DataExtent);
  void GetDataExtent(int num, int *extent);
  vtkImageGetExtentMacro(DataExtent);
  
  // Description:
  // Set the image dimensions.  This also computes the increments.
  // Dimensions have to be set manually because, the header is ignored.
  void SetDataDimensions(int num, int *size);
  vtkImageSetMacro(DataDimensions,int);
  // Description:
  // Get the image dimensions. Note that the dimensions are different (+1)
  // than the max values of the image extent.  This is here for compatability
  // with the vtkVolume16 reader.
  void GetDataDimensions(int num, int *size);
  vtkImageGetMacro(DataDimensions,int);
  int *GetDataDimensions() {return this->DataDimensions;};  
  
  
  // Description:
  // Set/Get the spacing of the data in the file.
  void SetDataSpacing(int num, float *ratio);
  vtkImageSetMacro(DataSpacing,float);
  void GetDataSpacing(int num, float *ratio);
  vtkImageGetMacro(DataSpacing,float);
  float *GetDataSpacing() {return this->DataSpacing;};  
  
  // Description:
  // Set/Get the origin of the data (location of first pixel in the file).
  void SetDataOrigin(int num, float *ratio);
  vtkImageSetMacro(DataOrigin,float);
  void GetDataOrigin(int num, float *ratio);
  vtkImageGetMacro(DataOrigin,float);
  float *GetDataOrigin() {return this->DataOrigin;};  
  
  // Set/Get the memory organization in the file.  The first axis is
  // the most colocated in memory.  This order is also used as the 
  // coordinate system of the instance variables: DataOrigin, DataExtent, ...
  virtual void SetDataMemoryOrder(int dim, int *order);
  vtkImageSetMacro(DataMemoryOrder,int);
  void GetDataMemoryOrder(int dim, int *order);
  vtkImageGetMacro(DataMemoryOrder,int);
  int *GetDataMemoryOrder() {return this->DataMemoryOrder;};
  
  void UpdateImageInformation(vtkImageRegion *region);
  vtkImageCache *GetOutput();
  
  // Description:
  // Get the size of the header computed by this object.
  vtkGetMacro(HeaderSize, int);
  // Description:
  // If there is a tail on the file, you want to explicitly set the
  // header size.
  void SetHeaderSize(int size);
  
  // Description:
  // Set/Get the Data mask. The mask is for legacy compatablilty.
  vtkGetMacro(DataMask,unsigned short);
  void SetDataMask(int val) 
  {this->DataMask = ((unsigned short)(val)); this->Modified();};
  
  // Description:
  // These methods should be used instead of the SwapBytes methods.
  // They indicate the byte ordering of the file you are trying
  // to read in. These methods will then either swap or not swap
  // the bytes depending on the byte ordering of the machine it is
  // being run on. For example, reading in a BigEndian file on a
  // BigEndian machine will result in no swapping. Trying to read
  // the same file on a LittleEndian machine will result in swapping.
  // As a quick note most UNIX machines are BigEndian while PC's
  // and VAX tend to be LittleEndian. So if the file you are reading
  // in was generated on a VAX or PC, SetDataByteOrderToLittleEndian 
  // otherwise SetDataByteOrderToBigEndian. 
  void SetDataByteOrderToBigEndian();
  void SetDataByteOrderToLittleEndian();
  int GetDataByteOrder();
  void SetDataByteOrder(int);
  char *GetDataByteOrderAsString();

  // Description:
  // Set/Get the byte swapping to explicitely swap the bytes of a file.
  vtkSetMacro(SwapBytes,int);
  vtkGetMacro(SwapBytes,int);
  vtkBooleanMacro(SwapBytes,int);

  // Description:
  // These allow the the min and the max of the extent to be switched.
  // Because the aspect ratio cannot be negative, a new output 
  // origin is computed.
  void SetFlips(int num, int *flip);
  vtkImageSetMacro(Flips, int);
  void GetFlips(int num, int *flip);
  vtkImageGetMacro(Flips, int);
  
  // following should only be used by methods or template helpers, not users
  int DataScalarType;
  ifstream *File;
  int FileSize;
  int HeaderSize;
  // For seeking to the correct location in the files.
  // (should really be called DataIncrements instead of FileIncrements ...)
  int FileIncrements[VTK_IMAGE_DIMENSIONS];
  int FileExtent[VTK_IMAGE_EXTENT_DIMENSIONS];
  unsigned short DataMask;  // Mask each pixel with ...
  int SwapBytes;
  // Flips are flags for each axis specifying whether "reflect" the data.
  int Flips[VTK_IMAGE_DIMENSIONS];
  
protected:
  int ManualHeaderSize;
  int Initialized;
  char *FileName;

  int DataMemoryOrder[VTK_IMAGE_DIMENSIONS];
  int DataDimensions[VTK_IMAGE_DIMENSIONS];
  float DataSpacing[VTK_IMAGE_DIMENSIONS];
  float DataOrigin[VTK_IMAGE_DIMENSIONS];
  int DataExtent[VTK_IMAGE_EXTENT_DIMENSIONS];
  
  virtual void Initialize();
  virtual void UpdatePointData(vtkImageRegion *outRegion) = 0;    
  void UpdateFromFile(vtkImageRegion *region);
};

#endif


