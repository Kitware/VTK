/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVolumeRenderer.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1996 Ken Martin, Will Schroeder, Bill Lorensen.

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
// .NAME vtkVolumeRenderer - renders volumetric data
// .SECTION Description
// vtkVolumeRenderer handles volume data much like the vtkRenderer handles
// polygonal data. A vtkVolumeRenderer renders its image during the normal
// rendering cycle, after the renderer has rendered its surfaces, but
// before any doublebuffer switching is done. Many of the attributes this
// object requires for rendering are obtained from the renderer, which
// invokes its Render() method. This object must be associated with a 
// renderer in order to work.

// .SECTION see also
// vtkRenderer vtkVolume

#ifndef __vtkVolumeRenderer_h
#define __vtkVolumeRenderer_h

#include "vtkRenderer.h"
#include "vtkVolumeCollection.h"

class VTK_EXPORT vtkVolumeRenderer : public vtkObject
{
public:
  vtkVolumeRenderer();
  static vtkVolumeRenderer *New() {return new vtkVolumeRenderer;};
  char *GetClassName() {return "vtkVolumeRenderer";};
  void PrintSelf(ostream& os, vtkIndent indent);

  void AddVolume(vtkVolume *);
  void RemoveVolume(vtkVolume *);
  vtkVolumeCollection *GetVolumes();

  // Description:
  // Render its volumes to create a composite image.
  virtual void Render(vtkRenderer *);

  // Description:
  // Set/Get the ray step size in world coordinates. The step size you
  // select will make a big difference in the required rendering time
  // and possibly the results as well. A larger step size will render 
  // more quickly. Too large of a step size will result in under sampling
  // the volume(s), yielding less accurate results.
  vtkGetMacro(StepSize,float);
  vtkSetMacro(StepSize,float);

protected:
  float StepSize;
  vtkVolumeCollection Volumes;
  unsigned char *Image;
  void TraceOneRay(float p1[4], float p2[4],vtkVolume *vol, 
			   int steps,float *res);
  void Composite(float *rays,int steps,int numRays,
		 unsigned char *resultColor);
  void ComputeRayValues(vtkRenderer *,float foo[6][3], int *size, int *steps);

  vtkTransform Transform; //use to perform ray transformation
};

// Description:
// Get the list of volumes for this renderer.
inline vtkVolumeCollection *vtkVolumeRenderer::GetVolumes() 
  {return &(this->Volumes);};

#endif








