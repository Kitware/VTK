// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkLightKit
 * @brief   a simple but quality lighting kit
 *
 * vtkLightKit is designed to make general purpose lighting of vtk
 * scenes simple, flexible, and attractive (or at least not horribly
 * ugly without significant effort).  Use a LightKit when you want
 * more control over your lighting than you can get with the default
 * vtk light, which is a headlight located at the camera. (HeadLights
 * are very simple to use, but they don't show the shape of objects very
 * well, don't give a good sense of "up" and "down", and don't evenly
 * light the object.)
 *
 * A LightKit consists of three lights, a key light, a fill light, and
 * a headlight.  The main light is the key light.  It is usually
 * positioned so that it appears like an overhead light (like the sun,
 * or a ceiling light).  It is generally positioned to shine down on the
 * scene from about a 45 degree angle vertically and at least a little
 * offset side to side.  The key light usually at least about twice as
 * bright as the total of all other lights in the scene to provide good
 * modeling of object features.
 *
 * The other lights in the kit (the fill light, headlight, and a pair of
 * back lights) are weaker sources that provide extra
 * illumination to fill in the spots that the key light misses.  The
 * fill light is usually positioned across from or opposite from the
 * key light (though still on the same side of the object as the
 * camera) in order to simulate diffuse reflections from other objects
 * in the scene.  The headlight, always located at the position of the
 * camera, reduces the contrast between areas lit by the key and fill
 * light. The two back lights, one on the left of the object as seen
 * from the observer and one on the right, fill on the high-contrast
 * areas behind the object.  To enforce the relationship between the
 * different lights, the intensity of the fill, back and headlights
 * are set as a ratio to the key light brightness.  Thus, the
 * brightness of all the lights in the scene can be changed by
 * changing the key light intensity.
 *
 * All lights are directional lights (infinitely far away with no
 * falloff).  Lights move with the camera.
 *
 * For simplicity, the position of lights in the LightKit can only be
 * specified using angles: the elevation (latitude) and azimuth
 * (longitude) of each light with respect to the camera, expressed in
 * degrees.  (Lights always shine on the camera's lookat point.) For
 * example, a light at (elevation=0, azimuth=0) is located at the
 * camera (a headlight).  A light at (elevation=90, azimuth=0) is
 * above the lookat point, shining down.  Negative azimuth values move
 * the lights clockwise as seen above, positive values
 * counter-clockwise.  So, a light at (elevation=45, azimuth=-20) is
 * above and in front of the object and shining slightly from the left
 * side.
 *
 * vtkLightKit limits the colors that can be assigned to any light to
 * those of incandescent sources such as light bulbs and sunlight.  It
 * defines a special color spectrum called "warmth" from which light
 * colors can be chosen, where 0 is cold blue, 0.5 is neutral white,
 * and 1 is deep sunset red.  Colors close to 0.5 are "cool whites" and
 * "warm whites," respectively.
 *
 * Since colors far from white on the warmth scale appear less bright,
 * key-to-fill and key-to-headlight ratios are skewed by
 * key, fill, and headlight colors.  If the flag MaintainLuminance
 * is set, vtkLightKit will attempt to compensate for these perceptual
 * differences by increasing the brightness of more saturated colors.
 *
 * A LightKit is not explicitly part of the vtk pipeline.  Rather, it
 * is a composite object that controls the behavior of lights using a
 * unified user interface.  Every time a parameter of vtkLightKit is
 * adjusted, the properties of its lights are modified.
 *
 * @par Credits:
 * vtkLightKit was originally written and contributed to vtk by
 * Michael Halle (mhalle@bwh.harvard.edu) at the Surgical Planning
 * Lab, Brigham and Women's Hospital.
 */

#ifndef vtkLightKit_h
#define vtkLightKit_h

#include "vtkObject.h"
#include "vtkRenderingCoreModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class vtkLight;
class vtkPiecewiseFunction;
class vtkRenderer;

class VTKRENDERINGCORE_EXPORT vtkLightKit : public vtkObject
{
public:
  static vtkLightKit* New();
  vtkTypeMacro(vtkLightKit, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  enum LightKitType
  {
    TKeyLight,
    TFillLight,
    TBackLight,
    THeadLight
  };

  enum LightKitSubType
  {
    Warmth,
    Intensity,
    Elevation,
    Azimuth,
    KFRatio,
    KBRatio,
    KHRatio
  };

  ///@{
  /**
   * Set/Get the intensity of the key light.  The key light is the
   * brightest light in the scene.  The intensities of the other two
   * lights are ratios of the key light's intensity.
   */
  vtkSetMacro(KeyLightIntensity, double);
  vtkGetMacro(KeyLightIntensity, double);
  ///@}

  ///@{
  /**
   * Set/Get the key-to-fill ratio.  This ratio controls
   * how bright the fill light is compared to the key light: larger
   * values correspond to a dimmer fill light.  The purpose of the
   * fill light is to light parts of the object not lit by the key
   * light, while still maintaining contrast.  This type of lighting
   * may correspond to indirect illumination from the key light, bounced
   * off a wall, floor, or other object.  The fill light should never
   * be brighter than the key light:  a good range for the key-to-fill
   * ratio is between 2 and 10.
   */
  vtkSetClampMacro(KeyToFillRatio, double, 0.5, VTK_DOUBLE_MAX);
  vtkGetMacro(KeyToFillRatio, double);
  ///@}

  ///@{
  /**
   * Set/Get the key-to-headlight ratio.  Similar to the key-to-fill
   * ratio, this ratio controls how bright the headlight light is
   * compared to the key light: larger values correspond to a dimmer
   * headlight light.  The headlight is special kind of fill light,
   * lighting only the parts of the object that the camera can see.
   * As such, a headlight tends to reduce the contrast of a scene.  It
   * can be used to fill in "shadows" of the object missed by the key
   * and fill lights.  The headlight should always be significantly
   * dimmer than the key light:  ratios of 2 to 15 are typical.
   */
  vtkSetClampMacro(KeyToHeadRatio, double, 0.5, VTK_DOUBLE_MAX);
  vtkGetMacro(KeyToHeadRatio, double);
  ///@}

  ///@{
  /**
   * Set/Get the key-to-back light ratio.  This ratio controls
   * how bright the back lights are compared to the key light: larger
   * values correspond to dimmer back lights.  The back lights fill
   * in the remaining high-contrast regions behind the object.
   * Values between 2 and 10 are good.
   */
  vtkSetClampMacro(KeyToBackRatio, double, 0.5, VTK_DOUBLE_MAX);
  vtkGetMacro(KeyToBackRatio, double);
  ///@}

  ///@{
  /**
   * Set the warmth of each the lights.  Warmth is a parameter that
   * varies from 0 to 1, where 0 is "cold" (looks icy or lit by a very
   * blue sky), 1 is "warm" (the red of a very red sunset, or the
   * embers of a campfire), and 0.5 is a neutral white.  The warmth
   * scale is non-linear. Warmth values close to 0.5 are subtly
   * "warmer" or "cooler," much like a warmer tungsten incandescent
   * bulb, a cooler halogen, or daylight (cooler still).  Moving
   * further away from 0.5, colors become more quickly varying towards
   * blues and reds.  With regards to aesthetics, extremes of warmth
   * should be used sparingly.
   */
  vtkSetMacro(KeyLightWarmth, double);
  vtkGetMacro(KeyLightWarmth, double);
  ///@}

  vtkSetMacro(FillLightWarmth, double);
  vtkGetMacro(FillLightWarmth, double);

  vtkSetMacro(HeadLightWarmth, double);
  vtkGetMacro(HeadLightWarmth, double);

  vtkSetMacro(BackLightWarmth, double);
  vtkGetMacro(BackLightWarmth, double);

  ///@{
  /**
   * Returns the floating-point RGB values of each of the light's color.
   */
  vtkGetVectorMacro(KeyLightColor, double, 3);
  vtkGetVectorMacro(FillLightColor, double, 3);
  vtkGetVectorMacro(HeadLightColor, double, 3);
  vtkGetVectorMacro(BackLightColor, double, 3);
  ///@}

  ///@{
  /**
   * If MaintainLuminance is set, the LightKit will attempt to maintain
   * the apparent intensity of lights based on their perceptual brightnesses.
   * By default, MaintainLuminance is off.
   */
  vtkBooleanMacro(MaintainLuminance, vtkTypeBool);
  vtkGetMacro(MaintainLuminance, vtkTypeBool);
  vtkSetMacro(MaintainLuminance, vtkTypeBool);
  ///@}

  /**
   * Get/Set the position of the key, fill, and back lights
   * using angular methods.  Elevation corresponds to latitude,
   * azimuth to longitude.  It is recommended that the key light
   * always be on the viewer's side of the object and above the
   * object, while the fill light generally lights the part of the object
   * not lit by the fill light.  The headlight, which is always located
   * at the viewer, can then be used to reduce the contrast in the image.
   * There are a pair of back lights.  They are located at the same
   * elevation and at opposing azimuths (ie, one to the left, and one to
   * the right).  They are generally set at the equator (elevation = 0),
   * and at approximately 120 degrees (lighting from each side and behind).
   */
  void SetKeyLightAngle(double elevation, double azimuth);
  void SetKeyLightAngle(double angle[2]) { this->SetKeyLightAngle(angle[0], angle[1]); }

  void SetKeyLightElevation(double x) { this->SetKeyLightAngle(x, this->KeyLightAngle[1]); }

  void SetKeyLightAzimuth(double x) { this->SetKeyLightAngle(this->KeyLightAngle[0], x); }

  vtkGetVectorMacro(KeyLightAngle, double, 2);
  double GetKeyLightElevation()
  {
    double ang[2];
    this->GetKeyLightAngle(ang);
    return ang[0];
  }

  double GetKeyLightAzimuth()
  {
    double ang[2];
    this->GetKeyLightAngle(ang);
    return ang[1];
  }

  void SetFillLightAngle(double elevation, double azimuth);
  void SetFillLightAngle(double angle[2]) { this->SetFillLightAngle(angle[0], angle[1]); }

  void SetFillLightElevation(double x) { this->SetFillLightAngle(x, this->FillLightAngle[1]); }

  void SetFillLightAzimuth(double x) { this->SetFillLightAngle(this->FillLightAngle[0], x); }

  vtkGetVectorMacro(FillLightAngle, double, 2);
  double GetFillLightElevation()
  {
    double ang[2];
    this->GetFillLightAngle(ang);
    return ang[0];
  }

  double GetFillLightAzimuth()
  {
    double ang[2];
    this->GetFillLightAngle(ang);
    return ang[1];
  }

  void SetBackLightAngle(double elevation, double azimuth);
  void SetBackLightAngle(double angle[2]) { this->SetBackLightAngle(angle[0], angle[1]); }

  void SetBackLightElevation(double x) { this->SetBackLightAngle(x, this->BackLightAngle[1]); }

  void SetBackLightAzimuth(double x) { this->SetBackLightAngle(this->BackLightAngle[0], x); }

  vtkGetVectorMacro(BackLightAngle, double, 2);
  double GetBackLightElevation()
  {
    double ang[2];
    this->GetBackLightAngle(ang);
    return ang[0];
  }

  double GetBackLightAzimuth()
  {
    double ang[2];
    this->GetBackLightAngle(ang);
    return ang[1];
  }

  ///@{
  /**
   * Add lights to, or remove lights from, a renderer.
   * Lights may be added to more than one renderer, if desired.
   */
  void AddLightsToRenderer(vtkRenderer* renderer);
  void RemoveLightsFromRenderer(vtkRenderer* renderer);
  ///@}

  void DeepCopy(vtkLightKit* kit);

  void Modified() override;
  void Update();

  /**
   * Helper method to go from a enum type to a string type
   */
  static const char* GetStringFromType(int type);

  /**
   * Helper method to go from a enum subtype to a string subtype
   */
  static const char* GetStringFromSubType(int type);

  /**
   * Helper method to go from a enum subtype to a string subtype
   * The difference from GetStringFromSubType is that it returns
   * a shorter strings (useful for GUI with minimum space)
   */
  static const char* GetShortStringFromSubType(int subtype);

  /**
   * Return the possible subtype from a given type. You have to pass
   * in a number i [0,3] no check is done.
   */
  static LightKitSubType GetSubType(LightKitType type, int i);

protected:
  vtkLightKit();
  ~vtkLightKit() override;

  void WarmthToRGBI(double w, double rgb[3], double& i);
  void WarmthToRGB(double w, double rgb[3]);
  void InitializeWarmthFunctions();
  double WarmthToIntensity(double w);

  double KeyLightIntensity;
  double KeyToFillRatio;
  double KeyToHeadRatio;
  double KeyToBackRatio;

  vtkLight* KeyLight;
  double KeyLightWarmth;
  double KeyLightAngle[2];
  double KeyLightColor[3];

  vtkLight* FillLight;
  double FillLightWarmth;
  double FillLightAngle[2];
  double FillLightColor[3];

  double BackLightWarmth;
  double BackLightColor[3];

  vtkLight* BackLight0;
  vtkLight* BackLight1;

  double BackLightAngle[2];

  vtkLight* HeadLight;
  double HeadLightWarmth;
  double HeadLightColor[3];

  vtkTypeBool MaintainLuminance;

  vtkPiecewiseFunction* WarmthFunction[4]; // r, g, b, perceptual length

private:
  vtkLightKit(const vtkLightKit&) = delete;
  void operator=(const vtkLightKit&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
