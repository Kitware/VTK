/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLVolumeProVG500Mapper.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
// .NAME vtkOpenGLVolumeProVG500Mapper - Concrete class for VolumePRO mapper
//
// .SECTION Description
// vtkOpenGLVolumeProVG500Mapper is the concrete implementation of a 
// vtkVolumeProMapper based on the VG500 chip running with OpenGL. 
// Users should not create this class directly - a vtkVolumeProMapper will 
// automatically create the object of the right type.
//
// This class is not included in the contrib Makefile.in by default. If you
// want to add this class to your vtk build, you need to have the vli header
// and library files.  Please see the vtkVolumeProVG500Mapper.h file for
// instructions on how to use the vli library with vtk.
//
// For more information on the VolumePRO hardware, please see:
//
//   http://www.3dvolumegraphics.com/3dvolumegraphics/product/index.htm
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

class VTK_RENDERING_EXPORT vtkOpenGLVolumeProVG500Mapper : public vtkVolumeProVG500Mapper
{
public:
  vtkTypeMacro(vtkOpenGLVolumeProVG500Mapper,vtkVolumeProVG500Mapper);
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



