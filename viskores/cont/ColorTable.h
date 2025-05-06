//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef viskores_cont_ColorTable_h
#define viskores_cont_ColorTable_h

#include <viskores/Range.h>
#include <viskores/Types.h>

#include <viskores/cont/viskores_cont_export.h>

#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/ColorTableSamples.h>
#include <viskores/cont/ExecutionObjectBase.h>

#include <viskores/exec/ColorTable.h>

#include <set>

namespace viskores
{
namespace cont
{

namespace detail
{
struct ColorTableInternals;
}

/// \brief Color Table for coloring arbitrary fields
///
///
/// The `viskores::cont::ColorTable` allows for color mapping in RGB or HSV space and
/// uses a piecewise hermite functions to allow opacity interpolation that can be
/// piecewise constant, piecewise linear, or somewhere in-between
/// (a modified piecewise hermite function that squishes the function
/// according to a sharpness parameter).
///
/// For colors interpolation is handled using a piecewise linear function.
///
/// For opacity we define a piecewise function mapping. This mapping allows the addition
/// of control points, and allows the user to control the function between
/// the control points. A piecewise hermite curve is used between control
/// points, based on the sharpness and midpoint parameters. A sharpness of
/// 0 yields a piecewise linear function and a sharpness of 1 yields a
/// piecewise constant function. The midpoint is the normalized distance
/// between control points at which the curve reaches the median Y value.
/// The midpoint and sharpness values specified when adding a node are used
/// to control the transition to the next node with the last node's values being
/// ignored.
///
/// When adding opacity nodes without an explicit midpoint and sharpness we
/// will default to to Midpoint = 0.5 (halfway between the control points) and
/// Sharpness = 0.0 (linear).
///
/// ColorTable also contains which ColorSpace should be used for interpolation.
/// The color space is selected with the `viskores::ColorSpace` enumeration.
/// Currently the valid ColorSpaces are:
/// - `RGB`
/// - `HSV`
/// - `HSVWrap`
/// - `Lab`
/// - `Diverging`
///
/// In `HSVWrap` mode, it will take the shortest path
/// in Hue (going back through 0 if that is the shortest way around the hue
/// circle) whereas HSV will not go through 0 (in order to
/// match the current functionality of `vtkLookupTable`). In `Lab` mode,
/// it will take the shortest path in the Lab color space with respect to the
/// CIE Delta E 2000 color distance measure. `Diverging` is a special
/// mode where colors will pass through white when interpolating between two
/// saturated colors.
///
/// To map a field from a `viskores::cont::DataSet` through the color and opacity transfer
/// functions and into a RGB or RGBA array you should use `viskores::filter::FieldToColor`.
///
/// Note that modifications of `viskores::cont::ColorTable` are not thread safe. You should
/// not modify a `ColorTable` simultaneously in 2 or more threads. Also, you should not
/// modify a `ColorTable` that might be used in the execution environment. However,
/// the `ColorTable` can be used in multiple threads and on multiple devices as long
/// as no modifications are made.
///
class VISKORES_CONT_EXPORT ColorTable : public viskores::cont::ExecutionObjectBase
{
  std::shared_ptr<detail::ColorTableInternals> Internals;

public:
  enum struct Preset
  {
    Default,
    CoolToWarm,
    CoolToWarmExtended,
    Viridis,
    Inferno,
    Plasma,
    BlackBodyRadiation,
    XRay,
    Green,
    BlackBlueWhite,
    BlueToOrange,
    GrayToRed,
    ColdAndHot,
    BlueGreenOrange,
    YellowGrayBlue,
    RainbowUniform,
    Jet,
    RainbowDesaturated,
  };

  /// \brief Construct a color table from a preset
  ///
  /// Constructs a color table from a given preset, which might include a NaN color.
  /// The alpha table will have 2 entries of alpha = 1.0 with linear interpolation
  ///
  /// Note: these are a select set of the presets you can get by providing a string identifier.
  ///
  ColorTable(
    viskores::cont::ColorTable::Preset preset = viskores::cont::ColorTable::Preset::Default);

