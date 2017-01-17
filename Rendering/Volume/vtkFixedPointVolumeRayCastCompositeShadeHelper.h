/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkFixedPointVolumeRayCastCompositeShadeHelper.h
  Language:  C++

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   vtkFixedPointVolumeRayCastCompositeShadeHelper
 * @brief   A helper that generates composite images for the volume ray cast mapper
 *
 * This is one of the helper classes for the vtkFixedPointVolumeRayCastMapper.
 * It will generate composite images using an alpha blending operation.
 * This class should not be used directly, it is a helper class for
 * the mapper and has no user-level API.
 *
 * @sa
 * vtkFixedPointVolumeRayCastMapper
*/

#ifndef vtkFixedPointVolumeRayCastCompositeShadeHelper_h
#define vtkFixedPointVolumeRayCastCompositeShadeHelper_h

#include "vtkRenderingVolumeModule.h" // For export macro
#include "vtkFixedPointVolumeRayCastHelper.h"

class vtkFixedPointVolumeRayCastMapper;
class vtkVolume;

class VTKRENDERINGVOLUME_EXPORT vtkFixedPointVolumeRayCastCompositeShadeHelper : public vtkFixedPointVolumeRayCastHelper
{
public:
  static vtkFixedPointVolumeRayCastCompositeShadeHelper *New();
  vtkTypeMacro(vtkFixedPointVolumeRayCastCompositeShadeHelper,vtkFixedPointVolumeRayCastHelper);
  void PrintSelf( ostream& os, vtkIndent indent ) VTK_OVERRIDE;

  void  GenerateImage( int threadID,
                               int threadCount,
                               vtkVolume *vol,
                               vtkFixedPointVolumeRayCastMapper *mapper) VTK_OVERRIDE;

protected:
  vtkFixedPointVolumeRayCastCompositeShadeHelper();
  ~vtkFixedPointVolumeRayCastCompositeShadeHelper() VTK_OVERRIDE;

private:
  vtkFixedPointVolumeRayCastCompositeShadeHelper(const vtkFixedPointVolumeRayCastCompositeShadeHelper&) VTK_DELETE_FUNCTION;
  void operator=(const vtkFixedPointVolumeRayCastCompositeShadeHelper&) VTK_DELETE_FUNCTION;
};

#endif


