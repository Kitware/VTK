/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVolume.h
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
// .NAME vtkVolume - represents a volume (data & properties) in a rendered scene
//
// .SECTION Description
// vtkVolume is used to represent a volumetric entity in a rendering scene.
// It inherits functions related to the volume's position, orientation and
// origin from vtkProp3D. The volume maintains a reference to the
// volumetric data (i.e., the volume mapper). The volume also contains a
// reference to a volume property which contains all common volume rendering 
// parameters.

// .SECTION see also
// vtkVolumeMapper vtkVolumeProperty vtkProp3D

#ifndef __vtkVolume_h
#define __vtkVolume_h

#include "vtkProp3D.h"
#include "vtkTransform.h"
#include "vtkVolumeProperty.h"
#include "vtkVolumeMapper.h"

class vtkRenderer;
class vtkPropCollection;
class vtkVolumeCollection;
class vtkWindow;

class VTK_RENDERING_EXPORT vtkVolume : public vtkProp3D
{
public:
  vtkTypeMacro(vtkVolume,vtkProp3D);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Creates a Volume with the following defaults: origin(0,0,0) 
  // position=(0,0,0) scale=1 visibility=1 pickable=1 dragable=1
  // orientation=(0,0,0).
  static vtkVolume *New();

  // Description:
  // Set/Get the volume mapper.
  void SetMapper(vtkVolumeMapper *mapper);
  vtkGetObjectMacro(Mapper, vtkVolumeMapper);

  // Description:
  // Set/Get the volume property.
  void SetProperty(vtkVolumeProperty *property);
  vtkVolumeProperty *GetProperty();

  // Description: 
  // For some exporters and other other operations we must be
  // able to collect all the actors or volumes. This method
  // is used in that process.
  void GetVolumes(vtkPropCollection *vc);

  // Description:
  // Update the volume rendering pipeline by updating the volume mapper
  void Update();

  // Description:
  // Get the bounds - either all six at once 
  // (xmin, xmax, ymin, ymax, zmin, zmax) or one at a time.
  float *GetBounds();
  void GetBounds(float bounds[6]) { this->vtkProp3D::GetBounds( bounds ); };
  float GetMinXBound();
  float GetMaxXBound();
  float GetMinYBound();
  float GetMaxYBound();
  float GetMinZBound();
  float GetMaxZBound();

  // Description:
  // Return the MTime also considering the property etc.
  unsigned long int GetMTime();

  // Description:
  // Return the mtime of anything that would cause the rendered image to 
  // appear differently. Usually this involves checking the mtime of the 
  // prop plus anything else it depends on such as properties, mappers,
  // etc.
  unsigned long GetRedrawMTime();

  // Description:
  // Shallow copy of this vtkVolume. Overloads the virtual vtkProp method.
  void ShallowCopy(vtkProp *prop);

//BTX
  // Description:
  // WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
  // DO NOT USE THIS METHOD OUTSIDE OF THE RENDERING PROCESS
  // Support the standard render methods.
  // Depending on the mapper type, the volume may be rendered using
  // this method (FRAMEBUFFER volume such as texture mapping will
  // be rendered this way)
  int RenderTranslucentGeometry(vtkViewport *viewport);

  // Description:
  // WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
  // Release any graphics resources that are being consumed by this volume.
  // The parameter window could be used to determine which graphic
  // resources to release.
  void ReleaseGraphicsResources(vtkWindow *);

  // Description:
  // WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
  // DO NOT USE THIS METHOD OUTSIDE OF THE RENDERING PROCESS
  float *GetCorrectedScalarOpacityArray () { return this->CorrectedScalarOpacityArray; };

  // Description:
  // WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
  // DO NOT USE THIS METHOD OUTSIDE OF THE RENDERING PROCESS
  float *GetScalarOpacityArray () { return this->ScalarOpacityArray; };

  // Description:
  // WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
  // DO NOT USE THIS METHOD OUTSIDE OF THE RENDERING PROCESS
  float *GetGradientOpacityArray () { return this->GradientOpacityArray; };

  // Description:
  // WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
  // DO NOT USE THIS METHOD OUTSIDE OF THE RENDERING PROCESS
  float *GetGrayArray () { return this->GrayArray; };

  // Description:
  // WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
  // DO NOT USE THIS METHOD OUTSIDE OF THE RENDERING PROCESS
  float *GetRGBArray () { return this->RGBArray; };

  // Description:
  // WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
  // DO NOT USE THIS METHOD OUTSIDE OF THE RENDERING PROCESS
  float  GetGradientOpacityConstant () { return this->GradientOpacityConstant; };

  // Description:
  // WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
  // DO NOT USE THIS METHOD OUTSIDE OF THE RENDERING PROCESS
  float  GetArraySize () { return this->ArraySize; };

  // Description:
  // WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
  // DO NOT USE THIS METHOD OUTSIDE OF THE RENDERING PROCESS
  void UpdateTransferFunctions( vtkRenderer *ren );

  // Description:
  // WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
  // DO NOT USE THIS METHOD OUTSIDE OF THE RENDERING PROCESS
  void UpdateScalarOpacityforSampleSize( vtkRenderer *ren, float sample_distance );

//ETX

protected:
  vtkVolume();
  ~vtkVolume();
  vtkVolume(const vtkVolume&);
  void operator=(const vtkVolume&);

  vtkVolumeMapper              *Mapper;
  vtkVolumeProperty            *Property;

  // The rgb transfer function array - for unsigned char data this
  // is 256 elements, for short or unsigned short it is 65536 elements
  // This is a sample at each scalar value of the rgb transfer
  // function.  A time stamp is kept to know when it needs rebuilding
  float                        *RGBArray;
  vtkTimeStamp                 RGBArrayMTime;

  // The gray transfer function array - for unsigned char data this
  // is 256 elements, for short or unsigned short it is 65536 elements
  // This is a sample at each scalar value of the gray transfer
  // function.  A time stamp is kept to know when it needs rebuilding
  float                        *GrayArray;
  vtkTimeStamp                 GrayArrayMTime;

  // The scalar opacity transfer function array - for unsigned char data this
  // is 256 elements, for short or unsigned short it is 65536 elements
  // This is a sample at each scalar value of the opacity transfer
  // function.  A time stamp is kept to know when it needs rebuilding
  float                        *ScalarOpacityArray;
  vtkTimeStamp                 ScalarOpacityArrayMTime;

  // The corrected scalar opacity transfer function array - this is identical
  // to the opacity transfer function array when the step size is 1.
  // In other cases, it is corrected to reflect the new material thickness
  // modeled by a step size different than 1.
  float                        *CorrectedScalarOpacityArray;

  // CorrectedStepSize is the step size currently modeled by
  // CorrectedArray.  It is used to determine when the 
  // CorrectedArray needs to be updated to match SampleDistance
  // in the volume mapper.
  float                        CorrectedStepSize;

  // CorrectedSOArrayMTime - compared with OpacityArrayMTime for update
  vtkTimeStamp                 CorrectedScalarOpacityArrayMTime;

  // Number of elements in the rgb, gray, and opacity transfer function arrays
  int                          ArraySize;

  // The magnitude of gradient opacity transfer function array
  float                        GradientOpacityArray[256];
  float                        GradientOpacityConstant;
  vtkTimeStamp                 GradientOpacityArrayMTime;

  // Function to compute screen coverage of this volume
  float ComputeScreenCoverage( vtkViewport *vp );
};

#endif

