/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLVolumeProVG500Mapper.h
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
// .NAME vtkOpenGLVolumeProVG500Mapper - Concrete class for VolumePRO mapper
//
// .SECTION Description
// vtkOpenGLVolumeProVG500Mapper is the concrete implementation of a 
// vtkVolumeProMapper based on the VG500 chip running with OpenGL. 
// Users should not create this class directly - a vtkVolumeProMapper will 
// automatically create the object of the right type.
//
// This class is not included in the Rendering CMakeLists by default. If you
// want to add this class to your vtk build, you need to have the vli header
// and library files.  Please see the vtkVolumeProVG500Mapper.h file for
// instructions on how to use the vli library with vtk.
//
// For more information on the VolumePRO hardware, please see:
//
//   http://www.terarecon.com/3d_products.shtml
//
// If you encounter any problems with this class, please inform Kitware, Inc.
// at kitware@kitware.com.
//
//
// .SECTION See Also
// vtkVolumeMapper vtkVolumeProMapper vtkVolumeProVG500Mapper
//

#ifndef __vtkOpenGLVolumeProVG500Mapper_h
#define __vtkOpenGLVolumeProVG500Mapper_h

#include "vtkVolumeProVG500Mapper.h"

class VTK_VOLUMEPRO_EXPORT vtkOpenGLVolumeProVG500Mapper : public vtkVolumeProVG500Mapper
{
public:
  vtkTypeRevisionMacro(vtkOpenGLVolumeProVG500Mapper,vtkVolumeProVG500Mapper);
  static vtkOpenGLVolumeProVG500Mapper *New();

protected:
  vtkOpenGLVolumeProVG500Mapper() {};
  ~vtkOpenGLVolumeProVG500Mapper() {};

  // Render the hexagon returned by the hardware to the screen.
  void RenderHexagon( vtkRenderer  *ren, 
                      vtkVolume    *vol,
                      VLIPixel     *basePlane,
                      int          size[2],
                      VLIVector3D  hexagon[6], 
                      VLIVector2D  textureCoords[6] );
private:
  vtkOpenGLVolumeProVG500Mapper(const vtkOpenGLVolumeProVG500Mapper&);  // Not implemented.
  void operator=(const vtkOpenGLVolumeProVG500Mapper&);  // Not implemented.
};


#endif



