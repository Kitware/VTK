/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageReader.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
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

#include "vtkImageReader2.h"

class vtkTransform;

#define VTK_FILE_BYTE_ORDER_BIG_ENDIAN 0
#define VTK_FILE_BYTE_ORDER_LITTLE_ENDIAN 1

class VTK_IO_EXPORT vtkImageReader : public vtkImageReader2
{
public:
  static vtkImageReader *New();
  vtkTypeRevisionMacro(vtkImageReader,vtkImageSource);
  void PrintSelf(ostream& os, vtkIndent indent);   

  // Description:
  // Set/get the data VOI. You can limit the reader to only
  // read a subset of the data. 
  vtkSetVector6Macro(DataVOI,int);
  vtkGetVector6Macro(DataVOI,int);
  
  // Description:
  // Set/Get the Data mask.
  vtkGetMacro(DataMask,unsigned short);
  void SetDataMask(int val) 
       {if (val == this->DataMask) { return; }
        this->DataMask = ((unsigned short)(val)); this->Modified();}
  
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
  void ComputeInverseTransformedIncrements(int inIncr[3],
                                           int outIncr[3]);

  int OpenAndSeekFile(int extent[6], int slice);
protected:
  vtkImageReader();
  ~vtkImageReader();

  unsigned short DataMask;  // Mask each pixel with ...

  vtkTransform *Transform;

  void ComputeTransformedSpacing (float Spacing[3]);
  void ComputeTransformedOrigin (float origin[3]);
  void ComputeTransformedExtent(int inExtent[6],
                                int outExtent[6]);
  void ComputeTransformedIncrements(int inIncr[3],
                                    int outIncr[3]);

  int DataVOI[6];
  
  void ExecuteInformation();
  void ExecuteData(vtkDataObject *data);
private:
  vtkImageReader(const vtkImageReader&);  // Not implemented.
  void operator=(const vtkImageReader&);  // Not implemented.
};

#endif
