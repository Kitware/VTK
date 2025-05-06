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

#ifndef viskores_worklet_wavelets_waveletbase_h
#define viskores_worklet_wavelets_waveletbase_h

#include <viskores/worklet/wavelets/WaveletFilter.h>
#include <viskores/worklet/wavelets/WaveletTransforms.h>

#include <viskores/Math.h>
#include <viskores/cont/Algorithm.h>
#include <viskores/cont/ArrayGetValues.h>

namespace viskores
{
namespace worklet
{

namespace wavelets
{

// Functionalities are similar to MatWaveBase in VAPoR.
class WaveletBase
{
public:
  // Constructor
  WaveletBase(WaveletName name)
    : wname(name)
    , filter(name)
  {
    if (wname == CDF9_7 || wname == BIOR4_4 || wname == CDF5_3 || wname == BIOR2_2)
    {
      this->wmode = SYMW; // Default extension mode, see MatWaveBase.cpp
    }
    else if (wname == HAAR || wname == BIOR1_1 || wname == CDF8_4 || wname == BIOR3_3)
    {
      this->wmode = SYMH;
    }
  }

  // Returns length of approximation coefficients from a decomposition pass.
  viskores::Id GetApproxLength(viskores::Id sigInLen)
  {
    if (sigInLen % 2 != 0)
    {
      return ((sigInLen + 1) / 2);
    }
    else
    {
      return ((sigInLen) / 2);
    }
  }

  // Returns length of detail coefficients from a decomposition pass
  viskores::Id GetDetailLength(viskores::Id sigInLen)
  {
    if (sigInLen % 2 != 0)
    {
      return ((sigInLen - 1) / 2);
    }
    else
    {
      return ((sigInLen) / 2);
    }
  }

  // Returns length of coefficients generated in a decomposition pass
  viskores::Id GetCoeffLength(viskores::Id sigInLen)
  {
    return (GetApproxLength(sigInLen) + GetDetailLength(sigInLen));
  }
  viskores::Id GetCoeffLength2(viskores::Id sigInX, viskores::Id sigInY)
  {
    return (GetCoeffLength(sigInX) * GetCoeffLength(sigInY));
  }
  viskores::Id GetCoeffLength3(viskores::Id sigInX, viskores::Id sigInY, viskores::Id sigInZ)
  {
    return (GetCoeffLength(sigInX) * GetCoeffLength(sigInY) * GetCoeffLength(sigInZ));
  }

  // Returns maximum wavelet decomposition level
  viskores::Id GetWaveletMaxLevel(viskores::Id sigInLen)
  {
    viskores::Id filterLen = this->filter.GetFilterLength();
    viskores::Id level;
    this->WaveLengthValidate(sigInLen, filterLen, level);
    return level;
  }

  // perform a device copy. The whole 1st array to a certain start location of the 2nd array
  template <typename ArrayType1, typename ArrayType2>
  void DeviceCopyStartX(const ArrayType1& srcArray, ArrayType2& dstArray, viskores::Id startIdx)
  {
    using CopyType = viskores::worklet::wavelets::CopyWorklet;
    CopyType cp(startIdx);
    viskores::worklet::DispatcherMapField<CopyType> dispatcher(cp);
    dispatcher.Invoke(srcArray, dstArray);
  }

  // Assign zero value to a certain location of an array
  template <typename ArrayType>
  void DeviceAssignZero(ArrayType& array, viskores::Id index)
  {
    using ZeroWorklet = viskores::worklet::wavelets::AssignZeroWorklet;
    ZeroWorklet worklet(index);
    viskores::worklet::DispatcherMapField<ZeroWorklet> dispatcher(worklet);
    dispatcher.Invoke(array);
  }

  // Assign zeros to a certain row to a matrix
  template <typename ArrayType>
  void DeviceAssignZero2DRow(ArrayType& array,
                             viskores::Id dimX,
                             viskores::Id dimY, // input
                             viskores::Id rowIdx)
  {
    using AssignZero2DType = viskores::worklet::wavelets::AssignZero2DWorklet;
    AssignZero2DType zeroWorklet(dimX, dimY, -1, rowIdx);
    viskores::worklet::DispatcherMapField<AssignZero2DType> dispatcher(zeroWorklet);
    dispatcher.Invoke(array);
  }

