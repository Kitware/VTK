// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include <vtkFFT.h>

#include <vtkDataArrayRange.h>
#include <vtkMathUtilities.h>

#include <algorithm>
#include <array>
#include <memory>
#include <numeric>

static bool FuzzyCompare(const vtkFFT::ComplexNumber& result, const vtkFFT::ComplexNumber& test,
  vtkFFT::ScalarNumber epsilon)
{
  return (std::abs(result.r - test.r) < epsilon) && (std::abs(result.i - test.i) < epsilon);
}

static bool FuzzyCompare(const std::vector<double>& a, const std::vector<double>& b,
  double epsilon = std::numeric_limits<double>::epsilon())
{
  bool success = true;
  for (size_t i = 0; i < a.size(); ++i)
  {
    success &= vtkMathUtilities::FuzzyCompare(a[i], b[i], epsilon);
  }
  return success;
}

static bool FuzzyCompare(vtkFFT::vtkScalarNumberArray* a, const std::vector<double>& b,
  double epsilon = std::numeric_limits<double>::epsilon())
{
  bool success = true;
  for (size_t i = 0; i < b.size(); ++i)
  {
    success &= vtkMathUtilities::FuzzyCompare(a->GetValue(i), b[i], epsilon);
  }
  return success;
}

/**
 * Used for storing long and complex test results.
 */
struct TestResults
{
  // scipy.signal.csd(signal, signal, sample_rate, window, nfft, noverlap, nfft, False,
  // onesided, 'spectrum')
  static const std::vector<vtkFFT::ScalarNumber> ExpectedSpectrum;
  // scipy.signal.csd(signal, signal, sample_rate, window, nfft, noverlap, nfft, 'constant',
  // onesided, 'spectrum')
  static const std::vector<vtkFFT::ScalarNumber> ExpectedSpectrumDetrend;
  // scipy.signal.csd(signal, signal, sample_rate, window, nfft, noverlap, nfft, False, onesided,
  // 'density')
  static const std::vector<vtkFFT::ScalarNumber> ExpectedDensity;
  // scipy.signal.csd(complexSignal, complexSignal, sample_rate, window, nfft, noverlap, nfft,
  // False, onesided, 'density')
  static const std::vector<vtkFFT::ScalarNumber> ExpectedComplexDensity;
  // scipy.signal.spectrogram(signal, sample_rate, window, nfft, noverlap, nfft, False, onesided,
  // 'density', mode='complex')
  // print(np.transpose(result)[1])
  static const std::vector<vtkFFT::ComplexNumber> ExpectedStft;

  static const std::array<double, 2> Expected_freq500HzOctaveBaseTwo;
  static const std::array<double, 2> Expected_freq500HzThirdOctaveBaseTen;
  static const std::array<double, 2> Expected_freq8kHzHalfOctaveBaseTwo;
};

static int Test_fft_cplx();
static int Test_fft_direct();
static int Test_fft_inverse();
static int Test_fft_inverse_cplx();
static int Test_complex_module();
static int Test_fftfreq();
static int Test_rfftfreq();
static int Test_fft_direct_inverse();
static int Test_kernel_generation();
static int Test_csd();
static int Test_transpose();
static int Test_octave();

