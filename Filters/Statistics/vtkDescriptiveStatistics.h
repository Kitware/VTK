// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2010 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov
/**
 * @class   vtkDescriptiveStatistics
 * @brief   A class for univariate descriptive statistics
 *
 *
 * Given a selection of columns of interest in an input data table, this
 * class provides the following functionalities, depending on the chosen
 * execution options:
 * * Learn: calculate extremal values, sample mean, and M2, M3, and M4 aggregates
 *   (cf. P. Pebay, Formulas for robust, one-pass parallel computation of covariances
 *   and Arbitrary-Order Statistical Moments, Sandia Report SAND2008-6212, Sep 2008,
 *   http://infoserve.sandia.gov/sand_doc/2008/086212.pdf for details)
 * * Derive: calculate unbiased variance estimator, standard deviation estimator,
 *   two skewness estimators, and two kurtosis excess estimators.
 * * Assess: given an input data set, a reference value and a non-negative deviation,
 *   mark each datum with corresponding relative deviation (1-dimensional Mahlanobis
 *   distance). If the deviation is zero, then mark each datum which are equal to the
 *   reference value with 0, and all others with 1. By default, the reference value
 *   and the deviation are, respectively, the mean and the standard deviation of the
 *   input model.
 * * Test: calculate Jarque-Bera statistic and, if VTK to R interface is available,
 *   retrieve corresponding p-value for normality testing.
 *
 * Among the derived statistics, the variance, the standard deviation, the skewness
 * and the kurtosis can be estimated in two ways: using the sample version of those
 * statistics, or the population version. Specify whether a sample estimate or population
 * estimate is done by setting `SampleEstimate`. By default, `SampleEstimate == true`, hence
 * the sample version of the statistics is estimated,
 * which produces unbiased estimators (except for the sample standard deviation).
 * The sample estimate should be used for input that represent a subset of the whole
 * population of study. On the other hand, when `SampleEstimate == false`, the population
 * version of the statistics is estimated. If the input doesn't contain all the samples
 * from the population of study, then a bias is induced (the variance is slightly bigger than it
 * should be). One can read about Bessel's correction to understand better where this comes from.
 * That being said, on very large data, the difference between the 2 estimation formulas
 * becomes very low, so in those instances,
 * either state of `SampleEstimate` should yield very similar results
 * (see explicit formulas below).
 *
 * \verbatim
 *
 * The formulas used are as follows, writing \f( \bar{X} \f) the mean of \f( X \f) and \f( N \f)
 * the number of samples:
 * - Sample estimate:
 *   \f[
 *    Var{X} = s^2 = \frac{\sum_{k=1}^N \left(x_k - \bar{x}\right)^2 }{N - 1}
 *   \f]
 *   \f[
 *    Skew{X} = \frac{n}{(n - 1)(n - 2)}
 *    \frac{\sum_{k=1}^N \left(x_k - \bar{x}\right)^3 }{s^3}
 *   \f]
 *   \f[
 *    Kurt{X} = \frac{n(n + 1)}{(n - 1)(n - 2)(n - 3)}
 *    \frac{\sum_{k=1}^N \left(x_k - \bar{x}\right)^3 }{s^4}
 *    - 3 \frac{(n - 1)^2}{(n - 2)(n - 3)}
 *   \f]
 * - Population estimate:
 *   \f[
 *    Var{X} = \sigma^2 = \frac{\sum_{k=1}^N \left(x_k - \bar{x}\right)^2 }{N}
 *   \f]
 *   \f[
 *    Skew{X} = \frac{1}{N}\frac{\sum_{k=1}^N \left(x_k - \bar{x}\right)^3 }{\sigma^3}
 *   \f]
 *   \f[
 *    Kurt{X} = \frac{1}{N}\frac{\sum_{k=1}^N \left(x_k - \bar{x}\right)^3 }{\sigma^4} - 3
 *   \f]
 *
 * \f(\sigma\f) is the population standard deviation, and \f(s\f) is the sample standard deviation.
 * Note that the kurtosis is corrected so the kurtosis of a gaussian distribution should yield 0.
 *
 * In the instance where \f(\sigma = 0\f) or \f(s = 0\f), the skewness and kurtosis are undefined.
 * Thus they output a `NaN`. Similarly, if there are no samples, then all derived statistics
 * yield a `NaN`.
 *
 * \endverbatim
 *
 * @par Thanks:
 * Thanks to Philippe Pebay and David Thompson from Sandia National Laboratories
 * for implementing this class.
 * Updated by Philippe Pebay, Kitware SAS 2012
 */

#ifndef vtkDescriptiveStatistics_h
#define vtkDescriptiveStatistics_h