  // Assign zeros to a certain column to a matrix
  template <typename ArrayType>
  void DeviceAssignZero2DColumn(ArrayType& array,
                                viskores::Id dimX,
                                viskores::Id dimY, // input
                                viskores::Id colIdx)
  {
    using AssignZero2DType = viskores::worklet::wavelets::AssignZero2DWorklet;
    AssignZero2DType zeroWorklet(dimX, dimY, colIdx, -1);
    viskores::worklet::DispatcherMapField<AssignZero2DType> dispatcher(zeroWorklet);
    dispatcher.Invoke(array);
  }

  // Assign zeros to a plane that's perpendicular to the X axis (Left-Right direction)
  template <typename ArrayType>
  void DeviceAssignZero3DPlaneX(ArrayType& array, // input array
                                viskores::Id dimX,
                                viskores::Id dimY,
                                viskores::Id dimZ,  // dims of input
                                viskores::Id zeroX) // X idx to set zero
  {
    using AssignZero3DType = viskores::worklet::wavelets::AssignZero3DWorklet;
    AssignZero3DType zeroWorklet(dimX, dimY, dimZ, zeroX, -1, -1);
    viskores::worklet::DispatcherMapField<AssignZero3DType> dispatcher(zeroWorklet);
    dispatcher.Invoke(array);
  }

  // Assign zeros to a plane that's perpendicular to the Y axis (Top-Down direction)
  template <typename ArrayType>
  void DeviceAssignZero3DPlaneY(ArrayType& array, // input array
                                viskores::Id dimX,
                                viskores::Id dimY,
                                viskores::Id dimZ,  // dims of input
                                viskores::Id zeroY) // Y idx to set zero
  {
    using AssignZero3DType = viskores::worklet::wavelets::AssignZero3DWorklet;
    AssignZero3DType zeroWorklet(dimX, dimY, dimZ, -1, zeroY, -1);
    viskores::worklet::DispatcherMapField<AssignZero3DType> dispatcher(zeroWorklet);
    dispatcher.Invoke(array);
  }

  // Assign zeros to a plane that's perpendicular to the Z axis (Front-Back direction)
  template <typename ArrayType>
  void DeviceAssignZero3DPlaneZ(ArrayType& array, // input array
                                viskores::Id dimX,
                                viskores::Id dimY,
                                viskores::Id dimZ,  // dims of input
                                viskores::Id zeroZ) // Y idx to set zero
  {
    using AssignZero3DType = viskores::worklet::wavelets::AssignZero3DWorklet;
    AssignZero3DType zeroWorklet(dimX, dimY, dimZ, -1, -1, zeroZ);
    viskores::worklet::DispatcherMapField<AssignZero3DType> dispatcher(zeroWorklet);
    dispatcher.Invoke(array);
  }

  // Sort by the absolute value on device
  struct SortLessAbsFunctor
  {
    template <typename T>
    VISKORES_EXEC bool operator()(const T& x, const T& y) const
    {
      return viskores::Abs(x) < viskores::Abs(y);
    }
  };
  template <typename ArrayType>
  void DeviceSort(ArrayType& array)
  {
    viskores::cont::Algorithm::Sort(array, SortLessAbsFunctor());
  }

  // Reduce to the sum of all values on device
  template <typename ArrayType>
  typename ArrayType::ValueType DeviceSum(const ArrayType& array)
  {
    return viskores::cont::Algorithm::Reduce(array,
                                             static_cast<typename ArrayType::ValueType>(0.0));
  }

  // Helper functors for finding the max and min of an array
  struct minFunctor
  {
    template <typename FieldType>
    VISKORES_EXEC FieldType operator()(const FieldType& x, const FieldType& y) const
    {
      return Min(x, y);
    }
  };
  struct maxFunctor
  {
    template <typename FieldType>
    VISKORES_EXEC FieldType operator()(const FieldType& x, const FieldType& y) const
    {
      return viskores::Max(x, y);
    }
  };

  // Device Min and Max functions
  template <typename ArrayType>
  typename ArrayType::ValueType DeviceMax(const ArrayType& array)
  {
    typename ArrayType::ValueType initVal = viskores::cont::ArrayGetValue(0, array);
    return viskores::cont::Algorithm::Reduce(array, initVal, maxFunctor());
  }
  template <typename ArrayType>
  typename ArrayType::ValueType DeviceMin(const ArrayType& array)
  {
    typename ArrayType::ValueType initVal = viskores::cont::ArrayGetValue(0, array);
    return viskores::cont::Algorithm::Reduce(array, initVal, minFunctor());
  }

