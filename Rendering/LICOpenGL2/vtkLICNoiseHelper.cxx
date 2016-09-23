/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkLICNoiseHelper.h"

#include "vtkBase64Utilities.h"
#include "vtkGenericDataObjectReader.h"
#include "vtkImageData.h"
#include "vtkNoise200x200.h"

#include <cassert>

namespace
{

/**
integer log base 2
*/
int ilog2(unsigned int n)
{
  if (n == 0)
  {
    return -1;
  }
  unsigned int r = 0;
  while ((n >>= 1))
  {
    r += 1;
  }
  return r;
}

} // end anonymous namespace


//-----------------------------------------------------------------------------
void vtkLICRandomNoise2D::GetValidDimensionAndGrainSize(int type, int &sideLen, int &grainSize)
{
  // perlin noise both side len and grain size need to be powers of 2
  if (type == PERLIN)
  {
    sideLen = 1 << ilog2(sideLen);
    grainSize = 1 << ilog2(grainSize);
  }

  // grains can't be larger than the patch
  if (sideLen < grainSize)
  {
    sideLen = grainSize;
  }

  // generate noise with agiven grainSize size on the patch
  if (sideLen % grainSize)
  {
    // grainSize is not an even divsior of sideLen, adjust sideLen to
    // next larger even divisor
    sideLen = grainSize * (sideLen/grainSize + 1);
  }
}

//-----------------------------------------------------------------------------
int vtkLICRandomNoise2D::ShouldGenerateValue(double prob)
{
  if (this->ProbGen.GetRandomNumber() > (1.0 - prob))
  {
    return 1;
  }
  return 0;
}

//-----------------------------------------------------------------------------
float *vtkLICRandomNoise2D::Generate(
      int type,
      int &sideLen,
      int &grainSize,
      float minNoiseVal,
      float maxNoiseVal,
      int nLevels,
      double impulseProb,
      float impulseBgNoiseVal,
      int seed)
{
  this->GetValidDimensionAndGrainSize(type, sideLen, grainSize);

  switch (type)
  {
    case GAUSSIAN:
      return this->GenerateGaussian(
            sideLen,
            grainSize,
            minNoiseVal,
            maxNoiseVal,
            nLevels,
            impulseProb,
            impulseBgNoiseVal,
            seed);

    case UNIFORM:
      return this->GenerateUniform(
            sideLen,
            grainSize,
            minNoiseVal,
            maxNoiseVal,
            nLevels,
            impulseProb,
            impulseBgNoiseVal,
            seed);

    case PERLIN:
      return this->GeneratePerlin(
            sideLen,
            grainSize,
            minNoiseVal,
            maxNoiseVal,
            nLevels,
            impulseProb,
            impulseBgNoiseVal,
            seed);
  }
  return NULL;
}

