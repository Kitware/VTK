/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAbstractMapper3D.h
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
// .NAME vtkAbstractMapper3D - abstract class specifies interface to map 3D data
// .SECTION Description
// vtkAbstractMapper3D is an abstract class to specify interface between 3D
// data and graphics primitives or software rendering techniques. Subclasses
// of vtkAbstractMapper3D can be used for rendering geometry or rendering
// volumetric data.
//
// This class also defines an API to support hardware clipping planes (at most
// six planes can be defined). It also provides geometric data about the input
// data it maps, such as the bounding box and center.
//
// .SECTION See Also
// vtkAbstractMapper vtkMapper vtkPolyDataMapper vtkVolumeMapper

#ifndef __vtkAbstractMapper3D_h
#define __vtkAbstractMapper3D_h

#include "vtkAbstractMapper.h"
#include "vtkPlaneCollection.h"
#include "vtkPlane.h"

class vtkWindow;
class vtkDataSet;

class VTK_EXPORT vtkAbstractMapper3D : public vtkAbstractMapper
{
public:
  vtkTypeMacro(vtkAbstractMapper3D,vtkAbstractMapper);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Return bounding box (array of six floats) of data expressed as
  // (xmin,xmax, ymin,ymax, zmin,zmax).
  virtual float *GetBounds()=0;

  // Description:
  // Get the bounds for this mapper as (Xmin,Xmax,Ymin,Ymax,Zmin,Zmax).
  virtual void GetBounds(float bounds[6]);
  
  // Description:
  // Return the Center of this mapper's data.
  float *GetCenter();
  
  // Description:
  // Return the diagonal length of this mappers bounding box.
  float GetLength();

  // Description:
  // Is this a ray cast mapper? A subclass would return 1 if the
  // ray caster is needed to generate an image from this mapper.
  virtual int IsARayCastMapper() {return 0;};

  // Description:
  // Is this a "render into image" mapper? A subclass would return 1 if the
  // mapper produces an image by rendering into a software image buffer.
  virtual int IsARenderIntoImageMapper() {return 0;};

  // Description:
  // Update the network connected to this mapper.
  virtual void Update()=0;

  // Description:
  // Specify clipping planes to be applied when the data is mapped
  // (at most 6 clipping planes can be specified)
  void AddClippingPlane(vtkPlane *plane);
  void RemoveClippingPlane(vtkPlane *plane);

  // Description:
  // Get/Set the vtkPlaneCollection which specifies the 
  // clipping planes
  vtkSetObjectMacro(ClippingPlanes,vtkPlaneCollection);
  vtkGetObjectMacro(ClippingPlanes,vtkPlaneCollection);

protected:
  vtkAbstractMapper3D();
  ~vtkAbstractMapper3D();
  vtkAbstractMapper3D(const vtkAbstractMapper3D&) {};
  void operator=(const vtkAbstractMapper3D&) {};

  float Bounds[6];
  float Center[3];

  vtkPlaneCollection *ClippingPlanes;

};

#endif


