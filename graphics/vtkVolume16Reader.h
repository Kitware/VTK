/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVolume16Reader.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1996 Ken Martin, Will Schroeder, Bill Lorensen.

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
// .NAME vtkVolume16Reader - read 16 bit image files
// .SECTION Description
// vtkVolume16Reader is a source object that reads 16 bit image files.
//
// Volume16Reader creates structured point datasets. The dimension of the 
// dataset depends upon the number of files read. Reading a single file 
// results in a 2D image, while reading more than one file results in a 
// 3D volume.
//
// File names are created using FilePattern and FilePrefix as follows:
// sprintf (filename, FilePattern, FilePrefix, number);
// where number is in the range ImageRange[0] to ImageRange[1]. If
// ImageRange[1] <= ImageRange[0], then slice number ImageRange[0] is
// read. Thus to read an image set ImageRange[0] = ImageRange[1] = slice 
// number. The default behavior is to read a single file (i.e., image slice 1).
//
// The DataMask instance variable is used to read data files with imbedded
// connectivity or segmentation information. For example, some data has
// the high order bit set to indicate connected surface. The DataMask allows
// you to select this data. Other important ivars include HeaderSize, which
// allows you to skip over initial info, and SwapBytes, which turns on/off
// byte swapping.
//
// The Transform instance variable specifies a permutation transformation
// to map slice space into world space.
// .SECTION See Also
// vtkSliceCubes vtkMarchingCubes

#ifndef __vtkVolume16Reader_h
#define __vtkVolume16Reader_h

#include <stdio.h>
#include "vtkVolumeReader.h"
#include "vtkTransform.h"

class VTK_EXPORT vtkVolume16Reader : public vtkVolumeReader
{
public:
  vtkVolume16Reader();
  char *GetClassName() {return "vtkVolume16Reader";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Specify the dimensions for the data.
  vtkSetVector2Macro(DataDimensions,int);
  vtkGetVectorMacro(DataDimensions,int,2);

  // Description:
  // Specify a mask used to eliminate data in the data file (e.g.,
  // connectivity bits).
  vtkSetMacro(DataMask,short);
  vtkGetMacro(DataMask,short);

  // Description:
  // Specify the number of bytes to seek over at start of image.
  vtkSetMacro(HeaderSize,int);
  vtkGetMacro(HeaderSize,int);

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
  // in was generated on a VAX or PC, SetFileTypeLittleEndian otherwise
  // SetFileTypeBigEndian. 
  void SetFileTypeBigEndian();
  void SetFileTypeLittleEndian();

  // Description:
  // Turn on/off byte swapping.
  vtkSetMacro(SwapBytes,int);
  vtkGetMacro(SwapBytes,int);
  vtkBooleanMacro(SwapBytes,int);

  // Description:
  // Set/Get transformation matrix to transform the data from slice space
  // into world space. This matirx must be a permutation matrix. To qualify,
  // the sums of the rows must be + or - 1.
  vtkSetObjectMacro(Transform,vtkTransform);
  vtkGetObjectMacro(Transform,vtkTransform);

  // Other objects make use of these methods
  vtkStructuredPoints *GetImage(int ImageNumber);

protected:
  void Execute();
  int   DataDimensions[2];
  short DataMask;
  int   SwapBytes;
  int   HeaderSize;
  vtkTransform *Transform;

  void TransformSlice (short *slice, short *pixels, int k, int dimensions[3], int bounds[3]);
  void ComputeTransformedDimensions(int dimensions[3]);
  void ComputeTransformedBounds(int bounds[6]);
  void ComputeTransformedAspectRatio(float aspectRatio[3]);
  void ComputeTransformedOrigin(float origin[3]);
  void AdjustAspectRatioAndOrigin(int dimensions[3], float aspectRatio[3], float origin[3]);
  vtkScalars *ReadImage(int ImageNumber);
  vtkScalars *ReadVolume(int FirstImage, int LastImage);
  int Read16BitImage(FILE *fp, short *pixels, int xsize, int ysize, 
		     int skip, int swapBytes);

};

#endif