//-----------------------------------------------------------------------------
float *vtkLICRandomNoise2D::GenerateUniform(
      int sideLen,
      int grainSize,
      float minNoiseVal,
      float maxNoiseVal,
      int nLevels,
      double impulseProb,
      float impulseBgNoiseVal,
      int seed)
{
  // generate a patch of single pixel random values
  // with a uniform distribution and fixed number of levels
  nLevels = nLevels < 1 ? 1 : nLevels;
  int maxLevel = nLevels-1;
  float delta = maxLevel != 0 ? 1.0f/maxLevel : 0.0f;
  minNoiseVal = minNoiseVal < 0.0f ? 0.0f : minNoiseVal;
  maxNoiseVal = maxNoiseVal > 1.0f ? 1.0f : maxNoiseVal;
  float noiseRange = maxNoiseVal - minNoiseVal;
  impulseProb = impulseProb < 0.0 ? 0.0 : impulseProb;
  impulseProb = impulseProb > 1.0 ? 1.0 : impulseProb;
  impulseBgNoiseVal = impulseBgNoiseVal < 0.0f ? 0.0f : impulseBgNoiseVal;
  impulseBgNoiseVal = impulseBgNoiseVal > 1.0f ? 1.0f : impulseBgNoiseVal;
  this->ValueGen.SetSeed(seed);
  this->ProbGen.SetSeed(seed);
  const int sdim = sideLen/grainSize;
  const int sdim2 = sdim*sdim;
  float *rvals=(float*)malloc(sdim2*sizeof(float));
  for (int i=0; i<sdim2; ++i)
  {
    rvals[i] = impulseBgNoiseVal;
  }
  for (int j=0; j<sdim; ++j)
  {
     for (int i=0; i<sdim; ++i)
     {
       int idx=j*sdim+i;

       if ((impulseProb == 1.0) || this->ShouldGenerateValue(impulseProb))
       {
         int l = static_cast<int>(this->ValueGen.GetRandomNumber()*nLevels);
         l = l > maxLevel ? maxLevel : l; // needed for 1.0
         rvals[idx] = nLevels == 1 ? maxNoiseVal : minNoiseVal + (l*delta) * noiseRange;
       }
     }
  }

  // map single pixel random values onto a patch of values of
  // the requested grain size
  const int ncomp = 2;
  const int dim2 = sideLen*sideLen;
  const int ntup = ncomp*dim2;
  float *noise = (float*)malloc(ntup*sizeof(float));
  for (int j=0; j<sideLen; ++j)
  {
     for (int i=0; i<sideLen; ++i)
     {
       int idx=ncomp*(j*sideLen+i);

       int ii = i/grainSize;
       int jj = j/grainSize;
       int iidx = jj*sdim+ii;

       noise[idx] = rvals[iidx];
       noise[idx+1] = 1.0f; // alpha
     }
  }
  free(rvals);

  return noise;
}

