/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVolume16Reader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkVolume16Reader
 * @brief   read 16 bit image files
 *
 * vtkVolume16Reader is a source object that reads 16 bit image files.
 *
 * Volume16Reader creates structured point datasets. The dimension of the
 * dataset depends upon the number of files read. Reading a single file
 * results in a 2D image, while reading more than one file results in a
 * 3D volume.
 *
 * File names are created using FilePattern and FilePrefix as follows:
 * sprintf (filename, FilePattern, FilePrefix, number);
 * where number is in the range ImageRange[0] to ImageRange[1]. If
 * ImageRange[1] <= ImageRange[0], then slice number ImageRange[0] is
 * read. Thus to read an image set ImageRange[0] = ImageRange[1] = slice
 * number. The default behavior is to read a single file (i.e., image slice 1).
 *
 * The DataMask instance variable is used to read data files with imbedded
 * connectivity or segmentation information. For example, some data has
 * the high order bit set to indicate connected surface. The DataMask allows
 * you to select this data. Other important ivars include HeaderSize, which
 * allows you to skip over initial info, and SwapBytes, which turns on/off
 * byte swapping.
 *
 * The Transform instance variable specifies a permutation transformation
 * to map slice space into world space. vtkImageReader has replaced the
 * functionality of this class and should be used instead.
 *
 * @sa
 * vtkSliceCubes vtkMarchingCubes vtkImageReader
*/

#ifndef vtkVolume16Reader_h
#define vtkVolume16Reader_h

#include "vtkIOImageModule.h" // For export macro
#include "vtkVolumeReader.h"

class vtkTransform;
class vtkUnsignedCharArray;
class vtkUnsignedShortArray;

#define VTK_FILE_BYTE_ORDER_BIG_ENDIAN 0
#define VTK_FILE_BYTE_ORDER_LITTLE_ENDIAN 1

class VTKIOIMAGE_EXPORT vtkVolume16Reader : public vtkVolumeReader
{
public:
  vtkTypeMacro(vtkVolume16Reader,vtkVolumeReader);
  void PrintSelf(ostream& os, vtkIndent indent);

  /**
   * Construct object with NULL file prefix; file pattern "%s.%d"; image range
   * set to (1,1); data origin (0,0,0); data spacing (1,1,1); no data mask;
   * header size 0; and byte swapping turned off.
   */
  static vtkVolume16Reader *New();

  //@{
  /**
   * Specify the dimensions for the data.
   */
  vtkSetVector2Macro(DataDimensions,int);
  vtkGetVectorMacro(DataDimensions,int,2);
  //@}

  //@{
  /**
   * Specify a mask used to eliminate data in the data file (e.g.,
   * connectivity bits).
   */
  vtkSetMacro(DataMask,unsigned short);
  vtkGetMacro(DataMask,unsigned short);
  //@}

  //@{
  /**
   * Specify the number of bytes to seek over at start of image.
   */
  vtkSetMacro(HeaderSize,int);
  vtkGetMacro(HeaderSize,int);
  //@}

  //@{
  /**
   * These methods should be used instead of the SwapBytes methods.
   * They indicate the byte ordering of the file you are trying
   * to read in. These methods will then either swap or not swap
   * the bytes depending on the byte ordering of the machine it is
   * being run on. For example, reading in a BigEndian file on a
   * BigEndian machine will result in no swapping. Trying to read
   * the same file on a LittleEndian machine will result in swapping.
   * As a quick note most UNIX machines are BigEndian while PC's
   * and VAX tend to be LittleEndian. So if the file you are reading
   * in was generated on a VAX or PC, SetDataByteOrderToLittleEndian otherwise
   * SetDataByteOrderToBigEndian.
   */
  void SetDataByteOrderToBigEndian();
  void SetDataByteOrderToLittleEndian();
  int GetDataByteOrder();
  void SetDataByteOrder(int);
  const char *GetDataByteOrderAsString();
  //@}

  //@{
  /**
   * Turn on/off byte swapping.
   */
  vtkSetMacro(SwapBytes,int);
  vtkGetMacro(SwapBytes,int);
  vtkBooleanMacro(SwapBytes,int);
  //@}

  //@{
  /**
   * Set/Get transformation matrix to transform the data from slice space
   * into world space. This matrix must be a permutation matrix. To qualify,
   * the sums of the rows must be + or - 1.
   */
  virtual void SetTransform(vtkTransform*);
  vtkGetObjectMacro(Transform,vtkTransform);
  //@}

  /**
   * Other objects make use of these methods
   */
  vtkImageData *GetImage(int ImageNumber);

protected:
  vtkVolume16Reader();
  ~vtkVolume16Reader();

  virtual int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  virtual int RequestInformation(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  int   DataDimensions[2];
  unsigned short DataMask;
  int   SwapBytes;
  int   HeaderSize;
  vtkTransform *Transform;

  void TransformSlice (unsigned short *slice, unsigned short *pixels, int k, int dimensions[3], int bounds[3]);
  void ComputeTransformedDimensions(int dimensions[3]);
  void ComputeTransformedBounds(int bounds[6]);
  void ComputeTransformedSpacing(double Spacing[3]);
  void ComputeTransformedOrigin(double origin[3]);
  void AdjustSpacingAndOrigin(int dimensions[3], double Spacing[3], double origin[3]);
  void ReadImage(int ImageNumber, vtkUnsignedShortArray *);
  void ReadVolume(int FirstImage, int LastImage, vtkUnsignedShortArray *);
  int Read16BitImage(FILE *fp, unsigned short *pixels, int xsize, int ysize,
                     int skip, int swapBytes);

private:
  vtkVolume16Reader(const vtkVolume16Reader&) VTK_DELETE_FUNCTION;
  void operator=(const vtkVolume16Reader&) VTK_DELETE_FUNCTION;
};

#endif


