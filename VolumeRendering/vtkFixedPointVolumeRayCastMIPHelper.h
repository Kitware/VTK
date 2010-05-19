/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkFixedPointVolumeRayCastMIPHelper.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkFixedPointVolumeRayCastMIPHelper - A helper that generates MIP images for the volume ray cast mapper
// .SECTION Description
// This is one of the helper classes for the vtkFixedPointVolumeRayCastMapper.
// It will generate maximum intensity images.
// This class should not be used directly, it is a helper class for
// the mapper and has no user-level API.
//
// .SECTION see also
// vtkFixedPointVolumeRayCastMapper

#ifndef __vtkFixedPointVolumeRayCastMIPHelper_h
#define __vtkFixedPointVolumeRayCastMIPHelper_h

#include "vtkFixedPointVolumeRayCastHelper.h"

class vtkFixedPointVolumeRayCastMapper;
class vtkVolume;

class VTK_VOLUMERENDERING_EXPORT vtkFixedPointVolumeRayCastMIPHelper : public vtkFixedPointVolumeRayCastHelper
{
public:
  static vtkFixedPointVolumeRayCastMIPHelper *New();
  vtkTypeMacro(vtkFixedPointVolumeRayCastMIPHelper,vtkFixedPointVolumeRayCastHelper);
  void PrintSelf( ostream& os, vtkIndent indent );

  virtual void  GenerateImage( int threadID, 
                               int threadCount,
                               vtkVolume *vol,
                               vtkFixedPointVolumeRayCastMapper *mapper);

protected:
  vtkFixedPointVolumeRayCastMIPHelper();
  ~vtkFixedPointVolumeRayCastMIPHelper();

private:
  vtkFixedPointVolumeRayCastMIPHelper(const vtkFixedPointVolumeRayCastMIPHelper&);  // Not implemented.
  void operator=(const vtkFixedPointVolumeRayCastMIPHelper&);  // Not implemented.
};

#endif



