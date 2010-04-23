/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCompressCompositer.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkCompressCompositer - Implements compressed tree based compositing.
//
// .SECTION Description
// vtkCompressCompositer operates in multiple processes.  Each compositer has 
// a render window.  They use vtkMultiProcessController to communicate 
// the color and depth buffer to process 0's render window.
// It will not handle transparency.  Compositing is run length encoding
// of background pixels.
//
// SECTION See Also
// vtkCompositeManager.

#ifndef __vtkCompressCompositer_h
#define __vtkCompressCompositer_h

#include "vtkCompositer.h"

class vtkTimerLog;
class vtkDataArray;
class vtkFloatArray;

class VTK_PARALLEL_EXPORT vtkCompressCompositer : public vtkCompositer
{
public:
  static vtkCompressCompositer *New();
  vtkTypeMacro(vtkCompressCompositer,vtkCompositer);
  void PrintSelf(ostream& os, vtkIndent indent);

  virtual void CompositeBuffer(vtkDataArray *pBuf, vtkFloatArray *zBuf,
                               vtkDataArray *pTmp, vtkFloatArray *zTmp);

  // Description:
  // I am granting access to these methods and making them static
  // So I can create a TileDisplayCompositer which uses compression.
  static void Compress(vtkFloatArray *zIn, vtkDataArray *pIn,
                       vtkFloatArray *zOut, vtkDataArray *pOut);

  static void Uncompress(vtkFloatArray *zIn, vtkDataArray *pIn,
                         vtkFloatArray *zOut, vtkDataArray *pOut,
                         int finalLength);

  static void CompositeImagePair(vtkFloatArray *localZ, vtkDataArray *localP,
                                 vtkFloatArray *remoteZ, vtkDataArray *remoteP,
                                 vtkFloatArray *outZ, vtkDataArray *outP); 
protected:
  vtkCompressCompositer();
  ~vtkCompressCompositer();
  

  vtkDataArray *InternalPData;
  vtkFloatArray *InternalZData;

  vtkTimerLog *Timer;

private:
  vtkCompressCompositer(const vtkCompressCompositer&); // Not implemented
  void operator=(const vtkCompressCompositer&); // Not implemented
};

#endif
