/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLightKit.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Michael Halle, Brigham and Women's Hospital

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
// .NAME vtkLightKit - a simple but quality lighting kit
// .SECTION Description
// vtkLightKit is designed to make general purpose lighting of vtk
// scenes simple, flexible, and attractive (or at least not horribly
// ugly without significant effort).  Use a LightKit when you want
// more control over your lighting than you can get with the default
// vtk light, which is a headlight located at the camera. (Headlights
// are very simple to use, but they don't show the shape of objects very 
// well, don't give a good sense of "up" and "down", and don't evenly
// light the object.)
//
// A LightKit consists of three lights, a key light, a fill light, and
// a headlight.  The main light is the key light.  It is usually
// positioned so that it appears like an overhead light (like the sun,
// or a ceiling light).  It is generally positioned to shine down on the
// scene from about a 45 degree angle vertically and at least a little
// offset side to side.  The key light usually at least about twice as 
// bright as the total of all other lights in the scene to provide good
// modeling of object features.
// 
// The other two lights in the kit, the fill light and headlight, are
// weaker sources that provide extra illumination to fill in the spots
// that the key light misses.  The fill light is usually positioned
// across from or opposite from the key light (though still on the
// same side of the object as the camera) in order to simulate diffuse
// reflections from other objects in the scene.  The headlight, always
// located at the position of the camera, reduces the contrast between
// areas lit by the key and fill light. To enforce the relationship
// between the three lights, the intensity of the fill and headlights
// are set as a ratio to the key light brightness.  Thus, the brightness
// of all the lights in the scene can be changed by changing the key light 
// intensity.
//
// All lights are directional lights (infinitely far away with no
// falloff).  Lights move with the camera.
//
// For simplicity, the position of lights in the LightKit can only be
// specified using angles: the elevation (latitude) and azimuth
// (longitude) of each light with respect to the camera, expressed in
// degrees.  (Lights always shine on the camera's lookat point.) For
// example, a light at (elevation=0, azimuth=0) is located at the
// camera (a headlight).  A light at (elevation=90, azimuth=0) is
// above the lookat point, shining down.  Negative azimuth values move
// the lights clockwise as seen above, positive values
// counter-clockwise.  So, a light at (elevation=45, azimuth=-20) is
// above and in front of the object and shining slightly from the left
// side.
//
// vtkLightKit limits the colors that can be assigned to any light to
// those of incandescent sources such as light bulbs and sunlight.  It
// defines a special color spectrum called "warmth" from which light
// colors can be chosen, where 0 is cold blue, 0.5 is neutral white,
// and 1 is deep sunset red.  Colors close to 0.5 are "cool whites" and
// "warm whites," respectively. 
//
// Since colors far from white on the warmth scale appear less bright,
// key-to-fill and key-to-headlight ratios are skewed by 
// key, fill, and headlight colors.  If the flag MaintainLuminance
// is set, vtkLightKit will attempt to compensate for these perceptual
// differences by increasing the brightness of more saturated colors.
//
// A LightKit is not explicitly part of the vtk pipeline.  Rather, it
// is a composite object that controls the behavior of lights using a
// unified user interface.  Every time a parameter of vtkLightKit is
// adjusted, the properties of its lights are modified.
//
// .SECTION Credits
// vtkLightKit was originally written and contributed to vtk by
// Michael Halle (mhalle@bwh.harvard.edu) at the Surgical Planning
// Lab, Brigham and Women's Hospital.

#ifndef __vtkLightKit_h
#define __vtkLightKit_h

#include "vtkObject.h"
class vtkLight;
class vtkPiecewiseFunction;
class vtkRenderer;

