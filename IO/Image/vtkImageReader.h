/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkImageReader - Superclass of transformable binary file readers.
// .SECTION Description
// vtkImageReader provides methods needed to read a region from a file.
// It supports both transforms and masks on the input data, but as a result
// is more complicated and slower than its parent class vtkImageReader2.

// .SECTION See Also
// vtkBMPReader vtkPNMReader vtkTIFFReader

#ifndef __vtkImageReader_h
#define __vtkImageReader_h

#include "vtkIOImageModule.h" // For export macro
#include "vtkImageReader2.h"

class vtkTransform;

#define VTK_FILE_BYTE_ORDER_BIG_ENDIAN 0
#define VTK_FILE_BYTE_ORDER_LITTLE_ENDIAN 1

class VTKIOIMAGE_EXPORT vtkImageReader : public vtkImageReader2
{
public:
  static vtkImageReader *New();
  vtkTypeMacro(vtkImageReader,vtkImageReader2);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set/get the data VOI. You can limit the reader to only
  // read a subset of the data.
  vtkSetVector6Macro(DataVOI,int);
  vtkGetVector6Macro(DataVOI,int);

  // Description:
  // Set/Get the Data mask.  The data mask is a simply integer whose bits are
  // treated as a mask to the bits read from disk.  That is, the data mask is
  // bitwise-and'ed to the numbers read from disk.  This ivar is stored as 64
  // bits, the largest mask you will need.  The mask will be truncated to the
  // data size required to be read (using the least significant bits).
  vtkGetMacro(DataMask, vtkTypeUInt64);
  vtkSetMacro(DataMask, vtkTypeUInt64);

  // Description:
  // Set/Get transformation matrix to transform the data from slice space
  // into world space. This matrix must be a permutation matrix. To qualify,
  // the sums of the rows must be + or - 1.
  virtual void SetTransform(vtkTransform*);
  vtkGetObjectMacro(Transform,vtkTransform);

  // Warning !!!
  // following should only be used by methods or template helpers, not users
  void ComputeInverseTransformedExtent(int inExtent[6],
                                       int outExtent[6]);
  void ComputeInverseTransformedIncrements(vtkIdType inIncr[3],
                                           vtkIdType outIncr[3]);

  int OpenAndSeekFile(int extent[6], int slice);

  // Description:
  // Set/get the scalar array name for this data set.
  vtkSetStringMacro(ScalarArrayName);
  vtkGetStringMacro(ScalarArrayName);

protected:
  vtkImageReader();
  ~vtkImageReader();

  vtkTypeUInt64 DataMask;

  vtkTransform *Transform;

  void ComputeTransformedSpacing (double Spacing[3]);
  void ComputeTransformedOrigin (double origin[3]);
  void ComputeTransformedExtent(int inExtent[6],
                                int outExtent[6]);
  void ComputeTransformedIncrements(vtkIdType inIncr[3],
                                    vtkIdType outIncr[3]);

  int DataVOI[6];

  char *ScalarArrayName;

  virtual int RequestInformation(vtkInformation* request,
                                 vtkInformationVector** inputVector,
                                 vtkInformationVector* outputVector);

  void ExecuteDataWithInformation(vtkDataObject *data, vtkInformation *outInfo);
private:
  vtkImageReader(const vtkImageReader&);  // Not implemented.
  void operator=(const vtkImageReader&);  // Not implemented.
};

#endif
