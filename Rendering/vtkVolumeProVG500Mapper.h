/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVolumeProVG500Mapper.h
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
// .NAME vtkVolumeProVG500Mapper - Superclass for VG500 board
//
// .SECTION Description
// vtkVolumeProVG500Mapper is the superclass for VolumePRO volume rendering 
// mappers based on the VG500 chip. Subclasses are for underlying graphics 
// languages. Users should not create subclasses directly - 
// a vtkVolumeProMapper will automatically create the object of the right 
// type.
//
// This class is not included in the Rendering CMakeLists by default. If you
// want to add this class to your vtk build, you need to have the vli header
// and library files, and you will need to perform the following steps:
//
// 1. Run cmake, and set the VTK_USE_VOLUMEPRO flag to true.
// 2. If the libary file (VLI_LIBRARY_FOR_VG500) is not found by cmake, set
//    the path to that file, and rerun cmake.
// 3. If the header file (VLI_INCLUDE_PATH_FOR_VG500) is not found by cmake,
//    set the path to that file, and rerun cmake.
// 4. Rebuild VTK.
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
// vtkVolumeMapper vtkVolumeProMapper vtkOpenGLVolumeProVG500Mapper
//

#ifndef __vtkVolumeProVG500Mapper_h
#define __vtkVolumeProVG500Mapper_h

#include "vtkVolumeProMapper.h"

#ifdef _WIN32
#include "VolumePro/inc/vli.h"
#else
#include "vli/include/vli.h"
#endif

#ifdef VTK_USE_VOLUMEPRO
#define VTK_VOLUMEPRO_EXPORT VTK_RENDERING_EXPORT
#else
#define VTK_VOLUMEPRO_EXPORT 
#endif

class VTK_VOLUMEPRO_EXPORT vtkVolumeProVG500Mapper : public vtkVolumeProMapper
{
public:
  vtkTypeRevisionMacro(vtkVolumeProVG500Mapper,vtkVolumeProMapper);
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
  
private:
  vtkVolumeProVG500Mapper(const vtkVolumeProVG500Mapper&);  // Not implemented.
  void operator=(const vtkVolumeProVG500Mapper&);  // Not implemented.
};



#endif



