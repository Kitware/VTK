/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageVolume16Reader.h
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
// .NAME vtkImageVolume16Reader - read 16 bit image files
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


#ifndef __vtkImageVolume16Reader_h
#define __vtkImageVolume16Reader_h

#include "vtkImageSeriesReader.h"
#include "vtkTransform.h"

#define VTK_FILE_BYTE_ORDER_BIG_ENDIAN 0
#define VTK_FILE_BYTE_ORDER_LITTLE_ENDIAN 1

class VTK_EXPORT vtkImageVolume16Reader : public vtkImageSeriesReader
{
public:
  vtkImageVolume16Reader();
  ~vtkImageVolume16Reader();
  static vtkImageVolume16Reader *New() {return new vtkImageVolume16Reader;};
  const char *GetClassName() {return "vtkImageVolume16Reader";};
  void PrintSelf(ostream& os, vtkIndent indent);   

  // Description:
  // Set/Get transformation matrix to transform the data from slice space
  // into world space. This matirx must be a permutation matrix. To qualify,
  // the sums of the rows must be + or - 1.
  vtkSetObjectMacro(Transform,vtkTransform);
  vtkGetObjectMacro(Transform,vtkTransform);
  
protected:
  vtkTransform *Transform;
  
  void UpdateImageInformation(vtkImageRegion *region);
  void ComputeTransformedDataAxes(int axes[3]);
  void ComputeTransformedDataFlips(int flips[3]);
};

#endif


