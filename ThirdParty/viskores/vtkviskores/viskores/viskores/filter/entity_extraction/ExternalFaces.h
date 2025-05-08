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

#ifndef viskores_filter_entity_extraction_ExternalFaces_h
#define viskores_filter_entity_extraction_ExternalFaces_h

#include <viskores/filter/Filter.h>
#include <viskores/filter/entity_extraction/viskores_filter_entity_extraction_export.h>

namespace viskores
{
namespace worklet
{
struct ExternalFaces;
}
namespace filter
{
namespace entity_extraction
{
/// @brief Extract external faces of a geometry.
///
/// `ExternalFaces` is a filter that extracts all external faces from a
/// data set. An external face is defined is defined as a face/side of a cell
/// that belongs only to one cell in the entire mesh.
///
class VISKORES_FILTER_ENTITY_EXTRACTION_EXPORT ExternalFaces : public viskores::filter::Filter
{
public:
  ExternalFaces();
  ~ExternalFaces() override;

  // New Design: I am too lazy to make this filter thread-safe. Let's use it as an example of
  // thread un-safe filter.
  bool CanThread() const override { return false; }

  /// @brief Option to remove unused points and compact result int a smaller array.
  ///
  /// When CompactPoints is on, instead of copying the points and point fields
  /// from the input, the filter will create new compact fields without the
  /// unused elements.
  /// When off (the default), unused points will remain listed in the topology,
  /// but point fields and coordinate systems will be shallow-copied to the output.
  VISKORES_CONT bool GetCompactPoints() const { return this->CompactPoints; }
  /// @copydoc GetCompactPoints
  VISKORES_CONT void SetCompactPoints(bool value) { this->CompactPoints = value; }

  /// @brief Specify how polygonal data (polygons, lines, and vertices) will be handled.
  ///
  /// When on (the default), these cells will be passed to the output.
  /// When off, these cells will be removed from the output. (Because they have less than 3
  /// topological dimensions, they are not considered to have any "faces.")
  VISKORES_CONT bool GetPassPolyData() const { return this->PassPolyData; }
  /// @copydoc GetPassPolyData
  VISKORES_CONT void SetPassPolyData(bool value);

private:
  VISKORES_CONT viskores::cont::DataSet DoExecute(const viskores::cont::DataSet& input) override;

  viskores::cont::DataSet GenerateOutput(const viskores::cont::DataSet& input,
                                         viskores::cont::UnknownCellSet& outCellSet);

  VISKORES_CONT bool MapFieldOntoOutput(viskores::cont::DataSet& result,
                                        const viskores::cont::Field& field);

  bool CompactPoints = false;
  bool PassPolyData = true;

  // Note: This shared state as a data member requires us to explicitly implement the
  // constructor and destructor in the .cxx file, after the compiler actually have
  // seen the definition of worklet:ExternalFaces, even if the implementation of
  // the cstr/dstr is just = default. Otherwise the compiler does not know how to
  // allocate/free storage for the std::unique_ptr.
  std::unique_ptr<viskores::worklet::ExternalFaces> Worklet;
};
} // namespace entity_extraction
} // namespace filter
} // namespace viskores

#endif // viskores_filter_entity_extraction_ExternalFaces_h
