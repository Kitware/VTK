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
#ifndef viskores_cont_DataSetBuilderRectilinear_h
#define viskores_cont_DataSetBuilderRectilinear_h

#include <viskores/cont/ArrayCopy.h>
#include <viskores/cont/ArrayHandleCartesianProduct.h>
#include <viskores/cont/ArrayPortalToIterators.h>
#include <viskores/cont/CoordinateSystem.h>
#include <viskores/cont/DataSet.h>
#include <viskores/cont/serial/DeviceAdapterSerial.h>

namespace viskores
{
namespace cont
{

class VISKORES_CONT_EXPORT DataSetBuilderRectilinear
{
  template <typename T, typename U>
  VISKORES_CONT static void CopyInto(const std::vector<T>& input,
                                     viskores::cont::ArrayHandle<U>& output)
  {
    DataSetBuilderRectilinear::CopyInto(
      viskores::cont::make_ArrayHandle(input, viskores::CopyFlag::Off), output);
  }

  template <typename T, typename U>
  VISKORES_CONT static void CopyInto(const viskores::cont::ArrayHandle<T>& input,
                                     viskores::cont::ArrayHandle<U>& output)
  {
    viskores::cont::ArrayCopy(input, output);
  }

  template <typename T, typename U>
  VISKORES_CONT static void CopyInto(const T* input,
                                     viskores::Id len,
                                     viskores::cont::ArrayHandle<U>& output)
  {
    DataSetBuilderRectilinear::CopyInto(
      viskores::cont::make_ArrayHandle(input, len, viskores::CopyFlag::Off), output);
  }

public:
  VISKORES_CONT
  DataSetBuilderRectilinear();

  /// @brief Create a 1D retilinear `DataSet`.
  ///
  /// A rectilinear grid is specified with a scalar array for the point coordinates
  /// in the x direction.
  /// In this form, the coordinate array is specified with `std::vector`.
  /// The data is copied from the `std::vector`.
  ///
  /// @param[in] xvals An array of coordinates to use along the x dimension.
  /// @param[in] coordNm (optional) The name to register the coordinates as.
  template <typename T>
  VISKORES_CONT static viskores::cont::DataSet Create(const std::vector<T>& xvals,
                                                      const std::string& coordNm = "coords")
  {
    std::vector<T> yvals(1, 0), zvals(1, 0);
    return DataSetBuilderRectilinear::BuildDataSet(xvals, yvals, zvals, coordNm);
  }

  /// @brief Create a 1D retilinear `DataSet`.
  ///
  /// A rectilinear grid is specified with a scalar array for the point coordinates
  /// in the x direction.
  /// In this form, the coordinate array is specified with a standard C array.
  /// The data is copied from the array.
  ///
  /// @param[in] nx The size of the grid in the x direction (and length of the @a xvals array).
  /// @param[in] xvals An array of coordinates to use along the x dimension.
  /// @param[in] coordNm (optional) The name to register the coordinates as.
  template <typename T>
  VISKORES_CONT static viskores::cont::DataSet Create(viskores::Id nx,
                                                      T* xvals,
                                                      const std::string& coordNm = "coords")
  {
    T yvals = 0, zvals = 0;
    return DataSetBuilderRectilinear::BuildDataSet(nx, 1, 1, xvals, &yvals, &zvals, coordNm);
  }

  /// @brief Create a 1D retilinear `DataSet`.
  ///
  /// A rectilinear grid is specified with a scalar array for the point coordinates
  /// in the x direction.
  /// In this form, the coordinate array is specified with `viskores::cont::ArrayHandle`.
  /// The `ArrayHandle` is shared with the `DataSet`, so changing the `ArrayHandle`
  /// changes the `DataSet`.
  ///
  /// @param[in] xvals An array of coordinates to use along the x dimension.
  /// @param[in] coordNm (optional) The name to register the coordinates as.
  template <typename T>
  VISKORES_CONT static viskores::cont::DataSet Create(const viskores::cont::ArrayHandle<T>& xvals,
                                                      const std::string& coordNm = "coords")
  {
    viskores::cont::ArrayHandle<T> yvals, zvals;
    yvals.Allocate(1);
    yvals.WritePortal().Set(0, 0.0);
    zvals.Allocate(1);
    zvals.WritePortal().Set(0, 0.0);
    return DataSetBuilderRectilinear::BuildDataSet(xvals, yvals, zvals, coordNm);
  }

  /// @brief Create a 2D retilinear `DataSet`.
  ///
  /// A rectilinear grid is specified with separate arrays for the point coordinates
  /// in the x and y directions.
  /// In this form, the coordinate arrays are specified with `std::vector`.
  /// The data is copied from the `std::vector`s.
  ///
  /// @param[in] xvals An array of coordinates to use along the x dimension.
  /// @param[in] yvals An array of coordinates to use along the x dimension.
  /// @param[in] coordNm (optional) The name to register the coordinates as.
  template <typename T>
  VISKORES_CONT static viskores::cont::DataSet Create(const std::vector<T>& xvals,
                                                      const std::vector<T>& yvals,
                                                      const std::string& coordNm = "coords")
  {
    std::vector<T> zvals(1, 0);
    return DataSetBuilderRectilinear::BuildDataSet(xvals, yvals, zvals, coordNm);
  }

