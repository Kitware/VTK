/*=========================================================================

  Program:   Visualization Toolkit
  Module:    Texture.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file or its
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994

=========================================================================*/
// .NAME vtkTexture - handles properties associated with a texture map
// .SECTION Description
// vtkTexture is an object that handles loading and binding of texture
// maps. It obtains its data from a StructuredPoints input. Multiple 
// actors using the same texture map should share the same vtkTexture
// object.  This reduces the amount of memory being used. Currently
// only 2D etxture maps are supported event though the data pipeline
// supports 1,2, and 3D texture coordinates.
// .SECTION See Also
// See vtkRenderer for definition of #define's.

#ifndef __vtkTexture_hh
#define __vtkTexture_hh

#include "Render.hh"
#include "Object.hh"
#include "StrPts.hh"

class vtkRenderer;
class vtkTextureDevice;

class vtkTexture : public vtkObject
{
public:
  vtkTexture();
  char *GetClassName() {return "vtkTexture";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Renders a texture map. It first checks the MTimes to make sure
  // the texture maps Input is valid then it invokes the Load method.
  virtual void Render(vtkRenderer *ren);

  // Description:
  // Abstract interface to renderer. Each concrete subclass of vtkTexture
  // will load its data into graphics system in response to this method
  // invocation.
  virtual void Load(vtkRenderer *ren);

  // Description:
  // Turn on/off the repetition of the texture map when the texture
  // coords extend beyond the [0,1] range.
  vtkGetMacro(Repeat,int);
  vtkSetMacro(Repeat,int);
  vtkBooleanMacro(Repeat,int);

  // Description:
  // Turn on/off linear interpolation of the texture map when rendering.
  vtkGetMacro(Interpolate,int);
  vtkSetMacro(Interpolate,int);
  vtkBooleanMacro(Interpolate,int);

  // Description:
  // Specify 2D or 3D texture map.
  vtkSetObjectMacro(Input,vtkStructuredPoints);
  vtkGetObjectMacro(Input,vtkStructuredPoints);

protected:
  int   Repeat;
  int   Interpolate;
  vtkStructuredPoints *Input;
  vtkTextureDevice *Device;
};

#endif
