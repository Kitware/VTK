/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageSimpleCache.h
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
// .NAME vtkImageSimpleCache - Caches the last region generated.
// .SECTION Description
// vtkImageSimpleCache saves previously the last generated tile.
// If a subsequent region is contained in the cached data, the
// cached data is returned with no call to the filters Generate method.
// If the new region is not completely contained in the cached data,
// the cache is not used.


#ifndef __vtkImageSimpleCache_h
#define __vtkImageSimpleCache_h

#include "vtkImageCache.hh"
#include "vtkImageRegion.hh"

class vtkImageSimpleCache : public vtkImageCache
{
public:
  vtkImageSimpleCache();
  ~vtkImageSimpleCache();
  char *GetClassName() {return "vtkImageSimpleCache";};
  void ReleaseData();

protected:
  vtkImageData *CachedData;
  vtkTimeStamp GenerateTime;

  void GenerateCachedRegionData(vtkImageRegion *region); 
};

#endif


