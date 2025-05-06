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

#include <viskores/cont/DataSetBuilderExplicit.h>

namespace viskores
{
namespace cont
{

VISKORES_CONT
DataSetBuilderExplicitIterative::DataSetBuilderExplicitIterative() {}


VISKORES_CONT
void DataSetBuilderExplicitIterative::Begin(const std::string& coordName)
{
  this->coordNm = coordName;
  this->points.resize(0);
  this->shapes.resize(0);
  this->numIdx.resize(0);
  this->connectivity.resize(0);
}

//Define points.
VISKORES_CONT
viskores::cont::DataSet DataSetBuilderExplicitIterative::Create()
{
  DataSetBuilderExplicit dsb;
  return dsb.Create(points, shapes, numIdx, connectivity, coordNm);
}

VISKORES_CONT
viskores::Id DataSetBuilderExplicitIterative::AddPoint(const viskores::Vec3f& pt)
{
  points.push_back(pt);
  viskores::Id id = static_cast<viskores::Id>(points.size());
  //ID is zero-based.
  return id - 1;
}

VISKORES_CONT
viskores::Id DataSetBuilderExplicitIterative::AddPoint(const viskores::FloatDefault& x,
                                                       const viskores::FloatDefault& y,
                                                       const viskores::FloatDefault& z)
{
  points.push_back(viskores::make_Vec(x, y, z));
  viskores::Id id = static_cast<viskores::Id>(points.size());
  //ID is zero-based.
  return id - 1;
}

//Define cells.
VISKORES_CONT
void DataSetBuilderExplicitIterative::AddCell(viskores::UInt8 shape)
{
  this->shapes.push_back(shape);
  this->numIdx.push_back(0);
}

VISKORES_CONT
void DataSetBuilderExplicitIterative::AddCell(const viskores::UInt8& shape,
                                              const std::vector<viskores::Id>& conn)
{
  this->shapes.push_back(shape);
  this->numIdx.push_back(static_cast<viskores::IdComponent>(conn.size()));
  connectivity.insert(connectivity.end(), conn.begin(), conn.end());
}

VISKORES_CONT
void DataSetBuilderExplicitIterative::AddCell(const viskores::UInt8& shape,
                                              const viskores::Id* conn,
                                              const viskores::IdComponent& n)
{
  this->shapes.push_back(shape);
  this->numIdx.push_back(n);
  for (int i = 0; i < n; i++)
  {
    connectivity.push_back(conn[i]);
  }
}

VISKORES_CONT
void DataSetBuilderExplicitIterative::AddCellPoint(viskores::Id pointIndex)
{
  VISKORES_ASSERT(this->numIdx.size() > 0);
  this->connectivity.push_back(pointIndex);
  this->numIdx.back() += 1;
}
}
} // end namespace viskores::cont