  // Max absolute value of an array
  struct maxAbsFunctor
  {
    template <typename FieldType>
    VISKORES_EXEC FieldType operator()(const FieldType& x, const FieldType& y) const
    {
      return viskores::Max(viskores::Abs(x), viskores::Abs(y));
    }
  };
  template <typename ArrayType>
  typename ArrayType::ValueType DeviceMaxAbs(const ArrayType& array)
  {
    typename ArrayType::ValueType initVal = array.ReadPortal().Get(0);
    return viskores::cont::Algorithm::Reduce(array, initVal, maxAbsFunctor());
  }

  // Calculate variance of an array
  template <typename ArrayType>
  viskores::Float64 DeviceCalculateVariance(ArrayType& array)
  {
    viskores::Float64 mean = static_cast<viskores::Float64>(this->DeviceSum(array)) /
      static_cast<viskores::Float64>(array.GetNumberOfValues());

    viskores::cont::ArrayHandle<viskores::Float64> squaredDeviation;

    // Use a worklet
    using SDWorklet = viskores::worklet::wavelets::SquaredDeviation;
    SDWorklet sdw(mean);
    viskores::worklet::DispatcherMapField<SDWorklet> dispatcher(sdw);
    dispatcher.Invoke(array, squaredDeviation);

    viskores::Float64 sdMean = this->DeviceSum(squaredDeviation) /
      static_cast<viskores::Float64>(squaredDeviation.GetNumberOfValues());

    return sdMean;
  }

  // Copy a small rectangle to a big rectangle
  template <typename SmallArrayType, typename BigArrayType>
  void DeviceRectangleCopyTo(const SmallArrayType& smallRect,
                             viskores::Id smallX,
                             viskores::Id smallY,
                             BigArrayType& bigRect,
                             viskores::Id bigX,
                             viskores::Id bigY,
                             viskores::Id startX,
                             viskores::Id startY)
  {
    using CopyToWorklet = viskores::worklet::wavelets::RectangleCopyTo;
    CopyToWorklet cp(smallX, smallY, bigX, bigY, startX, startY);
    viskores::worklet::DispatcherMapField<CopyToWorklet> dispatcher(cp);
    dispatcher.Invoke(smallRect, bigRect);
  }

  // Copy a small cube to a big cube
  template <typename SmallArrayType, typename BigArrayType>
  void DeviceCubeCopyTo(const SmallArrayType& smallCube,
                        viskores::Id smallX,
                        viskores::Id smallY,
                        viskores::Id smallZ,
                        BigArrayType& bigCube,
                        viskores::Id bigX,
                        viskores::Id bigY,
                        viskores::Id bigZ,
                        viskores::Id startX,
                        viskores::Id startY,
                        viskores::Id startZ)
  {
    using CopyToWorklet = viskores::worklet::wavelets::CubeCopyTo;
    CopyToWorklet cp(smallX, smallY, smallZ, bigX, bigY, bigZ, startX, startY, startZ);
    viskores::worklet::DispatcherMapField<CopyToWorklet> dispatcher(cp);
    dispatcher.Invoke(smallCube, bigCube);
  }

  template <typename ArrayType>
  void Print2DArray(const std::string& str, const ArrayType& arr, viskores::Id dimX)
  {
    std::cerr << str << std::endl;
    auto portal = arr.ReadPortal();
    for (viskores::Id i = 0; i < arr.GetNumberOfValues(); i++)
    {
      std::cerr << portal.Get(i) << "  ";
      if (i % dimX == dimX - 1)
      {
        std::cerr << std::endl;
      }
    }
  }

protected:
  WaveletName wname;
  DWTMode wmode;
  WaveletFilter filter;

  void WaveLengthValidate(viskores::Id sigInLen, viskores::Id filterLength, viskores::Id& level)
  {
    if (sigInLen < filterLength)
    {
      level = 0;
    }
    else
    {
      level = static_cast<viskores::Id>(
        viskores::Floor(1.0 +
                        viskores::Log2(static_cast<viskores::Float64>(sigInLen) /
                                       static_cast<viskores::Float64>(filterLength))));
    }
  }

}; // class WaveletBase.

} // namespace wavelets

} // namespace worklet
} // namespace viskores

#endif
