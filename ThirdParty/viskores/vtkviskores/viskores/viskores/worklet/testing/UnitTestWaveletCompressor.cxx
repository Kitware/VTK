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

#include <viskores/worklet/WaveletCompressor.h>

#include <viskores/cont/ArrayHandlePermutation.h>
#include <viskores/cont/Timer.h>
#include <viskores/cont/testing/Testing.h>

#include <iomanip>
#include <vector>

namespace viskores
{
namespace worklet
{
namespace wavelets
{

class GaussianWorklet2D : public viskores::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldInOut);
  using ExecutionSignature = void(_1, WorkIndex);

  VISKORES_EXEC
  GaussianWorklet2D(viskores::Id dx,
                    viskores::Id dy,
                    viskores::Float64 a,
                    viskores::Float64 x,
                    viskores::Float64 y,
                    viskores::Float64 sx,
                    viskores::Float64 xy)
    : dimX(dx)
    , amp(a)
    , x0(x)
    , y0(y)
    , sigmaX(sx)
    , sigmaY(xy)
  {
    (void)dy;
    sigmaX2 = 2 * sigmaX * sigmaX;
    sigmaY2 = 2 * sigmaY * sigmaY;
  }

  VISKORES_EXEC
  void Sig1Dto2D(viskores::Id idx, viskores::Id& x, viskores::Id& y) const
  {
    x = idx % dimX;
    y = idx / dimX;
  }

  VISKORES_EXEC
  viskores::Float64 GetGaussian(viskores::Float64 x, viskores::Float64 y) const
  {
    viskores::Float64 power = (x - x0) * (x - x0) / sigmaX2 + (y - y0) * (y - y0) / sigmaY2;
    return viskores::Exp(power * -1.0) * amp;
  }

  template <typename T>
  VISKORES_EXEC void operator()(T& val, const viskores::Id& workIdx) const
  {
    viskores::Id x, y;
    Sig1Dto2D(workIdx, x, y);
    val = GetGaussian(static_cast<viskores::Float64>(x), static_cast<viskores::Float64>(y));
  }

private:                                  // see wikipedia page
  const viskores::Id dimX;                // 2D extent
  const viskores::Float64 amp;            // amplitude
  const viskores::Float64 x0, y0;         // center
  const viskores::Float64 sigmaX, sigmaY; // spread
  viskores::Float64 sigmaX2, sigmaY2;     // 2 * sigma * sigma
};

template <typename T>
class GaussianWorklet3D : public viskores::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldInOut);
  using ExecutionSignature = void(_1, WorkIndex);

  VISKORES_EXEC
  GaussianWorklet3D(viskores::Id dx, viskores::Id dy, viskores::Id dz)
    : dimX(dx)
    , dimY(dy)
    , dimZ(dz)
  {
    amp = (T)20.0;
    sigmaX = (T)dimX / (T)4.0;
    sigmaX2 = sigmaX * sigmaX * (T)2.0;
    sigmaY = (T)dimY / (T)4.0;
    sigmaY2 = sigmaY * sigmaY * (T)2.0;
    sigmaZ = (T)dimZ / (T)4.0;
    sigmaZ2 = sigmaZ * sigmaZ * (T)2.0;
  }

  VISKORES_EXEC
  void Sig1Dto3D(viskores::Id idx, viskores::Id& x, viskores::Id& y, viskores::Id& z) const
  {
    z = idx / (dimX * dimY);
    y = (idx - z * dimX * dimY) / dimX;
    x = idx % dimX;
  }

  VISKORES_EXEC
  T GetGaussian(T x, T y, T z) const
  {
    x -= (T)dimX / (T)2.0; // translate to center at (0, 0, 0)
    y -= (T)dimY / (T)2.0;
    z -= (T)dimZ / (T)2.0;
    T power = x * x / sigmaX2 + y * y / sigmaY2 + z * z / sigmaZ2;

    return viskores::Exp(power * (T)-1.0) * amp;
  }

  VISKORES_EXEC
  void operator()(T& val, const viskores::Id& workIdx) const
  {
    viskores::Id x, y, z;
    Sig1Dto3D(workIdx, x, y, z);
    val = GetGaussian((T)x, (T)y, (T)z);
  }

