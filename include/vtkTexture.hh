/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTexture.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1995 Ken Martin, Will Schroeder, Bill Lorensen.

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
#include "vtkObject.hh"
#include "vtkStructuredPoints.hh"

class vtkRenderer;
class vtkTextureDevice;

class vtkTexture : public vtkObject
{
public:
  vtkTexture();
  ~vtkTexture();
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
