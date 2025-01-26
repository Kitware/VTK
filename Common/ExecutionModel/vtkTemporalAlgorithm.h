// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkTemporalAlgorithm
 * @brief   Base class for temporal algorithms
 *
 * `vtkTemporalAlgorithm` is a class to template over a subclass of `vtkAlgorithm`.
 * It effectively implements `RequestData`, which, depending on the request,
 * will call `Initialize`, `Execute' and / or `Finalize`. Algorithms subclassing
 * `vtkTemporalAlgorithm` should provide a temporal cache that on which to accumulate
 * data in order to provide a complete output upon calling `Finalize`.
 * This algorithm class assumes temporal integration over input port 0, connection 0.
 * Time steps are gathered from its input information, and `UPDATE_TIME_STEP()` requests
 * are propagated to this input connection only. Filters taking multiple time series as inputs
 * should probably not inherit from this class.
 *
 * This class of algorithm handles 2 types of temporal integration, controlled by the member
 * `IntegrateFullTimeSeries`:
 * * When turned ON, integration is performed over the entire input time series for any requested,
 *   time step. effectively removing the temporalness of the outputs.
 * * When turned OFF, the output remains temporal. It is the result of integrating all time steps
 *   up to the time step requested downstream by `UPDATE_TIME_STEP()`.
 *
 * In any case, this algorithm will request all necessary time steps upstream in order to generate
 * the output, in chronological order, setting the information key `CONTINUE_EXECUTING()`. The
 * executive of this filter will iterate over all time requests until the output is generating.
 * `Initialize` will be called if the requested time step is more ancient than the last generated
 * time step. Then, at each iteration, `Execute` is called. Finally, when the last needed iteration
 * has completed, `Finalize` is called.
 *
 * There are cases where the user does not have access to the entire time series at once. This
 * compromises filters that have `IntegrateFullTimeSeries` ON, and which rely on knowledge provided
 * by the information key `TIME_STEPS()`. The implementation of this algorithm provides a special
 * mode for such circumstances. All the user needs to do is set the information key
 * `NO_PRIOR_TEMPORAL_ACCESS()` in the sources.
 * If the information key `NO_PRIOR_TEMPORAL_ACCESS()` is set on the first input on port 0,
 * then this class will assume that the user is requesting time
 * steps in chronological order using `UpdateTimeStep(double)` and will provide a complete
 * output at each temporal iteration. Effectively, at each iteration, `Execute` and `Finalize` are
 * called. `Initialize` is called at the first iteration, or when `NO_PRIOR_TEMPORAL_ACCESS()` is
 * set to `vtkStreamingDemandDrivenPipeline::NO_PRIOR_TEMPORAL_ACCESS_RESET`. Processed time steps
 * are gathered in an array added to the field data of the outputs. The name of this array is
 * `time_steps` and can be retrieved through the method `TimeStepsArrayName()`.
 *
 * @warning Python wrapping of subclasses require special handling. Here is an example
 * ensuring wrapping works as expected,
 * implementing a temporal algorithm as a `vtkPassInputTypeAlgorithm`:
 *
 * ```
 * #include <vtkPassInputTypeAlgorithm.h>
 * #include <vtkTemporalAlgorithm.h>
 *
 * #ifndef __VTK_WRAP__
 * #define vtkPassInputTypeAlgorithm vtkTemporalAlgorithm<vtkPassInputTypeAlgorithm>
 * #endif
 *
 * // We make the wrapper think that we inherit from vtkPassInputTypeAlgorithm
 * class MyTemporalFilter : vtkPassInputTypeAlgorithm
 * {
 * public:
 *   vtkTypeMacro(MyTemporalFilter, vtkPassInputTypeAlgorithm);
 *
 * // We do not need to trick the wrapper with the superclass name anymore
 * #ifndef __VTK_WRAP__
 * #undef vtkPassInputTypeAlgorithm
 * #endif
 *
 * // We didn't wrap the non-overridden API of vtkTemporalAlgorithm. We define
 * // the missing API using this macro
 * #if defined(__VTK_WRAP__) || defined(__WRAP_GCCXML)
 *   vtkCreateWrappedTemporalAlgorithmInterface();
 * #endif
 *
 * // ...
 * };
 * ```
 */

#ifndef vtkTemporalAlgorithm_h
#define vtkTemporalAlgorithm_h

#include "vtkAlgorithm.h"
#include "vtkCommonExecutionModelModule.h" // For export macro
#include "vtkNew.h"                        // For TimeSteps

VTK_ABI_NAMESPACE_BEGIN

class vtkDoubleArray;
class vtkInformation;
class vtkInformationVector;

template <class AlgorithmT>
class vtkTemporalAlgorithm : public AlgorithmT
{
public:
  ///@{
  /**
   * Standard methods for instantiation, type information, and printing.
   */
  vtkTemplateTypeMacro(vtkTemporalAlgorithm, AlgorithmT);
  ///@}

  /**
   * When `vtkStreamingDemandDrivenPipeline::NO_PRIOR_TEMPORAL_ACCESS()` is set, an array with this
   * name is populated in the output's field data listing all the time steps executed so far.
   */
  static const char* TimeStepsArrayName() { return "time_steps"; }

  static_assert(std::is_base_of<vtkAlgorithm, AlgorithmT>::value,
    "Template argument must inherit vtkAlgorithm");

protected:
  vtkTemporalAlgorithm();

  int RequestInformation(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;
  int RequestUpdateTime(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;
  int RequestUpdateExtent(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;
  int RequestData(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

  /**
   * Method called at first temporal iteration. This method wipes any temporal cache that could be
   * maintained and sets up all the internals needed to run `Execute` properly.
   * The parameters are passed as is by `RequestData`.
   *
   * @warning Within this method, `GetCurrentTimeIndex` and `GetCurrentTimeStep` **should not**
   * be called. One can assume the current index is 0.
   */
  virtual int Initialize(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) = 0;

  /**
   * Method called at each time step. The temporal cache is updated to represent the
   * data available in the input.
   * The parameters are passed as is by `RequestData`.
   */
  virtual int Execute(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) = 0;

  /**
   * Method that converts the temporal cache into the outputs.
   * The parameters are passed as is by `RequestData`.
   */
  virtual int Finalize(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) = 0;

  /**
   * Returns the current time index being executed (or finalized). First iteration is indexed at 0.
   */
  int GetCurrentTimeIndex() const;

  /**
   * Returns the current time step being executed (or finalized).
   */
  double GetCurrentTimeStep() const;

  /**
   * To be set in the constructon. If true, all time steps are requested upstream for any requested
   * time step dowstream. Downstream effectively lose temporality. Otherwise, the algorithm
   * integrates inputs up to the requested time step.
   */
  bool IntegrateFullTimeSeries = false;

  /**
   * When turned on, time steps will be requested backward upstream.
   *
   * @warning This is deprecated. This is only here for one release cycle for backward compatibility
   * of some subclasses. Please avoid setting this.
   */
  bool RunBackward = false;

  ///@{
  /**
   * When the information key `NO_PRIOR_TEMPORAL_ACCESS()` is not set on the input port, this is
   * used to keep track of which iteration we are currently executing, and when to terminate.
   */
  std::vector<double> InputTimeSteps;
  int TerminationTimeIndex = 0;
  int CurrentTimeIndex = 0;
  ///@}

  /**
   * Returns true if the cache must be reinitialized before executing the current time step.
   */
  bool MustReset() const;

  /**
   * Returns true if there are time steps missing that must be requested upstream.
   */
  bool MustContinue() const;

  /**
   * When true, the algorithm calls Finalize at each iteration. It is set to true if the first input
   * on port 0 has set the information key `NO_PRIOR_TEMPORAL_ACCESS()`. It is typically set for in
   * situ visualization.
   */
  bool NoPriorTimeStepAccess = false;

  /**
   * Array only used when the information key `NO_PRIOR_TEMPORAL_ACCESS()` is set.
   * It is put in the output's field data.
   */
  vtkNew<vtkDoubleArray> ProcessedTimeSteps;

private:
  vtkTemporalAlgorithm(const vtkTemporalAlgorithm&) = delete;
  void operator=(const vtkTemporalAlgorithm&) = delete;
};

VTK_ABI_NAMESPACE_END

#define vtkCreateWrappedTemporalAlgorithmInterface()                                               \
  static const char* TimeStepsArrayName();                                                         \
                                                                                                   \
protected:                                                                                         \
  int GetCurrentTimeIndex() const;                                                                 \
  double GetCurrentTimeStep() const;                                                               \
  bool IntegrateFullTimeSeries;                                                                    \
                                                                                                   \
public:

#include "vtkTemporalAlgorithm.txx"
#endif
