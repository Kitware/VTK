/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCompressCompositer.h
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
// .NAME vtkCompressCompositer - Implements compressed tree based compositing.
// .SECTION Description
// vtkCompressCompositer operates in multiple processes.  Each compositer has 
// a render window.  They use vtkMultiProcessControllers to comunicate 
// the color and depth buffer to process 0's render window.
// It will not handle transparency.  Compositing is run length encoding
// of back ground pixels.
// vtkCompositeManager.

#ifndef __vtkCompressCompositer_h
#define __vtkCompressCompositer_h

#include "vtkCompositer.h"
#include "vtkDataArray.h"
#include "vtkFloatArray.h"

class vtkTimerLog;

class VTK_PARALLEL_EXPORT vtkCompressCompositer : public vtkCompositer
{
public:
  static vtkCompressCompositer *New();
  vtkTypeRevisionMacro(vtkCompressCompositer,vtkCompositer);
  void PrintSelf(ostream& os, vtkIndent indent);

  virtual void CompositeBuffer(vtkDataArray *pBuf, vtkFloatArray *zBuf,
                               vtkDataArray *pTmp, vtkFloatArray *zTmp);

protected:
  vtkCompressCompositer();
  ~vtkCompressCompositer();
  vtkCompressCompositer(const vtkCompressCompositer&);
  void operator=(const vtkCompressCompositer&);
  
  void Compress(vtkFloatArray *zIn, vtkDataArray *pIn,
                vtkFloatArray *zOut, vtkDataArray *pOut);

  void Uncompress(vtkFloatArray *zIn, vtkDataArray *pIn,
                  vtkDataArray *pOut, int finalLength);

  void CompositeImagePair(vtkFloatArray *localZ, vtkDataArray *localP,
                          vtkFloatArray *remoteZ, vtkDataArray *remoteP,
                          vtkFloatArray *outZ, vtkDataArray *outP); 

  vtkDataArray *InternalPData;
  vtkFloatArray *InternalZData;

  vtkTimerLog *Timer;

};

#endif