class VTK_RENDERING_EXPORT vtkLightKit : public vtkObject
{
public:
  static vtkLightKit *New();
  vtkTypeMacro(vtkLightKit, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set/Get the intensity of the key light.  The key light is the
  // brightest light in the scene.  The intensities of the other two
  // lights are ratios of the key light's intensity.  
  vtkSetMacro(KeyLightIntensity, float);  
  vtkGetMacro(KeyLightIntensity, float);

  // Description: 
  // Set/Get the key-to-fill ratio.  This ratio controls
  // how bright the fill light is compared to the key light: larger
  // values correspond to a dimmer fill light.  The purpose of the
  // fill light is to light parts of the object not lit by the key
  // light, while still maintaining constrast.  This type of lighting
  // may correspond to indirect illumination from the key light, bounced
  // off a wall, floor, or other object.  The fill light should never
  // be brighter than the key light:  a good range for the key-to-fill
  // ratio is between 3 and 10.
  vtkSetClampMacro(KeyToFillRatio, float, 0.5, VTK_FLOAT_MAX);
  vtkGetMacro(KeyToFillRatio, float);

  // Description: 
  // Set/Get the key-to-headlight ratio.  Similar to the key-to-fill
  // ratio, this ratio controls how bright the headlight light is
  // compared to the key light: larger values correspond to a dimmer
  // headlight light.  The headlight is special kind of fill light,
  // lighting only the parts of the object that the camera can see.
  // As such, a headlight tends to reduce the contrast of a scene.  It
  // can be used to fill in "shadows" of the object missed by the key
  // and fill lights.  The headlight should always be significantly
  // dimmer than the key light:  ratios of 3 to 15 are typical.
  vtkSetClampMacro(KeyToHeadRatio, float, 0.5, VTK_FLOAT_MAX);
  vtkGetMacro(KeyToHeadRatio, float);

  // Description: 
  // Set the warmth of each the lights.  Warmth is a parameter that
  // varies from 0 to 1, where 0 is "cold" (looks icy or lit by a very
  // blue sky), 1 is "warm" (the red of a very red sunset, or the
  // embers of a campfire), and 0.5 is a neutral white.  The warmth
  // scale is non-linear. Warmth values close to 0.5 are subtly
  // "warmer" or "cooler," much like a warmer tungsten incandescent
  // bulb, a cooler halogen, or daylight (cooler still).  Moving
  // further away from 0.5, colors become more quickly varying towards
  // blues and reds.  With regards to aesthetics, extremes of warmth
  // should be used sparingly.
  vtkSetMacro(KeyLightWarmth, float);
  vtkGetMacro(KeyLightWarmth, float);

  vtkSetMacro(FillLightWarmth, float);
  vtkGetMacro(FillLightWarmth, float);

  vtkSetMacro(HeadlightWarmth, float);
  vtkGetMacro(HeadlightWarmth, float);

  // Description:
  // Returns the floating-point RGB values of each of the light's color.
  vtkGetVectorMacro(KeyLightColor,  float, 3);
  vtkGetVectorMacro(FillLightColor, float, 3);
  vtkGetVectorMacro(HeadlightColor, float, 3);

  // Description:
  // If MaintainLuminance is set, the LightKit will attempt to maintain
  // the apparent intensity of lights based on their perceptual brightnesses.
  // By default, MaintainLuminance is off.
  vtkBooleanMacro(MaintainLuminance, int);
  vtkGetMacro(MaintainLuminance, int);
  vtkSetMacro(MaintainLuminance, int);

  // Description: 
  // Get/Set the position of the key light and fill
  // light, using angular methods.  Elevation corresponds to latitude,
  // azimuth to longitude.  It is recommended that the key light
  // always be on the viewer's side of the object and above the
  // object, while the fill light generally lights the part of the object
  // not lit by the fill light.  The headlight, which is always located
  // at the viewer, can then be used to reduce the contrast in the image.
  void SetKeyLightAngle(float elevation, float azimuth);
  void SetKeyLightAngle(float angle[2]) { 
    this->SetKeyLightAngle(angle[0], angle[1]); };

  void SetKeyLightElevation(float x) {
    this->SetKeyLightAngle(x, this->KeyLightAngle[1]); };

  void SetKeyLightAzimuth(float x) {
    this->SetKeyLightAngle(this->KeyLightAngle[0], x); };

  vtkGetVectorMacro(KeyLightAngle, float, 2);
  float GetKeyLightElevation() {
    float ang[2]; this->GetKeyLightAngle(ang); return ang[0]; };

  float GetKeyLightAzimuth() {
    float ang[2]; this->GetKeyLightAngle(ang); return ang[1]; };

  void SetFillLightAngle(float elevation, float azimuth);
  void SetFillLightAngle(float angle[2]) { 
    this->SetFillLightAngle(angle[0], angle[1]); };

  void SetFillLightElevation(float x) {
    this->SetFillLightAngle(x, this->FillLightAngle[1]); };

  void SetFillLightAzimuth(float x) {
    this->SetFillLightAngle(this->FillLightAngle[0], x); };

  vtkGetVectorMacro(FillLightAngle, float, 2);
  float GetFillLightElevation() {
    float ang[2]; this->GetFillLightAngle(ang); return ang[0]; };

  float GetFillLightAzimuth() {
    float ang[2]; this->GetFillLightAngle(ang); return ang[1]; };

  // Description:
  // Add lights to, or remove lights from, a renderer.  
  // Lights may be added to more than one renderer, if desired.
  void AddLightsToRenderer(vtkRenderer *renderer);
  void RemoveLightsFromRenderer(vtkRenderer *renderer);

  void DeepCopy(vtkLightKit *kit);

  void Modified();
  void Update();

protected:
  vtkLightKit();
  ~vtkLightKit();

  void WarmthToRGBI(float w, float rgb[3], float& i);
  void WarmthToRGB(float w, float rgb[3]);
  void InitializeWarmthFunctions();
  float WarmthToIntensity(float w);


  float KeyLightIntensity;
  float KeyToFillRatio;
  float KeyToHeadRatio;
  
  vtkLight *KeyLight;
  float KeyLightWarmth;
  float KeyLightAngle[2];
  float KeyLightColor[3];

  vtkLight *FillLight;
  float FillLightWarmth;
  float FillLightAngle[2];
  float FillLightColor[3];

  vtkLight *Headlight;
  float HeadlightWarmth;
  float HeadlightColor[3];

  int MaintainLuminance;

  vtkPiecewiseFunction *WarmthFunction[4]; // r, g, b, perceptual length
private:
  vtkLightKit(const vtkLightKit&);  // Not implemented.
  void operator=(const vtkLightKit&);  // Not implemented.
};

#endif
