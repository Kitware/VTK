/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLightingMapPass.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkLightingMapPass - TO DO
// .SECTION Description
// Renders lighting information directly instead of final shaded colors.
// The information keys allow the selection of either normal rendering or
// luminance. For normals, the (nx, ny, nz) tuple are rendered directly into
// the (r,g,b) fragment. For luminance, the diffuse and specular intensities are
// rendered into the red and green channels, respectively. The blue channel is
// zero. For both luminances and normals, the alpha channel is set to 1.0 if
// present.
//
// .SECTION See Also
// vtkRenderPass vtkDefaultPass

#ifndef vtkLightingMapPass_h
#define vtkLightingMapPass_h

#include "vtkRenderingOpenGL2Module.h" // For export macro
#include "vtkDefaultPass.h"

class vtkInformationIntegerKey;

class VTKRENDERINGOPENGL2_EXPORT vtkLightingMapPass : public vtkDefaultPass
{
public:
  static vtkLightingMapPass *New();
  vtkTypeMacro(vtkLightingMapPass, vtkDefaultPass);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the type of lighting render to perform
  enum RenderMode { LUMINANCE, NORMALS };
  vtkSetMacro(RenderType, RenderMode);
  vtkGetMacro(RenderType, RenderMode);

  // Description:
  // If this key exists on the PropertyKeys of a prop, the active scalar array
  // on the prop will be rendered as its color. This key is mutually exclusive
  // with the RENDER_LUMINANCE key.
  static vtkInformationIntegerKey *RENDER_LUMINANCE();

  // Description:
  // if this key exists on the ProperyKeys of a prop, the active vector array on
  // the prop will be rendered as its color. This key is mutually exclusive with
  // the RENDER_LUMINANCE key.
  static vtkInformationIntegerKey *RENDER_NORMALS();

  //BTX
  // Description:
  // Perform rendering according to a render state \p s.
  // \pre s_exists: s!=0
  virtual void Render(const vtkRenderState *s);
  //ETX

 protected:
  // Description:
  // Default constructor.
  vtkLightingMapPass();

  // Description:
  // Destructor.
  virtual ~vtkLightingMapPass();

  // Description:
  // Opaque pass with key checking.
  // \pre s_exists: s!=0
  virtual void RenderOpaqueGeometry(const vtkRenderState *s);

 private:
  vtkLightingMapPass(const vtkLightingMapPass&);  // Not implemented.
  void operator=(const vtkLightingMapPass&);  // Not implemented.

  RenderMode RenderType;
};

#endif
