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
#ifndef viskores_cont_DataSetBuilderCurvilinear_h
#define viskores_cont_DataSetBuilderCurvilinear_h

#include <viskores/cont/ArrayHandleSOA.h>
#include <viskores/cont/CoordinateSystem.h>
#include <viskores/cont/DataSet.h>

namespace viskores
{
namespace cont
{

class VISKORES_CONT_EXPORT DataSetBuilderCurvilinear
{
public:
  VISKORES_CONT
  DataSetBuilderCurvilinear();

  template <typename T>
  VISKORES_CONT static viskores::cont::DataSet Create(const std::vector<T>& xVals,
                                                      const std::string& coordsNm = "coords")
  {
    VISKORES_ASSERT(xVals.size() > 0);

    std::vector<T> yVals(xVals.size(), 0), zVals(xVals.size(), 0);
    viskores::Id dim = static_cast<viskores::Id>(xVals.size());
    auto coords = viskores::cont::make_ArrayHandleSOA<viskores::Vec<T, 3>>({ xVals, yVals, zVals });

    return DataSetBuilderCurvilinear::Create(coords, { dim, 0, 0 }, 1, coordsNm);
  }

  template <typename T>
  VISKORES_CONT static viskores::cont::DataSet Create(const std::vector<T>& xVals,
                                                      const std::vector<T>& yVals,
                                                      const viskores::Id2& dims,
                                                      const std::string& coordsNm = "coords")
  {
    VISKORES_ASSERT(xVals.size() > 0 && xVals.size() == yVals.size());

    std::vector<T> zVals(xVals.size(), 0);
    auto coords = viskores::cont::make_ArrayHandleSOA<viskores::Vec<T, 3>>({ xVals, yVals, zVals });

    return DataSetBuilderCurvilinear::Create(coords, { dims[0], dims[1], 0 }, 2, coordsNm);
  }

  template <typename T>
  VISKORES_CONT static viskores::cont::DataSet Create(const std::vector<T>& xVals,
                                                      const std::vector<T>& yVals,
                                                      const std::vector<T>& zVals,
                                                      const viskores::Id3& dims,
                                                      const std::string& coordsNm = "coords")
  {
    VISKORES_ASSERT(xVals.size() > 0 && xVals.size() == yVals.size());
    VISKORES_ASSERT(xVals.size() == zVals.size());

    auto coords = viskores::cont::make_ArrayHandleSOA<viskores::Vec<T, 3>>({ xVals, yVals, zVals });

    return DataSetBuilderCurvilinear::Create(coords, dims, 3, coordsNm);
  }

  template <typename T>
  VISKORES_CONT static viskores::cont::DataSet Create(
    const std::vector<viskores::Vec<T, 3>>& points,
    const viskores::Id3& dims,
    const std::string& coordsNm = "coords")
  {
    auto coords = viskores::cont::make_ArrayHandle(points);
    return DataSetBuilderCurvilinear::Create(coords, dims, 3, coordsNm);
  }

  template <typename CoordsType>
  VISKORES_CONT static viskores::cont::DataSet Create(const CoordsType& coords,
                                                      const viskores::Id3& dims,
                                                      const std::string& coordsNm = "coords")
  {
    return DataSetBuilderCurvilinear::Create(coords, dims, 3, coordsNm);
  }

  template <typename CoordsType>
  VISKORES_CONT static viskores::cont::DataSet Create(const CoordsType& coords,
                                                      const viskores::Id2& dims,
                                                      const std::string& coordsNm = "coords")
  {
    return DataSetBuilderCurvilinear::Create(coords, { dims[0], dims[1], 0 }, 2, coordsNm);
  }

  template <typename CoordsType>
  VISKORES_CONT static viskores::cont::DataSet Create(const CoordsType& coords,
                                                      const std::string& coordsNm = "coords")
  {
    return DataSetBuilderCurvilinear::Create(
      coords, { coords.GetNumberOfValues(), 0, 0 }, 1, coordsNm);
  }

private:
  template <typename CoordsType>
  VISKORES_CONT static viskores::cont::DataSet Create(const CoordsType& coords,
                                                      const viskores::Id3& dims,
                                                      const viskores::Id& cellSetDim,
                                                      const std::string& coordsNm = "coords")
  {
    viskores::cont::DataSet ds;
    ds.AddCoordinateSystem(viskores::cont::CoordinateSystem(coordsNm, coords));

    if (cellSetDim == 3)
    {
      VISKORES_ASSERT(dims[0] >= 1 && dims[1] >= 1 && dims[2] >= 1);
      VISKORES_ASSERT(coords.GetNumberOfValues() == dims[0] * dims[1] * dims[2]);

      viskores::cont::CellSetStructured<3> cellSet;
      cellSet.SetPointDimensions(dims);
      ds.SetCellSet(cellSet);
    }
    else if (cellSetDim == 2)
    {
      VISKORES_ASSERT(dims[0] >= 1 && dims[1] >= 1 && dims[2] == 0);
      VISKORES_ASSERT(coords.GetNumberOfValues() == dims[0] * dims[1]);

      viskores::cont::CellSetStructured<2> cellSet;
      cellSet.SetPointDimensions({ dims[0], dims[1] });
      ds.SetCellSet(cellSet);
    }
    else if (cellSetDim == 1)
    {
      VISKORES_ASSERT(dims[0] >= 1 && dims[1] == 0 && dims[2] == 0);
      VISKORES_ASSERT(coords.GetNumberOfValues() == dims[0]);

      viskores::cont::CellSetStructured<1> cellSet;
      cellSet.SetPointDimensions(dims[0]);
      ds.SetCellSet(cellSet);
    }
    else
      throw viskores::cont::ErrorBadValue("Unsupported CellSetStructured dimension.");

    return ds;
  }
};

} // namespace cont
} // namespace viskores

#endif //viskores_cont_DataSetBuilderCurvilinear_h