  /// \brief Construct a color table from a preset color table
  ///
  /// Constructs a color table from a given preset, which might include a NaN color.
  /// The alpha table will have 2 entries of alpha = 1.0 with linear interpolation
  ///
  /// Note: Names are case insensitive
  /// Currently supports the following color tables:
  ///
  /// "Default"
  /// "Cool to Warm"
  /// "Cool to Warm Extended"
  /// "Viridis"
  /// "Inferno"
  /// "Plasma"
  /// "Black-Body Radiation"
  /// "X Ray"
  /// "Green"
  /// "Black - Blue - White"
  /// "Blue to Orange"
  /// "Gray to Red"
  /// "Cold and Hot"
  /// "Blue - Green - Orange"
  /// "Yellow - Gray - Blue"
  /// "Rainbow Uniform"
  /// "Jet"
  /// "Rainbow Desaturated"
  ///
  explicit ColorTable(const std::string& name);

  /// Construct a color table with a zero positions, and an invalid range
  ///
  /// Note: The color table will have 0 entries
  /// Note: The alpha table will have 0 entries
  explicit ColorTable(viskores::ColorSpace space);

  /// Construct a color table with a 2 positions
  ///
  /// Note: The color table will have 2 entries of rgb = {1.0,1.0,1.0}
  /// Note: The alpha table will have 2 entries of alpha = 1.0 with linear
  ///       interpolation
  ColorTable(const viskores::Range& range, viskores::ColorSpace space = viskores::ColorSpace::Lab);

  /// Construct a color table with 2 positions
  //
  /// Note: The alpha table will have 2 entries of alpha = 1.0 with linear
  ///       interpolation
  ColorTable(const viskores::Range& range,
             const viskores::Vec3f_32& rgb1,
             const viskores::Vec3f_32& rgb2,
             viskores::ColorSpace space = viskores::ColorSpace::Lab);

  /// Construct color and alpha and table with 2 positions
  ///
  /// Note: The alpha table will use linear interpolation
  ColorTable(const viskores::Range& range,
             const viskores::Vec4f_32& rgba1,
             const viskores::Vec4f_32& rgba2,
             viskores::ColorSpace space = viskores::ColorSpace::Lab);

  /// Construct a color table with a list of colors and alphas. For this version you must also
  /// specify a name.
  ///
  /// This constructor is mostly used for presets.
  ColorTable(
    const std::string& name,
    viskores::ColorSpace colorSpace,
    const viskores::Vec3f_64& nanColor,
    const std::vector<viskores::Float64>& rgbPoints,
    const std::vector<viskores::Float64>& alphaPoints = { 0.0, 1.0, 0.5, 0.0, 1.0, 1.0, 0.5, 0.0 });


  ~ColorTable();

  ColorTable& operator=(const ColorTable&) = default;
  ColorTable(const ColorTable&) = default;

  const std::string& GetName() const;
  void SetName(const std::string& name);

  bool LoadPreset(viskores::cont::ColorTable::Preset preset);

  /// Returns the name of all preset color tables
  ///
  /// This list will include all presets defined in viskores::cont::ColorTable::Preset and could
  /// include extras as well.
  ///
  static std::set<std::string> GetPresets();

  /// Load a preset color table
  ///
  /// Removes all existing all values in both color and alpha tables,
  /// and will reset the NaN Color if the color table has that information.
  /// Will not modify clamping, below, and above range state.
  ///
  /// Note: Names are case insensitive
  ///
  /// Currently supports the following color tables:
  /// "Default"
  /// "Cool to Warm"
  /// "Cool to Warm Extended"
  /// "Viridis"
  /// "Inferno"
  /// "Plasma"
  /// "Black-Body Radiation"
  /// "X Ray"
  /// "Green"
  /// "Black - Blue - White"
  /// "Blue to Orange"
  /// "Gray to Red"
  /// "Cold and Hot"
  /// "Blue - Green - Orange"
  /// "Yellow - Gray - Blue"
  /// "Rainbow Uniform"
  /// "Jet"
  /// "Rainbow Desaturated"
  ///
  bool LoadPreset(const std::string& name);

