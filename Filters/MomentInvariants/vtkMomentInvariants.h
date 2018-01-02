/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMomentInvariants.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*=========================================================================

Copyright (c) 2017, Los Alamos National Security, LLC

All rights reserved.

Copyright 2017. Los Alamos National Security, LLC.
This software was produced under U.S. Government contract DE-AC52-06NA25396
for Los Alamos National Laboratory (LANL), which is operated by
Los Alamos National Security, LLC for the U.S. Department of Energy.
The U.S. Government has rights to use, reproduce, and distribute this software.
NEITHER THE GOVERNMENT NOR LOS ALAMOS NATIONAL SECURITY, LLC MAKES ANY WARRANTY,
EXPRESS OR IMPLIED, OR ASSUMES ANY LIABILITY FOR THE USE OF THIS SOFTWARE.
If software is modified to produce derivative works, such modified software
should be clearly marked, so as not to confuse it with the version available
from LANL.

Additionally, redistribution and use in source and binary forms, with or
without modification, are permitted provided that the following conditions
are met:
-   Redistributions of source code must retain the above copyright notice,
    this list of conditions and the following disclaimer.
-   Redistributions in binary form must reproduce the above copyright notice,
    this list of conditions and the following disclaimer in the documentation
    and/or other materials provided with the distribution.
-   Neither the name of Los Alamos National Security, LLC, Los Alamos National
    Laboratory, LANL, the U.S. Government, nor the names of its contributors
    may be used to endorse or promote products derived from this software
    without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY LOS ALAMOS NATIONAL SECURITY, LLC AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL LOS ALAMOS NATIONAL SECURITY, LLC OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
/**
 * @class   vtkMomentInvariants
 * @brief   rotation invariant pattern detetction
 *
 * vtkMomentInvariants is a filter that performs pattern detection
 * it is able to determine the similarity independent from the orientation
 * of the template it takes the moments (momentData) as produced by
 * computeMoments and a pattern as inputs
 * 0. it produces a scalar field as output. Each point in this field contains
 * the similarity between the pattern and that point. it also puts out the
 * following things
 * 1. the normalized moments of the field
 * 2. the moments of the pattern
 * 1. the normalized moments of the pattern
 * the theory and the algorithm is described in Roxana Bujack and Hans Hagen:
 * "Moment Invariants for Multi-Dimensional Data"
 * http://www.informatik.uni-leipzig.de/~bujack/2017TensorDagstuhl.pdf
 * @par Thanks:
 * Developed by Roxana Bujack at Los Alamos National Laboratory.
 */

#ifndef vtkMomentInvariants_h
#define vtkMomentInvariants_h

#include "vtkDataSetAlgorithm.h"
#include "vtkDataSetAttributes.h"             // needed for vtkDataSetAttributes::FieldList
#include "vtkFiltersMomentInvariantsModule.h" // For export macro
#include "vtkTuple.h"                         // For internal API
#include <vector>                             // For internal API

class vtkImageData;
class vtkMomentsTensor;

class VTKFILTERSMOMENTINVARIANTS_EXPORT vtkMomentInvariants : public vtkDataSetAlgorithm
{
public:
  static vtkMomentInvariants* New();

