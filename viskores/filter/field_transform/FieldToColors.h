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

#ifndef viskores_filter_field_transform_FieldToColors_h
#define viskores_filter_field_transform_FieldToColors_h

#include <viskores/cont/ColorTable.h>
#include <viskores/filter/Filter.h>
#include <viskores/filter/field_transform/viskores_filter_field_transform_export.h>

namespace viskores
{
namespace filter
{
namespace field_transform
{
/// \brief Convert an arbitrary field to an RGB or RGBA field.
///
/// This filter is useful for generating colors that could be used for rendering or
/// other purposes.
///
class VISKORES_FILTER_FIELD_TRANSFORM_EXPORT FieldToColors : public viskores::filter::Filter
{
public:
  VISKORES_CONT
  explicit FieldToColors(const viskores::cont::ColorTable& table = viskores::cont::ColorTable());

  // Documentation of enumerations is behaving a little weird in Doxygen (version 1.9.7).
  // You cannot have a blank line in the documentation. Everything below it will be treated
  // as preformatted text. Also, the first line always seem to behave like `@brief` is used
  // even when it is not. It's easier to just document an associated method and copy the
  // documentation

  /// @brief Identifiers used to specify how `FieldToColors` should treat its input scalars.
  enum struct InputMode
  {
    /// @copydoc FieldToColors::SetMappingToScalar
    Scalar,
    /// @copydoc FieldToColors::SetMappingToMagnitude
    Magnitude,
    /// @copydoc FieldToColors::SetMappingToComponent
    Component,
  };

  /// @brief Identifiers used to specify what output `FieldToColors` will generate.
  enum struct OutputMode
  {
    /// @copydoc FieldToColors::SetOutputToRGB()
    RGB,
    /// @copydoc FieldToColors::SetOutputToRGBA()
    RGBA,
  };

  /// @brief Specifies the `viskores::cont::ColorTable` object to use to map field values to colors.
  void SetColorTable(const viskores::cont::ColorTable& table)
  {
    this->Table = table;
    this->ModifiedCount = -1;
  }
  /// @copydoc SetColorTable
  const viskores::cont::ColorTable& GetColorTable() const { return this->Table; }

  /// @brief Specify the mapping mode.
  void SetMappingMode(InputMode mode) { this->InputModeType = mode; }
  /// @brief Treat the field as a scalar field.
  ///
  /// It is an error to provide a field of any type that cannot be directly converted
  /// to a basic floating point number (such as a vector).
  void SetMappingToScalar() { this->InputModeType = InputMode::Scalar; }
  /// @brief Map the magnitude of the field.
  ///
  /// Given a vector field, the magnitude of each field value is taken before looking it up
  /// in the color table.
  void SetMappingToMagnitude() { this->InputModeType = InputMode::Magnitude; }
  /// @brief Map a component of a vector field as if it were a scalar.
  ///
  /// Given a vector field, a particular component is looked up in the color table as if
  /// that component were in a scalar field. The component to map is selected with
  /// `SetMappingComponent()`.
  void SetMappingToComponent() { this->InputModeType = InputMode::Component; }
  /// @brief Specify the mapping mode.
  InputMode GetMappingMode() const { return this->InputModeType; }
  /// @brief Returns true if this filter is in scalar mapping mode.
  bool IsMappingScalar() const { return this->InputModeType == InputMode::Scalar; }
  /// @brief Returns true if this filter is in magnitude mapping mode.
  bool IsMappingMagnitude() const { return this->InputModeType == InputMode::Magnitude; }
  /// @brief Returns true if this filter is vector component mapping mode.
  bool IsMappingComponent() const { return this->InputModeType == InputMode::Component; }

  /// @brief Specifies the component of the vector to use in the mapping.
  ///
  /// This only has an effect if the input mapping mode is set to
  /// `FieldToColors::InputMode::Component`.
  void SetMappingComponent(viskores::IdComponent comp) { this->Component = comp; }
  /// @copydoc SetMappingComponent
  viskores::IdComponent GetMappingComponent() const { return this->Component; }

  /// @brief Specify the output mode.
  void SetOutputMode(OutputMode mode) { this->OutputModeType = mode; }
  /// @brief Write out RGB fixed precision color values.
  ///
  /// Output colors are represented as RGB values with each component represented by an
  /// unsigned byte. Specifically, these are `viskores::Vec3ui_8` values.
  void SetOutputToRGB() { this->OutputModeType = OutputMode::RGB; }
  /// @brief Write out RGBA fixed precision color values.
  ///
  /// Output colors are represented as RGBA values with each component represented by an
  /// unsigned byte. Specifically, these are `viskores::Vec4ui_8` values.
  void SetOutputToRGBA() { this->OutputModeType = OutputMode::RGBA; }
  /// @brief Specify the output mode.
  OutputMode GetOutputMode() const { return this->OutputModeType; }
  /// @brief Returns true if this filter is in RGB output mode.
  bool IsOutputRGB() const { return this->OutputModeType == OutputMode::RGB; }
  /// @brief Returns true if this filter is in RGBA output mode.
  bool IsOutputRGBA() const { return this->OutputModeType == OutputMode::RGBA; }

  /// @brief Specifies how many samples to use when looking up color values.
  ///
  /// The implementation of `FieldToColors` first builds an array of color samples to quickly
  /// look up colors for particular values. The size of this lookup array can be adjusted with
  /// this parameter. By default, an array of 256 colors is used.
  void SetNumberOfSamplingPoints(viskores::Int32 count);
  /// @copydoc SetNumberOfSamplingPoints
  viskores::Int32 GetNumberOfSamplingPoints() const { return this->SampleCount; }

private:
  VISKORES_CONT viskores::cont::DataSet DoExecute(const viskores::cont::DataSet& input) override;

  viskores::cont::ColorTable Table;
  InputMode InputModeType = InputMode::Scalar;
  OutputMode OutputModeType = OutputMode::RGBA;
  viskores::cont::ColorTableSamplesRGB SamplesRGB;
  viskores::cont::ColorTableSamplesRGBA SamplesRGBA;
  viskores::IdComponent Component = 0;
  viskores::Int32 SampleCount = 256;
  viskores::Id ModifiedCount = -1;
};

} // namespace field_transform
} // namespace filter
} // namespace viskores

#endif // viskores_filter_field_transform_FieldToColors_h