  /// Make a deep copy of the current color table
  ///
  /// The ColorTable is implemented so that all stack based copies are 'shallow'
  /// copies. This means that they all alter the same internal instance. But
  /// sometimes you need to make an actual fully independent copy.
  ColorTable MakeDeepCopy();

  ///
  viskores::ColorSpace GetColorSpace() const;
  void SetColorSpace(viskores::ColorSpace space);

  /// If clamping is disabled values that lay out side
  /// the color table range are colored based on Below
  /// and Above settings.
  ///
  /// By default clamping is enabled
  void SetClampingOn() { this->SetClamping(true); }
  void SetClampingOff() { this->SetClamping(false); }
  void SetClamping(bool state);
  bool GetClamping() const;

  /// Color to use when clamping is disabled for any value
  /// that is below the given range
  ///
  /// Default value is {0,0,0}
  void SetBelowRangeColor(const viskores::Vec3f_32& c);
  const viskores::Vec3f_32& GetBelowRangeColor() const;

  /// Color to use when clamping is disabled for any value
  /// that is above the given range
  ///
  /// Default value is {0,0,0}
  void SetAboveRangeColor(const viskores::Vec3f_32& c);
  const viskores::Vec3f_32& GetAboveRangeColor() const;

  ///
  void SetNaNColor(const viskores::Vec3f_32& c);
  const viskores::Vec3f_32& GetNaNColor() const;

  /// Remove all existing values in both color and alpha tables.
  /// Does not remove the clamping, below, and above range state or colors.
  void Clear();

  /// Remove only color table values
  void ClearColors();

  /// Remove only alpha table values
  void ClearAlpha();

  /// Reverse the rgb values inside the color table
  void ReverseColors();

  /// Reverse the alpha, mid, and sharp values inside the opacity table.
  ///
  /// Note: To keep the shape correct the mid and sharp values of the last
  /// node are not included in the reversal
  void ReverseAlpha();

  /// Returns min and max position of all function points
  const viskores::Range& GetRange() const;

  /// Rescale the color and opacity transfer functions to match the
  /// input range.
  void RescaleToRange(const viskores::Range& range);

  // Functions for Colors

  /// Adds a point to the color function. If the point already exists, it
  /// will be updated to the new value.
  ///
  /// Note: rgb values need to be between 0 and 1.0 (inclusive).
  /// Return the index of the point (0 based), or -1 osn error.
  viskores::Int32 AddPoint(viskores::Float64 x, const viskores::Vec3f_32& rgb);

  /// Adds a point to the color function. If the point already exists, it
  /// will be updated to the new value.
  ///
  /// Note: hsv values need to be between 0 and 1.0 (inclusive).
  /// Return the index of the point (0 based), or -1 on error.
  viskores::Int32 AddPointHSV(viskores::Float64 x, const viskores::Vec3f_32& hsv);

  /// Add a line segment to the color function. All points which lay between x1 and x2
  /// (inclusive) are removed from the function.
  ///
  /// Note: rgb1, and rgb2 values need to be between 0 and 1.0 (inclusive).
  /// Return the index of the point x1 (0 based), or -1 on error.
  viskores::Int32 AddSegment(viskores::Float64 x1,
                             const viskores::Vec3f_32& rgb1,
                             viskores::Float64 x2,
                             const viskores::Vec3f_32& rgb2);

  /// Add a line segment to the color function. All points which lay between x1 and x2
  /// (inclusive) are removed from the function.
  ///
  /// Note: hsv1, and hsv2 values need to be between 0 and 1.0 (inclusive)
  /// Return the index of the point x1 (0 based), or -1 on error
  viskores::Int32 AddSegmentHSV(viskores::Float64 x1,
                                const viskores::Vec3f_32& hsv1,
                                viskores::Float64 x2,
                                const viskores::Vec3f_32& hsv2);

