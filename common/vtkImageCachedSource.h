/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageCachedSource.h
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
// .NAME vtkImageCachedSource - Source of data for pipeline.
// .SECTION Description
// vtkImageCachedSource is the supperclass for all sources and filters.
// The method UpdateRegion, called by the cache, is the major interface
// to the source.

// .SECTION See Also
// vtkImageCache vtkImageRegion


#ifndef __vtkImageCachedSource_h
#define __vtkImageCachedSource_h

#include "vtkObject.h"
#include "vtkImageData.h"
#include "vtkImageSource.h"
class vtkImageRegion;
class vtkImageCache;


class VTK_EXPORT vtkImageCachedSource : public vtkObject
{
public:
  vtkImageCachedSource();
  ~vtkImageCachedSource();
  const char *GetClassName() {return "vtkImageCachedSource";};
  void PrintSelf(ostream& os, vtkIndent indent);

  virtual void InterceptCacheUpdate(vtkImageRegion *region);
  virtual void Update(vtkImageRegion *region); 
  virtual void Update();
  virtual void UpdateImageInformation(vtkImageRegion *region) = 0;
  virtual void UpdateImageInformation();

  virtual unsigned long GetPipelineMTime();
  vtkImageCache *GetOutput();

  virtual void SetCache(vtkImageCache *cache);
  vtkImageCache *GetCache();

  virtual void SetReleaseDataFlag(int value);
  int  GetReleaseDataFlag();
  vtkBooleanMacro(ReleaseDataFlag, int);
  
  void SetOutputScalarTypeToFloat(){this->SetOutputScalarType(VTK_FLOAT);}
  void SetOutputScalarTypeToInt(){this->SetOutputScalarType(VTK_INT);}
  void SetOutputScalarTypeToShort(){this->SetOutputScalarType(VTK_SHORT);}
  void SetOutputScalarTypeToUnsignedShort()
    {this->SetOutputScalarType(VTK_UNSIGNED_SHORT);}
  void SetOutputScalarTypeToUnsignedChar()
    {this->SetOutputScalarType(VTK_UNSIGNED_CHAR);}
  void SetOutputScalarType(int type);
  int  GetOutputScalarType();
  
  // Description:
  // Different methods for setting and getting the axes.  
  // The only function of the Axes instance variable is to specify 
  // a context for the other instance variables: Extent, Increments.
  // It also affects the behavior of GetPointer methods.  Axes vector is
  // used to label the arguements.  When the axes get modified, the region
  // superficially gets transposed, because all the variables and methods
  // are affected.
  // The affect of this variable is local to this region, and does not
  // change the behavior of any filter that operates on this region.
  
  virtual void SetAxes(int dim, int *axes);
  vtkImageSetMacro(Axes,int);
  void GetAxes(int dim, int *axes);
  vtkImageGetMacro(Axes,int);
  int *GetAxes() {return this->Axes;};

  virtual void SetStartMethod(void (*f)(void *), void *arg);
  virtual void SetEndMethod(void (*f)(void *), void *arg);
  virtual void SetStartMethodArgDelete(void (*f)(void *));
  virtual void SetEndMethodArgDelete(void (*f)(void *));
  
protected:
  vtkImageCache *Output;
  // The number of dimensions expected/handled by the execute method
  int ExecuteDimensionality; 
  int Axes[VTK_IMAGE_DIMENSIONS]; 
  int ExecuteScalars;
  int ExecuteVectors;

  void (*StartMethod)(void *);
  void (*StartMethodArgDelete)(void *);
  void *StartMethodArg;
  void (*EndMethod)(void *);
  void (*EndMethodArgDelete)(void *);
  void *EndMethodArg;
  
  virtual void RecursiveLoopUpdate(int dim, vtkImageRegion *region); 
  virtual void Execute(vtkImageRegion *region); 
  virtual void CheckCache();
};


#endif