  vtkTypeMacro(vtkMomentInvariants, vtkDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * standard pipeline input for port 1
   * This is the pattern, a vtkDataSet of scalar, vector, or matrix type.
   */
  void SetPatternData(vtkDataObject* input) { this->SetInputData(0, input); };

  /**
   * standard pipeline input for port 1
   * This is the pattern, a vtkDataSet of scalar, vector, or matrix type.
   */
  void SetPatternConnection(vtkAlgorithmOutput* algOutput)
  {
    this->SetInputConnection(0, algOutput);
  };

  /**
   * standard pipeline input for port 1
   * This is the vtkImageData field of which the moments are calculated. The
   * locations at which the moments are calculated are the points of the grid
   * in input.
   */
  void SetMomentData(vtkDataObject* input) { this->SetInputData(1, input); };

  /**
   * standard pipeline input for port 1
   * This is the vtkImageData field of which the moments are calculated. The
   * locations at which the moments are calculated are the points of the grid
   * in input.
   */
  void SetMomentConnection(vtkAlgorithmOutput* algOutput)
  {
    this->SetInputConnection(1, algOutput);
  };

  //@{
  /**
   * Set/Get the maximal order up to which the moments are computed.
   */
  vtkSetMacro(Order, unsigned int);
  vtkGetMacro(Order, unsigned int);
  //@}

  //@{
  /**
   * Set/Get the resolution of the integration.
   */
  vtkSetMacro(NumberOfIntegrationSteps, int);
  vtkGetMacro(NumberOfIntegrationSteps, int);
  //@}

  //@{
  /**
   * Set/Get the resolution of the integration.
   */
  vtkSetMacro(AngleResolution, int);
  vtkGetMacro(AngleResolution, int);
  //@}

  //@{
  /**
   * Set/Get the resolution of the integration.
   */
  vtkSetMacro(Eps, double);
  vtkGetMacro(Eps, double);
  //@}

  //@{
  /**
   * Set/Get the index of the array in the point data of which the momens are computed.
   */
  vtkSetMacro(NameOfPointData, std::string);
  vtkGetMacro(NameOfPointData, std::string);
  //@}

  //@{
  /**
   * Set/Get if the user wants invariance w.r.t. outer translation
   * that is addition of a constant
   */
  vtkSetMacro(IsTranslation, bool);
  vtkGetMacro(IsTranslation, bool);
  //@}

  //@{
  /**
   * Set/Get if the user wants invariance w.r.t. outer translation
   * that is addition of a constant
   */
  vtkSetMacro(IsScaling, bool);
  vtkGetMacro(IsScaling, bool);
  //@}

  //@{
  /**
   * Set/Get if the user wants invariance w.r.t. total rotation
   */
  vtkSetMacro(IsRotation, bool);
  vtkGetMacro(IsRotation, bool);
  //@}

  //@{
  /**
   * Set/Get if the user wants invariance w.r.t. total reflection
   */
  vtkSetMacro(IsReflection, bool);
  vtkGetMacro(IsReflection, bool);
  //@}

  /**
   * Get the number of basis functions in the momentData
   * equals \sum_{i=0}^order dimension^o
   */
  vtkGetMacro(NumberOfBasisFunctions, int);

  /**
   * Get the different integration radii from the momentData
   */
  std::vector<double> GetRadii() { return this->Radii; }

  /**
   * Get the different integration radii from the momentData as constant length array for python
   * wrapping
   */
  void GetRadiiArray(double radiiArray[10])
  {
    for (int i = 0; i < 10; ++i)
    {
      radiiArray[i] = 0;
    }
    for (size_t i = 0; i < this->Radii.size(); ++i)
    {
      radiiArray[i] = this->Radii.at(i);
    }
  };

  /**
   * Get the number of the different integration radii from the momentData
   */
  int GetNumberOfRadii() { return this->Radii.size(); };

  /**
   * Get the different integration radii from the momentData as string. Convenience function.
   */
  std::string GetStringRadii(int i) { return std::to_string(this->Radii.at(i)).c_str(); };

  /**
   * Get the translation factor
   */
  double GetTranslationFactor(int radius, int p, int q, int r)
  {
    return this->TranslationFactor[radius + p * this->Radii.size() +
      q * this->Radii.size() * (this->Order + 1) +
      r * this->Radii.size() * (this->Order + 1) * (this->Order + 1)];
  };

  /**
   * Get the translation factor
   */
  void SetTranslationFactor(int radius, int p, int q, int r, double value)
  {
    this->TranslationFactor[radius + p * this->Radii.size() +
      q * this->Radii.size() * (this->Order + 1) +
      r * this->Radii.size() * (this->Order + 1) * (this->Order + 1)] = value;
  };

protected:
  /**
   * constructior setting defaults
   */
  vtkMomentInvariants();

  /**
   * destructor
   */
  ~vtkMomentInvariants() override;

  int RequestUpdateExtent(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  /** main executive of the program, reads the input, calles the
   * functions, and produces the utput.
   * @param inputVector: the input information
   * @param outputVector: the output information
   */
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

private:
  vtkMomentInvariants(const vtkMomentInvariants&) = delete;
  void operator=(const vtkMomentInvariants&) = delete;

  /**
   * Integration radius of the pattern
   */
  double RadiusPattern;

  /**
   * center of the pattern
   */
  double* CenterPattern;

  /**
   * Dimension can be 2 or 3
   */
  int Dimension;

  /**
   * Rank of the momentData is 0 for scalars, 1 for vectors, 3 for matrices
   */
  int FieldRank;

  /**
   * Maximal order up to which the moments are calculated
   */
  unsigned int Order;

  /**
   * Integration radius
   */
  std::vector<double> Radii;

  /**
   * How fine is the discrete integration done?
   */
  int NumberOfIntegrationSteps;

  /**
   * If the pattern has multiple fields in its point data, you can choose the one that the moments
   * shall be calculated of
   */
  std::string NameOfPointData;

  /**
   * the number of fields in the momentData
   * equals NumberOfBasisFunctions * numberOfRadii
   */
  size_t NumberOfFields;

  /**
   * the number of basis functions in the momentData
   * equals \sum_{i=0}^order dimension^o
   */
  size_t NumberOfBasisFunctions;

  /**
   * if the user wants invariance w.r.t. outer translation
   * that is addition of a constant
   */
  bool IsTranslation;

  /**
   * if the user wants invariance w.r.t. outer scaling
   * that is multiplication of a constant
   */
  bool IsScaling;

  /**
   * if the user wants invariance w.r.t. total rotation
   */
  bool IsRotation;

  /**
   * if the user wants invariance w.r.t. total reflection
   */
  bool IsReflection;

  /**
   * if the algorithm is not able to find dominant contractions for the normalization w.r.t.
   * rotation, it has to default back to looking "everywhere". This parameter determines how fine
   * this is performed in 2D, we divide phi=[0,...,2Pi] into that many equidistant steps in 3D, we
   * divide phi=[0,...,2Pi] into that many equidistant steps and theta=[0,...,Pi] in half that many
   * steps to determine the rotation axis. Then, we use anther AngleResolution different rotation
   * angles in [0,...,2Pi] to cover all positions the number of comparisons is AngleResolution in 2D
   * and 0.5 * AngleResolution^3 in 3D
   */
  int AngleResolution;

  /**
   * this parameter determines if the algorithm is not able to find dominant contractions for the
   * normalization w.r.t. rotation. Non-zero means > eps
   */
  double Eps;

  /**
   * this contains the moments of the pattern
   */
  std::vector<vtkMomentsTensor> MomentsPattern;

  /**
   * this contains the moments of the pattern after normalization w.r.t. translation. c00 should be
   * 0 now
   */
  std::vector<vtkMomentsTensor> MomentsPatternTNormal;

  /**
   * this contains the moments of the pattern after normalization w.r.t. scaling. the norm should be
   * 1 now
   */
  std::vector<vtkMomentsTensor> MomentsPatternTSNormal;

  /**
   * this contains the moments of the pattern after normalization w.r.t. rotation. several standard
   * positions can occur. it contains all orientations of the moments of the pattern. during the
   * detection later, we will compare the moments of the field to all these version of the pattern
   */
  std::vector<std::vector<vtkMomentsTensor> > MomentsPatternNormal;

  /**
   * this contains the translational factors necessary for normalization w.r.t. translation
   * we have radius and then p,q,r
   */
  double* TranslationFactor;

  /**
   * the agorithm has two input ports
   * port 0 is the pattern, which is a vtkDataSet of scalar, vector, or matrix type
   * port 1 is the output of computeMoments, which is vtkImageData
   */
  int FillInputPortInformation(int port, vtkInformation* info) override;

  /**
   * the agorithm generates 4 outputs, all are vtkImageData
   * the first two have the topology of the momentData
   * a field storing the similarity to the pattern for all radii in a scalar field each
   * the normalized moments of the field
   * the latter two have extent 0, they only have 1 point in each field
   * the moments of the pattern
   * the first standard position of the normalized moments of the pattern
   */
  int FillOutputPortInformation(int port, vtkInformation* info) override;

  /**
   * Make sure that the user has not entered weird values.
   * @param pattern: the pattern that we will look for
   */
  void CheckValidity(vtkImageData* pattern);

  /**
   * this functions reads out the parameters from the pattern and checks if they assume reasonable
   * values
   * @param pattern: the pattern that we will look for
   */
  void InterpretPattern(vtkImageData* pattern);

  /**
   * this functions reads out the parameters from the momentData and checks if they assume
   * reasonable values and if they match the ones from the pattern
   * @param moments: the moment data
   */
  void InterpretField(vtkImageData* moments);

  /**
   * calculation of the moments of the pattern and its invariants.
   * we choose, which contractions (dominantContractions) can be used for the normalization of this
   * particular pattern, i.e. the ones that are nt zero or linearly dependent. They will later be
   * used for the normalization of the field moments, too.
   * @param dominantContractions: the vectors that can be used for the normalization of this
   * particular pattern, i.e. the ones that are nt zero or linearly dependent
   * @param pattern: the pattern
   * @param originalMomentsPattern: the moments of the pattern
   * @param normalizedMomentsPattern: the normalized moments of the pattern. It
   * visualizes how the standard position of this particular pattern looks like
   */
  void HandlePattern(std::vector<std::vector<vtkMomentsTensor> >& dominantContractions,
    vtkImageData* pattern,
    vtkImageData* originalMomentsPattern,
    vtkImageData* normalizedMomentsPattern);

  /**
   * main part of the pattern detetction
   * the moments of the field at each point are normalized and compared to the moments of the
   * pattern
   * @param dominantContractions: the dominant contractions, i.e. vectors for the normalization
   * w.r.t. rotation
   * @param moments: the moments of the field
   * @param normalizedMoments: the moment invariants of the field
   * @param pattern: the pattern
   * @param similarityFields: the output of this algorithm. it has the topology of moments and will
   * have a number of scalar fields euqal to NumberOfRadii. each point contains the similarity of
   * its surrounding (of size radius) to the pattern
   */
  void HandleField(std::vector<std::vector<vtkMomentsTensor> >& dominantContractions,
    vtkImageData* moments,
    vtkImageData* normalizedMoments,
    vtkImageData* pattern,
    vtkImageData* similarityFields);

  /**
   * this computes the translational factors necessary for normalization w.r.t. translation
   * we have radius and then p,q,r
   * @param pattern: the pattern
   */
  void BuildTranslationalFactorArray(vtkImageData* pattern);

  /**
   * normalization with respect to outer transaltion, i.e. the result will be invariant to adding a
   * constant
   * the translational factor is evaluated with the same stencil as the moments
   * @param moments: the moments at one point stored in a vector of tensors
   * @param radius: the integration radius over which the moments were computed
   * @param isTranslation: if normalization w.tr.t. translation is desired by the user
   * @param stencil: the points in this stencil are used to numerically approximate the integral
   * @return the translationally normalized moments
   */
  std::vector<vtkMomentsTensor> NormalizeT(std::vector<vtkMomentsTensor>& moments,
    double radius,
    bool isTranslation,
    vtkImageData* stencil);

  /**
   * normalization with respect to outer transaltion, i.e. the result will be invariant to adding a
   * constant
   * the translational factor is evaluated with the same stencil as the moments
   * @param moments: the moments at one point stored in a vector of tensors
   * @param radiusIndex: the index pointing to the integration radius over which the moments were
   * computed
   * @param isTranslation: if normalization w.tr.t. translation is desired by the user
   * @return the translationally normalized moments
   */
  std::vector<vtkMomentsTensor> NormalizeT(std::vector<vtkMomentsTensor>& moments,
    int radiusIndex,
    bool isTranslation);

  /**
   * normalization with respect to outer transaltion, i.e. the result will be invariant to adding a
   * constant
   * the translational factor is evaluated from the analytic formula
   * @param moments: the moments at one point stored in a vector of tensors
   * @param radius: the integration radius over which the moments were computed
   * @param isTranslation: if normalization w.tr.t. translation is desired by the user
   * @return the translationally normalized moments
   */
  std::vector<vtkMomentsTensor> NormalizeTAnalytic(std::vector<vtkMomentsTensor>& moments,
    double radius,
    bool isTranslation);

  /**
   * normalization with respect to outer scaling, i.e. the result will be invariant to multiplying a
   * constant
   * @param moments: the moments at one point stored in a vector of tensors
   * @param isScaling: if normalization w.tr.t. scalin is desired by the user
   * @return the scale normalized moments
   */
  std::vector<vtkMomentsTensor> NormalizeS(std::vector<vtkMomentsTensor>& moments,
    bool isScaling,
    double radius);

  /** normalization of the pattern with respect to rotation and reflection
   * @param dominantContractions: the vectors used for the normalization
   * @param isRotation: if the user wants normalization w.r.t rotation
   * @param isReflection: if the user wants normalization w.r.t reflection
   * @param moments: the moments at a given point
   */
  std::vector<vtkMomentsTensor> NormalizeR(std::vector<vtkMomentsTensor>& dominantContractions,
    bool isRotation,
    bool isReflection,
    std::vector<vtkMomentsTensor>& moments);

  /** calculation of the dominant contraction
   * there can be multiple dominant contractions due to EV or 3D
   * dominantContractions.at( i ) contains 1 vector in 2D and 2 in 3D
   * dominantContractions.size() = 1 if no EV, 2 if 1 EV, 4 if 2EV are chosen
   * if no contraction was found dominantContractions.size() = 0
   * if only one contraction was found in 3D dominantContractions.at(i).size() = 1 instead of 2
   * @param momentsPattern: the moments of the pattern
   * @return the dominant contractions, i.e. the biggest vectors that can be used for the
   * normalizaion w.r.t. rotation
   */
  std::vector<std::vector<vtkMomentsTensor> > CalculateDominantContractions(
    std::vector<vtkMomentsTensor>& momentsPattern);

  /** the dominant contractions are stored as a vector of integers that encode which tensors were
   * multiplied and contracted to form them. This function applies these excat instructions to the
   * moments in the field. That way, these can be normalized in the same way as the pattern was,
   * which is crucial for the comparison.
   * @param dominantContractions: the vectors that can be used for the normalization of this
   * particular pattern, i.e. the ones that are nt zero or linearly dependent
   * @param moments: the moments at one point
   */
  std::vector<vtkMomentsTensor> ReproduceContractions(
    std::vector<vtkMomentsTensor>& dominantContractions,
    std::vector<vtkMomentsTensor>& moments);

  /** if no dominant contractions could be found to be non-zero, the algorithm defaults back to
   * looking for all possible orientations of the given template the parameter AngleResolution
   * determines what "everywhere" means in 2D, we divide phi=[0,...,2Pi] into that many equidistant
   * steps in 3D, we divide phi=[0,...,2Pi] into that many equidistant steps and theta=[0,...,Pi] in
   * half that many steps to determine the rotation axis. Then, we use anther AngleResolution
   * different rotation angles in [0,...,2Pi] to cover all positions
   * @param momentsPatternNormal: this contains all orientations of the moments of the pattern.
   * during the detection later, we will compare the moments of the field to all these version of
   * the pattern
   * @param momentsPatternTranslationalNormal: this contains the moments that are not invariant to
   * orientation yet
   */
  void LookEverywhere(std::vector<std::vector<vtkMomentsTensor> >& momentsPatternNormal,
    std::vector<vtkMomentsTensor>& momentsPatternTranslationalNormal);

  /** if only one dominant contraction could be found to be non-zero, but no second one to be
   * linearly independent from the first one, the algorithm, will rotate the first contraction to
   * the x-axis and the look for all possible orientations of the given template around this axis.
   * In principal, it reduces the 3D problem to a 2D problem. the parameter AngleResolution determines
   * what "everywhere" means we divide phi=[0,...,2Pi] into that many equidistant steps
   * @param dominantContractions: the vectors used for the normalization
   * @param momentsPatternNormal: this contains all orientations of the moments of the pattern.
   * during the detection later, we will compare the moments of the field to all these version of
   * the pattern
   */
  void LookEverywhere(std::vector<std::vector<vtkMomentsTensor> >& dominantContractions,
    std::vector<std::vector<vtkMomentsTensor> >& momentsPatternNormal);

  /**
   * this functions uses the moments, weighs them with their corresponding basis function and adds
   * them up to approximate the value of the original function. The more moments are given, the
   * better the approximation, like in a taylor series
   * @param p: the location (3D point) at which the reconstructed field is evaluated
   * @param moments: the moments at a given location, which is used for the reconstruction
   * @param center: location, where the moments are given
   * @return: the value of the reconstructed function can be scalar, vector, or matrix valued
   */
  template<size_t S>
  vtkTuple<double, S> Reconstruct(double* p,
    std::vector<vtkMomentsTensor>& moments,
    double* center);
};

#endif