  /// Get the location, and rgb information for an existing point in the opacity function.
  ///
  /// Note: components 1-3 are rgb and will have values between 0 and 1.0 (inclusive)
  /// Return the index of the point (0 based), or -1 on error.
  bool GetPoint(viskores::Int32 index, viskores::Vec4f_64&) const;

  /// Update the location, and rgb information for an existing point in the color function.
  /// If the location value for the index is modified the point is removed from
  /// the function and re-inserted in the proper sorted location.
  ///
  /// Note: components 1-3 are rgb and must have values between 0 and 1.0 (inclusive).
  /// Return the new index of the updated point (0 based), or -1 on error.
  viskores::Int32 UpdatePoint(viskores::Int32 index, const viskores::Vec4f_64&);

  /// Remove the Color function point that exists at exactly x
  ///
  /// Return true if the point x exists and has been removed
  bool RemovePoint(viskores::Float64 x);

  /// Remove the Color function point n
  ///
  /// Return true if n >= 0 && n < GetNumberOfPoints
  bool RemovePoint(viskores::Int32 index);

  /// Returns the number of points in the color function
  viskores::Int32 GetNumberOfPoints() const;

  // Functions for Opacity

  /// Adds a point to the opacity function. If the point already exists, it
  /// will be updated to the new value. Uses a midpoint of 0.5 (halfway between the control points)
  /// and sharpness of 0.0 (linear).
  ///
  /// Note: alpha needs to be a value between 0 and 1.0 (inclusive).
  /// Return the index of the point (0 based), or -1 on error.
  viskores::Int32 AddPointAlpha(viskores::Float64 x, viskores::Float32 alpha)
  {
    return AddPointAlpha(x, alpha, 0.5f, 0.0f);
  }

  /// Adds a point to the opacity function. If the point already exists, it
  /// will be updated to the new value.
  ///
  /// Note: alpha, midpoint, and sharpness values need to be between 0 and 1.0 (inclusive)
  /// Return the index of the point (0 based), or -1 on error.
  viskores::Int32 AddPointAlpha(viskores::Float64 x,
                                viskores::Float32 alpha,
                                viskores::Float32 midpoint,
                                viskores::Float32 sharpness);

  /// Add a line segment to the opacity function. All points which lay between x1 and x2
  /// (inclusive) are removed from the function. Uses a midpoint of
  /// 0.5 (halfway between the control points) and sharpness of 0.0 (linear).
  ///
  /// Note: alpha values need to be between 0 and 1.0 (inclusive)
  /// Return the index of the point x1 (0 based), or -1 on error
  viskores::Int32 AddSegmentAlpha(viskores::Float64 x1,
                                  viskores::Float32 alpha1,
                                  viskores::Float64 x2,
                                  viskores::Float32 alpha2)
  {
    viskores::Vec2f_32 mid_sharp(0.5f, 0.0f);
    return AddSegmentAlpha(x1, alpha1, x2, alpha2, mid_sharp, mid_sharp);
  }

  /// Add a line segment to the opacity function. All points which lay between x1 and x2
  /// (inclusive) are removed from the function.
  ///
  /// Note: alpha, midpoint, and sharpness values need to be between 0 and 1.0 (inclusive)
  /// Return the index of the point x1 (0 based), or -1 on error
  viskores::Int32 AddSegmentAlpha(viskores::Float64 x1,
                                  viskores::Float32 alpha1,
                                  viskores::Float64 x2,
                                  viskores::Float32 alpha2,
                                  const viskores::Vec2f_32& mid_sharp1,
                                  const viskores::Vec2f_32& mid_sharp2);

  /// Get the location, alpha, midpoint and sharpness information for an existing
  /// point in the opacity function.
  ///
  /// Note: alpha, midpoint, and sharpness values all will be between 0 and 1.0 (inclusive)
  /// Return the index of the point (0 based), or -1 on error.
  bool GetPointAlpha(viskores::Int32 index, viskores::Vec4f_64&) const;

