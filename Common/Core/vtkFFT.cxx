/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkFFT.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkFFT.h"

#include "vtkMath.h"
#include "vtkObjectFactory.h"

//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkFFT);

namespace
{
//------------------------------------------------------------------------------
int fft_frame_size_bits(int dataSize)
{
  int k = 0; //  Length n in bits
  while ((1 << k) < dataSize)
  {
    k++;
  }
  return k;
}

//------------------------------------------------------------------------------
int fft_frame_size(int dataSize)
{
  return 1 << fft_frame_size_bits(dataSize);
}

//------------------------------------------------------------------------------
void make_frame(
  vtkFFT::ComplexNumber* __restrict out, const double* __restrict in, const int inCount)
{
  int n = fft_frame_size(inCount);

  for (int i = 0; i < inCount; i++)
  {
    out[i].real(static_cast<vtkFFT::FFT_UNIT_TYPE>(in[i]));
    out[i].imag(0.0);
  }

  if (inCount < n)
  {
    memset(out + inCount, 0, sizeof(vtkFFT::ComplexNumber) * (n - inCount));
  }
}

//------------------------------------------------------------------------------
vtkFFT::ComplexNumber* prepareComplexArray(const double* in, const int inCount, int* outCount)
{
  int n = fft_frame_size(inCount);
  vtkFFT::ComplexNumber* tmp = new vtkFFT::ComplexNumber[n];
  make_frame(tmp, in, inCount);
  *outCount = n;
  return tmp;
}

//------------------------------------------------------------------------------
vtkFFT::ComplexNumber* prepareComplexArray(
  const vtkFFT::ComplexNumber* in, const int inCount, int* outCount)
{
  int n = fft_frame_size(inCount);

  vtkFFT::ComplexNumber* tmp = new vtkFFT::ComplexNumber[n];
  for (int i = 0; i < inCount; i++)
  {
    tmp[i] = in[i];
  }

  for (int i = inCount; i < n; i++)
  {
    tmp[i] = { 0, 0 };
  }

  *outCount = n;
  return tmp;
}

//------------------------------------------------------------------------------
void fft_in_out_perm(int* perm, int k)
{
  int high1 = -1, n = 1 << k;

  perm[0] = 0;
  for (int i = 1; i < n; i++)
  {
    if ((i & (i - 1)) == 0) // Double check. If i is, then i-1 will consist of a pile of units.
    {
      high1++;
    }
    perm[i] = perm[i ^ (1 << high1)];  // Turn over the rest
    perm[i] |= (1 << (k - high1 - 1)); // Add a high bit
  }
}

//------------------------------------------------------------------------------
void fft_roots(vtkFFT::ComplexNumber* roots, int n)
{
  for (int i = 0; i < n / 2; i++)
  {
    double alpha = 2 * vtkMath::Pi() * i / n;
    roots[i].real(static_cast<vtkFFT::FFT_UNIT_TYPE>(cos(alpha)));
    roots[i].imag(static_cast<vtkFFT::FFT_UNIT_TYPE>(sin(alpha)));
  }
}

//------------------------------------------------------------------------------
void fft_core(vtkFFT::ComplexNumber* __restrict out, const vtkFFT::ComplexNumber* __restrict in,
  const vtkFFT::ComplexNumber* __restrict roots, const int* __restrict rev, int n)
{
  for (int i = 0; i < n; i++)
  {
    out[i] = in[rev[i]];
  }

  for (int len = 1; len < n; len <<= 1)
  {
    int rstep = n / (len * 2);
    for (int pdest = 0; pdest < n; pdest += len)
    {
      const vtkFFT::ComplexNumber* __restrict r = roots;
      for (int i = 0; i < len; i++, pdest++, r += rstep)
      {
        vtkFFT::ComplexNumber* __restrict a = out + pdest;
        vtkFFT::ComplexNumber* __restrict b = a + len;

        auto real = r->real() * b->real() - r->imag() * b->imag();
        auto imag = r->imag() * b->real() + r->real() * b->imag();

        b->real(a->real() - real);
        b->imag(a->imag() - imag);
        a->real(a->real() + real);
        a->imag(a->imag() + imag);
      }
    }
  }
}

//------------------------------------------------------------------------------
void fft(const vtkFFT::ComplexNumber* in, const int inCount, vtkFFT::ComplexNumber*& outData)
{
  int k = fft_frame_size_bits(inCount);
  int n = 1 << k;

  int* rev = new int[n];
  vtkFFT::ComplexNumber* roots = new vtkFFT::ComplexNumber[n / 2];

  fft_in_out_perm(rev, k);
  fft_roots(roots, n);

  if (outData == 0x0)
  {
    outData = new vtkFFT::ComplexNumber[n];
  }

  fft_core(outData, in, roots, rev, n);

  delete[] roots;
  delete[] rev;
}

//------------------------------------------------------------------------------
void fft_post_inverse(vtkFFT::ComplexNumber* data, int n)
{
  auto tmpCoef = 1.0 / n;
  for (int i = 0; i < n; i++)
  {
    data[i] *= tmpCoef;
  }

  for (int i = 1; i < ((n - 1) / 2 + 1); i++)
  {
    std::swap(data[i], data[n - i]);
  }
}
} // end namespace

//------------------------------------------------------------------------------
void vtkFFT::FftDirect(const double* in, const int inCount, int* outCount, ComplexNumber*& outData)
{
  ComplexNumber* tmp = prepareComplexArray(in, inCount, outCount);
  fft(tmp, *outCount, outData);
  delete[] tmp;
}

//------------------------------------------------------------------------------
void vtkFFT::FftInverse(
  const ComplexNumber* in, const int inCount, int* outCount, ComplexNumber*& outData)
{
  ComplexNumber* tmp = prepareComplexArray(in, inCount, outCount);
  fft(tmp, *outCount, outData);
  fft_post_inverse(outData, fft_frame_size(inCount));
  delete[] tmp;
}

//------------------------------------------------------------------------------
void vtkFFT::ComplexesToDoubles(
  double* __restrict out, const ComplexNumber* __restrict in, const int inCount)
{
  for (int i = 0; i < inCount; i++)
  {
    out[i] = in[i].real();
  }
}

//------------------------------------------------------------------------------
double vtkFFT::ComplexModule(const ComplexNumber& in)
{
  return sqrt(in.real() * in.real() + in.imag() * in.imag());
}

//------------------------------------------------------------------------------
std::vector<double> vtkFFT::RFftFreq(int windowLength, double sampleSpacing)
{
  std::vector<double> res;
  double val = 1.0 / (windowLength * sampleSpacing);
  int N = (windowLength + 1) / 2;
  for (int i = 0; i < N; i++)
  {
    res.push_back(i * val);
  }

  return res;
}

//------------------------------------------------------------------------------
void vtkFFT::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
