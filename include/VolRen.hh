/*=========================================================================

  Program:   Visualization Toolkit
  Module:    VolRen.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file or its
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994

=========================================================================*/
// .NAME vtkVolumeRenderer - renders volumetric data
// .SECTION Description
// vtkVolumeRenderer handles volume data much like the vtkRenderer handles
// polygonal data. A vtkVolumeRenderer renders its image during the normal
// rendering cycle, after the Renderer has rendered its surfaces, but
// before any doublebuffer switching is done. Many of the attributes this
// object requires for rendering are obtained from the Renderer which
// invokes its Render method.

#ifndef __vtkVolumeRenderer_hh
#define __vtkVolumeRenderer_hh

#include "Renderer.hh"
#include "VolumeC.hh"

class vtkVolumeRenderer : public vtkObject
{
public:
  vtkVolumeRenderer();
  char *GetClassName() {return "vtkVolumeRenderer";};
  void PrintSelf(ostream& os, vtkIndent indent);

  void AddVolume(vtkVolume *);
  void RemoveVolume(vtkVolume *);
  vtkVolumeCollection *GetVolumes();

  // Description:
  // Create an image.
  virtual void Render(vtkRenderer *);

  // Description:
  // Get the ray step size in world coordinates.
  vtkGetMacro(StepSize,float);
  // Description:
  // Set the ray step size in world coordinates.
  vtkSetMacro(StepSize,float);

protected:
  float StepSize;
  vtkVolumeCollection Volumes;
  unsigned char *Image;
  void TraceOneRay(float p1[4], float p2[4],vtkVolume *vol, 
			   int steps,float *res);
  void Composite(float *rays,int steps,int numRays,
		 unsigned char *resultColor);
  void CalcRayValues(vtkRenderer *,float foo[6][3], int *size, int *steps);

  vtkTransform Transform; //use to perform ray transformation
};

// Description:
// Get the list of Volumes for this renderer.
inline vtkVolumeCollection *vtkVolumeRenderer::GetVolumes() 
  {return &(this->Volumes);};

#endif








