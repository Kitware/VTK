/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTreeComposite.h
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

// This software and ancillary information known as vtk_ext (and
// herein called "SOFTWARE") is made available under the terms
// described below.  The SOFTWARE has been approved for release with
// associated LA_CC Number 99-44, granted by Los Alamos National
// Laboratory in July 1999.
//
// Unless otherwise indicated, this SOFTWARE has been authored by an
// employee or employees of the University of California, operator of
// the Los Alamos National Laboratory under Contract No. W-7405-ENG-36
// with the United States Department of Energy.
//
// The United States Government has rights to use, reproduce, and
// distribute this SOFTWARE.  The public may copy, distribute, prepare
// derivative works and publicly display this SOFTWARE without charge,
// provided that this Notice and any statement of authorship are
// reproduced on all copies.
//
// Neither the U. S. Government, the University of California, nor the
// Advanced Computing Laboratory makes any warranty, either express or
// implied, nor assumes any liability or responsibility for the use of
// this SOFTWARE.
//
// If SOFTWARE is modified to produce derivative works, such modified
// SOFTWARE should be clearly marked, so as not to confuse it with the
// version available from Los Alamos National Laboratory.


// .NAME vtkTreeComposite - Implements tree based compositing.
// .SECTION Description
// vtkTreeComposite operates in multiple processes.  Each compositer has 
// a render window.  They use vtkMultiProcessControllers to comunicate 
// the color and depth buffer to process 0's render window.
// It will not handle transparency well.
// vtkCompositeManager.

#ifndef __vtkTreeComposite_h
#define __vtkTreeComposite_h

#include "vtkCompositer.h"


class VTK_PARALLEL_EXPORT vtkTreeComposite : public vtkCompositer
{
public:
  static vtkTreeComposite *New();
  vtkTypeRevisionMacro(vtkTreeComposite,vtkCompositer);
  void PrintSelf(ostream& os, vtkIndent indent);

  virtual void CompositeBuffer(vtkDataArray *pBuf, vtkFloatArray *zBuf,
                               vtkDataArray *pTmp, vtkFloatArray *zTmp);

protected:
  vtkTreeComposite();
  ~vtkTreeComposite();
  vtkTreeComposite(const vtkTreeComposite&);
  void operator=(const vtkTreeComposite&);
  
};

#endif
