/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageSource.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

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
// .NAME vtkImageSource - Source of data for pipeline.
// .SECTION Description
// vtkImageSource objects can be used as Input to a consumer in a pipeline.
// If for some reason you want to make a source that does not have a cache,
// it can be a subclass of vtkImageSource.
// The only requirement for a source is that it have a RequestRegion method.
// It will be compatible in any vtkImage pipline (splitting and all).


#ifndef __vtkImageSource_h
#define __vtkImageSource_h

#include "vtkObject.hh"
#include "vtkImageRegion.hh"


class vtkImageSource : public vtkObject 
{
public:
  char *GetClassName() {return "vtkImageSource";};

  virtual vtkImageRegion *RequestRegion(int Offset[3], int Size[3]); 
  virtual vtkImageSource *GetOutput();
  virtual void GetBoundary(int *offset, int *size);
  virtual unsigned long GetPipelineMTime();

  // Description:
  // After a failing request, "SplitFactor" suggests that the request should
  // be broken into "SplitFactor" number of pieces for the request to suceed.
  vtkGetMacro(SplitFactor,int);
protected:
  int SplitFactor;
};

#endif


