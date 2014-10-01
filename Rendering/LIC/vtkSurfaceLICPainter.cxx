/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSurfaceLICPainter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSurfaceLICPainter.h"

#include "vtkInformation.h"
#include "vtkScalarsToColors.h"
#include "vtkScalarsToColorsPainter.h"

#include "vtkPainterCommunicator.h"
#include "vtkSurfaceLICComposite.h"
#include "vtkBase64Utilities.h"
#include "vtkBoundingBox.h"
#include "vtkCellData.h"
#include "vtkColorMaterialHelper.h"
#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataSet.h"
#include "vtkFrameBufferObject2.h"
#include "vtkRenderbuffer.h"
#include "vtkPixelBufferObject.h"
#include "vtkPixelExtent.h"
#include "vtkGarbageCollector.h"
#include "vtkGenericDataObjectReader.h"
#include "vtkImageData.h"
#include "vtkLightingHelper.h"
#include "vtkLineIntegralConvolution2D.h"
#include "vtkMatrix4x4.h"
#include "vtkMath.h"
#include "vtkMinimalStandardRandomSequence.h"
#include "vtkNoise200x200.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkOpenGLExtensionManager.h"
#include "vtkOpenGLModelViewProjectionMonitor.h"
#include "vtkOpenGLLightMonitor.h"
#include "vtkBackgroundColorMonitor.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkProperty.h"
#include "vtkRenderer.h"
#include "vtkCamera.h"
#include "vtkShader2Collection.h"
#include "vtkShader2.h"
#include "vtkShaderProgram2.h"
#include "vtkSmartPointer.h"
#include "vtkTextureObject.h"
#include "vtkUniformVariables.h"
#include "vtkWeakPointer.h"
#include "vtkUnsignedCharArray.h"
#include "vtkFloatArray.h"
#include "vtkgl.h"
#include "vtkOpenGLError.h"

#include <cassert>
#include <cstring>
#include <algorithm>
#include <limits>
#include <vector>
#include <deque>
#include <cstdlib>

using std::vector;
using std::deque;
using std::string;

typedef vtkLineIntegralConvolution2D vtkLIC2D;

// use parallel timer for benchmarks and scaling
// if not defined vtkTimerLOG is used.
// #define vtkSurfaceLICPainterTIME
#if !defined(vtkSurfaceLICPainterTIME)
#include "vtkTimerLog.h"
#endif

// write intermediate results to disk for debugging
#define vtkSurfaceLICPainterDEBUG 0
#if vtkSurfaceLICPainterDEBUG >= 2
#include "vtkTextureIO.h"
#include <sstream>
using std::ostringstream;
//----------------------------------------------------------------------------
static
string mpifn(vtkPainterCommunicator *comm, const char *fn)
{
  ostringstream oss;
  oss << comm->GetRank() << "_" << fn;
  return oss.str();
}
#endif

// Enable stream min/max computations. Streaming is accomplished
// via PBO+glReadPixels to read just the regions we are updating.
// Without streaming PBO+glGetTexImage is used to uplaod the entire
// screen sized texture, of which (in parallel) we are updating only
// a small part of.
#define STREAMING_MIN_MAX

// store depths in a texture. if not a renderbuffer object is used.
// NOTE: this must be on because of a slight diffference in how
// texture filtering is implemented by os mesa.
#define USE_DEPTH_TEXTURE

extern const char* vtkSurfaceLICPainter_GeomFs;
extern const char* vtkSurfaceLICPainter_GeomVs;
extern const char* vtkSurfaceLICPainter_SC;
extern const char* vtkSurfaceLICPainter_CE;
extern const char* vtkSurfaceLICPainter_DCpy;

namespace vtkSurfaceLICPainterUtil
{

inline
double vtkClamp(double val, const double& min, const double& max)
{
  val = (val < min)? min : val;
  val = (val > max)? max : val;
  return val;
}

// Description
// find min/max of unmasked fragments across all regions
// download the entire screen then search each region
void FindMinMax(
      vtkTextureObject *tex,
      deque<vtkPixelExtent> &blockExts,
      float &min,
      float &max)
{
  // download entire screen
  vtkPixelBufferObject *pbo = tex->Download();
  float *pHSLColors = static_cast<float*>(pbo->MapPackedBuffer());
  // search regions
  int size0 = tex->GetWidth();
  size_t nBlocks = blockExts.size();
  for (size_t e=0; e<nBlocks; ++e)
    {
    const vtkPixelExtent &blockExt = blockExts[e];
    for (int j=blockExt[2]; j<=blockExt[3]; ++j)
      {
      for (int i=blockExt[0]; i<=blockExt[1]; ++i)
        {
        size_t id = 4*(size0*j+i);
        if (pHSLColors[id+3] != 0.0f)
          {
          float L = pHSLColors[id+2];
          min = min > L ? L : min;
          max = max < L ? L : max;
          }
        }
      }
    }
  pbo->UnmapPackedBuffer();
  pbo->Delete();
  #if  vtkSurfaceLICPainterDEBUG >= 1
  cerr << "min=" << min << " max=" << max << endl;
  #endif
}

// Description
// find min/max of unmasked fragments across all regions
// download each search each region individually
void StreamingFindMinMax(
      vtkFrameBufferObject2 *fbo,
      deque<vtkPixelExtent> &blockExts,
      float &min,
      float &max)
{
  size_t nBlocks = blockExts.size();
  // initiate download
  fbo->ActivateReadBuffer(1U);
  vtkStaticCheckFrameBufferStatusMacro(vtkgl::FRAMEBUFFER_EXT);
  vector<vtkPixelBufferObject*> pbos(nBlocks, NULL);
  for (size_t e=0; e<nBlocks; ++e)
    {
    pbos[e] = fbo->Download(
          blockExts[e].GetData(),
          VTK_FLOAT,
          4,
          GL_FLOAT,
          GL_RGBA);
    }
  fbo->RemoveTexColorAttachment(vtkgl::DRAW_FRAMEBUFFER_EXT, 0U);
  fbo->RemoveTexColorAttachment(vtkgl::DRAW_FRAMEBUFFER_EXT, 1U);
  fbo->DeactivateDrawBuffers();
  fbo->DeactivateReadBuffer();
  // map search and release each region
  for (size_t e=0; e<nBlocks; ++e)
    {
    vtkPixelBufferObject *&pbo = pbos[e];
    float *pColors = (float*)pbo->MapPackedBuffer();

    size_t n = blockExts[e].Size();
    for (size_t i = 0; i<n; ++i)
      {
      if (pColors[4*i+3] != 0.0f)
        {
        float L = pColors[4*i+2];
        min = min > L ? L : min;
        max = max < L ? L : max;
        }
      }
    pbo->UnmapPackedBuffer();
    pbo->Delete();
    pbo = NULL;
    }
  #if vtkSurfaceLICPainterDEBUG >= 1
  cerr << "min=" << min << " max=" << max << endl;
  #endif
}

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

/**
An interface to a random number generator. We can't use
c stdlib since we're not gauranteed to get consistent.
sequences across platform or library version and that
would prevent consistent output during regression tests.
*/
class RandomNumberGeneratorInterface
{
public:
  RandomNumberGeneratorInterface()
    {
    this->RNG = vtkMinimalStandardRandomSequence::New();
    }

  ~RandomNumberGeneratorInterface()
    {
    this->RNG->Delete();
    }

  /**
  Seed the random number generator
  */
  void SetSeed(int seedVal)
    {
    #if 0
    srand(seedVal);
    #else
    this->RNG->SetSeed(seedVal);
    #endif
    }

  /**
  Get a random number in the range of 0 to 1.
  */
  double GetRandomNumber()
  {
    #if 0
    double val = static_cast<double>(rand())/RAND_MAX;
    #else
    double val = this->RNG->GetValue();
    this->RNG->Next();
    #endif
    return val;
  }

private:
  void operator=(const RandomNumberGeneratorInterface &); // not implemented
  RandomNumberGeneratorInterface(const RandomNumberGeneratorInterface &); // not implemented

private:
  vtkMinimalStandardRandomSequence *RNG;
};

/**
2D Noise Generator. Generate arrays for use as noise texture
in the LIC algorithm. Can generate noise with uniform or Gaussian
distributions, with a desired number of noise levels, and a
desired frequency (f < 1 is impulse noise).
*/
class RandomNoise2D
{
public:
  RandomNoise2D(){}

  // Description:
  // Generate a patch of random gray scale values along with an
  // alpha channel (in vtk array format). The data should be
  // deleted by later calling DeleteValues. Grain size and sideLen
  // may be modified to match the noise generator requirements,
  // returned arrays will be sized accordingly.
  //
  // type              - UNIFORM=0, GAUSSIAN=1, PERLIN=2
  // sideLen           - side length of square patch in pixels (in/out)
  // grainSize         - grain size of noise values in pixels (in/out)
  // nLevels           - number of noise intesity levels
  // minNoiseVal       - set the min for noise pixels (position distribution)
  // maxNoiseVal       - set the max for noise pixels (position distribution)
  // impulseProb       - probability of impulse noise,1 touches every pixel
  // impulseBgNoiseVal - set the background color for impulse noise
  // seed              - seed for random number generator
  enum {
    UNIFORM = 0,
    GAUSSIAN = 1,
    PERLIN = 2
    };
  float *Generate(
        int type,
        int &sideLen,
        int &grainLize,
        float minNoiseVal,
        float maxNoiseVal,
        int nLevels,
        double impulseProb,
        float impulseBgNoiseVal,
        int seed);

  // Description
  // Delete the passed in array of values.
  void DeleteValues(unsigned char *vals){ free(vals); }

private:
  // Description:
  // Generate noise with a uniform distribution.
  float *GenerateUniform(
        int sideLen,
        int grainLize,
        float minNoiseVal,
        float maxNoiseVal,
        int nLevels,
        double impulseProb,
        float impulseBgNoiseVal,
        int seed);

  // Description:
  // Generate noise with a Gaussian distribution.
  float *GenerateGaussian(
        int sideLen,
        int grainLize,
        float minNoiseVal,
        float maxNoiseVal,
        int nLevels,
        double impulseProb,
        float impulseBgNoiseVal,
        int seed);

  // Description:
  // Generate Perlin noise with a Gaussian distribution.
  float *GeneratePerlin(
        int sideLen,
        int grainLize,
        float minNoiseVal,
        float maxNoiseVal,
        int nLevels,
        double impulseProb,
        float impulseBgNoiseVal,
        int seed);

  // Description:
  // A way of controling the probability (from 0.0 to 1.0) that you
  // generate values. returns 1 if you should generate a value.
  // for example this is used to control the frequency of impulse
  // noise.
  int ShouldGenerateValue(double prob);

