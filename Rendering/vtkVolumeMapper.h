/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVolumeMapper.h
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
// .NAME vtkVolumeMapper - Abstract class for a volume mapper

// .SECTION Description
// vtkVolumeMapper is the abstract definition of a volume mapper.  Several
// basic types of volume mappers are supported. There are ray casters, which
// produce an image that must be merged with geometry, there are hardware
// methods that blend with geometry, and some combinations of these.

// .SECTION see also
// vtkVolumeRayCastMapper

#ifndef __vtkVolumeMapper_h
#define __vtkVolumeMapper_h

#include "vtkAbstractMapper3D.h"
#include "vtkImageData.h"

class vtkRenderer;
class vtkVolume;

#define VTK_CROP_SUBVOLUME              0x0002000
#define VTK_CROP_FENCE                  0x2ebfeba
#define VTK_CROP_INVERTED_FENCE         0x5140145
#define VTK_CROP_CROSS                  0x0417410
#define VTK_CROP_INVERTED_CROSS         0x7be8bef

class vtkWindow;

class VTK_RENDERING_EXPORT vtkVolumeMapper : public vtkAbstractMapper3D
{
public:
  vtkTypeMacro(vtkVolumeMapper,vtkAbstractMapper3D);
  void PrintSelf( ostream& os, vtkIndent index );

  // Description:
  // Update the volume rendering pipeline by updating the scalar input
  virtual void Update();

  // Description:
  // Set/Get the input data
  void SetInput( vtkImageData * );
  vtkImageData *GetInput();

  // Description:
  // Turn On/Off orthogonal cropping. (Clipping planes are
  // perpendicular to the coordinate axes.)
  vtkSetMacro(Cropping,int);
  vtkGetMacro(Cropping,int);
  vtkBooleanMacro(Cropping,int);

  // Description:
  // Set/Get the Cropping Region Planes ( xmin, xmax, ymin, ymax, zmin, zmax )
  vtkSetVector6Macro( CroppingRegionPlanes, float );
  vtkGetVectorMacro(  CroppingRegionPlanes, float, 6 );

  // Description:
  // Set the flags for the cropping regions. The clipping planes divide the
  // volume into 27 regions - there is one bit for each region. The regions 
  // start from the one containing voxel (0,0,0), moving along the x axis 
  // fastest, the y axis next, and the z axis slowest. These are represented 
  // from the lowest bit to bit number 27 in the integer containing the 
  // flags. There are several convenience functions to set some common 
  // configurations - subvolume (the default), fence (between any of the 
  // clip plane pairs), inverted fence, cross (between any two of the 
  // clip plane pairs) and inverted cross.
  vtkSetClampMacro( CroppingRegionFlags, int, 0x0, 0x7ffffff );
  vtkGetMacro( CroppingRegionFlags, int );
  void SetCroppingRegionFlagsToSubVolume() 
    {this->SetCroppingRegionFlags( VTK_CROP_SUBVOLUME );};
  void SetCroppingRegionFlagsToFence() 
    {this->SetCroppingRegionFlags( VTK_CROP_FENCE );};
  void SetCroppingRegionFlagsToInvertedFence() 
    {this->SetCroppingRegionFlags( VTK_CROP_INVERTED_FENCE );};
  void SetCroppingRegionFlagsToCross() 
    {this->SetCroppingRegionFlags( VTK_CROP_CROSS );};
  void SetCroppingRegionFlagsToInvertedCross() 
    {this->SetCroppingRegionFlags( VTK_CROP_INVERTED_CROSS );};

  // Description:
  // Set/Get the rgb texture input data
  void SetRGBTextureInput( vtkImageData *rgbTexture );
  virtual vtkImageData *GetRGBTextureInput();

  // Description:
  // Return bounding box (array of six floats) of data expressed as
  // (xmin,xmax, ymin,ymax, zmin,zmax).
  virtual float *GetBounds();
  virtual void GetBounds(float bounds[6])
    { this->vtkAbstractMapper3D::GetBounds(bounds); };


//BTX
  // Description:
  // WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
  virtual float GetGradientMagnitudeScale() {return 1.0;};
  virtual float GetGradientMagnitudeBias()  {return 0.0;};
  

  // Description:
  // WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
  // DO NOT USE THIS METHOD OUTSIDE OF THE RENDERING PROCESS
  // Render the volume
  virtual void Render(vtkRenderer *ren, vtkVolume *vol)=0;

  // Description:
  // WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
  // Release any graphics resources that are being consumed by this mapper.
  // The parameter window could be used to determine which graphic
  // resources to release.
  virtual void ReleaseGraphicsResources(vtkWindow *) {};
//ETX


protected:
  vtkVolumeMapper();
  ~vtkVolumeMapper();
  vtkVolumeMapper(const vtkVolumeMapper&);
  void operator=(const vtkVolumeMapper&);

  int                  Cropping;
  float                CroppingRegionPlanes[6];
  int                  CroppingRegionFlags;
  vtkTimeStamp         BuildTime;
};


#endif


