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
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#ifndef viskores_filter_mesh_info_MeshQualityWarpage_h
#define viskores_filter_mesh_info_MeshQualityWarpage_h

#include <viskores/filter/Filter.h>
#include <viskores/filter/mesh_info/viskores_filter_mesh_info_export.h>

namespace viskores
{
namespace filter
{
namespace mesh_info
{

/// @brief Compute the flatness of cells.
///
/// This only produces values for quadrilaterals. It is defined as the cosine of the minimum
/// dihedral angle formed by the planes intersecting in diagonals (to the fourth power).
///
/// This metric will be 1 for a perfectly flat quadrilateral and be lower as the
/// quadrilateral deviates from the plane. A good quality quadrilateral will have a
/// value in the range [0.3, 1]. Poorer quality cells having lower values down to -1,
/// although malformed cells might have an infinite value.
///
/// Note that the value of this filter is consistent with the equivalent metric in VisIt,
/// and it differs from the implementation in the Verdict library. The Verdict library
/// returns 1 - value.
class VISKORES_FILTER_MESH_INFO_EXPORT MeshQualityWarpage : public viskores::filter::Filter
{
public:
  MeshQualityWarpage();

private:
  viskores::cont::DataSet DoExecute(const viskores::cont::DataSet& input) override;
};

} // namespace mesh_info
} // namespace filter
} // namespace viskores

#endif //viskores_filter_mesh_info_MeshQualityWarpage_h