  /// Update the location, alpha, midpoint and sharpness information for an existing
  /// point in the opacity function.
  /// If the location value for the index is modified the point is removed from
  /// the function and re-inserted in the proper sorted location
  ///
  /// Note: alpha, midpoint, and sharpness values need to be between 0 and 1.0 (inclusive)
  /// Return the new index of the updated point (0 based), or -1 on error.
  viskores::Int32 UpdatePointAlpha(viskores::Int32 index, const viskores::Vec4f_64&);

  /// Remove the Opacity function point that exists at exactly x
  ///
  /// Return true if the point x exists and has been removed
  bool RemovePointAlpha(viskores::Float64 x);

  /// Remove the Opacity function point n
  ///
  /// Return true if n >= 0 && n < GetNumberOfPointsAlpha
  bool RemovePointAlpha(viskores::Int32 index);

  /// Returns the number of points in the alpha function
  viskores::Int32 GetNumberOfPointsAlpha() const;

  /// Fill the Color table from a viskores::Float64 pointer
  ///
  /// The viskores::Float64 pointer is required to have the layout out of [X1, R1,
  /// G1, B1, X2, R2, G2, B2, ..., Xn, Rn, Gn, Bn] where n is the
  /// number of nodes.
  /// This will remove any existing color control points.
  ///
  /// Note: n represents the length of the array, so ( n/4 == number of control points )
  ///
  /// Note: This is provided as a interoperability method with VTK
  /// Will return false and not modify anything if n is <= 0 or ptr == nullptr
  bool FillColorTableFromDataPointer(viskores::Int32 n, const viskores::Float64* ptr);

  /// Fill the Color table from a viskores::Float32 pointer
  ///
  /// The viskores::Float64 pointer is required to have the layout out of [X1, R1,
  /// G1, B1, X2, R2, G2, B2, ..., Xn, Rn, Gn, Bn] where n is the
  /// number of nodes.
  /// This will remove any existing color control points.
  ///
  /// Note: n represents the length of the array, so ( n/4 == number of control points )
  ///
  /// Note: This is provided as a interoperability method with VTK
  /// Will return false and not modify anything if n is <= 0 or ptr == nullptr
  bool FillColorTableFromDataPointer(viskores::Int32 n, const viskores::Float32* ptr);

  /// Fill the Opacity table from a viskores::Float64 pointer
  ///
  /// The viskores::Float64 pointer is required to have the layout out of [X1, A1, M1, S1, X2, A2, M2, S2,
  /// ..., Xn, An, Mn, Sn] where n is the number of nodes. The Xi values represent the value to
  /// map, the Ai values represent alpha (opacity) value, the Mi values represent midpoints, and
  /// the Si values represent sharpness. Use 0.5 for midpoint and 0.0 for sharpness to have linear
  /// interpolation of the alpha.
  ///
  /// This will remove any existing opacity control points.
  ///
  /// Note: n represents the length of the array, so ( n/4 == number of control points )
  ///
  /// Will return false and not modify anything if n is <= 0 or ptr == nullptr
  bool FillOpacityTableFromDataPointer(viskores::Int32 n, const viskores::Float64* ptr);

  /// Fill the Opacity table from a viskores::Float32 pointer
  ///
  /// The viskores::Float32 pointer is required to have the layout out of [X1, A1, M1, S1, X2, A2, M2, S2,
  /// ..., Xn, An, Mn, Sn] where n is the number of nodes. The Xi values represent the value to
  /// map, the Ai values represent alpha (opacity) value, the Mi values represent midpoints, and
  /// the Si values represent sharpness. Use 0.5 for midpoint and 0.0 for sharpness to have linear
  /// interpolation of the alpha.
  ///
  /// This will remove any existing opacity control points.
  ///
  /// Note: n represents the length of the array, so ( n/4 == number of control points )
  ///
  /// Will return false and not modify anything if n is <= 0 or ptr == nullptr
  bool FillOpacityTableFromDataPointer(viskores::Int32 n, const viskores::Float32* ptr);


