/*=========================================================================

  Program:   Visualization Library
  Module:    VolRen.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file or its
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994

=========================================================================*/
// .NAME vlVolumeRenderer - renders volumetric data
// .SECTION Description
// vlVolumeRenderer handles volume data much like the vlRenderer handles
// polygonal data. A vlVolumeRenderer renders its image during the normal
// rendering cycle, after the Renderer has rendered its surfaces, but
// before any doublebuffer switching is done. Many of the attributes this
// object requires for rendering are obtained from the Renderer which
// invokes its Render method.

#ifndef __vlVolumeRenderer_hh
#define __vlVolumeRenderer_hh

#include "Renderer.hh"
#include "VolumeC.hh"

class vlVolumeRenderer : public vlObject
{
public:
  vlVolumeRenderer();
  char *GetClassName() {return "vlVolumeRenderer";};
  void PrintSelf(ostream& os, vlIndent indent);

  void AddVolume(vlVolume *);
  void RemoveVolume(vlVolume *);
  vlVolumeCollection *GetVolumes();

  // Description:
  // Create an image.
  virtual void Render(vlRenderer *);

  // Description:
  // Get the ray step size in world coordinates.
  vlGetMacro(StepSize,float);
  // Description:
  // Set the ray step size in world coordinates.
  vlSetMacro(StepSize,float);

protected:
  float StepSize;
  vlVolumeCollection Volumes;
  unsigned char *Image;
  void TraceOneRay(float p1[4], float p2[4],vlVolume *vol, 
			   int steps,float *res);
  void Composite(float *rays,int steps,int numRays,
		 unsigned char *resultColor);
  void CalcRayValues(vlRenderer *,float foo[6][3], int *size, int *steps);

  vlTransform Transform; //use to perform ray transformation
};

// Description:
// Get the list of Volumes for this renderer.
inline vlVolumeCollection *vlVolumeRenderer::GetVolumes() 
  {return &(this->Volumes);};

#endif