  /// @brief Create a 2D retilinear `DataSet`.
  ///
  /// A rectilinear grid is specified with separate arrays for the point coordinates
  /// in the x and y directions.
  /// In this form, the coordinate arrays are specified with standard C arrays.
  /// The data is copied from the arrays.
  ///
  /// @param[in] nx The size of the grid in the x direction (and length of the @a xvals array).
  /// @param[in] ny The size of the grid in the x direction (and length of the @a yvals array).
  /// @param[in] xvals An array of coordinates to use along the x dimension.
  /// @param[in] yvals An array of coordinates to use along the x dimension.
  /// @param[in] coordNm (optional) The name to register the coordinates as.
  template <typename T>
  VISKORES_CONT static viskores::cont::DataSet Create(viskores::Id nx,
                                                      viskores::Id ny,
                                                      T* xvals,
                                                      T* yvals,
                                                      const std::string& coordNm = "coords")
  {
    T zvals = 0;
    return DataSetBuilderRectilinear::BuildDataSet(nx, ny, 1, xvals, yvals, &zvals, coordNm);
  }

  /// @brief Create a 2D retilinear `DataSet`.
  ///
  /// A rectilinear grid is specified with separate arrays for the point coordinates
  /// in the x and y directions.
  /// In this form, the coordinate arrays are specified with `viskores::cont::ArrayHandle`.
  /// The `ArrayHandle`s are shared with the `DataSet`, so changing the `ArrayHandle`s
  /// changes the `DataSet`.
  ///
  /// @param[in] xvals An array of coordinates to use along the x dimension.
  /// @param[in] yvals An array of coordinates to use along the x dimension.
  /// @param[in] coordNm (optional) The name to register the coordinates as.
  template <typename T>
  VISKORES_CONT static viskores::cont::DataSet Create(const viskores::cont::ArrayHandle<T>& xvals,
                                                      const viskores::cont::ArrayHandle<T>& yvals,
                                                      const std::string& coordNm = "coords")
  {
    viskores::cont::ArrayHandle<T> zvals;
    zvals.Allocate(1);
    zvals.WritePortal().Set(0, 0.0);
    return DataSetBuilderRectilinear::BuildDataSet(xvals, yvals, zvals, coordNm);
  }

  /// @brief Create a 3D retilinear `DataSet`.
  ///
  /// A rectilinear grid is specified with separate arrays for the point coordinates
  /// in the x, y, and z directions.
  /// In this form, the coordinate arrays are specified with standard C arrays.
  /// The data is copied from the arrays.
  ///
  /// @param[in] nx The size of the grid in the x direction (and length of the @a xvals array).
  /// @param[in] ny The size of the grid in the x direction (and length of the @a yvals array).
  /// @param[in] nz The size of the grid in the x direction (and length of the @a zvals array).
  /// @param[in] xvals An array of coordinates to use along the x dimension.
  /// @param[in] yvals An array of coordinates to use along the x dimension.
  /// @param[in] zvals An array of coordinates to use along the x dimension.
  /// @param[in] coordNm (optional) The name to register the coordinates as.
  template <typename T>
  VISKORES_CONT static viskores::cont::DataSet Create(viskores::Id nx,
                                                      viskores::Id ny,
                                                      viskores::Id nz,
                                                      T* xvals,
                                                      T* yvals,
                                                      T* zvals,
                                                      const std::string& coordNm = "coords")
  {
    return DataSetBuilderRectilinear::BuildDataSet(nx, ny, nz, xvals, yvals, zvals, coordNm);
  }

  /// @brief Create a 3D retilinear `DataSet`.
  ///
  /// A rectilinear grid is specified with separate arrays for the point coordinates
  /// in the x, y, and z directions.
  /// In this form, the coordinate arrays are specified with `std::vector`.
  /// The data is copied from the `std::vector`s.
  ///
  /// @param[in] xvals An array of coordinates to use along the x dimension.
  /// @param[in] yvals An array of coordinates to use along the x dimension.
  /// @param[in] zvals An array of coordinates to use along the x dimension.
  /// @param[in] coordNm (optional) The name to register the coordinates as.
  template <typename T>
  VISKORES_CONT static viskores::cont::DataSet Create(const std::vector<T>& xvals,
                                                      const std::vector<T>& yvals,
                                                      const std::vector<T>& zvals,
                                                      const std::string& coordNm = "coords")
  {
    return DataSetBuilderRectilinear::BuildDataSet(xvals, yvals, zvals, coordNm);
  }