int UnitTestFFT(int, char*[])
{
  int status = 0;

  status += Test_fft_cplx();
  status += Test_fft_direct();
  status += Test_fft_inverse();
  status += Test_fft_inverse_cplx();
  status += Test_complex_module();
  status += Test_fftfreq();
  status += Test_rfftfreq();
  status += Test_fft_direct_inverse();
  status += Test_kernel_generation();
  status += Test_csd();
  status += Test_transpose();
  status += Test_octave();

  if (status != 0)
  {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

int Test_fft_cplx()
{
  std::cout << "Test_fft_cplx..";

  static constexpr auto countIn = 16;
  static constexpr auto countOut = countIn;
  auto comparator = [](vtkFFT::ComplexNumber l, vtkFFT::ComplexNumber r)
  { return FuzzyCompare(l, r, std::numeric_limits<vtkFFT::ScalarNumber>::epsilon()); };
  auto status = 0;
  // Test with zeroes
  {
    std::vector<vtkFFT::ComplexNumber> zeroes(countIn);
    std::fill(zeroes.begin(), zeroes.end(), vtkFFT::ComplexNumber{ 0.0, 0.0 });

    auto resultZeroes = vtkFFT::Fft(zeroes);

    std::vector<vtkFFT::ComplexNumber> expected(countOut);
    std::fill(expected.begin(), expected.end(), vtkFFT::ComplexNumber{ 0.0, 0.0 });
    auto is_equal = std::equal(expected.begin(), expected.end(), resultZeroes.begin(), comparator);
    if (!is_equal)
    {
      status++;
    }
  }

  // Test with 1 freq
  {
    std::vector<vtkFFT::ComplexNumber> f1(countIn);
    for (std::size_t i = 0; i < countIn; ++i)
    {
      f1[i] = vtkFFT::ComplexNumber{ static_cast<vtkFFT::ScalarNumber>(i % 2), 0.0 };
    }

    auto res = vtkFFT::Fft(f1);

    std::vector<vtkFFT::ComplexNumber> expected(countOut);
    std::fill(expected.begin(), expected.end(), vtkFFT::ComplexNumber{ 0.0, 0.0 });
    expected[0] = vtkFFT::ComplexNumber{ 8.0, 0.0 };
    expected[8] = vtkFFT::ComplexNumber{ -8.0, 0.0 };
    auto is_equal = std::equal(expected.begin(), expected.end(), res.begin(), comparator);
    if (!is_equal)
    {
      status++;
    }
  }

  // Test with C API
  {
    std::vector<vtkFFT::ComplexNumber> f1(countIn);
    for (std::size_t i = 0; i < countIn; ++i)
    {
      f1[i] = vtkFFT::ComplexNumber{ static_cast<vtkFFT::ScalarNumber>(i % 2), 0.0 };
    }

    std::vector<vtkFFT::ComplexNumber> res(countIn);
    vtkFFT::Fft(f1.data(), f1.size(), res.data());

    std::vector<vtkFFT::ComplexNumber> expected(countOut);
    std::fill(expected.begin(), expected.end(), vtkFFT::ComplexNumber{ 0.0, 0.0 });
    expected[0] = vtkFFT::ComplexNumber{ 8.0, 0.0 };
    expected[8] = vtkFFT::ComplexNumber{ -8.0, 0.0 };
    auto is_equal = std::equal(expected.begin(), expected.end(), res.begin(), comparator);
    if (!is_equal)
    {
      std::cerr << "..Error when doing FFT with 1 freq with C API..";
      status++;
    }
  }

  // Test vtkDataArrays API
  {
    vtkNew<vtkFFT::vtkScalarNumberArray> array;
    array->SetNumberOfComponents(2);
    array->SetNumberOfTuples(countIn);
    for (std::size_t i = 0; i < countIn; ++i)
    {
      array->SetTuple2(i, static_cast<vtkFFT::ScalarNumber>(i % 2), 0.0);
    }
    std::vector<vtkFFT::ComplexNumber> expected(countOut);
    std::fill(expected.begin(), expected.end(), vtkFFT::ComplexNumber{ 0.0, 0.0 });
    expected[0] = vtkFFT::ComplexNumber{ 8.0, 0.0 };
    expected[8] = vtkFFT::ComplexNumber{ -8.0, 0.0 };

    auto res = vtkFFT::Fft(array);

    if (res->GetNumberOfComponents() != 2)
    {
      std::cerr << ".vtkFFT::Fft(vtkScalarNumberArray*) wrong number of components.";
      status++;
    }
    else if (res->GetNumberOfTuples() != countOut)
    {
      std::cerr << ".vtkFFT::Fft(vtkScalarNumberArray*) wrong number of tuples.";
      status++;
    }
    else
    {
      bool is_equal =
        std::equal(expected.begin(), expected.end(), vtk::DataArrayTupleRange(res).begin(),
          [](vtkFFT::ComplexNumber x, decltype(vtk::DataArrayTupleRange(res).begin())::value_type y)
          { return (x.r == y[0]) && (x.i == y[1]); });
      if (!is_equal)
      {
        std::cerr << "..Error when using vtkDataArrays API..";
        status++;
      }
    }
  }

  std::cout << (status ? "..FAILED" : ".PASSED") << std::endl;
  return status;
}

int Test_fft_direct()
{
  std::cout << "Test_fft_direct..";

  static constexpr std::size_t countIn = 16;
  static constexpr std::size_t countOut = (countIn / 2) + 1;
  auto comparator = [](vtkFFT::ComplexNumber l, vtkFFT::ComplexNumber r)
  { return FuzzyCompare(l, r, std::numeric_limits<vtkFFT::ScalarNumber>::epsilon()); };
  auto status = 0;
  // zeroes
  std::vector<vtkFFT::ScalarNumber> zeroes(countIn, 0.0);
  auto resultZeroes = vtkFFT::RFft(zeroes);
  std::vector<vtkFFT::ComplexNumber> expectedZeroes(countOut, vtkFFT::ComplexNumber{ 0.0, 0.0 });
  auto is_equal =
    std::equal(expectedZeroes.begin(), expectedZeroes.end(), resultZeroes.begin(), comparator);
  if (!is_equal)
  {
    status++;
  }

  // ones
  std::vector<vtkFFT::ScalarNumber> ones(countIn, 1.0);
  auto resultOnes = vtkFFT::RFft(ones);
  std::vector<vtkFFT::ComplexNumber> expectedOnes(countOut, vtkFFT::ComplexNumber{ 0.0, 0.0 });
  expectedOnes[0] = { static_cast<vtkFFT::ScalarNumber>(countIn), 0.0 };
  is_equal = std::equal(expectedOnes.begin(), expectedOnes.end(), resultOnes.begin(), comparator);
  if (!is_equal)
  {
    status++;
  }

  // ones with vtk arrays
  vtkNew<vtkFFT::vtkScalarNumberArray> vtkOnes;
  vtkOnes->SetNumberOfComponents(1);
  vtkOnes->SetNumberOfTuples(countIn);
  vtkOnes->Fill(1.0);
  auto resultVtkOnes = vtkFFT::RFft(vtkOnes);
  if (resultVtkOnes->GetNumberOfComponents() != 2)
  {
    std::cerr << ".vtkFFT::RFft(vtkOnes) wrong number of components." << std::endl;
    status++;
  }
  else if (resultVtkOnes->GetNumberOfTuples() != countOut)
  {
    std::cerr << ".vtkFFT::RFft(vtkOnes) wrong number of tuples." << std::endl;
    status++;
  }
  else
  {
    auto resRange = vtk::DataArrayTupleRange(resultVtkOnes);
    auto iter = resRange.cbegin();
    if (!vtkMathUtilities::FuzzyCompare((*iter)[0], 16.0) ||
      !vtkMathUtilities::FuzzyCompare((*iter)[1], 0.0))
    {
      std::cerr << ".vtkFFT::RFft(vtkOnes) wrong first value." << std::endl;
      status++;
    }
    for (iter++; iter != resRange.cend(); ++iter)
    {
      if (!vtkMathUtilities::FuzzyCompare((*iter)[0], 0.0) ||
        !vtkMathUtilities::FuzzyCompare((*iter)[1], 0.0))
      {
        std::cerr << ".vtkFFT::RFft(vtkOnes) wrong values." << std::endl;
        status++;
      }
    }
  }

  std::cout << (status ? "..FAILED" : ".PASSED") << std::endl;
  return status;
}

int Test_fft_inverse()
{
  std::cout << "Test_fft_inverse..";

  static constexpr auto countIn = 9;
  static constexpr auto countOut = (countIn - 1) * 2;
  auto comparator = [](vtkFFT::ScalarNumber l, vtkFFT::ScalarNumber r)
  {
    return vtkMathUtilities::FuzzyCompare(
      l, r, std::numeric_limits<vtkFFT::ScalarNumber>::epsilon());
  };
  auto status = 0;
  // zeroes
  std::vector<vtkFFT::ComplexNumber> zeroes(countIn);
  std::generate(zeroes.begin(), zeroes.end(), []() { return vtkFFT::ComplexNumber{ 0.0, 0.0 }; });

  auto resultZeroes = vtkFFT::IRFft(zeroes);

  std::vector<vtkFFT::ScalarNumber> expectedZeroes(countOut);
  std::generate(expectedZeroes.begin(), expectedZeroes.end(), []() { return 0.0; });
  auto is_equal =
    std::equal(expectedZeroes.begin(), expectedZeroes.end(), resultZeroes.begin(), comparator);
  if (!is_equal)
  {
    status++;
  }

  // ones
  std::vector<vtkFFT::ComplexNumber> ones(countIn);
  std::generate(ones.begin(), ones.end(), []() { return vtkFFT::ComplexNumber{ 0.0, 0.0 }; });
  ones[0] = { 16.0, 0.0 };

  auto resultOnes = vtkFFT::IRFft(ones);

  std::vector<vtkFFT::ScalarNumber> expectedOnes(countOut);
  std::generate(expectedOnes.begin(), expectedOnes.end(), []() { return 1.0; });
  is_equal = std::equal(expectedOnes.begin(), expectedOnes.end(), resultOnes.begin(), comparator);
  if (!is_equal)
  {
    status++;
  }

  std::cout << (status ? "..FAILED" : ".PASSED") << std::endl;
  return status;
}

int Test_fft_inverse_cplx()
{
  std::cout << "Test_fft_inverse_cplx..";

  auto comparator = [](vtkFFT::ComplexNumber l, vtkFFT::ComplexNumber r)
  {
    return vtkMathUtilities::FuzzyCompare(
             l.r, r.r, std::numeric_limits<vtkFFT::ScalarNumber>::epsilon()) +
      vtkMathUtilities::FuzzyCompare(
        l.i, r.i, std::numeric_limits<vtkFFT::ScalarNumber>::epsilon());
  };
  int status = 0;

  // zeroes
  std::vector<vtkFFT::ComplexNumber> zeroes(9);
  std::fill(zeroes.begin(), zeroes.end(), vtkFFT::ComplexNumber{ 0.0, 0.0 });
  auto resultZeroes = vtkFFT::IFft(zeroes);
  bool equal = std::equal(zeroes.begin(), zeroes.end(), resultZeroes.begin(), comparator);
  status += static_cast<int>(!equal);

  // ones
  std::vector<vtkFFT::ComplexNumber> signal(9);
  std::fill(zeroes.begin(), zeroes.end(), vtkFFT::ComplexNumber{ 0.0, 0.0 });
  signal[0] = vtkFFT::ComplexNumber{ 9.0, 0.0 };
  std::vector<vtkFFT::ComplexNumber> expectedSignal(9);
  std::fill(zeroes.begin(), zeroes.end(), vtkFFT::ComplexNumber{ 1.0, 0.0 });
  auto resultSignal = vtkFFT::IFft(signal);
  equal =
    std::equal(expectedSignal.begin(), expectedSignal.end(), resultSignal.begin(), comparator);
  status += static_cast<int>(!equal);

  std::cout << (status ? "..FAILED" : ".PASSED") << std::endl;
  return status;
}

int Test_complex_module()
{
  int status = 0;
  std::cout << "Test_complex_module..";

  vtkFFT::ComplexNumber complexNumber1 = { 3, 4 };
  double module1 = vtkFFT::Abs(complexNumber1);
  double test1 = 5;
  if (!vtkMathUtilities::FuzzyCompare(module1, test1, std::numeric_limits<double>::epsilon()))
  {
    std::cerr << "Expected " << test1 << " but got " << module1 << " difference is "
              << module1 - test1 << std::endl;
    status++;
  }

  std::cout << (status ? "..FAILED" : ".PASSED") << std::endl;
  return status;
}

int Test_fftfreq()
{
  int status = 0;
  std::cout << "Test_fftfreq..";

  constexpr double sampleSpacing = 1.0;
  std::vector<double> frequencies = vtkFFT::FftFreq(8, sampleSpacing);
  std::vector<double> expected1 = { 0., 0.125, 0.25, 0.375, -0.5, -0.375, -0.25, -0.125 };

  if (!(frequencies.size() == expected1.size()))
  {
    std::cerr << "Difference size: expected " << expected1.size() << " but got "
              << frequencies.size() << std::endl;
    status++;
  }

  for (size_t i = 0; i < frequencies.size(); i++)
  {
    const double& expected = expected1[i];
    const double& real = frequencies[i];

    if (!vtkMathUtilities::FuzzyCompare(real, expected, std::numeric_limits<double>::epsilon()))
    {
      std::cerr << "Expected " << expected << " but got " << real << " difference is "
                << expected - real << std::endl;
      status++;
    }
  }

  frequencies = vtkFFT::FftFreq(9, sampleSpacing);
  std::vector<double> expected2 = { 0.0, 0.111111111, 0.222222222, 0.333333333, 0.444444444,
    -0.444444444, -0.333333333, -0.222222222, -0.111111111 };
  if (!(frequencies.size() == expected2.size()))
  {
    std::cerr << "Difference size: expected " << expected2.size() << " but got "
              << frequencies.size() << std::endl;
    status++;
  }

  for (size_t i = 0; i < frequencies.size(); i++)
  {
    const double& expected = expected2[i];
    const double& real = frequencies[i];

    if (!vtkMathUtilities::FuzzyCompare(real, expected, 1.0e-6))
    {
      std::cerr << "Expected " << expected << " but got " << real << " difference is "
                << expected - real << std::endl;
      status++;
    }
  }

  std::cout << (status ? "..FAILED" : ".PASSED") << std::endl;
  return status;
}

int Test_rfftfreq()
{
  int status = 0;
  std::cout << "Test_rfftfreq..";

  constexpr auto samplingFrequency = 1000;
  constexpr auto windowLength = 1000;
  double sampleSpacing = 1.0 / samplingFrequency;
  std::vector<double> frequencies = vtkFFT::RFftFreq(windowLength, sampleSpacing);

  std::vector<double> test1((windowLength / 2) + 1);
  std::iota(test1.begin(), test1.end(), 0);

  if (!(frequencies.size() == test1.size()))
  {
    std::cerr << "Difference size: expected " << test1.size() << " but got " << frequencies.size()
              << std::endl;
    status++;
  }

  for (size_t i = 0; i < frequencies.size(); i++)
  {
    const auto& expected = test1[i];
    const auto& real = frequencies[i];

    if (!vtkMathUtilities::FuzzyCompare(real, expected, std::numeric_limits<double>::epsilon()))
    {
      std::cerr << "Expected " << expected << " but got " << real << " difference is "
                << expected - real << std::endl;
      status++;
    }
  }

  std::cout << (status ? "..FAILED" : ".PASSED") << std::endl;
  return status;
}

int Test_fft_direct_inverse()
{
  int status = 0;
  std::cout << "Test_fft_direct_inverse..";

  static constexpr auto countIn = 1000;
  std::vector<double> input(countIn);
  auto val = 0;
  std::generate(input.begin(), input.end(), [&val]() { return std::sin(val++); });

  auto spectrum = vtkFFT::RFft(input);

  auto result = vtkFFT::IRFft(spectrum);

  for (auto i = 0; i < countIn; i++)
  {
    if (!vtkMathUtilities::FuzzyCompare(input[i], result[i], 1e-06))
    {
      std::cerr << "Expected " << input[i] << " but got " << result[i] << " difference is "
                << input[i] - result[i] << std::endl;
      status++;
    }
  }

  std::cout << (status ? "..FAILED" : ".PASSED") << std::endl;
  return status;
}

// Reference values have been generated using the Scipy project
int Test_kernel_generation()
{
  int status = 0;
  std::cout << "Test_kernel_generation..";
  constexpr double epsilon = 0.000001;

  std::vector<double> result(10);
  std::vector<double> kernel(10);

  // ---
  result = { 0., 0.22222222, 0.44444444, 0.66666667, 0.88888889, 0.88888889, 0.66666667, 0.44444444,
    0.22222222, 0. };
  vtkFFT::GenerateKernel1D(kernel.data(), 10, vtkFFT::BartlettGenerator);
  if (!FuzzyCompare(kernel, result, epsilon))
  {
    std::cerr << std::endl << " - Wrong Bartlett kernel";
    status += 1;
  }

  // ---
  result = { -1.38777878e-17, 5.08696327e-02, 2.58000502e-01, 6.30000000e-01, 9.51129866e-01,
    9.51129866e-01, 6.30000000e-01, 2.58000502e-01, 5.08696327e-02, -1.38777878e-17 };
  vtkFFT::GenerateKernel1D(kernel.data(), 10, vtkFFT::BlackmanGenerator);
  if (!FuzzyCompare(kernel, result, epsilon))
  {
    std::cerr << std::endl << " - Wrong Blackman kernel";
    status += 1;
  }

  // ---
  result = { 0., 0.11697778, 0.41317591, 0.75, 0.96984631, 0.96984631, 0.75, 0.41317591, 0.11697778,
    0. };
  vtkFFT::GenerateKernel1D(kernel.data(), 10, vtkFFT::HanningGenerator);
  if (!FuzzyCompare(kernel, result, epsilon))
  {
    std::cerr << std::endl << " - Wrong Hanning kernel";
    status += 1;
  }

  // ---
  result = { 0., 0.34202, 0.642788, 0.866025, 0.984808, 0.984808, 0.866025, 0.642788, 0.34202, 0 };
  vtkFFT::GenerateKernel1D(kernel.data(), 10, vtkFFT::SineGenerator);
  if (!FuzzyCompare(kernel, result, epsilon))
  {
    std::cerr << std::endl << " - Wrong Sine kernel";
    status += 1;
  }

  // ---
  std::fill(result.begin(), result.end(), 1.0);
  vtkFFT::GenerateKernel1D(kernel.data(), 10, vtkFFT::RectangularGenerator);
  if (!FuzzyCompare(kernel, result, epsilon))
  {
    std::cerr << std::endl << " - Wrong Rectangular kernel";
    status += 1;
  }

  std::cout << (status ? "..FAILED" : ".PASSED") << std::endl;
  return status;
}

int Test_csd()
{
  int status = 0;
  std::cout << "Test_csd..";

  constexpr double sample_rate = 500.0e6;
  constexpr double time_step = 1 / sample_rate;
  constexpr double carrier_freq = 10.0e6;
  constexpr std::size_t num_samples = 1024;
  constexpr std::size_t nfft = 256;
  constexpr int noverlap = 128;
  constexpr bool onesided = true;

  std::vector<vtkFFT::ScalarNumber> signal(num_samples);
  std::vector<vtkFFT::ComplexNumber> complexSignal(num_samples);
  vtkNew<vtkFFT::vtkScalarNumberArray> vtkSignal;
  vtkSignal->SetNumberOfComponents(1);
  vtkSignal->SetNumberOfTuples(num_samples);
  {
    unsigned int i = 0;
    auto cplxIter = complexSignal.begin();
    for (vtkFFT::ScalarNumber& x : signal)
    {
      x = std::sin(2.0 * vtkMath::Pi() * carrier_freq * (time_step * i));
      vtkSignal->SetValue(i, x);
      *cplxIter = vtkFFT::ComplexNumber{ x, 0.0 };
      ++cplxIter;
      ++i;
    }
  }

  std::vector<vtkFFT::ScalarNumber> window(nfft);
  vtkFFT::GenerateKernel1D(window.data(), nfft, vtkFFT::RectangularGenerator);

  std::vector<vtkFFT::ScalarNumber> result =
    vtkFFT::Csd(signal, window, sample_rate, noverlap, false, onesided, vtkFFT::Scaling::Density);
  if (!::FuzzyCompare(result, TestResults::ExpectedDensity, 1e-14))
  {
    std::cerr << "..vtkFFT::Csd(scaling=Density) FAILED" << std::endl;
    status++;
  }

  result =
    vtkFFT::Csd(signal, window, sample_rate, noverlap, false, onesided, vtkFFT::Scaling::Spectrum);
  if (!::FuzzyCompare(result, TestResults::ExpectedSpectrum, 1e-8))
  {
    std::cerr << "..vtkFFT::Csd(scaling=Spectrum) FAILED" << std::endl;
    status++;
  }

  result =
    vtkFFT::Csd(signal, window, sample_rate, noverlap, true, onesided, vtkFFT::Scaling::Spectrum);
  if (!::FuzzyCompare(result, TestResults::ExpectedSpectrumDetrend, 1e-8))
  {
    std::cerr << "..vtkFFT::Csd(detrend=true) FAILED" << std::endl;
    status++;
  }

  result = vtkFFT::Csd(
    complexSignal, window, sample_rate, noverlap, false, onesided, vtkFFT::Scaling::Density);
  if (!::FuzzyCompare(result, TestResults::ExpectedComplexDensity, 1e-14))
  {
    std::cerr << "..vtkFFT::Csd(complexSignal) FAILED" << std::endl;
    status++;
  }

  auto vtkResult = vtkFFT::Csd(
    vtkSignal, window, sample_rate, noverlap, true, onesided, vtkFFT::Scaling::Spectrum);
  if (!::FuzzyCompare(vtkResult, TestResults::ExpectedSpectrumDetrend, 1e-8))
  {
    std::cerr << "..vtkFFT::Csd(vtkFFT::vtkScalarNumberArray) FAILED" << std::endl;
    status++;
  }

  unsigned int shape[2];
  std::vector<vtkFFT::ComplexNumber> resSpectro = vtkFFT::Spectrogram(signal, window, sample_rate,
    noverlap, false, onesided, vtkFFT::Scaling::Density, vtkFFT::SpectralMode::STFT, shape);
  for (std::size_t i = 0; i < TestResults::ExpectedStft.size(); ++i)
  {
    if (!FuzzyCompare(TestResults::ExpectedStft[i], resSpectro[i + shape[1]], 1e-9))
    {
      std::cerr << "..vtkFFT::Spectrogram(vtkFFT::STFT) FAILED" << std::endl;
      status++;
      break;
    }
  }

  std::cout << (status ? "..FAILED" : ".PASSED") << std::endl;
  return status;
}

int Test_transpose()
{
  std::cout << "Test_transpose..";
  unsigned int shape[2] = { 4, 3 };
  std::vector<int> input(shape[0] * shape[1]);
  std::iota(input.begin(), input.end(), 0);

  const std::vector<int> inputCopy = input;

  const std::vector<int> result = { 0, 3, 6, 9, 1, 4, 7, 10, 2, 5, 8, 11 };

  vtkFFT::Transpose(input.data(), shape);

  int status = 0;
  if (!std::equal(input.cbegin(), input.cend(), result.cbegin()))
  {
    std::cerr << "transposed matrix FAILED" << std::endl;
    status++;
  }

  if (shape[0] != 3 && shape[1] != 4)
  {
    std::cerr << "shape is not the expected result FAILED" << std::endl;
    status++;
  }

  vtkFFT::Transpose(input.data(), shape);

  if (!std::equal(input.cbegin(), input.cend(), inputCopy.cbegin()))
  {
    std::cerr << "transposed twice matrix FAILED" << std::endl;
    status++;
  }

  std::cout << (status ? "..FAILED" : ".PASSED") << std::endl;
  return status;
}

int Test_octave()
{
  int status = 0;
  std::cout << "Test_octave..";

  const std::array<double, 2> freq500HzOctaveBaseTwo =
    vtkFFT::GetOctaveFrequencyRange(vtkFFT::Octave::Hz_500, /* Octave */
      vtkFFT::OctaveSubdivision::Full,                      /* OctaveSubdivision -> octave */
      true                                                  /* BaseTwoOctave -> base-two */
    );

  const std::array<double, 2> freq500HzThirdOctaveBaseTen =
    vtkFFT::GetOctaveFrequencyRange(vtkFFT::Octave::Hz_500, /* Octave */
      vtkFFT::OctaveSubdivision::SecondThird, /* OctaveSubdivision -> second third-octave */
      false                                   /* BaseTwoOctave -> base-ten */
    );
  const std::array<double, 2> freq8kHzHalfOctaveBaseTwo =
    vtkFFT::GetOctaveFrequencyRange(vtkFFT::Octave::kHz_8, /* Octave */
      vtkFFT::OctaveSubdivision::FirstHalf, /* OctaveSubdivision -> first half-octave */
      true                                  /* BaseTwoOctave -> base-two */
    );

  if (!vtkMathUtilities::FuzzyCompare(
        freq500HzOctaveBaseTwo[0], TestResults::Expected_freq500HzOctaveBaseTwo[0], 0.001) ||
    !vtkMathUtilities::FuzzyCompare(
      freq500HzOctaveBaseTwo[1], TestResults::Expected_freq500HzOctaveBaseTwo[1], 0.001))
  {
    std::cerr << "..Octave frequencies base-two FAILED" << std::endl
              << "Expected (" << TestResults::Expected_freq500HzOctaveBaseTwo[0] << ", "
              << TestResults::Expected_freq500HzOctaveBaseTwo[1] << ") but got ("
              << freq500HzOctaveBaseTwo[0] << ", " << freq500HzOctaveBaseTwo[1] << ")" << std::endl;
    status++;
  }
  if (!vtkMathUtilities::FuzzyCompare(freq500HzThirdOctaveBaseTen[0],
        TestResults::Expected_freq500HzThirdOctaveBaseTen[0], 0.001) ||
    !vtkMathUtilities::FuzzyCompare(
      freq500HzThirdOctaveBaseTen[1], TestResults::Expected_freq500HzThirdOctaveBaseTen[1], 0.001))
  {
    std::cerr << "..Third-octave frequencies base-ten FAILED" << std::endl
              << "Expected (" << TestResults::Expected_freq500HzThirdOctaveBaseTen[0] << ", "
              << TestResults::Expected_freq500HzThirdOctaveBaseTen[1] << ") but got ("
              << freq500HzThirdOctaveBaseTen[0] << ", " << freq500HzThirdOctaveBaseTen[1] << ")"
              << std::endl;
    status++;
  }
  if (!vtkMathUtilities::FuzzyCompare(
        freq8kHzHalfOctaveBaseTwo[0], TestResults::Expected_freq8kHzHalfOctaveBaseTwo[0], 0.001) ||
    !vtkMathUtilities::FuzzyCompare(
      freq8kHzHalfOctaveBaseTwo[1], TestResults::Expected_freq8kHzHalfOctaveBaseTwo[1], 0.001))
  {
    std::cerr << "..Half-octave frequencies base-two FAILED" << std::endl
              << "Expected (" << TestResults::Expected_freq8kHzHalfOctaveBaseTwo[0] << ", "
              << TestResults::Expected_freq8kHzHalfOctaveBaseTwo[1] << ") but got ("
              << freq8kHzHalfOctaveBaseTwo[0] << ", " << freq8kHzHalfOctaveBaseTwo[1] << ")"
              << std::endl;
    status++;
  }

  std::cout << (status ? "..FAILED" : ".PASSED") << std::endl;
  return status;
}

// ----------------------------------------------------------------------------
// Raw test data

const std::vector<vtkFFT::ScalarNumber> TestResults::ExpectedSpectrum = { 3.09720960e-04,
  6.87180446e-04, 9.53471680e-04, 1.77702632e-03, 5.80014112e-03, 4.78877655e-01, 8.66665619e-03,
  1.88031998e-03, 8.02178225e-04, 4.45530776e-04, 2.85094629e-04, 1.99252385e-04, 1.47864401e-04,
  1.14580456e-04, 9.17279964e-05, 7.53179220e-05, 6.31079993e-05, 5.37579837e-05, 4.64259031e-05,
  4.05606037e-05, 3.57886127e-05, 3.18492915e-05, 2.85560685e-05, 2.57724052e-05, 2.33964200e-05,
  2.13507743e-05, 1.95758546e-05, 1.80250762e-05, 1.66615834e-05, 1.54558896e-05, 1.43841629e-05,
  1.34269624e-05, 1.25682935e-05, 1.17948939e-05, 1.10956871e-05, 1.04613601e-05, 9.88403427e-06,
  9.35700593e-06, 8.87454018e-06, 8.43170640e-06, 8.02424543e-06, 7.64846186e-06, 7.30113611e-06,
  6.97945214e-06, 6.68093782e-06, 6.40341531e-06, 6.14495974e-06, 5.90386446e-06, 5.67861181e-06,
  5.46784840e-06, 5.27036409e-06, 5.08507403e-06, 4.91100334e-06, 4.74727394e-06, 4.59309312e-06,
  4.44774379e-06, 4.31057587e-06, 4.18099896e-06, 4.05847580e-06, 3.94251662e-06, 3.83267418e-06,
  3.72853943e-06, 3.62973759e-06, 3.53592483e-06, 3.44678517e-06, 3.36202790e-06, 3.28138513e-06,
  3.20460969e-06, 3.13147326e-06, 3.06176466e-06, 2.99528834e-06, 2.93186301e-06, 2.87132044e-06,
  2.81350432e-06, 2.75826932e-06, 2.70548014e-06, 2.65501072e-06, 2.60674352e-06, 2.56056883e-06,
  2.51638416e-06, 2.47409371e-06, 2.43360787e-06, 2.39484272e-06, 2.35771967e-06, 2.32216502e-06,
  2.28810967e-06, 2.25548875e-06, 2.22424135e-06, 2.19431024e-06, 2.16564164e-06, 2.13818498e-06,
  2.11189269e-06, 2.08671999e-06, 2.06262477e-06, 2.03956735e-06, 2.01751039e-06, 1.99641872e-06,
  1.97625921e-06, 1.95700066e-06, 1.93861370e-06, 1.92107066e-06, 1.90434550e-06, 1.88841370e-06,
  1.87325222e-06, 1.85883936e-06, 1.84515476e-06, 1.83217929e-06, 1.81989500e-06, 1.80828506e-06,
  1.79733375e-06, 1.78702635e-06, 1.77734912e-06, 1.76828931e-06, 1.75983503e-06, 1.75197529e-06,
  1.74469994e-06, 1.73799966e-06, 1.73186588e-06, 1.72629085e-06, 1.72126752e-06, 1.71678958e-06,
  1.71285144e-06, 1.70944819e-06, 1.70657560e-06, 1.70423012e-06, 1.70240885e-06, 1.70110954e-06,
  1.70033059e-06, 8.50035525e-07 };
const std::vector<vtkFFT::ScalarNumber> TestResults::ExpectedSpectrumDetrend = { 2.59851423e-34,
  6.87180446e-04, 9.53471680e-04, 1.77702632e-03, 5.80014112e-03, 4.78877655e-01, 8.66665619e-03,
  1.88031998e-03, 8.02178225e-04, 4.45530776e-04, 2.85094629e-04, 1.99252385e-04, 1.47864401e-04,
  1.14580456e-04, 9.17279964e-05, 7.53179220e-05, 6.31079993e-05, 5.37579837e-05, 4.64259031e-05,
  4.05606037e-05, 3.57886127e-05, 3.18492915e-05, 2.85560685e-05, 2.57724052e-05, 2.33964200e-05,
  2.13507743e-05, 1.95758546e-05, 1.80250762e-05, 1.66615834e-05, 1.54558896e-05, 1.43841629e-05,
  1.34269624e-05, 1.25682935e-05, 1.17948939e-05, 1.10956871e-05, 1.04613601e-05, 9.88403427e-06,
  9.35700593e-06, 8.87454018e-06, 8.43170640e-06, 8.02424543e-06, 7.64846186e-06, 7.30113611e-06,
  6.97945214e-06, 6.68093782e-06, 6.40341531e-06, 6.14495974e-06, 5.90386446e-06, 5.67861181e-06,
  5.46784840e-06, 5.27036409e-06, 5.08507403e-06, 4.91100334e-06, 4.74727394e-06, 4.59309312e-06,
  4.44774379e-06, 4.31057587e-06, 4.18099896e-06, 4.05847580e-06, 3.94251662e-06, 3.83267418e-06,
  3.72853943e-06, 3.62973759e-06, 3.53592483e-06, 3.44678517e-06, 3.36202790e-06, 3.28138513e-06,
  3.20460969e-06, 3.13147326e-06, 3.06176466e-06, 2.99528834e-06, 2.93186301e-06, 2.87132044e-06,
  2.81350432e-06, 2.75826932e-06, 2.70548014e-06, 2.65501072e-06, 2.60674352e-06, 2.56056883e-06,
  2.51638416e-06, 2.47409371e-06, 2.43360787e-06, 2.39484272e-06, 2.35771967e-06, 2.32216502e-06,
  2.28810967e-06, 2.25548875e-06, 2.22424135e-06, 2.19431024e-06, 2.16564164e-06, 2.13818498e-06,
  2.11189269e-06, 2.08671999e-06, 2.06262477e-06, 2.03956735e-06, 2.01751039e-06, 1.99641872e-06,
  1.97625921e-06, 1.95700066e-06, 1.93861370e-06, 1.92107066e-06, 1.90434550e-06, 1.88841370e-06,
  1.87325222e-06, 1.85883936e-06, 1.84515476e-06, 1.83217929e-06, 1.81989500e-06, 1.80828506e-06,
  1.79733375e-06, 1.78702635e-06, 1.77734912e-06, 1.76828931e-06, 1.75983503e-06, 1.75197529e-06,
  1.74469994e-06, 1.73799966e-06, 1.73186588e-06, 1.72629085e-06, 1.72126752e-06, 1.71678958e-06,
  1.71285144e-06, 1.70944819e-06, 1.70657560e-06, 1.70423012e-06, 1.70240885e-06, 1.70110954e-06,
  1.70033059e-06, 8.50035525e-07 };
const std::vector<vtkFFT::ScalarNumber> TestResults::ExpectedDensity = { 1.58577131e-10,
  3.51836388e-10, 4.88177500e-10, 9.09837476e-10, 2.96967226e-09, 2.45185359e-07, 4.43732797e-09,
  9.62723831e-10, 4.10715251e-10, 2.28111757e-10, 1.45968450e-10, 1.02017221e-10, 7.57065734e-11,
  5.86651937e-11, 4.69647342e-11, 3.85627761e-11, 3.23112956e-11, 2.75240877e-11, 2.37700624e-11,
  2.07670291e-11, 1.83237697e-11, 1.63068372e-11, 1.46207070e-11, 1.31954714e-11, 1.19789670e-11,
  1.09315964e-11, 1.00228375e-11, 9.22883903e-12, 8.53073071e-12, 7.91341545e-12, 7.36469139e-12,
  6.87460474e-12, 6.43496629e-12, 6.03898570e-12, 5.68099178e-12, 5.35621635e-12, 5.06062555e-12,
  4.79078704e-12, 4.54376457e-12, 4.31703368e-12, 4.10841366e-12, 3.91601247e-12, 3.73818169e-12,
  3.57347949e-12, 3.42064016e-12, 3.27854864e-12, 3.14621939e-12, 3.02277860e-12, 2.90744924e-12,
  2.79953838e-12, 2.69842641e-12, 2.60355790e-12, 2.51443371e-12, 2.43060426e-12, 2.35166368e-12,
  2.27724482e-12, 2.20701485e-12, 2.14067147e-12, 2.07793961e-12, 2.01856851e-12, 1.96232918e-12,
  1.90901219e-12, 1.85842565e-12, 1.81039351e-12, 1.76475401e-12, 1.72135829e-12, 1.68006919e-12,
  1.64076016e-12, 1.60331431e-12, 1.56762351e-12, 1.53358763e-12, 1.50111386e-12, 1.47011606e-12,
  1.44051421e-12, 1.41223389e-12, 1.38520583e-12, 1.35936549e-12, 1.33465268e-12, 1.31101124e-12,
  1.28838869e-12, 1.26673598e-12, 1.24600723e-12, 1.22615947e-12, 1.20715247e-12, 1.18894849e-12,
  1.17151215e-12, 1.15481024e-12, 1.13881157e-12, 1.12348684e-12, 1.10880852e-12, 1.09475071e-12,
  1.08128905e-12, 1.06840064e-12, 1.05606388e-12, 1.04425848e-12, 1.03296532e-12, 1.02216638e-12,
  1.01184471e-12, 1.00198434e-12, 9.92570214e-13, 9.83588177e-13, 9.75024894e-13, 9.66867815e-13,
  9.59105135e-13, 9.51725753e-13, 9.44719239e-13, 9.38075797e-13, 9.31786239e-13, 9.25841953e-13,
  9.20234880e-13, 9.14957489e-13, 9.10002751e-13, 9.05364125e-13, 9.01035533e-13, 8.97011347e-13,
  8.93286370e-13, 8.89855824e-13, 8.86715333e-13, 8.83860915e-13, 8.81288970e-13, 8.78996266e-13,
  8.76979939e-13, 8.75237474e-13, 8.73766709e-13, 8.72565823e-13, 8.71633331e-13, 8.70968084e-13,
  8.70569263e-13, 4.35218189e-13 };
const std::vector<vtkFFT::ScalarNumber> TestResults::ExpectedComplexDensity = { 1.58577131e-10,
  1.75918194e-10, 2.44088750e-10, 4.54918738e-10, 1.48483613e-09, 1.22592680e-07, 2.21866398e-09,
  4.81361915e-10, 2.05357626e-10, 1.14055879e-10, 7.29842251e-11, 5.10086106e-11, 3.78532867e-11,
  2.93325968e-11, 2.34823671e-11, 1.92813880e-11, 1.61556478e-11, 1.37620438e-11, 1.18850312e-11,
  1.03835146e-11, 9.16188485e-12, 8.15341862e-12, 7.31035352e-12, 6.59773572e-12, 5.98948352e-12,
  5.46579821e-12, 5.01141877e-12, 4.61441951e-12, 4.26536536e-12, 3.95670773e-12, 3.68234569e-12,
  3.43730237e-12, 3.21748314e-12, 3.01949285e-12, 2.84049589e-12, 2.67810817e-12, 2.53031277e-12,
  2.39539352e-12, 2.27188229e-12, 2.15851684e-12, 2.05420683e-12, 1.95800624e-12, 1.86909084e-12,
  1.78673975e-12, 1.71032008e-12, 1.63927432e-12, 1.57310969e-12, 1.51138930e-12, 1.45372462e-12,
  1.39976919e-12, 1.34921321e-12, 1.30177895e-12, 1.25721686e-12, 1.21530213e-12, 1.17583184e-12,
  1.13862241e-12, 1.10350742e-12, 1.07033573e-12, 1.03896980e-12, 1.00928425e-12, 9.81164591e-13,
  9.54506094e-13, 9.29212824e-13, 9.05196756e-13, 8.82377005e-13, 8.60679144e-13, 8.40034593e-13,
  8.20380081e-13, 8.01657155e-13, 7.83811753e-13, 7.66793815e-13, 7.50556931e-13, 7.35058032e-13,
  7.20257106e-13, 7.06116946e-13, 6.92602915e-13, 6.79682745e-13, 6.67326342e-13, 6.55505620e-13,
  6.44194345e-13, 6.33367991e-13, 6.23003615e-13, 6.13079737e-13, 6.03576235e-13, 5.94474246e-13,
  5.85756076e-13, 5.77405120e-13, 5.69405785e-13, 5.61743421e-13, 5.54404260e-13, 5.47375355e-13,
  5.40644527e-13, 5.34200318e-13, 5.28031941e-13, 5.22129242e-13, 5.16482661e-13, 5.11083192e-13,
  5.05922357e-13, 5.00992169e-13, 4.96285107e-13, 4.91794088e-13, 4.87512447e-13, 4.83433907e-13,
  4.79552567e-13, 4.75862877e-13, 4.72359619e-13, 4.69037898e-13, 4.65893119e-13, 4.62920976e-13,
  4.60117440e-13, 4.57478744e-13, 4.55001375e-13, 4.52682062e-13, 4.50517766e-13, 4.48505673e-13,
  4.46643185e-13, 4.44927912e-13, 4.43357666e-13, 4.41930458e-13, 4.40644485e-13, 4.39498133e-13,
  4.38489969e-13, 4.37618737e-13, 4.36883355e-13, 4.36282911e-13, 4.35816666e-13, 4.35484042e-13,
  4.35284632e-13, 4.35218189e-13, 4.35284632e-13, 4.35484042e-13, 4.35816666e-13, 4.36282911e-13,
  4.36883355e-13, 4.37618737e-13, 4.38489969e-13, 4.39498133e-13, 4.40644485e-13, 4.41930458e-13,
  4.43357666e-13, 4.44927912e-13, 4.46643185e-13, 4.48505673e-13, 4.50517766e-13, 4.52682062e-13,
  4.55001375e-13, 4.57478744e-13, 4.60117440e-13, 4.62920976e-13, 4.65893119e-13, 4.69037898e-13,
  4.72359619e-13, 4.75862877e-13, 4.79552567e-13, 4.83433907e-13, 4.87512447e-13, 4.91794088e-13,
  4.96285107e-13, 5.00992169e-13, 5.05922357e-13, 5.11083192e-13, 5.16482661e-13, 5.22129242e-13,
  5.28031941e-13, 5.34200318e-13, 5.40644527e-13, 5.47375355e-13, 5.54404260e-13, 5.61743421e-13,
  5.69405785e-13, 5.77405120e-13, 5.85756076e-13, 5.94474246e-13, 6.03576235e-13, 6.13079737e-13,
  6.23003615e-13, 6.33367991e-13, 6.44194345e-13, 6.55505620e-13, 6.67326342e-13, 6.79682745e-13,
  6.92602915e-13, 7.06116946e-13, 7.20257106e-13, 7.35058032e-13, 7.50556931e-13, 7.66793815e-13,
  7.83811753e-13, 8.01657155e-13, 8.20380081e-13, 8.40034593e-13, 8.60679144e-13, 8.82377005e-13,
  9.05196756e-13, 9.29212824e-13, 9.54506094e-13, 9.81164591e-13, 1.00928425e-12, 1.03896980e-12,
  1.07033573e-12, 1.10350742e-12, 1.13862241e-12, 1.17583184e-12, 1.21530213e-12, 1.25721686e-12,
  1.30177895e-12, 1.34921321e-12, 1.39976919e-12, 1.45372462e-12, 1.51138930e-12, 1.57310969e-12,
  1.63927432e-12, 1.71032008e-12, 1.78673975e-12, 1.86909084e-12, 1.95800624e-12, 2.05420683e-12,
  2.15851684e-12, 2.27188229e-12, 2.39539352e-12, 2.53031277e-12, 2.67810817e-12, 2.84049589e-12,
  3.01949285e-12, 3.21748314e-12, 3.43730237e-12, 3.68234569e-12, 3.95670773e-12, 4.26536536e-12,
  4.61441951e-12, 5.01141877e-12, 5.46579821e-12, 5.98948352e-12, 6.59773572e-12, 7.31035352e-12,
  8.15341862e-12, 9.16188485e-12, 1.03835146e-11, 1.18850312e-11, 1.37620438e-11, 1.61556478e-11,
  1.92813880e-11, 2.34823671e-11, 2.93325968e-11, 3.78532867e-11, 5.10086106e-11, 7.29842251e-11,
  1.14055879e-10, 2.05357626e-10, 4.81361915e-10, 2.21866398e-09, 1.22592680e-07, 1.48483613e-09,
  4.54918738e-10, 2.44088750e-10, 1.75918194e-10 };
const std::vector<vtkFFT::ComplexNumber> TestResults::ExpectedStft = { { -1.04453772e-05,
                                                                         +0.00000000e+00 },
  { -1.08899718e-05, +2.42711241e-06 }, { -1.24639215e-05, +5.50894674e-06 },
  { -1.63062675e-05, +1.06608924e-05 }, { -2.80052035e-05, +2.39474184e-05 },
  { -2.41221833e-04, +2.51665381e-04 }, { 3.07955556e-05, -3.74575082e-05 },
  { 1.36619618e-05, -1.87554989e-05 }, { 8.54211347e-06, -1.29159588e-05 },
  { 6.12883187e-06, -1.00130607e-05 }, { 4.74760160e-06, -8.25282435e-06 },
  { 3.86522519e-06, -7.05933012e-06 }, { 3.25982984e-06, -6.19008306e-06 },
  { 2.82300689e-06, -5.52478926e-06 }, { 2.49572979e-06, -4.99675611e-06 },
  { 2.24324048e-06, -4.56589683e-06 }, { 2.04381678e-06, -4.20658848e-06 },
  { 1.88323268e-06, -3.90164116e-06 }, { 1.75181199e-06, -3.63906305e-06 },
  { 1.64276396e-06, -3.41021492e-06 }, { 1.55119576e-06, -3.20870271e-06 },
  { 1.47350189e-06, -3.02968447e-06 }, { 1.40697332e-06, -2.86942077e-06 },
  { 1.34953977e-06, -2.72497386e-06 }, { 1.29959521e-06, -2.59400108e-06 },
  { 1.25587706e-06, -2.47460944e-06 }, { 1.21738086e-06, -2.36525130e-06 },
  { 1.18329883e-06, -2.26464795e-06 }, { 1.15297505e-06, -2.17173293e-06 },
  { 1.12587213e-06, -2.08560911e-06 }, { 1.10154625e-06, -2.00551597e-06 },
  { 1.07962809e-06, -1.93080417e-06 }, { 1.05980823e-06, -1.86091575e-06 },
  { 1.04182576e-06, -1.79536833e-06 }, { 1.02545940e-06, -1.73374265e-06 },
  { 1.01052043e-06, -1.67567238e-06 }, { 9.96847092e-07, -1.62083595e-06 },
  { 9.84300041e-07, -1.56894986e-06 }, { 9.72758727e-07, -1.51976310e-06 },
  { 9.62118405e-07, -1.47305259e-06 }, { 9.52287706e-07, -1.42861936e-06 },
  { 9.43186627e-07, -1.38628533e-06 }, { 9.34744868e-07, -1.34589064e-06 },
  { 9.26900448e-07, -1.30729131e-06 }, { 9.19598551e-07, -1.27035731e-06 },
  { 9.12790543e-07, -1.23497090e-06 }, { 9.06433157e-07, -1.20102520e-06 },
  { 9.00487795e-07, -1.16842293e-06 }, { 8.94919932e-07, -1.13707536e-06 },
  { 8.89698611e-07, -1.10690138e-06 }, { 8.84796008e-07, -1.07782668e-06 },
  { 8.80187059e-07, -1.04978305e-06 }, { 8.75849136e-07, -1.02270777e-06 },
  { 8.71761769e-07, -9.96543038e-07 }, { 8.67906401e-07, -9.71235513e-07 },
  { 8.64266182e-07, -9.46735874e-07 }, { 8.60825778e-07, -9.22998449e-07 },
  { 8.57571213e-07, -8.99980876e-07 }, { 8.54489728e-07, -8.77643805e-07 },
  { 8.51569655e-07, -8.55950633e-07 }, { 8.48800306e-07, -8.34867259e-07 },
  { 8.46171877e-07, -8.14361876e-07 }, { 8.43675362e-07, -7.94404774e-07 },
  { 8.41302477e-07, -7.74968167e-07 }, { 8.39045590e-07, -7.56026037e-07 },
  { 8.36897662e-07, -7.37553993e-07 }, { 8.34852191e-07, -7.19529141e-07 },
  { 8.32903168e-07, -7.01929969e-07 }, { 8.31045028e-07, -6.84736241e-07 },
  { 8.29272614e-07, -6.67928902e-07 }, { 8.27581142e-07, -6.51489988e-07 },
  { 8.25966170e-07, -6.35402547e-07 }, { 8.24423566e-07, -6.19650567e-07 },
  { 8.22949486e-07, -6.04218909e-07 }, { 8.21540352e-07, -5.89093245e-07 },
  { 8.20192824e-07, -5.74260000e-07 }, { 8.18903788e-07, -5.59706305e-07 },
  { 8.17670337e-07, -5.45419945e-07 }, { 8.16489752e-07, -5.31389316e-07 },
  { 8.15359491e-07, -5.17603387e-07 }, { 8.14277174e-07, -5.04051660e-07 },
  { 8.13240573e-07, -4.90724137e-07 }, { 8.12247599e-07, -4.77611288e-07 },
  { 8.11296292e-07, -4.64704018e-07 }, { 8.10384814e-07, -4.51993647e-07 },
  { 8.09511438e-07, -4.39471877e-07 }, { 8.08674544e-07, -4.27130772e-07 },
  { 8.07872607e-07, -4.14962735e-07 }, { 8.07104193e-07, -4.02960488e-07 },
  { 8.06367954e-07, -3.91117053e-07 }, { 8.05662621e-07, -3.79425733e-07 },
  { 8.04986999e-07, -3.67880094e-07 }, { 8.04339965e-07, -3.56473955e-07 },
  { 8.03720457e-07, -3.45201367e-07 }, { 8.03127478e-07, -3.34056603e-07 },
  { 8.02560087e-07, -3.23034142e-07 }, { 8.02017398e-07, -3.12128661e-07 },
  { 8.01498575e-07, -3.01335022e-07 }, { 8.01002831e-07, -2.90648257e-07 },
  { 8.00529425e-07, -2.80063566e-07 }, { 8.00077658e-07, -2.69576301e-07 },
  { 7.99646871e-07, -2.59181959e-07 }, { 7.99236444e-07, -2.48876174e-07 },
  { 7.98845794e-07, -2.38654708e-07 }, { 7.98474373e-07, -2.28513445e-07 },
  { 7.98121664e-07, -2.18448383e-07 }, { 7.97787183e-07, -2.08455624e-07 },
  { 7.97470475e-07, -1.98531373e-07 }, { 7.97171113e-07, -1.88671928e-07 },
  { 7.96888699e-07, -1.78873676e-07 }, { 7.96622858e-07, -1.69133084e-07 },
  { 7.96373243e-07, -1.59446699e-07 }, { 7.96139529e-07, -1.49811137e-07 },
  { 7.95921415e-07, -1.40223082e-07 }, { 7.95718621e-07, -1.30679280e-07 },
  { 7.95530889e-07, -1.21176533e-07 }, { 7.95357983e-07, -1.11711696e-07 },
  { 7.95199685e-07, -1.02281675e-07 }, { 7.95055797e-07, -9.28834157e-08 },
  { 7.94926141e-07, -8.35139072e-08 }, { 7.94810556e-07, -7.41701734e-08 },
  { 7.94708899e-07, -6.48492707e-08 }, { 7.94621046e-07, -5.55482839e-08 },
  { 7.94546889e-07, -4.62643224e-08 }, { 7.94486338e-07, -3.69945167e-08 },
  { 7.94439319e-07, -2.77360148e-08 }, { 7.94405774e-07, -1.84859787e-08 },
  { 7.94385664e-07, -9.24158072e-09 }, { 7.94378963e-07, +0.00000000e+00 } };

const std::array<double, 2> TestResults::Expected_freq500HzOctaveBaseTwo = { 353.553, 707.107 };
const std::array<double, 2> TestResults::Expected_freq500HzThirdOctaveBaseTen = { 446.684,
  562.341 };
const std::array<double, 2> TestResults::Expected_freq8kHzHalfOctaveBaseTwo = { 5656.854, 8000.0 };
