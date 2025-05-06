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

#ifndef viskores_filter_entity_extraction_ExtractStructured_h
#define viskores_filter_entity_extraction_ExtractStructured_h

#include <viskores/RangeId3.h>
#include <viskores/filter/Filter.h>
#include <viskores/filter/entity_extraction/viskores_filter_entity_extraction_export.h>

namespace viskores
{
namespace filter
{
namespace entity_extraction
{
/// \brief Select a piece (e.g., volume of interest) and/or subsample structured points dataset
///
/// Select or subsample a portion of an input structured dataset. The selected
/// portion of interested is referred to as the Volume Of Interest, or VOI.
/// The output of this filter is a structured dataset. The filter treats input
/// data of any topological dimension (i.e., point, line, plane, or volume) and
/// can generate output data of any topological dimension.
///
/// To use this filter set the VOI ivar which are i-j-k min/max indices that
/// specify a rectangular region in the data. (Note that these are 0-offset.)
/// You can also specify a sampling rate to subsample the data.
///
/// Typical applications of this filter are to extract a slice from a volume
/// for image processing, subsampling large volumes to reduce data size, or
/// extracting regions of a volume with interesting data.
///
class VISKORES_FILTER_ENTITY_EXTRACTION_EXPORT ExtractStructured : public viskores::filter::Filter
{
public:
  /// @brief Specifies what volume of interest (VOI) should be extracted by the filter.
  ///
  /// The VOI is specified using the 3D indices of the structured mesh. Meshes with fewer
  /// than 3 dimensions will ignore the extra dimensions in the VOI. The VOI is inclusive
  /// on the minium index and exclusive on the maximum index.
  ///
  /// By default the VOI is the entire input.
  VISKORES_CONT
  viskores::RangeId3 GetVOI() const { return this->VOI; }

  /// @copydoc GetVOI
  VISKORES_CONT
  void SetVOI(viskores::Id i0,
              viskores::Id i1,
              viskores::Id j0,
              viskores::Id j1,
              viskores::Id k0,
              viskores::Id k1)
  {
    this->VOI = viskores::RangeId3(i0, i1, j0, j1, k0, k1);
  }
  /// @copydoc GetVOI
  VISKORES_CONT
  void SetVOI(viskores::Id extents[6]) { this->VOI = viskores::RangeId3(extents); }
  /// @copydoc GetVOI
  VISKORES_CONT
  void SetVOI(viskores::Id3 minPoint, viskores::Id3 maxPoint)
  {
    this->VOI = viskores::RangeId3(minPoint, maxPoint);
  }
  /// @copydoc GetVOI
  VISKORES_CONT
  void SetVOI(const viskores::RangeId3& voi) { this->VOI = voi; }

  /// @brief Specifies the sample rate of the VOI.
  ///
  /// The input data can be subsampled by selecting every n-th value.
  /// The sampling can be different in each dimension.
  /// The default sampling rate is (1,1,1), meaning that no subsampling will occur.
  VISKORES_CONT
  viskores::Id3 GetSampleRate() const { return this->SampleRate; }

  /// @copydoc GetSampleRate
  VISKORES_CONT
  void SetSampleRate(viskores::Id i, viskores::Id j, viskores::Id k)
  {
    this->SampleRate = viskores::Id3(i, j, k);
  }

  /// @copydoc GetSampleRate
  VISKORES_CONT
  void SetSampleRate(viskores::Id3 sampleRate) { this->SampleRate = sampleRate; }

  /// @brief Specifies if the outer boundary should always be included.
  ///
  /// When a subsample rate is specified, it is possible that some of the
  /// boundary samples will not be included in the sampling. If this is the
  /// case and `IncludeBoundary` is set to true, then an extra sample is
  /// set in the output and the values on the boundary are included. For example,
  /// say the input has resolution (5, 5, 1) (and the VOI matches), and the sample
  /// rate is set to (3, 3, 1). If `IncludeBoundary` is false, then the output will
  /// have the 4 points that correspond to the 3D indices (0, 0, 0), (3, 0, 0),
  /// (0, 3, 0), and (3, 3, 0) of the input. This misses the outer boundary at
  /// index 4 in the x and y directions. If `IncludeBoundary is set to true, then
  /// the output will have the 9 points that correspond to the 3D indices (0, 0, 0),
  /// (3, 0, 0), (4, 0, 0), (0, 3, 0), (3, 3, 0), (4, 3, 0), (0, 4, 0), (3, 4, 0),
  /// and (4, 4, 0) to capture this outer boundary.
  VISKORES_CONT
  bool GetIncludeBoundary() const { return this->IncludeBoundary; }
  /// @copydoc GetIncludeBoundary
  VISKORES_CONT
  void SetIncludeBoundary(bool value) { this->IncludeBoundary = value; }

  // Set if VOI is specified in global (rather than in local) point indices
  // (NOTE: Deprecated this method since this does not seem to work as
  // expected and there are no tests for it. Furthermore, neither Viskores nor
  // VTK-h/Ascent seem to use this method. If your are using this method
  // somewhere else and think it should remain, please open a merge request to
  // "de-deprecate" it and add a test and documentation of the expected
  // behavior.)
  VISKORES_DEPRECATED(2.1)
  VISKORES_CONT
  void SetIncludeOffset(bool value) { this->IncludeOffset = value; }

private:
  VISKORES_CONT
  viskores::cont::DataSet DoExecute(const viskores::cont::DataSet& input) override;

  viskores::RangeId3 VOI = viskores::RangeId3(0, -1, 0, -1, 0, -1);
  viskores::Id3 SampleRate = { 1, 1, 1 };
  bool IncludeBoundary = false;
  bool IncludeOffset = false;
};

} // namespace entity_extraction
} // namespace filter
} // namespace viskores

#endif // viskores_filter_entity_extraction_ExtractStructured_h