//-----------------------------------------------------------------------------
float *vtkLICRandomNoise2D::GenerateGaussian(
      int sideLen,
      int grainSize,
      float minNoiseVal,
      float maxNoiseVal,
      int nLevels,
      double impulseProb,
      float impulseBgNoiseVal,
      int seed)
{
  // the distribution becomes Gaussian as N goes to infinity
  const int N = 2048;

  // generate a patch of single pixel random values
  // with a gaussian distribution
  impulseProb = impulseProb < 0.0 ? 0.0 : impulseProb;
  impulseProb = impulseProb > 1.0 ? 1.0 : impulseProb;
  impulseBgNoiseVal = impulseBgNoiseVal < 0.0f ? 0.0f : impulseBgNoiseVal;
  impulseBgNoiseVal = impulseBgNoiseVal > 1.0f ? 1.0f : impulseBgNoiseVal;
  this->ValueGen.SetSeed(seed);
  this->ProbGen.SetSeed(seed);
  const int sdim = sideLen/grainSize;
  const int sdim2 = sdim*sdim;
  float *rvals = (float*)malloc(sdim2*sizeof(float));
  for (int i=0; i<sdim2; ++i)
  {
    rvals[i] = 0.0f;
  }
  for (int j=0; j<sdim; ++j)
  {
    for (int i=0; i<sdim; ++i)
    {
      int idx = j*sdim+i;

      if ((impulseProb == 1.0) || this->ShouldGenerateValue(impulseProb))
      {
        double val = 0.0;
        for (int q=0; q<N; ++q)
        {
          val += this->ValueGen.GetRandomNumber();
        }
        rvals[idx] = static_cast<float>(val);
      }
    }
  }

  // normalize noise field from eps to nLevels onto 0 to 1
  // and restrict to the requested number of levels
  // min/max
  float minVal = static_cast<float>(N+1);
  float maxVal = 0.0f;
  for (int i=0; i<sdim2; ++i)
  {
    // for impulseProb < 1 background is 0 but pixels that are touched
    // have a much larger value, after normalization the gaussian
    // distribution is compressed and localized near 1. We can fix this
    // by ignoring zero values.
    minVal = impulseProb == 1.0 ?
            (rvals[i] < minVal ? rvals[i] : minVal) :
            (rvals[i] < minVal && rvals[i] > 0.0f ? rvals[i] : minVal);

    maxVal = rvals[i]>maxVal ? rvals[i] : maxVal;
  }
  float maxMinDiff = maxVal-minVal;
  // because we ignore zero when impulseProb<1 we have to be careful
  // here so that we can support one noise level.
  minVal = maxMinDiff == 0.0f ? 0.0f : minVal;
  maxMinDiff = maxMinDiff == 0.0f ? (maxVal == 0.0f ? 1.0f : maxVal) : maxMinDiff;

  nLevels = nLevels < 1 ? 1 : nLevels;
  int maxLevel = nLevels-1;
  float delta = maxLevel != 0 ? 1.0f/maxLevel : 0.0f;
  minNoiseVal = minNoiseVal < 0.0f ? 0.0f : minNoiseVal;
  maxNoiseVal = maxNoiseVal > 1.0f ? 1.0f : maxNoiseVal;
  float noiseRange = maxNoiseVal - minNoiseVal;
  for (int i=0; i<sdim2; ++i)
  {
    // normalize
    float val = rvals[i] < minVal ? rvals[i] : (rvals[i] - minVal)/maxMinDiff;
    // restrict
    int l = static_cast<int>(val*nLevels);
    l = l > maxLevel ? maxLevel : l;
    rvals[i]
       = rvals[i] < minVal ? impulseBgNoiseVal
       : nLevels == 1 ? maxNoiseVal : minNoiseVal + (l*delta) * noiseRange;
  }

  // map single pixel random values onto a patch of values of
  // the requested grain size
  const int ncomp = 2;
  const int dim2 = sideLen*sideLen;
  const int ntup = ncomp*dim2;
  float *noise = (float*)malloc(ntup*sizeof(float));
  for (int j=0; j<sideLen; ++j)
  {
     for (int i=0; i<sideLen; ++i)
     {
       int idx = ncomp*(j*sideLen+i);

       int ii = i/grainSize;
       int jj = j/grainSize;
       int iidx = jj*sdim+ii;

       noise[idx] = rvals[iidx];
       noise[idx+1] = 1.0; // alpha
     }
  }
  free(rvals);

  return noise;
}

