/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVolumeRayCastCompositeFunction.h
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
// .NAME vtkVolumeRayCastCompositeFunction - a ray function for compositing

// .SECTION Description
// vtkVolumeRayCastCompositeFunction is a ray function that can be used
// within a vtkVolumeRayCastMapper. This function performs compositing along
// the ray according to the properties stored in the vtkVolumeProperty for
// the volume. 

// .SECTION See Also
// vtkVolumeRayCastMapper vtkVolumeProperty vtkVolume

#ifndef __vtkVolumeRayCastCompositeFunction_h
#define __vtkVolumeRayCastCompositeFunction_h

#include "vtkVolumeRayCastFunction.h"

#define VTK_COMPOSITE_CLASSIFY_FIRST 0
#define VTK_COMPOSITE_INTERPOLATE_FIRST 1

class VTK_EXPORT vtkVolumeRayCastCompositeFunction : public vtkVolumeRayCastFunction
{
public:
  vtkVolumeRayCastCompositeFunction();
  ~vtkVolumeRayCastCompositeFunction();
  static vtkVolumeRayCastCompositeFunction *New() {
    return new vtkVolumeRayCastCompositeFunction;};
  const char *GetClassName() {return "vtkVolumeRayCastCompositeFunction";};
  void PrintSelf( ostream& os, vtkIndent index );

  // Description:
  // Bogus routine right now until I figure out how to get to the
  // volume's properties from here....
  float GetZeroOpacityThreshold( vtkVolume *vol );

  // Description:
  // Set the CompositeMethod to either Classify First or Interpolate First
  vtkSetClampMacro( CompositeMethod, int,
        VTK_COMPOSITE_CLASSIFY_FIRST, VTK_COMPOSITE_INTERPOLATE_FIRST );
  vtkGetMacro(CompositeMethod,int);
  void SetCompositeMethodToInterpolateFirst()
    {this->SetCompositeMethod(VTK_COMPOSITE_INTERPOLATE_FIRST);}
  void SetCompositeMethodToClassifyFirst() 
    {this->SetCompositeMethod(VTK_COMPOSITE_CLASSIFY_FIRST);}
  char *GetCompositeMethodAsString(void);

//BTX
  void CastRay( struct VolumeRayCastRayInfoStruct *rayInfo,
		struct VolumeRayCastVolumeInfoStruct *volumeInfo);
//ETX
protected:
//BTX
  void SpecificFunctionInitialize( vtkRenderer *ren,
				   vtkVolume   *vol,
				   struct VolumeRayCastVolumeInfoStruct *volumeInfo,
				   vtkVolumeRayCastMapper *mapper );
//ETX
  
  int           CompositeMethod;
};

// Description:
// Return the composite method as a descriptive character string.
inline char *vtkVolumeRayCastCompositeFunction::GetCompositeMethodAsString(void)
{
  if( this->CompositeMethod == VTK_COMPOSITE_INTERPOLATE_FIRST )
    {
    return "Interpolate First";
    }
  if( this->CompositeMethod == VTK_COMPOSITE_CLASSIFY_FIRST )
    {
    return "Classify First";
    }
  else
    {
    return "Unknown";
    }
}

#endif
