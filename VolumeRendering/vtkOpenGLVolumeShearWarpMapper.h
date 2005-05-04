/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLVolumeShearWarpMapper.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkOpenGLVolumeShearWarpMapper - Class for a Shear Warp Volume Mapper
//
// .SECTION Description
// vtkVolumeShearWarpMapper is a base class for volume mappers using
// the shear-warp factorization algorithm.
//
// .SECTION see also
// vtkVolumeMapper vtkVolumeShearWarpMapper
//
// .SECTION Thanks
// Thanks to Stefan Bruckner for developing and contributing this code
// and to Namkug Kim for some fixing and tidying of the code
//
// .SECTION References
// P. Lacroute. "Fast Volume Rendering Using a Shear-
// Warp Factorization of the Viewing Transformation"
// PhD thesis, Stanford University, 1995.
//
// P. Lacroute and M. Levoy. "Fast volume rendering using
// a shear-warp factorization of the viewing transformation"
// Proceedings of the 21st annual conference
// on Computer graphics and interactive techniques,
// pages 451-458, 1994.
//
// "The InverseWarp: Non-Invasive Integration of Shear-Warp
// Volume Rendering into Polygon Rendering Pipelines"
// Stefan Bruckner, Dieter Schmalstiegy, Helwig Hauserz,
// M. Eduard Groller

#ifndef __vtkOpenGLVolumeShearWarpMapper_h
#define __vtkOpenGLVolumeShearWarpMapper_h

#include "vtkVolumeShearWarpMapper.h"

class VTK_VOLUMERENDERING_EXPORT vtkOpenGLVolumeShearWarpMapper : public vtkVolumeShearWarpMapper
{
public:
//  vtkTypeMacro(vtkOpenGLVolumeShearWarpMapper,vtkVolumeShearWarpMapper);
  void PrintSelf(ostream& os, vtkIndent indent);

  static vtkOpenGLVolumeShearWarpMapper *New();
  vtkTypeRevisionMacro(vtkOpenGLVolumeShearWarpMapper,vtkVolumeShearWarpMapper);

protected:
  vtkOpenGLVolumeShearWarpMapper();
  ~vtkOpenGLVolumeShearWarpMapper();

  virtual void RenderTexture(vtkRenderer *ren, vtkVolume *vol);

private:
  vtkOpenGLVolumeShearWarpMapper(const vtkOpenGLVolumeShearWarpMapper&);  // Not implemented.
  void operator=(const vtkOpenGLVolumeShearWarpMapper&);  // Not implemented.
};


#endif