private:
  const viskores::Id dimX, dimY, dimZ; // extent
  T amp;                               // amplitude
  T sigmaX, sigmaY, sigmaZ;            // spread
  T sigmaX2, sigmaY2, sigmaZ2;         // sigma * sigma * 2
};
}
}
}

template <typename ArrayType>
void FillArray2D(ArrayType& array, viskores::Id dimX, viskores::Id dimY)
{
  using WorkletType = viskores::worklet::wavelets::GaussianWorklet2D;
  WorkletType worklet(dimX,
                      dimY,
                      100.0,
                      static_cast<viskores::Float64>(dimX) / 2.0,  // center
                      static_cast<viskores::Float64>(dimY) / 2.0,  // center
                      static_cast<viskores::Float64>(dimX) / 4.0,  // spread
                      static_cast<viskores::Float64>(dimY) / 4.0); // spread
  viskores::worklet::DispatcherMapField<WorkletType> dispatcher(worklet);
  dispatcher.Invoke(array);
}
template <typename ArrayType>
void FillArray3D(ArrayType& array, viskores::Id dimX, viskores::Id dimY, viskores::Id dimZ)
{
  using WorkletType = viskores::worklet::wavelets::GaussianWorklet3D<typename ArrayType::ValueType>;
  WorkletType worklet(dimX, dimY, dimZ);
  viskores::worklet::DispatcherMapField<WorkletType> dispatcher(worklet);
  dispatcher.Invoke(array);
}

void TestDecomposeReconstruct3D(viskores::Float64 cratio)
{
  viskores::Id sigX = 45;
  viskores::Id sigY = 45;
  viskores::Id sigZ = 45;
  viskores::Id sigLen = sigX * sigY * sigZ;

  // make input data array handle
  viskores::cont::ArrayHandle<viskores::Float32> inputArray;
  inputArray.Allocate(sigLen);
  FillArray3D(inputArray, sigX, sigY, sigZ);

  viskores::cont::ArrayHandle<viskores::Float32> outputArray;

  // Use a WaveletCompressor
  viskores::worklet::wavelets::WaveletName wname = viskores::worklet::wavelets::BIOR4_4;
  viskores::worklet::WaveletCompressor compressor(wname);

  viskores::Id XMaxLevel = compressor.GetWaveletMaxLevel(sigX);
  viskores::Id YMaxLevel = compressor.GetWaveletMaxLevel(sigY);
  viskores::Id ZMaxLevel = compressor.GetWaveletMaxLevel(sigZ);
  viskores::Id nLevels = viskores::Min(viskores::Min(XMaxLevel, YMaxLevel), ZMaxLevel);

  // Decompose
  compressor.WaveDecompose3D(inputArray, nLevels, sigX, sigY, sigZ, outputArray, false);

  compressor.SquashCoefficients(outputArray, cratio);

  // Reconstruct
  viskores::cont::ArrayHandle<viskores::Float32> reconstructArray;
  compressor.WaveReconstruct3D(outputArray, nLevels, sigX, sigY, sigZ, reconstructArray, false);
  outputArray.ReleaseResources();

  //compressor.EvaluateReconstruction(inputArray, reconstructArray);

  auto reconstructPortal = reconstructArray.ReadPortal();
  auto inputPortal = inputArray.ReadPortal();
  for (viskores::Id i = 0; i < reconstructArray.GetNumberOfValues(); i++)
  {
    VISKORES_TEST_ASSERT(test_equal(reconstructPortal.Get(i), inputPortal.Get(i)),
                         "WaveletCompressor 3D failed...");
  }
}

