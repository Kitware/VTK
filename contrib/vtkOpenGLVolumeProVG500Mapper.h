/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLVolumeProVG500Mapper.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1998 Ken Martin, Will Schroeder, Bill Lorensen.

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
// If you encouter any problems with this class, please inform Kitware, Inc.
// at kitware@kitware.com.
//
//
// .SECTION See Also
// vtkVolumeMapper vtkVolumeProMapper vtkVolumeProVG500Mapper
//

#ifndef __vtkOpenGLVolumeProVG500Mapper_h
#define __vtkOpenGLVolumeProVG500Mapper_h

#include "vtkVolumeProVG500Mapper.h"

class VTK_EXPORT vtkOpenGLVolumeProVG500Mapper : public vtkVolumeProVG500Mapper
{
public:
  const char *GetClassName() {return "vtkOpenGLVolumeProVG500Mapper";};
  static vtkOpenGLVolumeProVG500Mapper *New();

protected:
  vtkOpenGLVolumeProVG500Mapper() {};
  ~vtkOpenGLVolumeProVG500Mapper() {};
  vtkOpenGLVolumeProVG500Mapper(const vtkOpenGLVolumeProVG500Mapper&) {};
  void operator=(const vtkOpenGLVolumeProVG500Mapper&) {};

  // Render the hexagon returned by the hardware to the screen.
  void RenderHexagon( vtkRenderer  *ren, 
		      vtkVolume    *vol,
		      VLIPixel     *basePlane,
		      int          size[2],
		      VLIVector3D  hexagon[6], 
		      VLIVector2D  textureCoords[6] );
};


#endif



