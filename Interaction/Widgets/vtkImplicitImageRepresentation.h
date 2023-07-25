// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkImplicitImageRepresentation
 * @brief   a representation for a vtkImplicitPlaneWidget2 which reslices a volume
 *
 * This class is a specialization of the vtkImplicitPlaneRepresentation. It
 * is specialized to resample volumes across a plane. It is similar to
 * vtkImagePlaneWidget, except the combination of vtkImplicitPlaneWidget2 and
 * vtkImplicitImageRepresentation is a second generation widget design, with
 * a much simpler and intuitive plane manipulation.
 *
 * See the documentation for vtkImplicitPlaneWidget2 and
 * vtkImplicitPlaneRepresentation for usage information. One notable difference
 * is that the methods PlaceImage() are preferred to the superclasses'
 * PlaceWidget() method.
 *
 * @sa
 * vtkImplicitPlaneWidget2 vtkImagePlaneWidget
 */

#ifndef vtkImplicitImageRepresentation_h
#define vtkImplicitImageRepresentation_h

#include "vtkImagePlaneWidget.h" //For enums
#include "vtkImplicitPlaneRepresentation.h"
#include "vtkInteractionWidgetsModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class vtkActor;
class vtkImageData;
class vtkImageMapToColors;
class vtkImageReslice;
class vtkLookupTable;
class vtkMatrix4x4;
class vtkTexture;
class vtkTextureMapToPlane;

class VTKINTERACTIONWIDGETS_EXPORT vtkImplicitImageRepresentation
  : public vtkImplicitPlaneRepresentation
{
public:
  /**
   * Instantiate the class.
   */
  static vtkImplicitImageRepresentation* New();

  ///@{
  /**
   * Standard methods for the class.
   */
  vtkTypeMacro(vtkImplicitImageRepresentation, vtkImplicitPlaneRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  ///@}

  ///@{
  /**
   * Various ways to specify the vtkImageData* input for the
   * vtkImageReslice; and perform PlaceWidget().
   */
  void PlaceImage(vtkImageData* img);
  void PlaceImage(vtkAlgorithmOutput* aout);
  ///@}

  ///@{
  /**
   * Methods to interface with the vtkImplicitPlaneWidget2. Most of the required
   * methods are implemented by this class's superclass.
   */
  void BuildRepresentation() override;
  ///@}

  ///@{
  /**
   * Let the user control the lookup table. NOTE: apply this method BEFORE
   * applying the SetLookupTable method. The default is Off.
   */
  vtkSetMacro(UserControlledLookupTable, bool);
  vtkGetMacro(UserControlledLookupTable, bool);
  vtkBooleanMacro(UserControlledLookupTable, bool);
  ///@}

  ///@{
  /**
   * Set/Get the internal lookuptable (lut) to one defined by the user, or,
   * alternatively, to the lut of another representation or widget.  In this way,
   * a set of three orthogonal planes can share the same lut so that
   * window-levelling is performed uniformly among planes.  The default
   * internal lut can be re- set/allocated by setting to nullptr.
   */
  virtual void SetLookupTable(vtkLookupTable*);
  vtkGetObjectMacro(LookupTable, vtkLookupTable);
  ///@}

  ///@{
  /**
   * Specify whether to interpolate the texture or not. When off, the
   * reslice interpolation is nearest neighbour regardless of how the
   * interpolation is set through the API. Set before setting the
   * vtkImageData input. Default is On.
   */
  vtkSetMacro(TextureInterpolate, bool);
  vtkGetMacro(TextureInterpolate, bool);
  vtkBooleanMacro(TextureInterpolate, bool);
  ///@}

  ///@{
  /**
   * Set the interpolation to use when texturing the plane.
   */
  void SetResliceInterpolate(int);
  vtkGetMacro(ResliceInterpolate, int);
  void SetResliceInterpolateToNearestNeighbour()
  {
    this->SetResliceInterpolate(VTK_NEAREST_RESLICE);
  }
  void SetResliceInterpolateToLinear() { this->SetResliceInterpolate(VTK_LINEAR_RESLICE); }
  void SetResliceInterpolateToCubic() { this->SetResliceInterpolate(VTK_CUBIC_RESLICE); }
  ///@}

  ///@{
  /**
   * Convenience method to get the vtkImageMapToColors filter used by this
   * widget.  The user can properly render other transparent actors in a
   * scene by calling the filter's SetOutputFormatToRGB and
   * PassAlphaToOutputOff.
   */
  virtual void SetColorMap(vtkImageMapToColors*);
  vtkGetObjectMacro(ColorMap, vtkImageMapToColors);
  ///@}

  /**
   * Convenience method to get the vtkImageReslice filter used by this
   * widget.
   */
  vtkGetObjectMacro(Reslice, vtkImageReslice);

  /**
   * This method modifies the texture pipeline in order to generate texture
   * coordinates.
   */
  void SetCropPlaneToBoundingBox(bool) override;

protected:
  vtkImplicitImageRepresentation();
  ~vtkImplicitImageRepresentation() override;

  bool UserControlledLookupTable;
  bool TextureInterpolate;
  int ResliceInterpolate;
  double OriginalWindow;
  double OriginalLevel;

  // These classes implement the imaging pipeline. Note that we
  // use the superclass plane to draw the image texture.
  vtkImageData* ImageData;
  vtkImageReslice* Reslice;
  vtkMatrix4x4* ResliceAxes;
  vtkImageMapToColors* ColorMap;
  vtkTexture* Texture;
  vtkLookupTable* LookupTable;
  vtkLookupTable* CreateDefaultLookupTable();
  void UpdatePlane();
  void GenerateTexturePlane();
  void CreateDefaultProperties() override;

  // This enables texture mapping on the cropped plane
  vtkTextureMapToPlane* TextureMapToPlane;

private:
  vtkImplicitImageRepresentation(const vtkImplicitImageRepresentation&) = delete;
  void operator=(const vtkImplicitImageRepresentation&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
