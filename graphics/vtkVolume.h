/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVolume.h
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
#include "vtkVolumeRayCastStructures.h"

class vtkRenderer;
class vtkPropCollection;
class vtkVolumeCollection;
class vtkWindow;

class VTK_EXPORT vtkVolume : public vtkProp3D
{
public:
  const char *GetClassName() {return "vtkVolume";};
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
  // Make a shallow copy of this volume.
  void ShallowCopy(vtkVolume *volume);

  // Description:
  // For legacy compatibility. Do not use.
  // This method will disappear after vtk 3.0.
  void SetVolumeProperty(vtkVolumeProperty *property) 
    {vtkErrorMacro(<<"Obsolete method. Change SetVolumeProperty to SetProperty");this->SetProperty (property);};
  vtkVolumeProperty *GetVolumeProperty() 
    {vtkErrorMacro(<<"Obsolete method. Change GetVolumeProperty to GetProperty "); return this->GetProperty();};

  // Description:
  // For legacy compatibility. Do not use.
  // This method will disappear after vtk 3.0.
  vtkVolumeMapper *GetVolumeMapper()
    {vtkErrorMacro(<<"Obsolete method. Change GetVolumeMapper to GetMapper."); return this->GetMapper();};
  void SetVolumeMapper(vtkVolumeMapper *mapper) 
    {vtkErrorMacro(<<"Obsolete method. Change SetVolumeMapper to SetMapper.");this->SetMapper (mapper);};

  // Description:
  // For legacy compatibility. Do not use.
  // This method will disappear after vtk 3.0.
  void SetVolumeProperty(vtkVolumeProperty& property) 
    {this->SetProperty(&property);}

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
  // DO NOT USE THIS METHOD OUTSIDE OF THE RENDERING PROCESS
  // Support the standard render methods.
  // Depending on the mapper type, the volume may be rendered using
  // this method (SOFTWAREBUFFER volumes such as multiray will be
  // rendered this way)
  int RenderIntoImage(vtkViewport *viewport);

  // Description:
  // WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
  // DO NOT USE THIS METHOD OUTSIDE OF THE RENDERING PROCESS
  // Support the standard render methods. These are the methods
  // to access the image generated by the RenderIntoImage() 
  // method.
  float *GetRGBAImage();
  float *GetZImage();

  // Description:
  // WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
  // Do we need to ray cast this prop?
  int RequiresRayCasting();

  // Description:
  // WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
  // Does this prop render into an image?
  int RequiresRenderingIntoImage();

  // Description:
  // WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
  // DO NOT USE THIS METHOD OUTSIDE OF THE RENDERING PROCESS
  // Support for ray casting if the mapper requires it - 
  // do the initialization and save up all the info required into
  // a structure that will later be passed into the mapper when
  // each ray is cast.
  virtual int InitializeRayCasting( vtkViewport *viewport);

  // Description:
  // WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
  // DO NOT USE THIS METHOD OUTSIDE OF THE RENDERING PROCESS
  // Support for texture mapping if the mapper requires it -
  // currently this only initializes transfer functions
  virtual void InitializeTextureMapping( vtkViewport *viewport, 
		float sampleDistance );

  // Description:
  // WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
  // DO NOT USE THIS METHOD OUTSIDE OF THE RENDERING PROCESS
  // Support for ray casting if the mapper requires it - 
  // cast a ray that is defined in viewing coordinates 
  virtual int CastViewRay( VTKRayCastRayInfo *rayInfo );
 
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
  vtkVolume(const vtkVolume&) {};
  void operator=(const vtkVolume&) {};

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
  // modelled by a step size different than 1.
  float                        *CorrectedScalarOpacityArray;

  // CorrectedStepSize is the step size corrently modelled by
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

  VTKRayCastVolumeInfo *VolumeInfo;
};

#endif

