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
#include <iostream>
#include <vector>

#include <viskores/cont/testing/Testing.h>
#include <viskores/worklet/splatkernels/Gaussian.h>
#include <viskores/worklet/splatkernels/Spline3rdOrder.h>


using Vector = viskores::Vec3f_64;

// Simpson integradion rule
double SimpsonIntegration(const std::vector<double>& y, const std::vector<double>& x)
{
  std::size_t n = x.size() - 1;
  const double aux = 2. * (x[n] - x[0]) / (3. * static_cast<double>(n));
  double val = 0.5 * (y[0] * x[0] + y[n] * x[n]);
  for (std::size_t i = 2; i < n; i += 2)
  {
    val += 2 * y[i - 1] + y[i];
  }
  val += 2 * y[n - 1];
  return aux * val;
}

// Integrade a kernel in 3D
template <typename Kernel>
double IntegralOfKernel(const Kernel& ker)
{
  const double supportlength = ker.maxDistance();
  const int npoint = 15000;
  std::vector<double> x;
  std::vector<double> y;
  for (int i = 0; i < npoint; i++)
  {
    const double r = static_cast<double>(i) * supportlength / static_cast<double>(npoint);
    x.push_back(r);
    y.push_back(ker.w(r) * r * r);
  }
  return 4.0 * M_PI * SimpsonIntegration(y, x);
}

// Same integration, but using the variable smoothing length interface
template <typename Kernel>
double IntegralOfKernel(const Kernel& ker, double h)
{
  const double supportlength = ker.maxDistance();
  const int npoint = 15000;
  std::vector<double> x;
  std::vector<double> y;
  for (int i = 0; i < npoint; i++)
  {
    const double r = static_cast<double>(i) * supportlength / static_cast<double>(npoint);
    x.push_back(r);
    y.push_back(ker.w(h, r) * r * r);
  }
  return 4.0 * M_PI * SimpsonIntegration(y, x);
}

int TestSplatKernels()
{
  const double eps = 1e-4;
  double s;
  double smoothinglength;

  std::cout << "Testing Gaussian 3D fixed h kernel integration \n";
  for (int i = 0; i < 100; ++i)
  {
    smoothinglength = 0.01 + i * (10.0 / 100.0);
    s = IntegralOfKernel(viskores::worklet::splatkernels::Gaussian<3>(smoothinglength));
    VISKORES_TEST_ASSERT(fabs(s - 1.0) < eps, "Gaussian 3D integration failure");
  }

  std::cout << "Testing Gaussian 3D variable h kernel integration \n";
  for (int i = 0; i < 100; ++i)
  {
    smoothinglength = 0.01 + i * (10.0 / 100.0);
    s = IntegralOfKernel(viskores::worklet::splatkernels::Gaussian<3>(smoothinglength),
                         smoothinglength);
    VISKORES_TEST_ASSERT(fabs(s - 1.0) < eps, "Gaussian 3D integration failure");
  }

  //  s  = IntegralOfKernel(viskores::worklet::splatkernels::Gaussian<2>(smoothinglength));
  //  VISKORES_TEST_ASSERT ( fabs(s - 1.0) < eps, "Gaussian 2D integration failure");

  std::cout << "Testing Spline3rdOrder 3D kernel integration \n";
  for (int i = 0; i < 100; ++i)
  {
    smoothinglength = 0.01 + i * (10.0 / 100.0);
    s = IntegralOfKernel(viskores::worklet::splatkernels::Spline3rdOrder<3>(smoothinglength));
    VISKORES_TEST_ASSERT(fabs(s - 1.0) < eps, "Spline3rdOrder 3D integration failure");
  }

  //  s  = IntegralOfKernel(viskores::worklet::splatkernels::Spline3rdOrder<2>(smoothinglength));
  //  VISKORES_TEST_ASSERT ( fabs(s - 1.0) < eps, "Spline3rdOrder 2D integration failure");

  /*
  s  = IntegralOfKernel(KernelBox(ndim, smoothinglength));
  if ( fabs(s - 1.0) > eps) {
    return EXIT_FAILURE;
  }
  s  = IntegralOfKernel(KernelCusp(ndim, smoothinglength));
  if ( fabs(s - 1.0) > eps) {
    return EXIT_FAILURE;
  }
  s  = IntegralOfKernel(KernelGaussian(ndim, smoothinglength));
  if ( fabs(s - 1.0) > eps) {
    return EXIT_FAILURE;
  }
  s  = IntegralOfKernel(KernelQuadratic(ndim, smoothinglength));
  if ( fabs(s - 1.0) > eps) {
    return EXIT_FAILURE;
  }
  s  = IntegralOfKernel(KernelSpline3rdOrder(ndim, smoothinglength));
  if ( fabs(s - 1.0) > eps) {
    return EXIT_FAILURE;
  }
  s  = IntegralOfKernel(KernelSpline5thOrder(ndim, smoothinglength));
  if ( fabs(s - 1.0) > eps) {
    return EXIT_FAILURE;
  }
  s  = IntegralOfKernel(KernelWendland(ndim, smoothinglength));
  if ( fabs(s - 1.0) > eps) {
    return EXIT_FAILURE;
  }
*/
  return EXIT_SUCCESS;
}

int UnitTestSplatKernels(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(TestSplatKernels, argc, argv);
}