  // Description:
  // Get a valid the length of the side of the patch and grains size in pixels
  // given a desired patch side length and a grain size. This ensures that all
  // grains are the same size.
  void GetValidDimensionAndGrainSize(int type, int &dim, int &grainSize);

private:
  RandomNumberGeneratorInterface ValueGen;
  RandomNumberGeneratorInterface ProbGen;
};

//-----------------------------------------------------------------------------
void RandomNoise2D::GetValidDimensionAndGrainSize(int type, int &sideLen, int &grainSize)
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
int RandomNoise2D::ShouldGenerateValue(double prob)
{
  if (this->ProbGen.GetRandomNumber() > (1.0 - prob))
    {
    return 1;
    }
  return 0;
}

//-----------------------------------------------------------------------------
float *RandomNoise2D::Generate(
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
float *RandomNoise2D::GenerateUniform(
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
  float delta = 1.0f/maxLevel;
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
float *RandomNoise2D::GenerateGaussian(
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
  float delta = 1.0f/maxLevel;
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
float *RandomNoise2D::GeneratePerlin(
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
vtkImageData *vtkGetNoiseResource()
{
  std::string base64string;
  for (unsigned int cc=0; cc < file_noise200x200_vtk_nb_sections; cc++)
    {
    base64string += reinterpret_cast<const char*>(file_noise200x200_vtk_sections[cc]);
    }

  unsigned char* binaryInput
     = new unsigned char[file_noise200x200_vtk_decoded_length + 10];

  unsigned long binarylength = vtkBase64Utilities::Decode(
        reinterpret_cast<const unsigned char*>(base64string.c_str()),
        static_cast<unsigned long>(base64string.length()),
        binaryInput);

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

};
using namespace vtkSurfaceLICPainterUtil;

/**
Internal data
*/
class vtkSurfaceLICPainter::vtkInternals
{
public:
  vtkSmartPointer<vtkOpenGLLightMonitor> LightMonitor[vtkLightingHelper::VTK_MAX_LIGHTS];
  vtkSmartPointer<vtkOpenGLModelViewProjectionMonitor> ViewMonitor;
  vtkSmartPointer<vtkBackgroundColorMonitor> BGMonitor;

  vtkWeakPointer<vtkOpenGLRenderWindow> Context;
  bool GLSupport;
  int Viewsize[2];
  long long LastInputDataSetMTime;
  long long LastPropertyMTime;
  long long LastLUTMTime;

  deque<vtkPixelExtent> BlockExts;
  vtkPixelExtent DataSetExt;

  bool ContextNeedsUpdate;
  bool OutputDataNeedsUpdate;
  bool CommunicatorNeedsUpdate;
  bool GeometryNeedsUpdate;
  bool GatherNeedsUpdate;
  bool LICNeedsUpdate;
  bool ColorNeedsUpdate;

  vtkPainterCommunicator *Communicator;

  #ifdef USE_DEPTH_TEXTURE
  vtkSmartPointer<vtkTextureObject> DepthImage;
  #else
  vtkSmartPointer<vtkRenderbuffer> DepthImage;
  #endif
  vtkSmartPointer<vtkTextureObject> GeometryImage;
  vtkSmartPointer<vtkTextureObject> VectorImage;
  vtkSmartPointer<vtkTextureObject> CompositeVectorImage;
  vtkSmartPointer<vtkTextureObject> MaskVectorImage;
  vtkSmartPointer<vtkTextureObject> CompositeMaskVectorImage;
  vtkSmartPointer<vtkTextureObject> NoiseImage;
  vtkSmartPointer<vtkTextureObject> LICImage;
  vtkSmartPointer<vtkTextureObject> RGBColorImage;
  vtkSmartPointer<vtkTextureObject> HSLColorImage;
  vtkSmartPointer<vtkImageData> Noise;

  vtkSmartPointer<vtkFrameBufferObject2> FBO;

  vtkSmartPointer<vtkShaderProgram2> RenderGeometryPass;
  vtkSmartPointer<vtkShaderProgram2> ColorPass;
  vtkSmartPointer<vtkShaderProgram2> ColorEnhancePass;
  vtkSmartPointer<vtkShaderProgram2> CopyPass;
  vtkSmartPointer<vtkLightingHelper> LightingHelper;
  vtkSmartPointer<vtkColorMaterialHelper> ColorMaterialHelper;

  vtkSmartPointer<vtkSurfaceLICComposite> Compositor;
  vtkSmartPointer<vtkLineIntegralConvolution2D> LICer;

  int FieldAssociation;
  int FieldAttributeType;
  std::string FieldName;
  bool FieldNameSet;
  bool HasVectors;



  // Description:
  // Constructor
  vtkInternals()
    {
    const int nLights = vtkLightingHelper::VTK_MAX_LIGHTS;
    for (int i=0; i<nLights; ++i)
      {
      this->LightMonitor[i] = vtkSmartPointer<vtkOpenGLLightMonitor>::New();
      this->LightMonitor[i]->SetLightId(i);
      }

    this->ViewMonitor = vtkSmartPointer<vtkOpenGLModelViewProjectionMonitor>::New();
    this->BGMonitor = vtkSmartPointer<vtkBackgroundColorMonitor>::New();

    this->Viewsize[0] = this->Viewsize[1] = 0;
    this->LastInputDataSetMTime = 0;
    this->LastPropertyMTime = 0;
    this->LastLUTMTime = 0;
    this->GLSupport = false;

    this->ContextNeedsUpdate = true;
    this->OutputDataNeedsUpdate = true;
    this->CommunicatorNeedsUpdate = true;
    this->GeometryNeedsUpdate = true;
    this->LICNeedsUpdate = true;
    this->GatherNeedsUpdate = true;
    this->ColorNeedsUpdate = true;

    this->Communicator = new vtkPainterCommunicator;

    this->HasVectors = false;
    this->FieldNameSet = false;
    this->FieldAttributeType = 0;
    this->FieldAssociation = 0;

    this->LightingHelper = vtkSmartPointer<vtkLightingHelper>::New();
    this->ColorMaterialHelper = vtkSmartPointer<vtkColorMaterialHelper>::New();
    }

  // Description:
  // Destructor
  ~vtkInternals()
    {
    this->ClearGraphicsResources();

    const int nLights = vtkLightingHelper::VTK_MAX_LIGHTS;
    for (int i=0; i<nLights; ++i)
      {
      this->LightMonitor[i] = NULL;
      }
    this->ViewMonitor = NULL;
    this->BGMonitor = NULL;

    this->LightingHelper = NULL;
    this->ColorMaterialHelper = NULL;

    if (this->Communicator)
      {
      delete this->Communicator;
      }
    }

  // Description:
  // Check for OpenGL support
  static bool IsSupported(vtkOpenGLRenderWindow *context)
    {
    if (context == NULL)
      {
      vtkGenericWarningMacro("OpenGL render window required");
      return false;
      }

    bool lic2d = vtkLineIntegralConvolution2D::IsSupported(context);

    bool floatFormats
      = vtkTextureObject::IsSupported(context, true, true, false);

    bool renderbuffer = true;
    #if !defined(USE_DEPTH_TEXTURE)
    renderbuffer = vtkRenderbuffer::IsSupported(context);
    #endif

    bool support = lic2d && floatFormats && renderbuffer;

    if (!support)
      {
      vtkOpenGLExtensionManager *manager = context->GetExtensionManager();
      vtkGenericWarningMacro(
        << "SurfaceLIC is not supported" << endl
        << context->GetClassName() << endl
        << manager->GetDriverGLVendor() << endl
        << manager->GetDriverGLVersion() << endl
        << manager->GetDriverGLRenderer() << endl
        << "LIC support = " << lic2d << endl
        << "floating point texture formats = " << floatFormats << endl
        << "render buffers = " << renderbuffer);
      return false;
      }
    return true;
    }

  // Description:
  // Free textures and shader programs we're holding a reference to.
  void ClearGraphicsResources()
    {
    this->ClearTextures();

    this->RenderGeometryPass = NULL;
    this->ColorPass = NULL;
    this->ColorEnhancePass = NULL;
    this->CopyPass = NULL;

    this->Compositor = NULL;
    this->LICer = NULL;
    this->FBO = NULL;

    this->LightingHelper->Initialize(0, VTK_SHADER_TYPE_VERTEX);
    this->ColorMaterialHelper->Initialize(0);
    }

  // Description:
  // Free textures we're holding a reference to.
  void ClearTextures()
    {
    this->DepthImage = NULL;
    this->GeometryImage = NULL;
    this->VectorImage = NULL;
    this->MaskVectorImage = NULL;
    this->CompositeVectorImage = NULL;
    this->CompositeMaskVectorImage = NULL;
    this->NoiseImage = NULL;
    this->LICImage = NULL;
    this->RGBColorImage = NULL;
    this->HSLColorImage = NULL;
    }

  // Description:
  // Allocate textures.
  void AllocateTextures(
        vtkRenderWindow *context,
        int *viewsize)
    {
    this->AllocateDepthTexture(context, viewsize, this->DepthImage);
    this->AllocateTexture(context, viewsize, this->GeometryImage, vtkTextureObject::Nearest);
    this->AllocateTexture(context, viewsize, this->VectorImage, vtkTextureObject::Linear);
    this->AllocateTexture(context, viewsize, this->MaskVectorImage, vtkTextureObject::Linear);
    this->AllocateTexture(context, viewsize, this->CompositeVectorImage, vtkTextureObject::Linear);
    this->AllocateTexture(context, viewsize, this->CompositeMaskVectorImage, vtkTextureObject::Linear);
    this->AllocateTexture(context, viewsize, this->LICImage, vtkTextureObject::Nearest);
    this->AllocateTexture(context, viewsize, this->RGBColorImage, vtkTextureObject::Nearest);
    this->AllocateTexture(context, viewsize, this->HSLColorImage, vtkTextureObject::Nearest);
    }

  // Description:
  // Allocate a size texture, store in the given smart pointer.
  void AllocateTexture(
        vtkRenderWindow *context,
        int *viewsize,
        vtkSmartPointer<vtkTextureObject> &tex,
        int filter = vtkTextureObject::Nearest)
    {
    if ( !tex )
      {
      vtkTextureObject * newTex = vtkTextureObject::New();
      newTex->SetContext(context);
      newTex->SetBaseLevel(0);
      newTex->SetMaxLevel(0);
      newTex->SetWrapS(vtkTextureObject::ClampToEdge);
      newTex->SetWrapT(vtkTextureObject::ClampToEdge);
      newTex->SetMinificationFilter(filter);
      newTex->SetMagnificationFilter(filter);
      newTex->SetBorderColor(0.0f, 0.0f, 0.0f, 0.0f);
      newTex->Create2D(viewsize[0], viewsize[1], 4, VTK_FLOAT, false);
      newTex->SetAutoParameters(0);
      tex = newTex;
      newTex->Delete();
      }
    }

  // Description:
  // Allocate a size texture, store in the given smart pointer.
  #ifdef USE_DEPTH_TEXTURE
  void AllocateDepthTexture(
        vtkRenderWindow *context,
        int *viewsize,
        vtkSmartPointer<vtkTextureObject> &tex)
    {
    if ( !tex )
      {
      vtkTextureObject * newTex = vtkTextureObject::New();
      newTex->SetContext(context);
      newTex->AllocateDepth(viewsize[0], viewsize[1], vtkTextureObject::Float32);
      newTex->SetAutoParameters(0);
      tex = newTex;
      newTex->Delete();
      }
    }
  #else
  void AllocateDepthTexture(
        vtkRenderWindow *context,
        int *viewsize,
        vtkSmartPointer<vtkRenderbuffer> &buf)
    {
    if ( !buf )
      {
      vtkRenderbuffer * newBuf = vtkRenderbuffer::New();
      newBuf->SetContext(context);
      newBuf->CreateDepthAttachment(viewsize[0], viewsize[1]);
      buf = newBuf;
      newBuf->Delete();
      }
    }
  #endif

  // Description:
  // After LIC has been computed reset/clean internal state
  void Updated()
    {
    this->ContextNeedsUpdate = false;
    this->OutputDataNeedsUpdate = false;
    this->CommunicatorNeedsUpdate = false;
    this->GeometryNeedsUpdate = false;
    this->GatherNeedsUpdate = false;
    this->LICNeedsUpdate = false;
    this->ColorNeedsUpdate = false;
    }

  // Description:
  // Force all stages to re-execute. Necessary if the
  // context or communicator changes.
  void UpdateAll()
    {
    this->ContextNeedsUpdate = true;
    this->OutputDataNeedsUpdate= true;
    this->CommunicatorNeedsUpdate= true;
    this->GeometryNeedsUpdate= true;
    this->GatherNeedsUpdate= true;
    this->LICNeedsUpdate= true;
    this->ColorNeedsUpdate= true;
    }

  // Description:
  // Convert viewport to texture coordinates
  void ViewportQuadTextureCoords(GLfloat *tcoords)
    {
    tcoords[0] = tcoords[2] = 0.0f;
    tcoords[1] = tcoords[3] = 1.0f;
    }

  // Description:
  // Convert a viewport to a bounding box and it's texture coordinates for a
  // screen size texture.
  void ViewportQuadPoints(const vtkPixelExtent &viewportExt, GLfloat *quadpts)
    {
    viewportExt.GetData(quadpts);
    }

  // Description:
  // Convert a viewport to a bounding box and it's texture coordinates for a
  // screen size texture.
  void ViewportQuadTextureCoords(
        const vtkPixelExtent &viewExt,
        const vtkPixelExtent &viewportExt,
        GLfloat *tcoords)
    {
    GLfloat viewsize[2];
    viewExt.Size(viewsize);

    // cell to node
    vtkPixelExtent next(viewportExt);
    next.CellToNode();
    next.GetData(tcoords);

    tcoords[0] = tcoords[0]/viewsize[0];
    tcoords[1] = tcoords[1]/viewsize[0];
    tcoords[2] = tcoords[2]/viewsize[1];
    tcoords[3] = tcoords[3]/viewsize[1];
    }

  // Description:
  // Convert the entire view to a bounding box and it's texture coordinates for
  // a screen size texture.
  void ViewQuadPoints(GLfloat *quadpts)
    {
    quadpts[0] = quadpts[2] = 0.0f;
    quadpts[1] = quadpts[3] = 1.0f;
    }

  // Description:
  // Convert the entire view to a bounding box and it's texture coordinates for
  // a screen size texture.
  void ViewQuadTextureCoords(GLfloat *tcoords)
    {
    tcoords[0] = tcoords[2] = 0.0f;
    tcoords[1] = tcoords[3] = 1.0f;
    }

  // Description:
  // Render a quad (to trigger a shader to run)
  void RenderQuad(
        const vtkPixelExtent &viewExt,
        const vtkPixelExtent &viewportExt,
        int nTexUnits)
    {
    // cell to node
    vtkPixelExtent next(viewportExt);
    next.CellToNode();

    GLfloat quadPts[4];
    next.GetData(quadPts);

    GLfloat quadTCoords[4];
    this->ViewportQuadTextureCoords(viewExt, viewportExt, quadTCoords);

    int ids[8] = {0,2, 1,2, 1,3, 0,3};

    glBegin(GL_QUADS);
    for (int q=0; q<4; ++q)
      {
      int qq = 2*q;
      for (int i=0; i<nTexUnits; ++i)
        {
        GLenum texUnit = vtkgl::TEXTURE0+i;
        vtkgl::MultiTexCoord2f(texUnit, quadTCoords[ids[qq]], quadTCoords[ids[qq+1]]);
        }
      glVertex2f(quadPts[ids[qq]], quadPts[ids[qq+1]]);
      }
    glEnd();
    }

  // Description:
  // Test to see if some lighting parameters had changed since the
  // last render internal.
  bool LightingChanged()
    {
    bool anyChanged = false;
    const int nLights = vtkLightingHelper::VTK_MAX_LIGHTS;
    for (int i=0; i<nLights; ++i) // must look at all
      {
      if ( this->LightMonitor[i]->StateChanged() )
        {
        anyChanged = true;
        }
      }
    return anyChanged;
    }

  // Description:
  // Test to see if some model view related parameters had changed since
  // the last render internal.
  bool ViewChanged()
    {
    return this->ViewMonitor->StateChanged();
    }

  // Description:
  // Test to see if background colors or mode has changed since the
  // last render internal.
  bool BackgroundChanged(vtkRenderer *ren)
    {
    return this->BGMonitor->StateChanged(ren);
    }

  // Description:
  // Compute the index into the 4x4 OpenGL ordered matrix.
  inline
  int idx(int row, int col) { return 4*col+row; }

  // Description:
  // given a axes aligned bounding box in
  // normalized device coordinates test for
  // view frustum visibility.
  // if all points are outside one of the
  // view frustum planes then this box
  // is not visible. we might have false
  // positive where more than one clip
  // plane intersects the box.
  bool VisibilityTest(double ndcBBox[24])
    {
    // check all points in the direction d
    // at the same time.
    for (int d=0; d<3; ++d)
      {
      if (((ndcBBox[     d] < -1.0)
        && (ndcBBox[3  + d] < -1.0)
        && (ndcBBox[6  + d] < -1.0)
        && (ndcBBox[9  + d] < -1.0)
        && (ndcBBox[12 + d] < -1.0)
        && (ndcBBox[15 + d] < -1.0)
        && (ndcBBox[18 + d] < -1.0)
        && (ndcBBox[21 + d] < -1.0))
        ||((ndcBBox[     d] > 1.0)
        && (ndcBBox[3  + d] > 1.0)
        && (ndcBBox[6  + d] > 1.0)
        && (ndcBBox[9  + d] > 1.0)
        && (ndcBBox[12 + d] > 1.0)
        && (ndcBBox[15 + d] > 1.0)
        && (ndcBBox[18 + d] > 1.0)
        && (ndcBBox[21 + d] > 1.0)) )
        {
        return false;
        }
      }
    return true;
    }

  // Description:
  // Given world space bounds,
  // compute bounding boxes in clip and normalized device
  // coordinates and perform view frustum visiblity test.
  // return true if the bounds are visible. If so the passed
  // in extent object is initialized with the corresponding
  //screen space extents.
  bool ProjectBounds(
          double PMV[16],
          int viewsize[2],
          double bounds[6],
          vtkPixelExtent &screenExt)
    {
    // this is how to get the 8 corners of a bounding
    // box from the VTK bounds
    int bbIds[24] = {
          0,2,4,
          1,2,4,
          1,3,4,
          0,3,4,
          0,2,5,
          1,2,5,
          1,3,5,
          0,3,5
          };

    // normalized device coordinate bounding box
    double ndcBBox[24];
    for (int q = 0; q<8; ++q)
      {
      int qq = 3*q;
      // bounding box corner
      double wx = bounds[bbIds[qq  ]];
      double wy = bounds[bbIds[qq+1]];
      double wz = bounds[bbIds[qq+2]];
      // to clip coordinates
      ndcBBox[qq  ] = wx * PMV[idx(0,0)] + wy * PMV[idx(0,1)] + wz * PMV[idx(0,2)] + PMV[idx(0,3)];
      ndcBBox[qq+1] = wx * PMV[idx(1,0)] + wy * PMV[idx(1,1)] + wz * PMV[idx(1,2)] + PMV[idx(1,3)];
      ndcBBox[qq+2] = wx * PMV[idx(2,0)] + wy * PMV[idx(2,1)] + wz * PMV[idx(2,2)] + PMV[idx(2,3)];
      double ndcw   = wx * PMV[idx(3,0)] + wy * PMV[idx(3,1)] + wz * PMV[idx(3,2)] + PMV[idx(3,3)];

      // TODO
      // if the point is past the near clipping plane
      // we need to do something more robust. this ensures
      // the correct result but its inefficient
      if (ndcw < 0.0)
        {
        screenExt = vtkPixelExtent(viewsize[0], viewsize[1]);
        //cerr << "W<0!!!!!!!!!!!!!" << endl;
        return true;
        }

      // to normalized device coordinates
      ndcw = (ndcw == 0.0 ? 1.0 : 1.0/ndcw);
      ndcBBox[qq  ] *= ndcw;
      ndcBBox[qq+1] *= ndcw;
      ndcBBox[qq+2] *= ndcw;
      }

    // compute screen extent only if the object
    // is inside the view frustum.
    if (VisibilityTest(ndcBBox))
      {
      // these bounds are visible. compute screen
      // space exents
      double vx  = viewsize[0] - 1.0;
      double vy  = viewsize[1] - 1.0;
      double vx2 = viewsize[0] * 0.5;
      double vy2 = viewsize[1] * 0.5;
      vtkBoundingBox box;
      for (int q=0; q<8; ++q)
        {
        int qq = 3*q;
        double sx = (ndcBBox[qq  ] + 1.0) * vx2;
        double sy = (ndcBBox[qq+1] + 1.0) * vy2;
        box.AddPoint(
          vtkClamp(sx, 0.0, vx),
          vtkClamp(sy, 0.0, vy),
          0.0);
        }
      // to screen extent
      const double *s0 = box.GetMinPoint();
      const double *s1 = box.GetMaxPoint();
      screenExt[0] = static_cast<int>(s0[0]);
      screenExt[1] = static_cast<int>(s1[0]);
      screenExt[2] = static_cast<int>(s0[1]);
      screenExt[3] = static_cast<int>(s1[1]);
      return true;
      }

    // these bounds aren't visible
    return false;
    }

  // Description:
  // Compute screen space extents for each block in the input
  // dataset and for the entire dataset. Only visible blocks
  // are used in the computations.
  int ProjectBounds(
        vtkDataObject *dobj,
        int viewsize[2],
        vtkPixelExtent &dataExt,
        deque<vtkPixelExtent> &blockExts)
    {
    // get the modelview projection matrix
    GLdouble P[16];
    GLdouble MV[16];
    GLdouble PMV[16];
    glGetDoublev(GL_PROJECTION_MATRIX, P);
    glGetDoublev(GL_MODELVIEW_MATRIX, MV);
    for ( int c = 0; c < 4; c ++ )
      {
      for ( int r = 0; r < 4; r ++ )
        {
        PMV[c*4+r]
          = P[idx(r,0)] * MV[idx(0,c)]
          + P[idx(r,1)] * MV[idx(1,c)]
          + P[idx(r,2)] * MV[idx(2,c)]
          + P[idx(r,3)] * MV[idx(3,c)];
        }
      }
    // dataset case
    vtkDataSet* ds = dynamic_cast<vtkDataSet*>(dobj);
    if (ds && ds->GetNumberOfCells())
      {
      double bounds[6];
      ds->GetBounds(bounds);
      if ( vtkBoundingBox::IsValid(bounds)
        && this->ProjectBounds(PMV, viewsize, bounds, dataExt) )
        {
        // the dataset is visible
        // add its extent
        blockExts.push_back(dataExt);
        return 1;
        }
      //cerr << "ds " << ds << " not visible " << endl;
      return 0;
      }
    // composite dataset case
    vtkCompositeDataSet* cd = dynamic_cast<vtkCompositeDataSet*>(dobj);
    if (cd)
      {
      // process each block's bounds
      vtkBoundingBox bbox;
      vtkCompositeDataIterator* iter = cd->NewIterator();
      for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
        {
        ds = dynamic_cast<vtkDataSet*>(iter->GetCurrentDataObject());
        if (ds && ds->GetNumberOfCells())
          {
          double bounds[6];
          ds->GetBounds(bounds);
          vtkPixelExtent screenExt;
          if ( vtkBoundingBox::IsValid(bounds)
            && this->ProjectBounds(PMV, viewsize, bounds, screenExt) )
            {
            // this block is visible
            // save it's screen extent
            // and accumulate its bounds
            blockExts.push_back(screenExt);
            bbox.AddBounds(bounds);
            }
          //else { cerr << "leaf " << ds << " not visible " << endl << endl;}
          }
        }
      iter->Delete();
      // process accumulated dataset bounds
      double bounds[6];
      bbox.GetBounds(bounds);
      if ( vtkBoundingBox::IsValid(bounds)
        && this->ProjectBounds(PMV, viewsize, bounds, dataExt) )
        {
        return 1;
        }
      return 0;
      }
    //cerr << "ds " << ds << " no cells " << endl;
    return 0;
    }

  // Description:
  // Shrink an extent to tightly bound non-zero values
  void GetPixelBounds(float *rgba, int ni, vtkPixelExtent &ext)
    {
    vtkPixelExtent text;
    for (int j=ext[2]; j<=ext[3]; ++j)
      {
      for (int i=ext[0]; i<=ext[1]; ++i)
        {
        if (rgba[4*(j*ni+i)+3] > 0.0f)
          {
          text[0] = text[0] > i ? i : text[0];
          text[1] = text[1] < i ? i : text[1];
          text[2] = text[2] > j ? j : text[2];
          text[3] = text[3] < j ? j : text[3];
          }
        }
      }
    ext = text;
    }

  // Description:
  // Shrink a set of extents to tightly bound non-zero values
  // cull extent if it's empty
  void GetPixelBounds(float *rgba, int ni, deque<vtkPixelExtent> &blockExts)
    {
    vector<vtkPixelExtent> tmpExts(blockExts.begin(),blockExts.end());
    blockExts.clear();
    size_t nBlocks = tmpExts.size();
    for (size_t b=0; b<nBlocks; ++b)
      {
      vtkPixelExtent &tmpExt = tmpExts[b];
      GetPixelBounds(rgba, ni, tmpExt);
      if (!tmpExt.Empty())
        {
        blockExts.push_back(tmpExt);
        }
      }
    }
};

//----------------------------------------------------------------------------
vtkObjectFactoryNewMacro(vtkSurfaceLICPainter);

//----------------------------------------------------------------------------
vtkSurfaceLICPainter::vtkSurfaceLICPainter()
{
  this->Internals = new vtkInternals();
  this->Output = 0;

  this->Enable = 1;
  this->AlwaysUpdate = 0;

  this->StepSize = 1;
  this->NumberOfSteps = 20;
  this->NormalizeVectors = 1;

  this->EnhancedLIC = 1;

  this->EnhanceContrast = 0;
  this->LowLICContrastEnhancementFactor = 0.0;
  this->HighLICContrastEnhancementFactor = 0.0;
  this->LowColorContrastEnhancementFactor = 0.0;
  this->HighColorContrastEnhancementFactor = 0.0;
  this->AntiAlias = 0;
  this->ColorMode = COLOR_MODE_BLEND;
  this->LICIntensity = 0.8;
  this->MapModeBias = 0.0;

  this->GenerateNoiseTexture = 0;
  this->NoiseType = NOISE_TYPE_GAUSSIAN;
  this->NoiseTextureSize = 200;
  this->MinNoiseValue = 0.0;
  this->MaxNoiseValue = 0.8;
  this->NoiseGrainSize = 1;
  this->NumberOfNoiseLevels = 256;
  this->ImpulseNoiseProbability = 1.0;
  this->ImpulseNoiseBackgroundValue = 0.0;
  this->NoiseGeneratorSeed = 1;

  this->MaskOnSurface = 0;
  this->MaskThreshold = 0.0;
  this->MaskIntensity = 0.0;
  this->MaskColor[0] = 0.5;
  this->MaskColor[1] = 0.5;
  this->MaskColor[2] = 0.5;

  this->CompositeStrategy = COMPOSITE_AUTO;

  this->SetInputArrayToProcess(
        vtkDataObject::FIELD_ASSOCIATION_POINTS_THEN_CELLS,
        vtkDataSetAttributes::VECTORS);
}

//----------------------------------------------------------------------------
vtkSurfaceLICPainter::~vtkSurfaceLICPainter()
{
  #if vtkSurfaceLICPainterDEBUG >= 1
  cerr << "=====vtkSurfaceLICPainter::~vtkSurfaceLICPainter" << endl;
  #endif

  this->ReleaseGraphicsResources(this->Internals->Context);
  delete this->Internals;

  if (this->Output)
    {
    this->Output->Delete();
    this->Output = 0;
    }
}

//----------------------------------------------------------------------------
void vtkSurfaceLICPainter::SetInputArrayToProcess(
      int fieldAssociation,
      const char* name)
{
  if ( !this->Internals->FieldNameSet
    || (this->Internals->FieldAssociation != fieldAssociation)
    || (this->Internals->FieldName != name) )
    {
    this->Internals->FieldAssociation = fieldAssociation;
    this->Internals->FieldName = name;
    this->Internals->FieldNameSet = true;
    this->Internals->HasVectors = false;
    this->Internals->UpdateAll();
    this->Modified();
    }
}

//----------------------------------------------------------------------------
void vtkSurfaceLICPainter::SetInputArrayToProcess(
      int fieldAssociation,
      int fieldAttributeType)
{
  if ( (this->Internals->FieldAssociation != fieldAssociation)
    || (this->Internals->FieldAttributeType != fieldAttributeType)
    || this->Internals->FieldNameSet )
    {
    this->Internals->FieldAssociation = fieldAssociation;
    this->Internals->FieldAttributeType = fieldAttributeType;
    this->Internals->FieldNameSet = false;
    this->Internals->HasVectors = false;
    this->Internals->UpdateAll();
    this->Modified();
    }
}

//----------------------------------------------------------------------------
void vtkSurfaceLICPainter::ReleaseGraphicsResources(vtkWindow* win)
{
  this->Internals->ClearGraphicsResources();
  this->Internals->Context = NULL;
  if (this->Output)
    {
    this->Output->Delete();
    this->Output = NULL;
    }
  this->Superclass::ReleaseGraphicsResources(win);
}

//----------------------------------------------------------------------------
#define vtkSetMonitoredParameterMacro(_name, _type, _code)  \
void vtkSurfaceLICPainter::Set##_name (_type val)           \
{                                                           \
  if (val == this->_name)                                   \
    {                                                       \
    return;                                                 \
    }                                                       \
  _code                                                     \
  this->_name = val;                                        \
  this->Modified();                                         \
}
// output dataset
vtkSetMonitoredParameterMacro(
      Enable,
      int,
      this->Internals->OutputDataNeedsUpdate = true;)
// lic
vtkSetMonitoredParameterMacro(
      GenerateNoiseTexture,
      int,
      this->Internals->Noise = NULL;
      this->Internals->NoiseImage = NULL;
      this->Internals->LICNeedsUpdate = true;)

vtkSetMonitoredParameterMacro(
      NoiseType,
      int,
      this->Internals->Noise = NULL;
      this->Internals->NoiseImage = NULL;
      this->Internals->LICNeedsUpdate = true;)

vtkSetMonitoredParameterMacro(
      NoiseTextureSize,
      int,
      this->Internals->Noise = NULL;
      this->Internals->NoiseImage = NULL;
      this->Internals->LICNeedsUpdate = true;)

vtkSetMonitoredParameterMacro(
      NoiseGrainSize,
      int,
      this->Internals->Noise = NULL;
      this->Internals->NoiseImage = NULL;
      this->Internals->LICNeedsUpdate = true;)

vtkSetMonitoredParameterMacro(
      MinNoiseValue,
      double,
      val = val < 0.0 ? 0.0 : val;
      val = val > 1.0 ? 1.0 : val;
      this->Internals->Noise = NULL;
      this->Internals->NoiseImage = NULL;
      this->Internals->LICNeedsUpdate = true;)

vtkSetMonitoredParameterMacro(
      MaxNoiseValue,
      double,
      val = val < 0.0 ? 0.0 : val;
      val = val > 1.0 ? 1.0 : val;
      this->Internals->Noise = NULL;
      this->Internals->NoiseImage = NULL;
      this->Internals->LICNeedsUpdate = true;)

vtkSetMonitoredParameterMacro(
      NumberOfNoiseLevels,
      int,
      this->Internals->Noise = NULL;
      this->Internals->NoiseImage = NULL;
      this->Internals->LICNeedsUpdate = true;)

vtkSetMonitoredParameterMacro(
      ImpulseNoiseProbability,
      double,
      val = val < 0.0 ? 0.0 : val;
      val = val > 1.0 ? 1.0 : val;
      this->Internals->Noise = NULL;
      this->Internals->NoiseImage = NULL;
      this->Internals->LICNeedsUpdate = true;)

vtkSetMonitoredParameterMacro(
      ImpulseNoiseBackgroundValue,
      double,
      val = val < 0.0 ? 0.0 : val;
      val = val > 1.0 ? 1.0 : val;
      this->Internals->Noise = NULL;
      this->Internals->NoiseImage = NULL;
      this->Internals->LICNeedsUpdate = true;)

vtkSetMonitoredParameterMacro(
      NoiseGeneratorSeed,
      int,
      this->Internals->Noise = NULL;
      this->Internals->NoiseImage = NULL;
      this->Internals->LICNeedsUpdate = true;)

// compositor
vtkSetMonitoredParameterMacro(
      CompositeStrategy,
      int,
      this->Internals->GatherNeedsUpdate = true;)

// lic/compositor
vtkSetMonitoredParameterMacro(
      NumberOfSteps,
      int,
      this->Internals->GatherNeedsUpdate = true;
      this->Internals->LICNeedsUpdate = true;)

vtkSetMonitoredParameterMacro(
      StepSize,
      double,
      this->Internals->GatherNeedsUpdate = true;
      this->Internals->LICNeedsUpdate = true;)

vtkSetMonitoredParameterMacro(
      NormalizeVectors,
      int,
      val = val < 0 ? 0 : val;
      val = val > 1 ? 1 : val;
      this->Internals->GatherNeedsUpdate = true;
      this->Internals->LICNeedsUpdate = true;)

vtkSetMonitoredParameterMacro(
      MaskThreshold,
      double,
      this->Internals->LICNeedsUpdate = true;)

vtkSetMonitoredParameterMacro(
      EnhancedLIC,
      int,
      this->Internals->GatherNeedsUpdate = true;
      this->Internals->LICNeedsUpdate = true;)

// lic
vtkSetMonitoredParameterMacro(
      LowLICContrastEnhancementFactor,
      double,
      val = val < 0.0 ? 0.0 : val;
      val = val > 1.0 ? 1.0 : val;
      this->Internals->LICNeedsUpdate = true;)

vtkSetMonitoredParameterMacro(
      HighLICContrastEnhancementFactor,
      double,
      val = val < 0.0 ? 0.0 : val;
      val = val > 1.0 ? 1.0 : val;
      this->Internals->LICNeedsUpdate = true;)

vtkSetMonitoredParameterMacro(
      AntiAlias,
      int,
      val = val < 0 ? 0 : val;
      this->Internals->GatherNeedsUpdate = true;
      this->Internals->LICNeedsUpdate = true;)

// geometry
vtkSetMonitoredParameterMacro(
      MaskOnSurface,
      int,
      val = val < 0 ? 0 : val;
      val = val > 1 ? 1 : val;
      this->Internals->GeometryNeedsUpdate = true;)

// colors
vtkSetMonitoredParameterMacro(
      ColorMode,
      int,
      this->Internals->ColorNeedsUpdate = true;)

vtkSetMonitoredParameterMacro(
      LICIntensity,
      double,
      val = val < 0.0 ? 0.0 : val;
      val = val > 1.0 ? 1.0 : val;
      this->Internals->ColorNeedsUpdate = true;)

vtkSetMonitoredParameterMacro(
      MaskIntensity,
      double,
      val = val < 0.0 ? 0.0 : val;
      val = val > 1.0 ? 1.0 : val;
      this->Internals->ColorNeedsUpdate = true;)

vtkSetMonitoredParameterMacro(
      MapModeBias,
      double,
      val = val <-1.0 ? -1.0 : val;
      val = val > 1.0 ?  1.0 : val;
      this->Internals->ColorNeedsUpdate = true;)

vtkSetMonitoredParameterMacro(
      LowColorContrastEnhancementFactor,
      double,
      val = val < 0.0 ? 0.0 : val;
      val = val > 1.0 ? 1.0 : val;
      this->Internals->ColorNeedsUpdate = true;)

vtkSetMonitoredParameterMacro(
      HighColorContrastEnhancementFactor,
      double,
      val = val < 0.0 ? 0.0 : val;
      val = val > 1.0 ? 1.0 : val;
      this->Internals->ColorNeedsUpdate = true;)

//----------------------------------------------------------------------------
void vtkSurfaceLICPainter::SetMaskColor(double *val)
{
  double rgb[3];
  for (int q=0; q<3; ++q)
    {
    rgb[q] = val[q];
    rgb[q] = rgb[q] < 0.0 ? 0.0 : rgb[q];
    rgb[q] = rgb[q] > 1.0 ? 1.0 : rgb[q];
    }
  if ( (rgb[0] == this->MaskColor[0])
    && (rgb[1] == this->MaskColor[1])
    && (rgb[2] == this->MaskColor[2]) )
    {
    return;
    }
  for (int q=0; q<3; ++q)
    {
    this->MaskColor[q] = rgb[q];
    }
  this->Internals->ColorNeedsUpdate = true;
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkSurfaceLICPainter::SetEnhanceContrast(int val)
{
  val = val < ENHANCE_CONTRAST_OFF ? ENHANCE_CONTRAST_OFF : val;
  val = val > ENHANCE_CONTRAST_BOTH ? ENHANCE_CONTRAST_BOTH : val;
  if (val == this->EnhanceContrast)
    {
    return;
    }

  switch ( this->EnhanceContrast )
    {
    case ENHANCE_CONTRAST_OFF:
      switch ( val )
        {
        case ENHANCE_CONTRAST_LIC:
        case ENHANCE_CONTRAST_BOTH:
          this->Internals->LICNeedsUpdate = true;
          break;
        case ENHANCE_CONTRAST_COLOR:
          this->Internals->ColorNeedsUpdate = true;
          break;
        }
      break;

    case ENHANCE_CONTRAST_LIC:
      switch ( val )
        {
        case ENHANCE_CONTRAST_OFF:
        case ENHANCE_CONTRAST_COLOR:
          this->Internals->LICNeedsUpdate = true;
          break;
        case ENHANCE_CONTRAST_BOTH:
          this->Internals->ColorNeedsUpdate = true;
          break;
        }
      break;

    case ENHANCE_CONTRAST_COLOR:
      switch ( val )
        {
        case ENHANCE_CONTRAST_LIC:
        case ENHANCE_CONTRAST_BOTH:
          this->Internals->LICNeedsUpdate = true;
          break;
        case ENHANCE_CONTRAST_OFF:
          this->Internals->ColorNeedsUpdate = true;
          break;
        }
      break;

    case ENHANCE_CONTRAST_BOTH:
      switch ( val )
        {
        case ENHANCE_CONTRAST_OFF:
          this->Internals->LICNeedsUpdate = true;
          break;
        case ENHANCE_CONTRAST_COLOR:
          this->Internals->LICNeedsUpdate = true;
        case ENHANCE_CONTRAST_LIC:
          this->Internals->ColorNeedsUpdate = true;
          break;
        }
      break;
    }

  this->EnhanceContrast = val;
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkSurfaceLICPainter::SetNoiseDataSet(vtkImageData *data)
{
  if (data == this->Internals->Noise)
    {
    return;
    }
  this->Internals->Noise = data;
  this->Internals->NoiseImage = NULL;
  this->Modified();
}

//----------------------------------------------------------------------------
vtkImageData *vtkSurfaceLICPainter::GetNoiseDataSet()
{
  if (this->Internals->Noise == NULL)
    {
    vtkImageData *noise = NULL;
    if ( this->GenerateNoiseTexture )
      {
      // report potential issues
      if ( this->NoiseGrainSize >= this->NoiseTextureSize )
        {
        vtkErrorMacro(
          "NoiseGrainSize must be smaller than NoiseTextureSize");
        }
      if ( this->MinNoiseValue >= this->MaxNoiseValue )
        {
        vtkErrorMacro(
          "MinNoiseValue must be smaller than MaxNoiseValue");
        }
      if ( (this->ImpulseNoiseProbability == 1.0)
        && (this->NumberOfNoiseLevels < 2) )
        {
        vtkErrorMacro(
          "NumberOfNoiseLevels must be greater than 1 "
          "when not generating impulse noise");
        }

      // generate a custom noise texture based on the
      // current settings.
      int noiseTextureSize = this->NoiseTextureSize;
      int noiseGrainSize = this->NoiseGrainSize;
      RandomNoise2D noiseGen;
      float *noiseValues = noiseGen.Generate(
            this->NoiseType,
            noiseTextureSize,
            noiseGrainSize,
            static_cast<float>(this->MinNoiseValue),
            static_cast<float>(this->MaxNoiseValue),
            this->NumberOfNoiseLevels,
            this->ImpulseNoiseProbability,
            static_cast<float>(this->ImpulseNoiseBackgroundValue),
            this->NoiseGeneratorSeed);
      if ( noiseValues == NULL )
        {
        vtkErrorMacro("Failed to generate noise.");
        }

      vtkFloatArray *noiseArray = vtkFloatArray::New();
      noiseArray->SetNumberOfComponents(2);
      noiseArray->SetName("noise");
      vtkIdType arraySize = 2*noiseTextureSize*noiseTextureSize;
      noiseArray->SetArray(noiseValues, arraySize, 0);

      noise = vtkImageData::New();
      noise->SetSpacing(1.0, 1.0, 1.0);
      noise->SetOrigin(0.0, 0.0, 0.0);
      noise->SetDimensions(noiseTextureSize, noiseTextureSize, 1);
      noise->GetPointData()->SetScalars(noiseArray);

      noiseArray->Delete();
      }
    else
      {
      // load a predefined noise texture.
      noise = vtkGetNoiseResource();
      }

    this->Internals->Noise = noise;
    this->Internals->NoiseImage = NULL;
    noise->Delete();
    noise = NULL;
    }

  return this->Internals->Noise;
}

//----------------------------------------------------------------------------
void vtkSurfaceLICPainter::UpdateNoiseImage(vtkRenderWindow *renWin)
{
  vtkImageData *noiseDataSet = this->GetNoiseDataSet();

  int ext[6];
  noiseDataSet->GetExtent(ext);
  unsigned int dataWidth = ext[1]-ext[0]+1;
  unsigned int dataHeight = ext[3]-ext[2]+1;

  vtkDataArray *noiseArray = noiseDataSet->GetPointData()->GetScalars();
  int dataType = noiseArray->GetDataType();
  void *data = noiseArray->GetVoidPointer(0);
  int dataComps = noiseArray->GetNumberOfComponents();
  unsigned int dataSize = noiseArray->GetNumberOfTuples()*dataComps;

  vtkPixelBufferObject *pbo = vtkPixelBufferObject::New();
  pbo->SetContext(renWin);
  pbo->Upload1D(dataType, data, dataSize, 1, 0);

  vtkTextureObject *tex = vtkTextureObject::New();
  tex->SetContext(renWin);
  tex->SetBaseLevel(0);
  tex->SetMaxLevel(0);
  tex->SetWrapS(vtkTextureObject::Repeat);
  tex->SetWrapT(vtkTextureObject::Repeat);
  tex->SetMinificationFilter(vtkTextureObject::Nearest);
  tex->SetMagnificationFilter(vtkTextureObject::Nearest);
  tex->Create2D(dataWidth, dataHeight, dataComps, pbo, false);
  tex->SetAutoParameters(0);
  pbo->Delete();

  this->Internals->NoiseImage = tex;
  tex->Delete();
}


//----------------------------------------------------------------------------
bool vtkSurfaceLICPainter::IsSupported(vtkRenderWindow *renWin)
{
  vtkOpenGLRenderWindow *context
    = vtkOpenGLRenderWindow::SafeDownCast(renWin);

  return vtkInternals::IsSupported(context);
}

//----------------------------------------------------------------------------
bool vtkSurfaceLICPainter::CanRenderSurfaceLIC(vtkActor *actor, int typeFlags)
{
  // check the render context for GL fetaure support
  // note this also handles non-opengl render window
  if ( this->Internals->ContextNeedsUpdate
    && !vtkSurfaceLICPainter::IsSupported(this->Internals->Context) )
    {
    vtkErrorMacro("SurfaceLIC is not supported");
    return false;
    }

  bool canRender = false;

  // check for common situations where surface lic
  // isn't computed.
  GLint polyMode[2];
  glGetIntegerv(GL_POLYGON_MODE, polyMode);

  int rep = actor->GetProperty()->GetRepresentation();

  if ( this->Enable
    && this->Internals->HasVectors
    && (rep == VTK_SURFACE)
    && (typeFlags & (vtkPainter::POLYS|vtkPainter::STRIPS))
    && (polyMode[0] == GL_FILL) // should I be checking backface mode too?
    && glIsEnabled(GL_LIGHTING) )
    {
    canRender = true;
    }

  #if vtkSurfaceLICPainterDEBUG >= 1
  cerr
    << this->Internals->Communicator->GetWorldRank()
    << " CanRender " << canRender << endl;
  #endif

  return canRender;
}

//----------------------------------------------------------------------------
void vtkSurfaceLICPainter::InitializeResources()
{
  bool initialized = true;

  // noise image
  if (!this->Internals->NoiseImage)
    {
    initialized = false;

    this->UpdateNoiseImage(this->Internals->Context);
    }

  // compositer for parallel operation
  if (!this->Internals->Compositor)
    {
    this->Internals->UpdateAll();
    vtkSurfaceLICComposite *compositor = vtkSurfaceLICComposite::New();
    compositor->SetContext(this->Internals->Context);
    this->Internals->Compositor = compositor;
    compositor->Delete();
    }

  // image LIC
  if (!this->Internals->LICer)
    {
    initialized = false;

    vtkLineIntegralConvolution2D *LICer = vtkLineIntegralConvolution2D::New();
    LICer->SetContext(this->Internals->Context);
    this->Internals->LICer = LICer;
    LICer->Delete();
    }

  // frame buffers
  if (!this->Internals->FBO)
    {
    initialized = false;

    vtkFrameBufferObject2 * fbo = vtkFrameBufferObject2::New();
    fbo->SetContext(this->Internals->Context);
    this->Internals->FBO = fbo;
    fbo->Delete();
    }

  // load shader codes
  if (!this->Internals->RenderGeometryPass)
    {
    initialized = false;

    vtkShaderProgram2 * prog = vtkShaderProgram2::New();
    prog->SetContext(this->Internals->Context);

    vtkShader2 *s = vtkShader2::New();
    s->SetSourceCode(vtkSurfaceLICPainter_GeomVs);
    s->SetType(VTK_SHADER_TYPE_VERTEX);
    s->SetContext(this->Internals->Context);

    vtkShader2 *s2 = vtkShader2::New();
    s2->SetSourceCode(vtkSurfaceLICPainter_GeomFs);
    s2->SetType(VTK_SHADER_TYPE_FRAGMENT);
    s2->SetContext(this->Internals->Context);

    prog->GetShaders()->AddItem(s);
    prog->GetShaders()->AddItem(s2);
    s->Delete();
    s2->Delete();

    this->Internals->LightingHelper->Initialize(prog, VTK_SHADER_TYPE_VERTEX);
    this->Internals->ColorMaterialHelper->Initialize(prog);

    prog->Build();
    if (prog->GetLastBuildStatus() != VTK_SHADER_PROGRAM2_LINK_SUCCEEDED)
      {
      vtkErrorMacro("geometry shader failed to build.");
      }

    this->Internals->RenderGeometryPass = prog;
    prog->Delete();
    }

  if (!this->Internals->ColorPass)
    {
    initialized = false;

    vtkShaderProgram2 *prog = vtkShaderProgram2::New();
    prog->SetContext(this->Internals->Context);

    vtkShader2 *s = vtkShader2::New();
    s->SetSourceCode(vtkSurfaceLICPainter_SC);
    s->SetType(VTK_SHADER_TYPE_FRAGMENT);
    s->SetContext(this->Internals->Context);
    prog->GetShaders()->AddItem(s);
    s->Delete();

    prog->Build();
    if (prog->GetLastBuildStatus() != VTK_SHADER_PROGRAM2_LINK_SUCCEEDED)
      {
      vtkErrorMacro("scalar color shader failed to build.");
      }

    this->Internals->ColorPass = prog;
    prog->Delete();
    }

  if (!this->Internals->ColorEnhancePass)
    {
    initialized = false;

    vtkShaderProgram2 *prog = vtkShaderProgram2::New();
    prog->SetContext(this->Internals->Context);

    vtkShader2 *s = vtkShader2::New();
    s->SetSourceCode(vtkSurfaceLICPainter_CE);
    s->SetType(VTK_SHADER_TYPE_FRAGMENT);
    s->SetContext(this->Internals->Context);
    prog->GetShaders()->AddItem(s);
    s->Delete();

    prog->Build();
    if (prog->GetLastBuildStatus() != VTK_SHADER_PROGRAM2_LINK_SUCCEEDED)
      {
      vtkErrorMacro("color contrast enhance shader failed to build.");
      }

    this->Internals->ColorEnhancePass = prog;
    prog->Delete();
    }

  if (!this->Internals->CopyPass)
    {
    initialized = false;

    vtkShaderProgram2 *prog = vtkShaderProgram2::New();
    prog->SetContext(this->Internals->Context);

    vtkShader2 *s = vtkShader2::New();
    s->SetSourceCode(vtkSurfaceLICPainter_DCpy);
    s->SetType(VTK_SHADER_TYPE_FRAGMENT);
    s->SetContext(this->Internals->Context);
    prog->GetShaders()->AddItem(s);
    s->Delete();

    prog->Build();
    if (prog->GetLastBuildStatus() != VTK_SHADER_PROGRAM2_LINK_SUCCEEDED)
      {
      vtkErrorMacro("color contrast enhance shader failed to build.");
      }

    this->Internals->CopyPass = prog;
    prog->Delete();
    }

  // if any of the above were not already initialized
  // then execute all stages
  if (!initialized)
    {
    this->Internals->UpdateAll();
    }
}

//----------------------------------------------------------------------------
bool vtkSurfaceLICPainter::NeedToColorLIC()
{
  if ( this->Internals->ColorNeedsUpdate
    || this->Internals->LICNeedsUpdate
    || this->Internals->GatherNeedsUpdate
    || this->Internals->GeometryNeedsUpdate
    || this->Internals->CommunicatorNeedsUpdate
    || this->Internals->OutputDataNeedsUpdate
    || this->Internals->ContextNeedsUpdate
    || this->AlwaysUpdate )
    {
    this->Internals->ColorNeedsUpdate = true;
    }

  #if vtkSurfaceLICPainterDEBUG >= 1
  cerr
    << this->Internals->Communicator->GetWorldRank()
    << " NeedToColorLIC " << this->Internals->ColorNeedsUpdate << endl;
  #endif
  return this->Internals->ColorNeedsUpdate;
}

//----------------------------------------------------------------------------
bool vtkSurfaceLICPainter::NeedToComputeLIC()
{
  if ( this->Internals->LICNeedsUpdate
    || this->Internals->GatherNeedsUpdate
    || this->Internals->GeometryNeedsUpdate
    || this->Internals->CommunicatorNeedsUpdate
    || this->Internals->OutputDataNeedsUpdate
    || this->Internals->ContextNeedsUpdate
    || this->AlwaysUpdate )
    {
    this->Internals->LICNeedsUpdate = true;
    }

  #if vtkSurfaceLICPainterDEBUG >= 1
  cerr
    << this->Internals->Communicator->GetWorldRank()
    << " NeedToComputeLIC " << this->Internals->LICNeedsUpdate << endl;
  #endif
  return this->Internals->LICNeedsUpdate;
}

//----------------------------------------------------------------------------
bool vtkSurfaceLICPainter::NeedToGatherVectors()
{
  if ( this->Internals->GatherNeedsUpdate
    || this->Internals->GeometryNeedsUpdate
    || this->Internals->OutputDataNeedsUpdate
    || this->Internals->CommunicatorNeedsUpdate
    || this->Internals->ContextNeedsUpdate
    || this->AlwaysUpdate )
    {
    this->Internals->GatherNeedsUpdate = true;
    }

  #if vtkSurfaceLICPainterDEBUG >= 1
  cerr
    << this->Internals->Communicator->GetWorldRank()
    << " NeedToGatherVectors "
    << this->Internals->GatherNeedsUpdate << endl;
  #endif
  return this->Internals->GatherNeedsUpdate;
}

//----------------------------------------------------------------------------
bool vtkSurfaceLICPainter::NeedToRenderGeometry(
      vtkRenderer *renderer,
      vtkActor *actor)
{
  // view changed or
  // user modifiable parameters
  if ( this->Internals->GeometryNeedsUpdate
    || this->Internals->CommunicatorNeedsUpdate
    || this->Internals->OutputDataNeedsUpdate
    || this->Internals->ContextNeedsUpdate
    || this->AlwaysUpdate )
    {
    this->Internals->GeometryNeedsUpdate = true;
    }

  // lights changed
  if ( this->Internals->LightingChanged() )
    {
    this->Internals->GeometryNeedsUpdate = true;
    }

  // props changed
  long long propMTime = actor->GetProperty()->GetMTime();
  if ( this->Internals->LastPropertyMTime != propMTime )
    {
    this->Internals->LastPropertyMTime = propMTime;
    this->Internals->GeometryNeedsUpdate = true;
    }

  // background colors changed
  if (this->Internals->BackgroundChanged(renderer))
    {
    this->Internals->GeometryNeedsUpdate = true;
    this->Internals->ColorNeedsUpdate = true;
    }

  #if vtkSurfaceLICPainterDEBUG >= 1
  cerr
    << this->Internals->Communicator->GetWorldRank()
    << " NeedToUpdateGeometry "
    << this->Internals->GeometryNeedsUpdate << endl;
  #endif
  return this->Internals->GeometryNeedsUpdate;
}

//----------------------------------------------------------------------------
bool vtkSurfaceLICPainter::NeedToUpdateCommunicator()
{
  // no comm or externally modfied paramters
  if ( this->Internals->CommunicatorNeedsUpdate
    || this->Internals->ContextNeedsUpdate
    || this->Internals->OutputDataNeedsUpdate
    || !this->Internals->Communicator
    || this->AlwaysUpdate )
    {
    this->Internals->CommunicatorNeedsUpdate = true;
    this->Internals->UpdateAll();
    }

  #if vtkSurfaceLICPainterDEBUG >= 1
  cerr
    << this->Internals->Communicator->GetWorldRank()
    << " NeedToUpdateCommunicator "
    << this->Internals->CommunicatorNeedsUpdate << endl;
  #endif

  return this->Internals->CommunicatorNeedsUpdate;
}

//----------------------------------------------------------------------------
bool vtkSurfaceLICPainter::NeedToUpdateOutputData()
{
  vtkDataObject *input = this->GetInput();
  // input dataset changed
  long long inputMTime = input->GetMTime();
  if ( (this->Internals->LastInputDataSetMTime < inputMTime)
    || !this->Output
    || this->AlwaysUpdate)
    {
    this->Internals->LastInputDataSetMTime = inputMTime;
    this->Internals->UpdateAll();
    }

  #if vtkSurfaceLICPainterDEBUG >= 1
  cerr
    << this->Internals->Communicator->GetWorldRank()
    << " NeedToUpdateOutputData " << this->Internals->OutputDataNeedsUpdate << endl;
  #endif
  return this->Internals->OutputDataNeedsUpdate;
}

//----------------------------------------------------------------------------
void vtkSurfaceLICPainter::ValidateContext(vtkRenderer *renderer)
{
  bool modified = false;

  vtkOpenGLRenderWindow *context
    = vtkOpenGLRenderWindow::SafeDownCast(renderer->GetRenderWindow());

  // context changed
  if (this->Internals->Context != context)
    {
    modified = true;
    if (this->Internals->Context)
      {
      this->ReleaseGraphicsResources(this->Internals->Context);
      }
    this->Internals->Context = context;
    }

  // viewport size changed
  int viewsize[2];
  renderer->GetTiledSize(&viewsize[0], &viewsize[1]);
  if ( this->Internals->Viewsize[0] != viewsize[0]
    || this->Internals->Viewsize[1] != viewsize[1] )
    {
    modified = true;

    // udpate view size
    this->Internals->Viewsize[0] = viewsize[0];
    this->Internals->Viewsize[1] = viewsize[1];

    // resize textures
    this->Internals->ClearTextures();
    this->Internals->AllocateTextures(context, viewsize);
    }

  // view changed
  if (this->Internals->ViewChanged())
    {
    modified = true;
    }

  // if anything changed execute all stages
  if (modified)
    {
    this->Internals->UpdateAll();
    }

  #if vtkSurfaceLICPainterDEBUG >= 1
  cerr
    << this->Internals->Communicator->GetWorldRank()
    << " NeedToUpdatContext " << modified << endl;
  #endif
}

//----------------------------------------------------------------------------
vtkPainterCommunicator *vtkSurfaceLICPainter::CreateCommunicator(int)
{
  return new vtkPainterCommunicator;
}

//----------------------------------------------------------------------------
void vtkSurfaceLICPainter::CreateCommunicator()
{
  // compute screen space pixel extent of local blocks and
  // union of local blocks. only blocks that pass view frustum
  // visibility test are used in the computation.

  vtkDataObject *input = this->GetInput();

  this->Internals->DataSetExt.Clear();
  this->Internals->BlockExts.clear();

  int includeRank = this->Internals->ProjectBounds(
          input,
          this->Internals->Viewsize,
          this->Internals->DataSetExt,
          this->Internals->BlockExts);

  if (this->Internals->Communicator)
    {
    delete this->Internals->Communicator;
    this->Internals->Communicator = NULL;
    }

  this->Internals->Communicator = this->CreateCommunicator(includeRank);

  #if vtkSurfaceLICPainterDEBUG >= 1
  cerr
    << this->Internals->Communicator->GetWorldRank()
    << " is rendering " << includeRank << endl;
  #endif
}

//-----------------------------------------------------------------------------
void vtkSurfaceLICPainter::ProcessInformation(vtkInformation* info)
{
  #if vtkSurfaceLICPainterDEBUG >= 1
  bool LUTNeedsUpdate = false;
  #endif

  // detect when the LUT has been modified
  if (info->Has(vtkScalarsToColorsPainter::LOOKUP_TABLE()))
    {
    vtkObjectBase *lutObj = info->Get(vtkScalarsToColorsPainter::LOOKUP_TABLE());
    vtkScalarsToColors *lut = vtkScalarsToColors::SafeDownCast(lutObj);
    long long lutMTime;
    if (lut && ((lutMTime = lut->GetMTime()) > this->Internals->LastLUTMTime))
      {
      this->Internals->LastLUTMTime = lutMTime;
      this->Internals->UpdateAll();
      #if vtkSurfaceLICPainterDEBUG >= 1
      LUTNeedsUpdate = true;
      #endif
      }
    }

  #if vtkSurfaceLICPainterDEBUG >= 1
  cerr
    << this->Internals->Communicator->GetWorldRank()
    << " NeedToUpdateLUT " << LUTNeedsUpdate << endl;
  #endif
}

//----------------------------------------------------------------------------
void vtkSurfaceLICPainter::SetUpdateAll()
{
  this->Internals->UpdateAll();
}

//----------------------------------------------------------------------------
void vtkSurfaceLICPainter::RenderInternal(
        vtkRenderer *renderer,
        vtkActor *actor,
        unsigned long typeflags,
        bool forceCompileOnly)
{
  #if vtkSurfaceLICPainterDEBUG >= 1
  cerr
    << this->Internals->Communicator->GetWorldRank()
    << " ===== " << this->GetClassName() << "::RenderInternal" << endl;
  #endif

  #ifdef vtkSurfaceLICPainterTIME
  this->StartTimerEvent("vtkSurfaceLICPainter::RenderInternal");
  #else
  vtkSmartPointer<vtkTimerLog> timer = vtkSmartPointer<vtkTimerLog>::New();
  timer->StartTimer();
  #endif

  vtkOpenGLClearErrorMacro();

  this->ValidateContext(renderer);

  if (this->NeedToUpdateOutputData())
    {
    // if the input data has changed we need to
    // reload vector attributes and recompute
    // all, but only if the output is valid.
    this->PrepareOutput();
    }

  if (this->NeedToUpdateCommunicator())
    {
    #ifdef vtkSurfaceLICPainterTIME
    this->StartTimerEvent("vtkSurfaceLICPainter::CreateCommunicator");
    #endif
    // create a communicator that contains only ranks
    // that have visible data. In parallel this is a
    // collective operation accross all ranks. In
    // serial this is a no-op.
    this->CreateCommunicator();
    #ifdef vtkSurfaceLICPainterTIME
    this->EndTimerEvent("vtkSurfaceLICPainter::CreateCommunicator");
    #endif
    }
  vtkPainterCommunicator *comm = this->Internals->Communicator;

  if (comm->GetIsNull())
    {
    // other rank's may have some visible data but we
    // have none and should not participate further
    #ifdef vtkSurfaceLICPainterTIME
    this->EndTimerEvent("vtkSurfaceLICPainter::RenderInternal");
    #endif
    return;
    }

  if (!this->CanRenderSurfaceLIC(actor, typeflags))
    {
    // we've determined that there's no work for us, or that the
    // requisite opengl extensions are not available. pass control on
    // to delegate renderer and return.
    this->Superclass::RenderInternal(renderer, actor, typeflags, forceCompileOnly);
    #ifdef vtkSurfaceLICPainterTIME
    this->EndTimerEvent("vtkSurfaceLICPainter::RenderInternal");
    #endif
    return;
    }

  // allocate rendering resources, initialize or update
  // textures and shaders.
  this->InitializeResources();

  // Save context and matrix state to be able to restore.
  glPushAttrib(GL_ALL_ATTRIB_BITS);
  glMatrixMode(GL_PROJECTION);
  glPushMatrix();
  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();

  vtkPixelExtent viewExt(
        this->Internals->Viewsize[0],
        this->Internals->Viewsize[1]);

  // save the active fbo and its draw buffer
  int prevDrawBuf = 0;
  glGetIntegerv(GL_DRAW_BUFFER, &prevDrawBuf);

  int prevFbo = 0;
  glGetIntegerv(vtkgl::DRAW_FRAMEBUFFER_BINDING_EXT, &prevFbo);

  // ------------------------------------------- render geometry, project vectors onto screen, etc
  if (this->NeedToRenderGeometry(renderer, actor))
    {
    #ifdef vtkSurfaceLICPainterTIME
    this->StartTimerEvent("vtkSurfaceLICPainter::RenderGeometry");
    #endif

    // setup our fbo
    vtkFrameBufferObject2 *fbo = this->Internals->FBO;
    fbo->SaveCurrentBindings();
    fbo->Bind(vtkgl::FRAMEBUFFER_EXT);
    fbo->AddDepthAttachment(vtkgl::DRAW_FRAMEBUFFER_EXT, this->Internals->DepthImage);
    fbo->AddColorAttachment(vtkgl::DRAW_FRAMEBUFFER_EXT, 0U, this->Internals->GeometryImage);
    fbo->AddColorAttachment(vtkgl::DRAW_FRAMEBUFFER_EXT, 1U, this->Internals->VectorImage);
    fbo->AddColorAttachment(vtkgl::DRAW_FRAMEBUFFER_EXT, 2U, this->Internals->MaskVectorImage);
    fbo->ActivateDrawBuffers(3);
    vtkCheckFrameBufferStatusMacro(vtkgl::FRAMEBUFFER_EXT);

    // clear internal color and depth buffers
    // the LIC'er requires *all* fragments in the vector
    // texture to be initialized to 0
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_SCISSOR_TEST);
    glClearColor(0.0, 0.0, 0.0, 0.0);
    glClear(GL_DEPTH_BUFFER_BIT|GL_COLOR_BUFFER_BIT);

    // setup projection shader
    vtkShaderProgram2 *geometryPass = this->Internals->RenderGeometryPass;

    vtkUniformVariables *uniforms = geometryPass->GetUniformVariables();
    uniforms->SetUniformft("uMaskOnSurface", this->MaskOnSurface);

    this->Internals->LightingHelper->EncodeLightState();
    this->Internals->ColorMaterialHelper->SetUniformVariables();

    // render geometry through delegate chain. not looping over blocks
    // here since composite dataset painter is in the chain.
    geometryPass->Use();

    typeflags &= (vtkPainter::POLYS|vtkPainter::STRIPS);
    this->Superclass::RenderInternal(renderer, actor, typeflags, forceCompileOnly);

    geometryPass->Restore();

    fbo->RemoveRenDepthAttachment(vtkgl::DRAW_FRAMEBUFFER_EXT);
    fbo->RemoveTexColorAttachment(vtkgl::DRAW_FRAMEBUFFER_EXT, 0U);
    fbo->RemoveTexColorAttachment(vtkgl::DRAW_FRAMEBUFFER_EXT, 1U);
    fbo->RemoveTexColorAttachment(vtkgl::DRAW_FRAMEBUFFER_EXT, 2U);
    fbo->DeactivateDrawBuffers();
    fbo->UnBind(vtkgl::FRAMEBUFFER_EXT);

    #ifdef vtkSurfaceLICPainterTIME
    this->EndTimerEvent("vtkSurfaceLICPainter::RenderGeometry");
    #endif
    #if vtkSurfaceLICPainterDEBUG >= 2
    vtkTextureIO::Write(
          mpifn(comm,"slicp_geometry_image.vtm"),
          this->Internals->GeometryImage,
          this->Internals->BlockExts);
    vtkTextureIO::Write(
          mpifn(comm,"slicp_vector_image.vtm"),
          this->Internals->VectorImage,
          this->Internals->BlockExts);
    vtkTextureIO::Write(
          mpifn(comm,"slicp_mask_vector_image.vtm"),
          this->Internals->MaskVectorImage,
          this->Internals->BlockExts);
    #if defined(USE_DEPTH_TEXTURE)
    vtkTextureIO::Write(
          mpifn(comm,"slicp_depth_image.vtm"),
          this->Internals->DepthImage,
          this->Internals->BlockExts);
    #endif
    #endif
    }

  // --------------------------------------------- compoiste vectors for parallel LIC
  if (this->NeedToGatherVectors())
    {
    #ifdef vtkSurfaceLICPainterTIME
    this->StartTimerEvent("vtkSurfaceLICPainter::GatherVectors");
    #endif

    // get tight screen space bounds to reduce communication/computation
    vtkPixelBufferObject *vecPBO = this->Internals->VectorImage->Download();
    void *pVecPBO = vecPBO->MapPackedBuffer();

    this->Internals->GetPixelBounds(
            (float*)pVecPBO,
            this->Internals->Viewsize[0],
            this->Internals->BlockExts);

    // initialize compositor
    this->Internals->Compositor->Initialize(
          viewExt,
          this->Internals->BlockExts,
          this->CompositeStrategy,
          this->StepSize,
          this->NumberOfSteps,
          this->NormalizeVectors,
          this->EnhancedLIC,
          this->AntiAlias);

    if (comm->GetMPIInitialized())
      {
      // parallel run
      // need to use the communicator provided by the rendering engine
      this->Internals->Compositor->SetCommunicator(comm);

      // build compositing program and set up the screen space decomp
      // with guard pixels
      int iErr = 0;
      iErr = this->Internals->Compositor->BuildProgram((float*)pVecPBO);
      if (iErr)
        {
        vtkErrorMacro("Failed to construct program, reason " << iErr);
        }

      // composite vectors
      vtkTextureObject *compositeVectors = this->Internals->CompositeVectorImage;
      iErr = this->Internals->Compositor->Gather(
              pVecPBO,
              VTK_FLOAT,
              4,
              compositeVectors);
      if (iErr)
        {
        vtkErrorMacro("Failed to composite vectors, reason  " << iErr);
        }

      // composite mask vectors
      vtkTextureObject *compositeMaskVectors = this->Internals->CompositeMaskVectorImage;
      vtkPixelBufferObject *maskVecPBO = this->Internals->MaskVectorImage->Download();
      void *pMaskVecPBO = maskVecPBO->MapPackedBuffer();
      iErr = this->Internals->Compositor->Gather(
              pMaskVecPBO,
              VTK_FLOAT,
              4,
              compositeMaskVectors);
      if (iErr)
        {
        vtkErrorMacro("Failed to composite mask vectors, reason " << iErr);
        }
      maskVecPBO->UnmapPackedBuffer();
      maskVecPBO->Delete();

      // restore the default communicator
      this->Internals->Compositor->RestoreDefaultCommunicator();

      #if vtkSurfaceLICPainterDEBUG >= 2
      vtkTextureIO::Write(
             mpifn(comm,"slicp_new_vector_image.vtm"),
             this->Internals->CompositeVectorImage,
             this->Internals->Compositor->GetDisjointGuardExtents());

      vtkTextureIO::Write(
             mpifn(comm,"slicp_new_mask_vector_image.vtm"),
             this->Internals->CompositeMaskVectorImage,
             this->Internals->Compositor->GetDisjointGuardExtents());
      #endif
      }
    else
      {
      // serial run
      // make the decomposition disjoint and add guard pixels
      this->Internals->Compositor->InitializeCompositeExtents((float*)pVecPBO);

      // use the lic decomp from here on out, in serial we have this
      // flexibility because we don't need to worry about ordered compositing
      // or IceT's scissor boxes
      this->Internals->BlockExts
         = this->Internals->Compositor->GetCompositeExtents();

      // pass through without compositing
      this->Internals->CompositeVectorImage = this->Internals->VectorImage;
      this->Internals->CompositeMaskVectorImage = this->Internals->MaskVectorImage;
      }

   vecPBO->UnmapPackedBuffer();
   vecPBO->Delete();

   #ifdef vtkSurfaceLICPainterTIME
   this->EndTimerEvent("vtkSurfaceLICPainter::GatherVectors");
   #endif
   }

  // ------------------------------------------- LIC on screen
  if ( this->NeedToComputeLIC() )
    {
    #if vtkSurfaceLICPainterDEBUG >= 2
    ostringstream oss;
    if ( this->GenerateNoiseTexture )
      {
      const char *noiseType[3]={"unif","gauss","perl"};
      oss
       << "slicp_noise_"
       << noiseType[this->NoiseType]
       << "_size_" << this->NoiseTextureSize
       << "_grain_" << this->NoiseGrainSize
       << "_minval_" << this->MinNoiseValue
       << "_maxval_" << this->MaxNoiseValue
       << "_nlevels_" << this->NumberOfNoiseLevels
       << "_impulseprob_" << this->ImpulseNoiseProbability
       << "_impulseprob_" << this->ImpulseNoiseBackgroundValue
       << ".vtk";
      }
    else
      {
      oss << "slicp_noise_default.vtk";
      }
    vtkTextureIO::Write(
          mpifn(comm, oss.str().c_str()),
          this->Internals->NoiseImage);
    #endif
    #ifdef vtkSurfaceLICPainterTIME
    this->StartTimerEvent("vtkSurfaceLICPainter::ComputeLIC");
    #endif

    // TODO -- this means that the steps size is a function
    // of aspect ratio which is pretty insane...
    // convert from window units to texture units
    // this isn't correct since there's no way to account
    // for anisotropy in the trasnform to texture space
    double tcScale[2] = {
          1.0/this->Internals->Viewsize[0],
          1.0/this->Internals->Viewsize[1]};

    double stepSize
      = this->StepSize*sqrt(tcScale[0]*tcScale[0]+tcScale[1]*tcScale[1]);

    stepSize = stepSize <= 0.0 ? 1.0e-10 : stepSize;

    // configure image lic
    vtkLineIntegralConvolution2D *LICer = this->Internals->LICer;

    LICer->SetStepSize(stepSize);
    LICer->SetNumberOfSteps(this->NumberOfSteps);
    LICer->SetEnhancedLIC(this->EnhancedLIC);
    switch (this->EnhanceContrast)
      {
      case ENHANCE_CONTRAST_LIC:
      case ENHANCE_CONTRAST_BOTH:
        LICer->SetEnhanceContrast(vtkLIC2D::ENHANCE_CONTRAST_ON);
        break;
      default:
        LICer->SetEnhanceContrast(vtkLIC2D::ENHANCE_CONTRAST_OFF);
      }
    LICer->SetLowContrastEnhancementFactor(this->LowLICContrastEnhancementFactor);
    LICer->SetHighContrastEnhancementFactor(this->HighLICContrastEnhancementFactor);
    LICer->SetAntiAlias(this->AntiAlias);
    LICer->SetComponentIds(0, 1);
    LICer->SetNormalizeVectors(this->NormalizeVectors);
    LICer->SetMaskThreshold(this->MaskThreshold);
    LICer->SetCommunicator(comm);

    // loop over composited extents
    const deque<vtkPixelExtent> &compositeExts
      = this->Internals->Compositor->GetCompositeExtents();

    const deque<vtkPixelExtent> &disjointGuardExts
      = this->Internals->Compositor->GetDisjointGuardExtents();

    this->Internals->LICImage.TakeReference(
         LICer->Execute(
              viewExt,            // screen extent
              disjointGuardExts,  // disjoint extent of valid vectors
              compositeExts,      // disjoint extent where lic is needed
              this->Internals->CompositeVectorImage,
              this->Internals->CompositeMaskVectorImage,
              this->Internals->NoiseImage));

    if (!this->Internals->LICImage)
      {
      vtkErrorMacro("Failed to compute image LIC");
      return;
      }

    #ifdef vtkSurfaceLICPainterTIME
    this->EndTimerEvent("vtkSurfaceLICPainter::ComputeLIC");
    #endif
    #if vtkSurfaceLICPainterDEBUG >= 2
    vtkTextureIO::Write(
          mpifn(comm,"slicp_lic.vtm"),
          this->Internals->LICImage,
          compositeExts);
    #endif

    // ------------------------------------------- move from LIC decomp back to geometry decomp
    if ( comm->GetMPIInitialized()
      && (this->Internals->Compositor->GetStrategy()!=COMPOSITE_INPLACE ) )
      {
      #ifdef vtkSurfaceLICPainterTIME
      this->StartTimerEvent("vtkSurfaceLICPainter::ScatterLIC");
      #endif

      // parallel run
      // need to use the communicator provided by the rendering engine
      this->Internals->Compositor->SetCommunicator(comm);

      vtkPixelBufferObject *licPBO = this->Internals->LICImage->Download();
      void *pLicPBO = licPBO->MapPackedBuffer();
      vtkTextureObject *newLicImage = NULL;
      int iErr = this->Internals->Compositor->Scatter(pLicPBO, VTK_FLOAT, 4, newLicImage);
      if (iErr)
        {
        vtkErrorMacro("Failed to scatter lic");
        }
      licPBO->UnmapPackedBuffer();
      licPBO->Delete();
      this->Internals->LICImage = NULL;
      this->Internals->LICImage = newLicImage;
      newLicImage->Delete();

      // restore the default communicator
      this->Internals->Compositor->RestoreDefaultCommunicator();

      #ifdef vtkSurfaceLICPainterTIME
      this->EndTimerEvent("vtkSurfaceLICPainter::ScatterLIC");
      #endif
      #if vtkSurfaceLICPainterDEBUG >= 2
      vtkTextureIO::Write(
            mpifn(comm,"slicp_new_lic.vtm"),
            this->Internals->LICImage,
            this->Internals->BlockExts);
      #endif
      }
    }

  // ------------------------------------------- combine scalar colors + LIC
  if ( this->NeedToColorLIC() )
    {
    #ifdef vtkSurfaceLICPainterTIME
    this->StartTimerEvent("vtkSurfaceLICPainter::ColorLIC");
    #endif
    vtkFrameBufferObject2 *fbo = this->Internals->FBO;
    fbo->SaveCurrentBindings();
    fbo->Bind(vtkgl::FRAMEBUFFER_EXT);
    fbo->InitializeViewport(this->Internals->Viewsize[0], this->Internals->Viewsize[1]);
    fbo->AddColorAttachment(vtkgl::DRAW_FRAMEBUFFER_EXT, 0U, this->Internals->RGBColorImage);
    fbo->AddColorAttachment(vtkgl::DRAW_FRAMEBUFFER_EXT, 1U, this->Internals->HSLColorImage);
    fbo->ActivateDrawBuffers(2U);
    vtkCheckFrameBufferStatusMacro(vtkgl::FRAMEBUFFER_EXT);

    #if 0
    glDisable(GL_SCISSOR_TEST);
    glClearColor(0.0, 0.0, 0.0, 0.0);
    glClear(GL_COLOR_BUFFER_BIT);
    #else
    // clear the parts of the screen which we will modify
    glEnable(GL_SCISSOR_TEST);
    glClearColor(0.0, 0.0, 0.0, 0.0);
    size_t nBlocks = this->Internals->BlockExts.size();
    for (size_t e=0; e<nBlocks; ++e)
      {
      vtkPixelExtent ext = this->Internals->BlockExts[e];
      ext.Grow(2); // halo for linear filtering
      ext &= viewExt;

      unsigned int extSize[2];
      ext.Size(extSize);

      glScissor(ext[0], ext[2], extSize[0], extSize[1]);
      glClear(GL_COLOR_BUFFER_BIT);
      }
    glDisable(GL_SCISSOR_TEST);
    #endif

    this->Internals->VectorImage->Activate(vtkgl::TEXTURE0);
    this->Internals->GeometryImage->Activate(vtkgl::TEXTURE1);
    this->Internals->LICImage->Activate(vtkgl::TEXTURE2);

    vtkShaderProgram2 *colorPass = this->Internals->ColorPass;
    vtkUniformVariables *uniforms = colorPass->GetUniformVariables();
    uniforms->SetUniformit("texVectors", 0);
    uniforms->SetUniformit("texGeomColors", 1);
    uniforms->SetUniformit("texLIC", 2);
    uniforms->SetUniformit("uScalarColorMode", this->ColorMode);
    uniforms->SetUniformft("uLICIntensity", this->LICIntensity);
    uniforms->SetUniformft("uMapBias", this->MapModeBias);
    uniforms->SetUniformft("uMaskIntensity", this->MaskIntensity);
    uniforms->SetUniformft("uMaskColor", 3, this->MaskColor);
    colorPass->Use();

    for (size_t e=0; e<nBlocks; ++e)
      {
      this->Internals->RenderQuad(viewExt, this->Internals->BlockExts[e], 1);
      }

    colorPass->Restore();

    this->Internals->VectorImage->Deactivate(vtkgl::TEXTURE0);
    this->Internals->GeometryImage->Deactivate(vtkgl::TEXTURE1);
    this->Internals->LICImage->Deactivate(vtkgl::TEXTURE2);

    #ifdef vtkSurfaceLICPainterTIME
    this->EndTimerEvent("vtkSurfaceLICPainter::ColorLIC");
    #endif

    // --------------------------------------------- color contrast enhance
    if ( ( this->EnhanceContrast == ENHANCE_CONTRAST_COLOR )
      || ( this->EnhanceContrast == ENHANCE_CONTRAST_BOTH ) )
      {
      #if vtkSurfaceLICPainterDEBUG >= 2
      vtkTextureIO::Write(
            mpifn(comm,"slic_color_rgb_in.vtm"),
            this->Internals->RGBColorImage,
            this->Internals->BlockExts);
      vtkTextureIO::Write(
            mpifn(comm,"slic_color_hsl_in.vtm"),
            this->Internals->HSLColorImage,
            this->Internals->BlockExts);
      #endif
      #ifdef vtkSurfaceLICPainterTIME
      this->StartTimerEvent("vtkSurfaceLICPainter::ContrastEnhance");
      #endif

      // find min/max lighness value for color contrast enhancement.
      float LMin = VTK_FLOAT_MAX;
      float LMax = -VTK_FLOAT_MAX;
      float LMaxMinDiff = VTK_FLOAT_MAX;

      #ifdef STREAMING_MIN_MAX
      StreamingFindMinMax(fbo, this->Internals->BlockExts, LMin, LMax);
      #else
      FindMinMax(
            this->Internals->HSLColorImage,
            this->Internals->BlockExts,
            LMin,
            LMax);
      #endif

      if ( this->Internals->BlockExts.size()
        && ((LMax <= LMin) || (LMin < 0.0f) || (LMax > 1.0f)) )
        {
        vtkErrorMacro(
          << comm->GetRank()
          << ": Invalid  range " << LMin << ", " << LMax
          << " for color contrast enhancement");
        LMin = 0.0;
        LMax = 1.0;
        LMaxMinDiff = 1.0;
        }

      // global collective reduction for parallel operation
      this->GetGlobalMinMax(comm, LMin, LMax);

      // set M and m as a fraction of the range.
      LMaxMinDiff = LMax-LMin;
      LMin += LMaxMinDiff*this->LowColorContrastEnhancementFactor;
      LMax -= LMaxMinDiff*this->HighColorContrastEnhancementFactor;
      LMaxMinDiff = LMax-LMin;

      // normalize shader
      fbo->AddColorAttachment(vtkgl::DRAW_FRAMEBUFFER_EXT, 0U, this->Internals->RGBColorImage);
      fbo->ActivateDrawBuffer(0U);
      vtkCheckFrameBufferStatusMacro(vtkgl::DRAW_FRAMEBUFFER_EXT);

      this->Internals->GeometryImage->Activate(vtkgl::TEXTURE0);
      this->Internals->HSLColorImage->Activate(vtkgl::TEXTURE1);
      this->Internals->LICImage->Activate(vtkgl::TEXTURE2);

      vtkShaderProgram2 *colorEnhancePass = this->Internals->ColorEnhancePass;
      uniforms = colorEnhancePass->GetUniformVariables();
      uniforms->SetUniformit("texGeomColors", 0);
      uniforms->SetUniformit("texHSLColors", 1);
      uniforms->SetUniformit("texLIC", 2);
      uniforms->SetUniformft("uLMin", LMin);
      uniforms->SetUniformft("uLMaxMinDiff", LMaxMinDiff);
      colorEnhancePass->Use();

      for (size_t e=0; e<nBlocks; ++e)
        {
        this->Internals->RenderQuad(viewExt, this->Internals->BlockExts[e], 1);
        }

      colorEnhancePass->Restore();

      this->Internals->GeometryImage->Deactivate(vtkgl::TEXTURE0);
      this->Internals->HSLColorImage->Deactivate(vtkgl::TEXTURE1);
      this->Internals->LICImage->Deactivate(vtkgl::TEXTURE2);

      fbo->RemoveTexColorAttachment(vtkgl::DRAW_FRAMEBUFFER_EXT, 0U);
      fbo->DeactivateDrawBuffers();

      #ifdef vtkSurfaceLICPainterTIME
      this->EndTimerEvent("vtkSurfaceLICPainter::ContrastEnhance");
      #endif
      }
    else
      {
      fbo->RemoveTexColorAttachment(vtkgl::DRAW_FRAMEBUFFER_EXT, 0U);
      fbo->RemoveTexColorAttachment(vtkgl::DRAW_FRAMEBUFFER_EXT, 1U);
      fbo->DeactivateDrawBuffers();
      }

    fbo->UnBind(vtkgl::FRAMEBUFFER_EXT);

    #if vtkSurfaceLICPainterDEBUG >= 2
    vtkTextureIO::Write(
           mpifn(comm,"slicp_new_rgb.vtm"),
           this->Internals->RGBColorImage,
           this->Internals->BlockExts);
    #endif
    }

  // ----------------------------------------------- depth test and copy to screen
  #ifdef vtkSurfaceLICPainterTIME
  this->StartTimerEvent("vtkSurfaceLICPainter::DepthCopy");
  #endif
  vtkgl::BindFramebufferEXT(vtkgl::FRAMEBUFFER_EXT, prevFbo);
  glDrawBuffer(prevDrawBuf);
  vtkFrameBufferObject2::InitializeViewport(
        this->Internals->Viewsize[0],
        this->Internals->Viewsize[1]);
  glEnable(GL_DEPTH_TEST);

  this->Internals->DepthImage->Activate(vtkgl::TEXTURE0);
  this->Internals->RGBColorImage->Activate(vtkgl::TEXTURE1);

  vtkShaderProgram2 *copyPass = this->Internals->CopyPass;
  vtkUniformVariables *uniforms = copyPass->GetUniformVariables();
  uniforms->SetUniformit("texDepth", 0);
  uniforms->SetUniformit("texRGBColors", 1);
  copyPass->Use();

  size_t nBlocks = this->Internals->BlockExts.size();
  for (size_t e=0; e<nBlocks; ++e)
    {
    this->Internals->RenderQuad(viewExt, this->Internals->BlockExts[e], 1);
    }

  copyPass->Restore();

  this->Internals->DepthImage->Deactivate(vtkgl::TEXTURE0);
  this->Internals->RGBColorImage->Deactivate(vtkgl::TEXTURE1);

  #ifdef vtkSurfaceLICPainterTIME
  this->EndTimerEvent("vtkSurfaceLICPainter::DepthCopy");
  #endif

  //
  this->Internals->Updated();

  // Essential to restore the context to what it was before we started messing
  // with it.
  glMatrixMode(GL_MODELVIEW);
  glPopMatrix();
  glMatrixMode(GL_PROJECTION);
  glPopMatrix();
  glPopAttrib();

  // clear opengl error flags and be absolutely certain that nothing failed.
  vtkOpenGLCheckErrorMacro("failed during surface lic painter");

  #ifdef vtkSurfaceLICPainterTIME
  this->EndTimerEvent("vtkSurfaceLICPainter::RenderInternal");
  #else
  timer->StopTimer();
  #endif
}

//-----------------------------------------------------------------------------
void vtkSurfaceLICPainter::ReportReferences(vtkGarbageCollector *collector)
{
  this->Superclass::ReportReferences(collector);

  vtkGarbageCollectorReport(collector, this->Output, "Output PolyData");
}

//----------------------------------------------------------------------------
vtkDataObject* vtkSurfaceLICPainter::GetOutput()
{
  #if vtkSurfaceLICPainterDEBUG >= 1
  cerr << "=====vtkSurfaceLICPainter::GetOutput" << endl;
  #endif

  if (this->Enable && this->Output)
    {
    return this->Output;
    }
  return this->GetInput();
}

//----------------------------------------------------------------------------
bool vtkSurfaceLICPainter::PrepareOutput()
{
  vtkDataObject* input = this->GetInput();
  if ((input == NULL) || !this->Enable)
    {
    if (this->Output)
      {
      this->Output->Delete();
      this->Output = NULL;
      this->Internals->HasVectors = false;
      }
    return false;
    }

  if (this->Internals->OutputDataNeedsUpdate)
    {
    if (this->Output)
      {
      this->Output->Delete();
      this->Output = NULL;
      }

    this->Output = input->NewInstance();
    this->Output->ShallowCopy(input);
    this->Internals->HasVectors = false;
    }

  if (!this->Internals->HasVectors)
    {
    this->Internals->HasVectors = this->VectorsToTCoords(this->Output);
    }

  return this->Internals->HasVectors;
}

//----------------------------------------------------------------------------
bool vtkSurfaceLICPainter::VectorsToTCoords(vtkDataObject *dataObj)
{
  bool hasVectors = false;

  vtkCompositeDataSet *cd = vtkCompositeDataSet::SafeDownCast(dataObj);
  if (cd)
    {
    vtkCompositeDataIterator* iter = cd->NewIterator();
    for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
      {
      vtkDataSet* ds = vtkDataSet::SafeDownCast(iter->GetCurrentDataObject());
      if (ds && ds->GetNumberOfCells())
        {
        this->ClearTCoords(ds);
        hasVectors |= this->VectorsToTCoords(ds);
        }
      }
    iter->Delete();
    return hasVectors;
    }

  vtkDataSet* ds = vtkDataSet::SafeDownCast(dataObj);
  if (ds && ds->GetNumberOfCells())
    {
    this->ClearTCoords(ds);
    hasVectors |= this->VectorsToTCoords(ds);
    }

  if ( hasVectors )
    {
    // force downstream updates (display lists, etc)
    this->Output->Modified();
    }

  return hasVectors;
}

//----------------------------------------------------------------------------
bool vtkSurfaceLICPainter::VectorsToTCoords(vtkDataSet *data)
{
  // don't use SafeDownCast here for rendering performance
  vtkDataArray *vectors = NULL;
  bool hasCellVectors = false;

  if (this->Internals->FieldNameSet)
    {
    vectors
       = vtkDataArray::SafeDownCast(
            this->GetInputArrayToProcess(
            this->Internals->FieldAssociation,
            this->Internals->FieldName.c_str(),
            data,
            &hasCellVectors));
    }
  else
    {
    vectors
       = vtkDataArray::SafeDownCast(
            this->GetInputArrayToProcess(
            this->Internals->FieldAssociation,
            this->Internals->FieldAttributeType,
            data,
            &hasCellVectors));
    }

  if ( vectors == NULL )
    {
    return false;
    }

  vtkDataSetAttributes *atts = NULL;
  if ( hasCellVectors )
    {
    atts = data->GetCellData();
    }
  else
    {
    atts = data->GetPointData();
    }

  int id = -1;
  int nArrays = atts->GetNumberOfArrays();
  for (int i=0; i<nArrays; ++i)
    {
    if ( atts->GetArray(i) == vectors )
      {
      id = i;
      break;
      }
    }
  atts->SetActiveAttribute(id, vtkDataSetAttributes::TCOORDS);
  return true;
}

//----------------------------------------------------------------------------
void vtkSurfaceLICPainter::ClearTCoords(vtkDataSet *data)
{
  data->GetCellData()->SetActiveAttribute(-1, vtkDataSetAttributes::TCOORDS);
  data->GetPointData()->SetActiveAttribute(-1, vtkDataSetAttributes::TCOORDS);
}

//----------------------------------------------------------------------------
void vtkSurfaceLICPainter::GetBounds(vtkDataObject* dobj, double bounds[6])
{
  #if vtkSurfaceLICPainterDEBUG >= 1
  cerr << "=====vtkSurfaceLICPainter::GetBounds" << endl;
  #endif
  // don't use SafeDownCast here for rendering performance

  vtkMath::UninitializeBounds(bounds);
  vtkDataSet* ds = vtkDataSet::SafeDownCast(dobj);
  if (ds)
    {
    ds->GetBounds(bounds);
    return;
    }

  vtkCompositeDataSet* cd = vtkCompositeDataSet::SafeDownCast(dobj);
  if (cd)
    {
    vtkBoundingBox bbox;
    vtkCompositeDataIterator* iter = cd->NewIterator();
    for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
      {
      ds = vtkDataSet::SafeDownCast(iter->GetCurrentDataObject());
      if (ds && ds->GetNumberOfCells())
        {
        ds->GetBounds(bounds);
        bbox.AddBounds(bounds);
        }
      }
    iter->Delete();
    bbox.GetBounds(bounds);
    return;
    }

  vtkErrorMacro("unsupported dataset " << dobj->GetClassName());
}

//----------------------------------------------------------------------------
void vtkSurfaceLICPainter::PrintSelf(ostream & os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os
    << indent << "NumberOfSteps=" << this->NumberOfSteps << endl
    << indent << "StepSize=" << this->StepSize << endl
    << indent << "NormalizeVectors=" << this->NormalizeVectors << endl
    << indent << "EnhancedLIC=" << this->EnhancedLIC << endl
    << indent << "EnhanceContrast=" << this->EnhanceContrast << endl
    << indent << "LowLICContrastEnhancementFactor=" << this->LowLICContrastEnhancementFactor << endl
    << indent << "HighLICContrastEnhancementFactor=" << this->HighLICContrastEnhancementFactor << endl
    << indent << "LowColorContrastEnhancementFactor=" << this->LowColorContrastEnhancementFactor << endl
    << indent << "HighColorContrastEnhancementFactor=" << this->HighColorContrastEnhancementFactor << endl
    << indent << "AntiAlias=" << this->AntiAlias << endl
    << indent << "MaskOnSurface=" << this->MaskOnSurface << endl
    << indent << "MaskThreshold=" << this->MaskThreshold << endl
    << indent << "MaskIntensity=" << this->MaskIntensity << endl
    << indent << "MaskColor=" << this->MaskColor[0] << ", " << this->MaskColor[1] << ", " << this->MaskColor[2] << endl
    << indent << "ColorMode=" << this->ColorMode << endl
    << indent << "LICIntensity=" << this->LICIntensity << endl
    << indent << "MapModeBias=" << this->MapModeBias << endl
    << indent << "GenerateNoiseTexture=" << this->GenerateNoiseTexture << endl
    << indent << "NoiseType=" << this->NoiseType << endl
    << indent << "NoiseTextureSize=" << this->NoiseTextureSize << endl
    << indent << "NoiseGrainSize=" << this->NoiseGrainSize << endl
    << indent << "MinNoiseValue=" << this->MinNoiseValue << endl
    << indent << "MaxNoiseValue=" << this->MaxNoiseValue << endl
    << indent << "NumberOfNoiseLevels=" << this->NumberOfNoiseLevels << endl
    << indent << "ImpulseNoiseProbablity=" << this->ImpulseNoiseProbability << endl
    << indent << "ImpulseNoiseBackgroundValue=" << this->ImpulseNoiseBackgroundValue << endl
    << indent << "NoiseGeneratorSeed=" << this->NoiseGeneratorSeed << endl
    << indent << "AlwaysUpdate=" << this->AlwaysUpdate << endl
    << indent << "Enable=" << this->Enable << endl
    << indent << "CompositeStrategy=" << this->CompositeStrategy << endl;
}