//-----------------------------------------------------------------------------
float *vtkLICRandomNoise2D::GeneratePerlin(
      int sideLen,
      int grainSize,
      float minNoiseVal,
      float maxNoiseVal,
      int nLevels,
      double impulseProb,
      float impulseBgNoiseVal,
      int seed)
{
  // note: requires power of 2 sideLen, and sideLen > grainSize
  const int ncomp = 2;
  const int dim2 = sideLen*sideLen;
  const int ntup = ncomp*dim2;
  float *noise = static_cast<float*>(malloc(ntup*sizeof(float)));
  for (int i=0; i<ntup; i+=2)
  {
    noise[i  ] = 0.0f;
    noise[i+1] = 1.0f; // alpha channel
  }

  impulseProb = impulseProb < 0.0 ? 0.0 : impulseProb;
  impulseProb = impulseProb > 1.0 ? 1.0 : impulseProb;
  impulseBgNoiseVal = impulseBgNoiseVal < 0.0f ? 0.0f : impulseBgNoiseVal;
  impulseBgNoiseVal = impulseBgNoiseVal > 1.0f ? 1.0f : impulseBgNoiseVal;
  minNoiseVal = minNoiseVal < 0.0f ? 0.0f : minNoiseVal;
  maxNoiseVal = maxNoiseVal > 1.0f ? 1.0f : maxNoiseVal;

  //int nIter = ilog2(static_cast<unsigned int>(sideLen-1<nLevels ? sideLen-1 : nLevels));
  int nIter = ilog2(static_cast<unsigned int>(grainSize));
  for (int w=0; w<nIter; ++w)
  {
    // reduce range with grain size
    float levelNoiseMin = 0.0f;
    float levelNoiseMax = 0.1f + 0.9f/static_cast<float>(1<<(nIter-1-w));
    //float levelNoiseMax = 1.0f - levelNoiseMin;
    // generate a level of noise
    int levelGrainSize = 1<<w;
    float *levelNoise = GenerateGaussian(
          sideLen,
          levelGrainSize,
          levelNoiseMin,
          levelNoiseMax,
          nLevels,
          impulseProb,
          impulseBgNoiseVal,
          seed);
    /*// smooth
    int nsp = w;
    for (int k=0; k<nsp; ++k)
      {
      for (int j=0; j<sideLen; ++j)
        {
         for (int i=0; i<sideLen; ++i)
           {
           float K[9] = {
             0.0191724, 0.100120, 0.0191724,
             0.1001200, 0.522831, 0.1001200,
             0.0191724, 0.100120, 0.0191724
             };
           float val=0.0;
           for (int q=0; q<3; ++q)
             {
             for (int p=0; p<3; ++p)
               {
               int ii = i+p-1;
               ii = ii < 0 ? i : ii;
               ii = ii >= sideLen ? i : ii;
               int jj = j+q-1;
               jj = jj < 0 ? j : jj;
               jj = jj >= sideLen ? j : jj;
               int idx = 2*(sideLen*jj+ii);
               val += levelNoise[idx]*K[q*3+p];
               }
             }
           levelNoise[2*(sideLen*j+i)] = val;
           }
        }
      }*/
    // accumulate
    for (int i=0; i<ntup; i+=2)
    {
      noise[i] += levelNoise[i];
    }
    free(levelNoise);
  }
  // normalize
  float minVal = static_cast<float>(nIter+1);
  float maxVal = 0.0f;
  for (int i=0; i<ntup; i+=2)
  {
    float val = noise[i];
    minVal = val<minVal ? val : minVal;
    maxVal = val>maxVal ? val : maxVal;
  }
  float maxMinDiff = maxVal - minVal;
  if ( maxMinDiff <= 0.0f )
  {
    maxMinDiff = 1.0f;
    minVal = 0.0f;
  }
  for (int i=0; i<ntup; i+=2)
  {
    noise[i] = (noise[i] - minVal) / maxMinDiff;
  }
  return noise;
}

/**
Load a predefiined texture that has been "pickled" in a string.
This texture is 200x200 pixles, has a Gaussian distribution, and
intensities ranging between 0 and 206. This is the texture that
is used when GenerateNoiseTexture is disabled.
*/
vtkImageData *vtkLICRandomNoise2D::GetNoiseResource()
{
  std::string base64string;
  for (unsigned int cc=0; cc < file_noise200x200_vtk_nb_sections; cc++)
  {
    base64string += reinterpret_cast<const char*>(file_noise200x200_vtk_sections[cc]);
  }

  unsigned char* binaryInput
     = new unsigned char[file_noise200x200_vtk_decoded_length + 10];

  unsigned long binarylength = static_cast<unsigned long>(
    vtkBase64Utilities::DecodeSafely(
        reinterpret_cast<const unsigned char*>(base64string.c_str()),
        base64string.length(),
        binaryInput,
        file_noise200x200_vtk_decoded_length + 10));

  assert("check valid_length"
    && (binarylength == file_noise200x200_vtk_decoded_length));

  vtkGenericDataObjectReader* reader = vtkGenericDataObjectReader::New();
  reader->ReadFromInputStringOn();

  reader->SetBinaryInputString(
        reinterpret_cast<char*>(binaryInput),
        static_cast<int>(binarylength));

  reader->Update();
  vtkImageData* data = vtkImageData::New();
  data->ShallowCopy(reader->GetOutput());

  delete [] binaryInput;
  reader->Delete();
  return data;
}
