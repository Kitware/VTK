/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkProperty.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
// .NAME vtkProperty - represent surface properties of a geometric object
// .SECTION Description
// vtkProperty is an object that represents lighting and other surface
// properties of a geometric object. The primary properties that can be 
// set are colors (overall, ambient, diffuse, specular, and edge color);
// specular power; opacity of the object; the representation of the
// object (points, wireframe, or surface); and the shading method to be 
// used (flat, Gouraud, and Phong). Also, some special graphics features
// like backface properties can be set and manipulated with this object.

// .SECTION See Also
// vtkActor vtkPropertyDevice

#ifndef __vtkProperty_h
#define __vtkProperty_h

#include "vtkRender.h"
#include "vtkObject.h"
#include "vtkStructuredPoints.h"

class vtkRenderer;
class vtkActor;

class VTK_RENDERING_EXPORT vtkProperty : public vtkObject
{
public:
  vtkTypeMacro(vtkProperty,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Construct object with object color, ambient color, diffuse color,
  // specular color, and edge color white; ambient coefficient=0; diffuse 
  // coefficient=0; specular coefficient=0; specular power=1; Gouraud shading;
  // and surface representation. Backface and frontface culling are off.
  static vtkProperty *New();

  // Description:
  // Assign one property to another. 
  void DeepCopy(vtkProperty *p);

  // Description:
  // This method causes the property to set up whatever is required for
  // its instance variables. This is actually handled by a subclass of
  // vtkProperty, which is created automatically. This
  // method includes the invoking actor as an argument which can
  // be used by property devices that require the actor.
  virtual void Render(vtkActor *,vtkRenderer *) {};

  // Description:
  // This method renders the property as a backface property. TwoSidedLighting
  // must be turned off to see any backface properties. Note that only
  // colors and opacity are used for backface properties. Other properties
  // such as Representation, Culling are specified by the Property.
  virtual void BackfaceRender(vtkActor *,vtkRenderer *) {};

  // Description:
  // Set the shading interpolation method for an object.
  vtkSetClampMacro(Interpolation,int,VTK_FLAT,VTK_PHONG);
  vtkGetMacro(Interpolation,int);
  void SetInterpolationToFlat() {this->SetInterpolation(VTK_FLAT);};
  void SetInterpolationToGouraud() {this->SetInterpolation(VTK_GOURAUD);};
  void SetInterpolationToPhong() {this->SetInterpolation(VTK_PHONG);};
  char *GetInterpolationAsString();

  // Description:
  // Control the surface geometry representation for the object.
  vtkSetClampMacro(Representation,int,VTK_POINTS,VTK_SURFACE);
  vtkGetMacro(Representation,int);
  void SetRepresentationToPoints() {this->SetRepresentation(VTK_POINTS);};
  void SetRepresentationToWireframe() {
    this->SetRepresentation(VTK_WIREFRAME);};
  void SetRepresentationToSurface() {this->SetRepresentation(VTK_SURFACE);};
  char *GetRepresentationAsString();

  // Description:
  // Set the color of the object. Has the side effect of setting the
  // ambient diffuse and specular colors as well. This is basically
  // a quick overall color setting method.
  void SetColor(float r,float g,float b);
  void SetColor(float a[3]) { this->SetColor(a[0], a[1], a[2]); };
  float *GetColor();
  void GetColor(float rgb[3]);

  // Description:
  // Set/Get the ambient lighting coefficient.
  vtkSetClampMacro(Ambient,float,0.0,1.0);
  vtkGetMacro(Ambient,float);

  // Description:
  // Set/Get the diffuse lighting coefficient.
  vtkSetClampMacro(Diffuse,float,0.0,1.0);
  vtkGetMacro(Diffuse,float);

  // Description:
  // Set/Get the specular lighting coefficient.
  vtkSetClampMacro(Specular,float,0.0,1.0);
  vtkGetMacro(Specular,float);

  // Description:
  // Set/Get the specular power.
  vtkSetClampMacro(SpecularPower,float,0.0,100.0);
  vtkGetMacro(SpecularPower,float);

  // Description:
  // Set/Get the object's opacity. 1.0 is totally opaque and 0.0 is completely
  // transparent.
  vtkSetClampMacro(Opacity,float,0.0,1.0);
  vtkGetMacro(Opacity,float);

  // Description:
  // Set/Get the ambient surface color. Not all renderers support separate
  // ambient and diffuse colors. From a physical standpoint it really
  // doesn't make too much sense to have both. For the rendering
  // libraries that don't support both, the diffuse color is used.
  vtkSetVector3Macro(AmbientColor,float);
  vtkGetVectorMacro(AmbientColor,float,3);

  // Description:
  // Set/Get the diffuse surface color.
  vtkSetVector3Macro(DiffuseColor,float);
  vtkGetVectorMacro(DiffuseColor,float,3);

  // Description:
  // Set/Get the specular surface color.
  vtkSetVector3Macro(SpecularColor,float);
  vtkGetVectorMacro(SpecularColor,float,3);

  // Description:
  // Turn on/off the visibility of edges. On some renderers it is
  // possible to render the edges of geometric primitives separately
  // from the interior.
  vtkGetMacro(EdgeVisibility,int);
  vtkSetMacro(EdgeVisibility,int);
  vtkBooleanMacro(EdgeVisibility,int);

  // Description:
  // Set/Get the color of primitive edges (if edge visibility is enabled).
  vtkSetVector3Macro(EdgeColor,float);
  vtkGetVectorMacro(EdgeColor,float,3);

  // Description:
  // Set/Get the width of a Line. The width is expressed in screen units.
  // This is only implemented for OpenGL. The default is 1.0.
  vtkSetClampMacro(LineWidth,float,0,VTK_LARGE_FLOAT);
  vtkGetMacro(LineWidth,float);

  // Description:
  // Set/Get the stippling pattern of a Line, as a 16-bit binary pattern 
  // (1 = pixel on, 0 = pixel off).
  // This is only implemented for OpenGL. The default is 0xFFFF.
  vtkSetMacro(LineStipplePattern,int);
  vtkGetMacro(LineStipplePattern,int);

  // Description:
  // Set/Get the stippling repeat factor of a Line, which specifies how
  // many times each bit in the pattern is to be repeated.
  // This is only implemented for OpenGL. The default is 1.
  vtkSetClampMacro(LineStippleRepeatFactor,int,1,VTK_LARGE_INTEGER);
  vtkGetMacro(LineStippleRepeatFactor,int);

  // Description:
  // Set/Get the diameter of a point. The size is expressed in screen units.
  // This is only implemented for OpenGL. The default is 1.0.
  vtkSetClampMacro(PointSize,float,0,VTK_LARGE_FLOAT);
  vtkGetMacro(PointSize,float);

  // Description:
  // Turn on/off fast culling of polygons based on orientation of normal 
  // with respect to camera. If backface culling is on, polygons facing
  // away from camera are not drawn.
  vtkGetMacro(BackfaceCulling,int);
  vtkSetMacro(BackfaceCulling,int);
  vtkBooleanMacro(BackfaceCulling,int);

  // Description:
  // Turn on/off fast culling of polygons based on orientation of normal 
  // with respect to camera. If frontface culling is on, polygons facing
  // towards camera are not drawn.
  vtkGetMacro(FrontfaceCulling,int);
  vtkSetMacro(FrontfaceCulling,int);
  vtkBooleanMacro(FrontfaceCulling,int);

protected:
  vtkProperty();
  ~vtkProperty() {};

  float Color[3];
  float AmbientColor[3];
  float DiffuseColor[3];
  float SpecularColor[3];
  float EdgeColor[3];
  float Ambient;
  float Diffuse;
  float Specular;
  float SpecularPower;
  float Opacity;
  float PointSize;
  float LineWidth;
  int   LineStipplePattern;
  int   LineStippleRepeatFactor;
  int   Interpolation; 
  int   Representation;
  int   EdgeVisibility;
  int   BackfaceCulling;
  int   FrontfaceCulling;
private:
  vtkProperty(const vtkProperty&);  // Not implemented.
  void operator=(const vtkProperty&);  // Not implemented.
};

// Description:
// Return the method of shading as a descriptive character string.
inline char *vtkProperty::GetInterpolationAsString(void)
{
  if ( this->Interpolation == VTK_FLAT )
    {
    return (char *)"Flat";
    }
  else if ( this->Interpolation == VTK_GOURAUD ) 
    {
    return (char *)"Gouraud";
    }
  else 
    {
    return (char *)"Phong";
    }
}


// Description:
// Return the method of shading as a descriptive character string.
inline char *vtkProperty::GetRepresentationAsString(void)
{
  if ( this->Representation == VTK_POINTS )
    {
    return (char *)"Points";
    }
  else if ( this->Representation == VTK_WIREFRAME ) 
    {
    return (char *)"Wireframe";
    }
  else 
    {
    return (char *)"Surface";
    }
}



#endif
