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
// origin from vtkProp. The volume also maintains a reference to the
// volumetric data (i.e., the volume mapper). The volume also contains a
// reference to a volume property which contains all common volume rendering 
// parameters.

// .SECTION see also
// vtkVolumeMapper vtkVolumeProperty vtkProp

#ifndef __vtkVolume_h
#define __vtkVolume_h

#include "vtkProp.h"
#include "vtkTransform.h"
#include "vtkVolumeMapper.h"
#include "vtkVolumeProperty.h"

class vtkRenderer;

class VTK_EXPORT vtkVolume : public vtkProp
{
 public:
  vtkVolume();
  ~vtkVolume();
  static vtkVolume *New() {return new vtkVolume;};
  const char *GetClassName() {return "vtkVolume";};
  void PrintSelf(ostream& os, vtkIndent indent);

  vtkVolume &operator=(const vtkVolume& volume);

  virtual void Render(vtkRenderer *ren);
  virtual void Update();

  // Description:
  // Set/Get the scale of the volume. Scaling in performed isotropically in
  // X,Y and Z. Any scale values that are zero will be automatically
  // converted to one. Non-isotropic scaling must be done in the 
  // scalar data provided to vtkVolumeMapper.
  vtkGetMacro(Scale,float);
  vtkSetMacro(Scale,float);

  // Description:
  // Get the matrix from the position, origin, scale and orientation
  // This matrix is cached, so multiple GetMatrix() calls will be
  // efficient.
  void GetMatrix(vtkMatrix4x4& m);

  // Description:
  // Get the bounds. GetBounds(),
  // GetXRange(), GetYRange(), and GetZRange return world coordinates.
  float *GetBounds();
  float GetMinXBound();
  float GetMaxXBound();
  float GetMinYBound();
  float GetMaxYBound();
  float GetMinZBound();
  float GetMaxZBound();

  // Description:
  // Set/Get the volume mapper.
  vtkSetObjectMacro(VolumeMapper,vtkVolumeMapper);
  vtkGetObjectMacro(VolumeMapper,vtkVolumeMapper);

  // Description:
  // Set/Get the volume property.
  void SetVolumeProperty(vtkVolumeProperty *property);
  void SetVolumeProperty(vtkVolumeProperty& property) {this->SetVolumeProperty(&property);};
  vtkVolumeProperty *GetVolumeProperty();

   unsigned long int GetMTime();//overload superclasses' implementation

protected:

  float             Scale;
  vtkMatrix4x4      Matrix;
  vtkTimeStamp      MatrixMTime;

  vtkVolumeMapper   *VolumeMapper;

  vtkVolumeProperty *VolumeProperty;
  int		    SelfCreatedProperty;
};

#endif

