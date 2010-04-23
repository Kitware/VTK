/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLVolumeProVP1000Mapper.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkOpenGLVolumeProVP1000Mapper - Concrete class for VolumePRO mapper
//
// .SECTION Description
// vtkOpenGLVolumeProVP1000Mapper is the concrete implementation of a 
// vtkVolumeProMapper based on the VP1000 chip running with OpenGL. 
// Users should not create this class directly - a vtkVolumeProMapper will 
// automatically create the object of the right type.
//
// This class is not included in the Rendering CMakeLists by default. If you
// want to add this class to your vtk build, you need to have the vli header
// and library files.  Please see the vtkVolumeProVP1000Mapper.h file for
// instructions on how to use the vli library with vtk.
//
// For more information on the VolumePRO hardware, please see:
//
//   http://www.terarecon.com/products/volumepro_prod.html
//
// If you encounter any problems with this class, please inform Kitware, Inc.
// at kitware@kitware.com.
//
//
// .SECTION See Also
// vtkVolumeMapper vtkVolumeProMapper vtkVolumeProVP1000Mapper
//

#ifndef __vtkOpenGLVolumeProVP1000Mapper_h
#define __vtkOpenGLVolumeProVP1000Mapper_h

#include "vtkVolumeProVP1000Mapper.h"

class VTK_VOLUMERENDERING_EXPORT vtkOpenGLVolumeProVP1000Mapper : public vtkVolumeProVP1000Mapper
{
public:
  vtkTypeMacro(vtkOpenGLVolumeProVP1000Mapper,vtkVolumeProVP1000Mapper);
  static vtkOpenGLVolumeProVP1000Mapper *New();
  virtual void PrintSelf(ostream& os, vtkIndent indent);

protected:
  vtkOpenGLVolumeProVP1000Mapper() {}
  ~vtkOpenGLVolumeProVP1000Mapper() {}

  // Render the hexagon returned by the hardware to the screen.
  void RenderImageBuffer( vtkRenderer  *ren,
                          vtkVolume    *vol,
                          int          size[2],
                          unsigned int *outData );
  
  // Get the OpenGL depth buffer values in a the form needed for the
  // VolumePro board
  virtual void GetDepthBufferValues( vtkRenderer *ren, int size[2],
                                     unsigned int *outData);
  
  // Render a bounding box of the volume because the texture map would be
  // too large
  virtual void RenderBoundingBox(vtkRenderer *ren, vtkVolume *vol);

private:
  vtkOpenGLVolumeProVP1000Mapper(const vtkOpenGLVolumeProVP1000Mapper&); // Not implemented
  void operator=(const vtkOpenGLVolumeProVP1000Mapper&); // Not implemented
};


#endif



