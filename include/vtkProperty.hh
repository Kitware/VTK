/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkProperty.hh
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
// .NAME vtkProperty - represent surface properties of a geometric object
// .SECTION Description
// vtkProperty is an object that represents lighting and other surface
// properties of a geometric object. The primary properties that can be 
// set are colors (object, ambient, diffuse, specular, and edge color),
// specular power, transparency of the object, the representation of the
// object (points, wireframe, or surface), and the shading method to be 
// used (flat, Gouraud, and Phong).
// .SECTION See Also
// See vtkRenderer for definition of #define's.

#ifndef __vtkProperty_hh
#define __vtkProperty_hh

#include "Render.hh"
#include "vtkObject.hh"
#include "vtkStructuredPoints.hh"

class vtkRenderer;
class vtkPropertyDevice;

class vtkProperty : public vtkObject
{
public:
  vtkProperty();
  ~vtkProperty();
  char *GetClassName() {return "vtkProperty";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Abstract interface to renderer. Each concrete subclass of vtkProperty
  // will load its data into graphics system in response to this method
  // invocation.
  virtual void Render(vtkRenderer *ren);

  void SetFlat (void);
  void SetGouraud (void);
  void SetPhong (void);
  void SetPoints (void);
  void SetWireframe (void);
  void SetSurface (void);

  // Description:
  // Get the method of representation for the object.
  vtkGetMacro(Representation,int);

  // Description:
  // Get the shading method for the object.
  vtkGetMacro(Interpolation,int);

  void SetColor(float r,float g,float b);
  void SetColor(float a[3]) { this->SetColor(a[0], a[1], a[2]); };
  vtkGetVectorMacro(Color,float,3);

  // Description:
  // Set ambient coefficient.
  vtkSetClampMacro(Ambient,float,0.0,1.0);
  vtkGetMacro(Ambient,float);

  // Description:
  // Set diffuse coefficient.
  vtkSetClampMacro(Diffuse,float,0.0,1.0);
  vtkGetMacro(Diffuse,float);

  // Description:
  // Set specular coefficient.
  vtkSetClampMacro(Specular,float,0.0,1.0);
  vtkGetMacro(Specular,float);

  // Description:
  // Set the specular power.
  vtkSetClampMacro(SpecularPower,float,0.0,100.0);
  vtkGetMacro(SpecularPower,float);

  // Description:
  // Set the object transparency.
  vtkSetClampMacro(Transparency,float,0.0,1.0);
  vtkGetMacro(Transparency,float);

  // Description:
  // Turn on/off the visibility of edges. On some renderers it is
  // possible to render the edges of geometric primitives separately
  // from the interior.
  vtkGetMacro(EdgeVisibility,int);
  vtkSetMacro(EdgeVisibility,int);
  vtkBooleanMacro(EdgeVisibility,int);

  // Description:
  // Turn backface properties on and off. (not implemented yet)
  vtkGetMacro(Backface,int);
  vtkSetMacro(Backface,int);
  vtkBooleanMacro(Backface,int);

  // Description:
  // Set the ambient light color.
  vtkSetVector3Macro(AmbientColor,float);
  vtkGetVectorMacro(AmbientColor,float,3);

  // Description:
  // Set the diffuse light color.
  vtkSetVector3Macro(DiffuseColor,float);
  vtkGetVectorMacro(DiffuseColor,float,3);

  // Description:
  // Set the specular color.
  vtkSetVector3Macro(SpecularColor,float);
  vtkGetVectorMacro(SpecularColor,float,3);

  // Description:
  // Set the color of edges (if edge visibility enabled).
  vtkSetVector3Macro(EdgeColor,float);
  vtkGetVectorMacro(EdgeColor,float,3);

protected:
  float Color[3];
  float AmbientColor[3];
  float DiffuseColor[3];
  float SpecularColor[3];
  float EdgeColor[3];
  float Ambient;
  float Diffuse;
  float Specular;
  float SpecularPower;
  float Transparency;
  int   Interpolation; 
  int   Representation;
  int   EdgeVisibility;
  int   Backface;
  vtkPropertyDevice *Device;
};

#endif
