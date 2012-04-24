/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkFixedPointVolumeRayCastCompositeGOHelper.h
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

// .NAME vtkFixedPointVolumeRayCastCompositeGOHelper - A helper that generates composite images for the volume ray cast mapper
// .SECTION Description
// This is one of the helper classes for the vtkFixedPointVolumeRayCastMapper.
// It will generate composite images using an alpha blending operation.
// This class should not be used directly, it is a helper class for
// the mapper and has no user-level API.
//
// .SECTION see also
// vtkFixedPointVolumeRayCastMapper

#ifndef __vtkFixedPointVolumeRayCastCompositeGOHelper_h
#define __vtkFixedPointVolumeRayCastCompositeGOHelper_h

#include "vtkRenderingVolumeModule.h" // For export macro
#include "vtkFixedPointVolumeRayCastHelper.h"

class vtkFixedPointVolumeRayCastMapper;
class vtkVolume;

class VTKRENDERINGVOLUME_EXPORT vtkFixedPointVolumeRayCastCompositeGOHelper : public vtkFixedPointVolumeRayCastHelper
{
public:
  static vtkFixedPointVolumeRayCastCompositeGOHelper *New();
  vtkTypeMacro(vtkFixedPointVolumeRayCastCompositeGOHelper,vtkFixedPointVolumeRayCastHelper);
  void PrintSelf( ostream& os, vtkIndent indent );

  virtual void  GenerateImage( int threadID,
                               int threadCount,
                               vtkVolume *vol,
                               vtkFixedPointVolumeRayCastMapper *mapper);

protected:
  vtkFixedPointVolumeRayCastCompositeGOHelper();
  ~vtkFixedPointVolumeRayCastCompositeGOHelper();

private:
  vtkFixedPointVolumeRayCastCompositeGOHelper(const vtkFixedPointVolumeRayCastCompositeGOHelper&);  // Not implemented.
  void operator=(const vtkFixedPointVolumeRayCastCompositeGOHelper&);  // Not implemented.
};

#endif