  /// \brief generate RGB colors using regular spaced samples along the range.
  ///
  /// Will use the current range of the color table to generate evenly spaced
  /// values using either viskores::Float32 or viskores::Float64 space.
  /// Will use viskores::Float32 space when the difference between the viskores::Float32 and viskores::Float64
  /// values when the range is within viskores::Float32 space and the following are within a tolerance:
  ///
  /// - (max-min) / numSamples
  /// - ((max-min) / numSamples) * numSamples
  ///
  /// Note: This will return false if the number of samples is less than 2
  bool Sample(viskores::Int32 numSamples,
              viskores::cont::ColorTableSamplesRGBA& samples,
              viskores::Float64 tolerance = 0.002) const;

  /// \brief generate a sample lookup table using regular spaced samples along the range.
  ///
  /// Will use the current range of the color table to generate evenly spaced
  /// values using either viskores::Float32 or viskores::Float64 space.
  /// Will use viskores::Float32 space when the difference between the viskores::Float32 and viskores::Float64
  /// values when the range is within viskores::Float32 space and the following are within a tolerance:
  ///
  /// - (max-min) / numSamples
  /// - ((max-min) / numSamples) * numSamples
  ///
  /// Note: This will return false if the number of samples is less than 2
  bool Sample(viskores::Int32 numSamples,
              viskores::cont::ColorTableSamplesRGB& samples,
              viskores::Float64 tolerance = 0.002) const;

  /// \brief generate RGBA colors using regular spaced samples along the range.
  ///
  /// Will use the current range of the color table to generate evenly spaced
  /// values using either viskores::Float32 or viskores::Float64 space.
  /// Will use viskores::Float32 space when the difference between the viskores::Float32 and viskores::Float64
  /// values when the range is within viskores::Float32 space and the following are within a tolerance:
  ///
  /// - (max-min) / numSamples
  /// - ((max-min) / numSamples) * numSamples
  ///
  /// Note: This will return false if the number of samples is less than 2
  bool Sample(viskores::Int32 numSamples,
              viskores::cont::ArrayHandle<viskores::Vec4ui_8>& colors,
              viskores::Float64 tolerance = 0.002) const;

  /// \brief generate RGB colors using regular spaced samples along the range.
  ///
  /// Will use the current range of the color table to generate evenly spaced
  /// values using either viskores::Float32 or viskores::Float64 space.
  /// Will use viskores::Float32 space when the difference between the viskores::Float32 and viskores::Float64
  /// values when the range is within viskores::Float32 space and the following are within a tolerance:
  ///
  /// - (max-min) / numSamples
  /// - ((max-min) / numSamples) * numSamples
  ///
  /// Note: This will return false if the number of samples is less than 2
  bool Sample(viskores::Int32 numSamples,
              viskores::cont::ArrayHandle<viskores::Vec3ui_8>& colors,
              viskores::Float64 tolerance = 0.002) const;


  /// \brief returns a virtual object pointer of the exec color table
  ///
  /// This pointer is only valid as long as the ColorTable is unmodified
  viskores::exec::ColorTable PrepareForExecution(viskores::cont::DeviceAdapterId deviceId,
                                                 viskores::cont::Token& token) const;

  /// \brief Returns the modified count for changes of the color table
  ///
  /// The `ModifiedCount` of the color table starts at 1 and gets incremented
  /// every time a change is made to the color table.
  /// The modified count allows consumers of a shared color table to keep track
  /// if the color table has been modified since the last time they used it.
  /// This is important for consumers that need to sample the color table.
  /// You only want to resample the color table if changes have been made.
  viskores::Id GetModifiedCount() const;

private:
  void UpdateArrayHandles() const;
};
}
} //namespace viskores::cont
#endif //viskores_cont_ColorTable_h
