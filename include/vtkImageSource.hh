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
// .NAME vtkImageSource - Source of vtkImageRegion in an image pipeline.
// .SECTION Description
// vtkImageSource objects can be used as Input to a consumer in an
// image pipeline.  Right now, the class structure is arranged for maximum
// flexability.  The subclass vtkImageCachedSource primarily used 
// for pipeline objects and is a more structured class.  If for some reason
// the application designer wants to create a uniqued tailored pipeline
// object, it can be created as a subclass vtkImageSource.  The new
// filter/source is interchangable with any other vtkImageCachedSource,
// but must handle its own data management.


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
  // If RequestRegion fails, "SplitFactor" has to be set.
  // If the failure was due to a memory limitation, SplitFactor 
  // suggests that the request should be broken into "SplitFactor" 
  // number of pieces for the request to suceed.  
  // If the failure is not memory related, and splitting the request
  // will not help, split factor should be set to zero.
  vtkGetMacro(SplitFactor,int);
protected:
  int SplitFactor;
};

#endif