#include "vtkFiltersStatisticsModule.h" // For export macro
#include "vtkStatisticsAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkStatisticalModel;
class vtkStringArray;
class vtkTable;
class vtkVariant;
class vtkDoubleArray;

class VTKFILTERSSTATISTICS_EXPORT vtkDescriptiveStatistics : public vtkStatisticsAlgorithm
{
public:
  vtkTypeMacro(vtkDescriptiveStatistics, vtkStatisticsAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  static vtkDescriptiveStatistics* New();

  /// Descriptive statistics requests are univariate.
  int GetMaximumNumberOfColumnsPerRequest() const override { return 1; }

  ///@{
  /**
   * Getter / Setter on `SampleEstimate`. When turned on, descriptive statistics
   * computed by this filter assume that the input data only holds a sample of the whole
   * population of study. In effect, the sample variance, the sample standard deviation,
   * the sample skewness and the sample kurtosis are estimated. When turned off, the population
   * variance, the population standard deviation, the population skewness and the population
   * kurtosis are estimated instead.
   *
   * Put another way, if the input data is a full description of the population being studied,
   * `SampleEstimate` should be turned off. If the input data is a sample of the population being
   * studied, then `SampleEstimate` should be turned on. By default, `SampleEstimate` is turned
   * on, as it is the most likely case.
   *
   * Please see class description for a full description of the formulas.
   *
   * @note For large data, the difference between the population estimate and the sample
   * estimate becomes small, so this parameter becomes of less worry.
   */
  vtkSetMacro(SampleEstimate, bool);
  vtkGetMacro(SampleEstimate, bool);
  vtkBooleanMacro(SampleEstimate, bool);
  ///@}

  ///@{
  /**
   * Set/get whether the deviations returned should be signed, or should
   * only have their magnitude reported.
   * The default is that signed deviations will be computed.
   */
  vtkSetMacro(SignedDeviations, vtkTypeBool);
  vtkGetMacro(SignedDeviations, vtkTypeBool);
  vtkBooleanMacro(SignedDeviations, vtkTypeBool);
  ///@}

  /**
   * Given a collection of models, calculate aggregate model
   */
  bool Aggregate(vtkDataObjectCollection*, vtkStatisticalModel*) override;

  ///@{
  /**
   * Return the mean, variance, standard deviation, skewness, and kurtosis for the given variable.
   *
   * This is a helper function that may be called **after** you have Update()'ed this filter with
   * both LearnOption and DerivedOption enabled; it uses the output model tables to fetch results
   * into a form more easily used for scripts that wish to use the model.
   *
   * This method returns false if a variable named \a fieldName is unavailable. In that
   * case, the other parameters (passed by reference) will be untouched. If true is returned,
   * then their values will be updated for the given \a fieldName.
   */
  bool GetDistributionForField(const std::string& fieldName, double& mean, double& variance,
    double& stdev, double& skewness, double& kurtosis);
  ///@}

  /// Provide a string that can be used to recreate an instance of this algorithm.
  void AppendAlgorithmParameters(std::string& algorithmParameters) const override;

  /// Implement the inverse of GetAlgorithmParameters(): given a parameter, update this algorithm.
  std::size_t ConsumeNextAlgorithmParameter(
    vtkStringToken parameterName, const std::string& algorithmParameters) override;

protected:
  vtkDescriptiveStatistics();
  ~vtkDescriptiveStatistics() override;

  /**
   * Execute the calculations required by the Learn option, given some input Data
   * NB: input parameters are unused.
   */
  void Learn(vtkTable*, vtkTable*, vtkStatisticalModel*) override;

  /**
   * Execute the calculations required by the Derive option.
   */
  void Derive(vtkStatisticalModel*) override;

  /**
   * Execute the calculations required by the Test option.
   */
  void Test(vtkTable*, vtkStatisticalModel*, vtkTable*) override;

  /**
   * Execute the calculations required by the Assess option.
   */
  void Assess(vtkTable* inData, vtkStatisticalModel* inMeta, vtkTable* outData) override
  {
    this->Superclass::Assess(inData, inMeta, outData, 1);
  }

  /**
   * Calculate p-value. This will be overridden using the object factory with an
   * R implementation if R is present.
   */
  virtual vtkDoubleArray* CalculatePValues(vtkDoubleArray*);

  /**
   * Provide the appropriate assessment functor.
   */
  void SelectAssessFunctor(vtkTable* outData, vtkDataObject* inMeta, vtkStringArray* rowNames,
    AssessFunctor*& dfunc) override;

  bool SampleEstimate;
  vtkTypeBool SignedDeviations;

private:
  vtkDescriptiveStatistics(const vtkDescriptiveStatistics&) = delete;
  void operator=(const vtkDescriptiveStatistics&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
