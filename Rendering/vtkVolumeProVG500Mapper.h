/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVolumeProVG500Mapper.h
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
// .NAME vtkVolumeProVG500Mapper - Superclass for VG500 board
//
// .SECTION Description
// vtkVolumeProVG500Mapper is the superclass for VolumePRO volume rendering 
// mappers based on the VG500 chip. Subclasses are for underlying graphics 
// languages. Users should not create subclasses directly - 
// a vtkVolumeProMapper will automatically create the object of the right 
// type.
//
// This class is not included in the contrib Makefile.in by default. If you
// want to add this class to your vtk build, you need to have the vli header
// and library files, and you will need to perform the following steps:
//
// 1. Make sure you are building with the contrib kit. On Unix add
//    --with-contrib to your configure line, on Windows check the
//   contrib box on PCMaker.
//
// 2. Edit the Makefile.in in contrib. Add the following three classes to
//    the CONCRETE list of classes:  vtkVolumeProVG500Mapper, 
//    vtkOpenGLVolumeProVG500Mapper. Please be
//    certain that there are no spaces after the "\" that separates lines.
//
// 3. Specify the include path for vli.h to the vtk make process.
//    For Windows, add the option -I "/path/to/vli/" to the Advanced Options 
//    Extra Compiler flags of pcmaker.  For example, the following works here:
//    -I "c:\Program Files/VolumePro/inc" -DVTK_USE_VLI
//    For UNIX, add the path to USER_CXXFLAGS as a -I option to the compiler.
//    Or you can edit vtkVolumeProMapper.h and specify the include path
//    for vli.h, or simply copy the vli.h file to your contrib directory.
//
// 4. Add a -DVTK_USE_VLI to the compile options.  
//    For Windows, add the flag in the Advanced Options Extra Compiler Flags
//    of pcmaker.  
//    For Unix, add the flag to USER_CXXFLAGS in user.make.
//
// 5. On Windows - add the vli.lib file to the Extra Linker Flags under
//    the Advanced Options. For example the following works here:
//    "c:\program files\volumepro\lib\vli.lib"
//    On Unix - add the vli shared object to the
//    KIT_LIBS in the Makefile.in in contrib.
//  
// 6. On Windows - make sure vli.dll is somewhere in your path before you
//    run vtk. You can put it in your vtkbin/lib or vtkbin/Debug/lib if
//    you want. On Unix - make sure the vli shared object is in your
//    shared library path before you run.
//
// 7. Reconfigure and rebuild vtk. You should now be able to create a
//    vtkVolumeProMapper which, if you have a VolumePRO board and the
//    device driver is running, should connect to the hardware and render
//    your volumes quickly.
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
// vtkVolumeMapper vtkVolumeProMapper vtkOpenGLVolumeProVG500Mapper
//

#ifndef __vtkVolumeProVG500Mapper_h
#define __vtkVolumeProVG500Mapper_h

#include "vtkVolumeProMapper.h"
#include "vli.h"

class VTK_EXPORT vtkVolumeProVG500Mapper : public vtkVolumeProMapper
{
public:
  vtkTypeMacro(vtkVolumeProVG500Mapper,vtkVolumeProMapper);
  static vtkVolumeProVG500Mapper *New();
 // Description:
  // Render the image using the hardware and place it in the frame buffer
  virtual void Render( vtkRenderer *, vtkVolume * );
  virtual int GetAvailableBoardMemory();
  virtual void GetLockSizesForBoardMemory( unsigned int type,
					   unsigned int *xSize,
					   unsigned int *ySize,
					   unsigned int *zSize );
protected:
  vtkVolumeProVG500Mapper();
  ~vtkVolumeProVG500Mapper();
  vtkVolumeProVG500Mapper(const vtkVolumeProVG500Mapper&);
  void operator=(const vtkVolumeProVG500Mapper&);
  
  // Update the camera - set the camera matrix
  void UpdateCamera( vtkRenderer *, vtkVolume * );

  // Update the lights
  void UpdateLights( vtkRenderer *, vtkVolume * );

  // Update the properties of the volume including transfer functions
  // and material properties
  void UpdateProperties( vtkRenderer *, vtkVolume * );

  // Update the volume - create it if necessary
  // Set the volume matrix.
  void UpdateVolume( vtkRenderer *, vtkVolume * );

  // Set the crop box (as defined in the vtkVolumeMapper superclass)
  void UpdateCropping( vtkRenderer *, vtkVolume * );

  // Set the cursor
  void UpdateCursor( vtkRenderer *, vtkVolume * );

  // Update the cut plane
  void UpdateCutPlane( vtkRenderer *, vtkVolume * );

  // Render the hexagon to the screen
  // Defined in the specific graphics implementation.
  virtual void RenderHexagon( vtkRenderer  * vtkNotUsed(ren), 
			      vtkVolume    * vtkNotUsed(vol),
			      VLIPixel     * vtkNotUsed(basePlane),
			      int          size[2],
			      VLIVector3D  hexagon[6], 
			      VLIVector2D  textureCoords[6] ) 
    {(void)size; (void)hexagon; (void)textureCoords;}

  // Make the base plane size a power of 2 for OpenGL
  void CorrectBasePlaneSize( VLIPixel *inBase, int inSize[2],
			     VLIPixel **outBase, int outSize[2],
			     VLIVector2D textureCoords[6] );

  // Keep track of the size of the data loaded so we know if we can
  // simply update when a change occurs or if we need to release and
  // create again
  int LoadedDataSize[3];
  
};



#endif



