/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkLICNoiseHelper
 *
 * A small collection of noise routines for LIC
*/

#ifndef vtkLICNoiseHelper_h
#define vtkLICNoiseHelper_h

#include "vtkRenderingLICOpenGL2Module.h" // for export

#include "vtkMinimalStandardRandomSequence.h"

class vtkImageData;

/**
An interface to a random number generator. We can't use
c stdlib since we're not gauranteed to get consistent.
sequences across platform or library version and that
would prevent consistent output during regression tests.
*/
class vtkLICRandomNumberGeneratorInterface
{
public:
  vtkLICRandomNumberGeneratorInterface()
  {
    this->RNG = vtkMinimalStandardRandomSequence::New();
  }

  ~vtkLICRandomNumberGeneratorInterface()
  {
    this->RNG->Delete();
  }

  /**
  Seed the random number generator
  */
  void SetSeed(int seedVal)
  {
    this->RNG->SetSeed(seedVal);
  }

  /**
  Get a random number in the range of 0 to 1.
  */
  double GetRandomNumber()
  {
    double val = this->RNG->GetValue();
    this->RNG->Next();
    return val;
  }

private:
  void operator=(const vtkLICRandomNumberGeneratorInterface &) VTK_DELETE_FUNCTION;
  vtkLICRandomNumberGeneratorInterface(const vtkLICRandomNumberGeneratorInterface &) VTK_DELETE_FUNCTION;

private:
  vtkMinimalStandardRandomSequence *RNG;
};

/**
2D Noise Generator. Generate arrays for use as noise texture
in the LIC algorithm. Can generate noise with uniform or Gaussian
distributions, with a desired number of noise levels, and a
desired frequency (f < 1 is impulse noise).
*/
class vtkLICRandomNoise2D
{
public:
  vtkLICRandomNoise2D(){}

  //@{
  /**
   * Generate a patch of random gray scale values along with an
   * alpha channel (in vtk array format). The data should be
   * deleted by later calling DeleteValues. Grain size and sideLen
   * may be modified to match the noise generator requirements,
   * returned arrays will be sized accordingly.

   * type              - UNIFORM=0, GAUSSIAN=1, PERLIN=2
   * sideLen           - side length of square patch in pixels (in/out)
   * grainSize         - grain size of noise values in pixels (in/out)
   * nLevels           - number of noise intesity levels
   * minNoiseVal       - set the min for noise pixels (position distribution)
   * maxNoiseVal       - set the max for noise pixels (position distribution)
   * impulseProb       - probability of impulse noise,1 touches every pixel
   * impulseBgNoiseVal - set the background color for impulse noise
   * seed              - seed for random number generator
   */
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
  //@}

  /**
   * Delete the passed in array of values.
   */
  void DeleteValues(unsigned char *vals){ free(vals); }

  static vtkImageData *GetNoiseResource();

private:
  /**
   * Generate noise with a uniform distribution.
   */
  float *GenerateUniform(
        int sideLen,
        int grainLize,
        float minNoiseVal,
        float maxNoiseVal,
        int nLevels,
        double impulseProb,
        float impulseBgNoiseVal,
        int seed);

  /**
   * Generate noise with a Gaussian distribution.
   */
  float *GenerateGaussian(
        int sideLen,
        int grainLize,
        float minNoiseVal,
        float maxNoiseVal,
        int nLevels,
        double impulseProb,
        float impulseBgNoiseVal,
        int seed);

  /**
   * Generate Perlin noise with a Gaussian distribution.
   */
  float *GeneratePerlin(
        int sideLen,
        int grainLize,
        float minNoiseVal,
        float maxNoiseVal,
        int nLevels,
        double impulseProb,
        float impulseBgNoiseVal,
        int seed);

  /**
   * A way of controling the probability (from 0.0 to 1.0) that you
   * generate values. returns 1 if you should generate a value.
   * for example this is used to control the frequency of impulse
   * noise.
   */
  int ShouldGenerateValue(double prob);

  /**
   * Get a valid the length of the side of the patch and grains size in pixels
   * given a desired patch side length and a grain size. This ensures that all
   * grains are the same size.
   */
  void GetValidDimensionAndGrainSize(int type, int &dim, int &grainSize);

private:
  vtkLICRandomNumberGeneratorInterface ValueGen;
  vtkLICRandomNumberGeneratorInterface ProbGen;
};

#endif
// VTK-HeaderTest-Exclude: vtkLICNoiseHelper.h