void TestDecomposeReconstruct2D(viskores::Float64 cratio)
{
  viskores::Id sigX = 150;
  viskores::Id sigY = 150;
  viskores::Id sigLen = sigX * sigY;

  // make input data array handle
  viskores::cont::ArrayHandle<viskores::Float64> inputArray;
  inputArray.Allocate(sigLen);
  FillArray2D(inputArray, sigX, sigY);

  viskores::cont::ArrayHandle<viskores::Float64> outputArray;

  // Use a WaveletCompressor
  viskores::worklet::wavelets::WaveletName wname = viskores::worklet::wavelets::CDF9_7;
  viskores::worklet::WaveletCompressor compressor(wname);

  viskores::Id XMaxLevel = compressor.GetWaveletMaxLevel(sigX);
  viskores::Id YMaxLevel = compressor.GetWaveletMaxLevel(sigY);
  viskores::Id nLevels = viskores::Min(XMaxLevel, YMaxLevel);
  std::vector<viskores::Id> L;
  compressor.WaveDecompose2D(inputArray, nLevels, sigX, sigY, outputArray, L);
  compressor.SquashCoefficients(outputArray, cratio);

  // Reconstruct
  viskores::cont::ArrayHandle<viskores::Float64> reconstructArray;
  compressor.WaveReconstruct2D(outputArray, nLevels, sigX, sigY, reconstructArray, L);
  outputArray.ReleaseResources();

  //compressor.EvaluateReconstruction(inputArray, reconstructArray);

  auto reconstructPortal = reconstructArray.ReadPortal();
  auto inputPortal = inputArray.ReadPortal();
  for (viskores::Id i = 0; i < reconstructArray.GetNumberOfValues(); i++)
  {
    VISKORES_TEST_ASSERT(test_equal(reconstructPortal.Get(i), inputPortal.Get(i)),
                         "WaveletCompressor 2D failed...");
  }
}

void TestDecomposeReconstruct1D(viskores::Float64 cratio)
{
  viskores::Id sigLen = 1000;

  // make input data array handle
  viskores::cont::ArrayHandle<viskores::Float64> inputArray;
  inputArray.Allocate(sigLen);
  auto wp = inputArray.WritePortal();
  for (viskores::Id i = 0; i < sigLen; i++)
  {
    wp.Set(i, 100.0 * viskores::Sin(static_cast<viskores::Float64>(i) / 100.0));
  }
  viskores::cont::ArrayHandle<viskores::Float64> outputArray;

  // Use a WaveletCompressor
  viskores::worklet::wavelets::WaveletName wname = viskores::worklet::wavelets::CDF9_7;
  viskores::worklet::WaveletCompressor compressor(wname);

  // User maximum decompose levels
  viskores::Id maxLevel = compressor.GetWaveletMaxLevel(sigLen);
  viskores::Id nLevels = maxLevel;

  std::vector<viskores::Id> L;

  // Decompose
  compressor.WaveDecompose(inputArray, nLevels, outputArray, L);

  // Squash small coefficients
  compressor.SquashCoefficients(outputArray, cratio);

  // Reconstruct
  viskores::cont::ArrayHandle<viskores::Float64> reconstructArray;
  compressor.WaveReconstruct(outputArray, nLevels, L, reconstructArray);

  //compressor.EvaluateReconstruction(inputArray, reconstructArray);
  auto reconstructPortal = reconstructArray.ReadPortal();
  auto inputPortal = inputArray.ReadPortal();
  for (viskores::Id i = 0; i < reconstructArray.GetNumberOfValues(); i++)
  {
    VISKORES_TEST_ASSERT(test_equal(reconstructPortal.Get(i), inputPortal.Get(i)),
                         "WaveletCompressor 1D failed...");
  }
}

void TestWaveletCompressor()
{
  viskores::Float64 cratio = 2.0; // X:1 compression, where X >= 1
  TestDecomposeReconstruct1D(cratio);
  TestDecomposeReconstruct2D(cratio);
  TestDecomposeReconstruct3D(cratio);
}

int UnitTestWaveletCompressor(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(TestWaveletCompressor, argc, argv);
}