  /// @brief Create a 3D retilinear `DataSet`.
  ///
  /// A rectilinear grid is specified with separate arrays for the point coordinates
  /// in the x, y, and z directions.
  /// In this form, the coordinate arrays are specified with `viskores::cont::ArrayHandle`.
  /// The `ArrayHandle`s are shared with the `DataSet`, so changing the `ArrayHandle`s
  /// changes the `DataSet`.
  ///
  /// @param[in] xvals An array of coordinates to use along the x dimension.
  /// @param[in] yvals An array of coordinates to use along the x dimension.
  /// @param[in] zvals An array of coordinates to use along the x dimension.
  /// @param[in] coordNm (optional) The name to register the coordinates as.
  template <typename T>
  VISKORES_CONT static viskores::cont::DataSet Create(const viskores::cont::ArrayHandle<T>& xvals,
                                                      const viskores::cont::ArrayHandle<T>& yvals,
                                                      const viskores::cont::ArrayHandle<T>& zvals,
                                                      const std::string& coordNm = "coords")
  {
    return DataSetBuilderRectilinear::BuildDataSet(xvals, yvals, zvals, coordNm);
  }

private:
  template <typename T>
  VISKORES_CONT static viskores::cont::DataSet BuildDataSet(const std::vector<T>& xvals,
                                                            const std::vector<T>& yvals,
                                                            const std::vector<T>& zvals,
                                                            const std::string& coordNm)
  {
    viskores::cont::ArrayHandle<viskores::FloatDefault> Xc, Yc, Zc;
    DataSetBuilderRectilinear::CopyInto(xvals, Xc);
    DataSetBuilderRectilinear::CopyInto(yvals, Yc);
    DataSetBuilderRectilinear::CopyInto(zvals, Zc);

    return DataSetBuilderRectilinear::BuildDataSet(Xc, Yc, Zc, coordNm);
  }

  template <typename T>
  VISKORES_CONT static viskores::cont::DataSet BuildDataSet(viskores::Id nx,
                                                            viskores::Id ny,
                                                            viskores::Id nz,
                                                            const T* xvals,
                                                            const T* yvals,
                                                            const T* zvals,
                                                            const std::string& coordNm)
  {
    viskores::cont::ArrayHandle<viskores::FloatDefault> Xc, Yc, Zc;
    DataSetBuilderRectilinear::CopyInto(xvals, nx, Xc);
    DataSetBuilderRectilinear::CopyInto(yvals, ny, Yc);
    DataSetBuilderRectilinear::CopyInto(zvals, nz, Zc);

    return DataSetBuilderRectilinear::BuildDataSet(Xc, Yc, Zc, coordNm);
  }

  template <typename T>
  VISKORES_CONT static viskores::cont::DataSet BuildDataSet(const viskores::cont::ArrayHandle<T>& X,
                                                            const viskores::cont::ArrayHandle<T>& Y,
                                                            const viskores::cont::ArrayHandle<T>& Z,
                                                            const std::string& coordNm)
  {
    viskores::cont::DataSet dataSet;

    //Convert all coordinates to floatDefault.
    viskores::cont::ArrayHandleCartesianProduct<viskores::cont::ArrayHandle<viskores::FloatDefault>,
                                                viskores::cont::ArrayHandle<viskores::FloatDefault>,
                                                viskores::cont::ArrayHandle<viskores::FloatDefault>>
      coords;

    viskores::cont::ArrayHandle<viskores::FloatDefault> Xc, Yc, Zc;
    DataSetBuilderRectilinear::CopyInto(X, Xc);
    DataSetBuilderRectilinear::CopyInto(Y, Yc);
    DataSetBuilderRectilinear::CopyInto(Z, Zc);

    coords = viskores::cont::make_ArrayHandleCartesianProduct(Xc, Yc, Zc);
    viskores::cont::CoordinateSystem cs(coordNm, coords);
    dataSet.AddCoordinateSystem(cs);

    // compute the dimensions of the cellset by counting the number of axes
    // with >1 dimension
    int ndims = 0;
    viskores::Id dims[3];
    if (Xc.GetNumberOfValues() > 1)
    {
      dims[ndims++] = Xc.GetNumberOfValues();
    }
    if (Yc.GetNumberOfValues() > 1)
    {
      dims[ndims++] = Yc.GetNumberOfValues();
    }
    if (Zc.GetNumberOfValues() > 1)
    {
      dims[ndims++] = Zc.GetNumberOfValues();
    }

    if (ndims == 1)
    {
      viskores::cont::CellSetStructured<1> cellSet;
      cellSet.SetPointDimensions(dims[0]);
      dataSet.SetCellSet(cellSet);
    }
    else if (ndims == 2)
    {
      viskores::cont::CellSetStructured<2> cellSet;
      cellSet.SetPointDimensions(viskores::make_Vec(dims[0], dims[1]));
      dataSet.SetCellSet(cellSet);
    }
    else if (ndims == 3)
    {
      viskores::cont::CellSetStructured<3> cellSet;
      cellSet.SetPointDimensions(viskores::make_Vec(dims[0], dims[1], dims[2]));
      dataSet.SetCellSet(cellSet);
    }
    else
    {
      throw viskores::cont::ErrorBadValue("Invalid cell set dimension");
    }

    return dataSet;
  }
};

} // namespace cont
} // namespace viskores

#endif //viskores_cont_DataSetBuilderRectilinear_h
