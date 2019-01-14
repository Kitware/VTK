/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExtractTimeSteps.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

    This software is distributed WITHOUT ANY WARRANTY; without even
    the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
    PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkExtractTimeSteps
 * @brief   extract specific time-steps from dataset
 *
 * vtkExtractTimeSteps extracts the specified time steps from the input dataset.
 * It has two modes, one to specify timesteps explicitly by their indices and one
 * to specify a range of timesteps to extract.
 *
 * When specifying timesteps explicitly the timesteps to be extracted are
 * specified by their indices. If no time step is specified, all of the input
 * time steps are extracted.
 *
 * When specifying a range, the beginning and end times are specified and the
 * timesteps in between are extracted.  This can be modified by the TimeStepInterval
 * property that sets the filter to extract every Nth timestep.
 *
 * This filter is useful when one wants to work with only a sub-set of the input
 * time steps.
*/

#ifndef vtkExtractTimeSteps_h
#define vtkExtractTimeSteps_h

#include "vtkFiltersExtractionModule.h" // for export macro
#include "vtkPassInputTypeAlgorithm.h"

#include <set> // for time step indices


class VTKFILTERSEXTRACTION_EXPORT vtkExtractTimeSteps : public vtkPassInputTypeAlgorithm
{
public:
  vtkTypeMacro(vtkExtractTimeSteps, vtkPassInputTypeAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  static vtkExtractTimeSteps *New();

  /**
   * Get the number of time steps that will be extracted
   */
  int GetNumberOfTimeSteps() const
  {
    return static_cast<int>(this->TimeStepIndices.size());
  }

  /**
   * Add a time step index. Not added if the index already exists.
   */
  void AddTimeStepIndex(int timeStepIndex);

  //@{
  /**
   * Get/Set an array of time step indices. For the Get function,
   * timeStepIndices should be big enough for GetNumberOfTimeSteps() values.
   */
  void SetTimeStepIndices(int count, const int *timeStepIndices);
  void GetTimeStepIndices(int *timeStepIndices) const;
  //@}

  /**
   * Generate a range of indices in [begin, end) with a step size of 'step'
   */
  void GenerateTimeStepIndices(int begin, int end, int step);

  //@{
  /**
   * Clear the time step indices
   */
  void ClearTimeStepIndices()
  {
    this->TimeStepIndices.clear();
    this->Modified();
  }
  //@}

  //@{
  /**
   * Get/Set whether to extract a range of timesteps.  When false, extracts
   * the time steps explicitly set with SetTimeStepIndices.  Defaults to false.
   */
  vtkGetMacro(UseRange, bool);
  vtkSetMacro(UseRange, bool);
  vtkBooleanMacro(UseRange, bool);
  //@}

  //@{
  /**
   * Get/Set the range of time steps to extract.
   */
  vtkGetVector2Macro(Range, int);
  vtkSetVector2Macro(Range, int);
  //@}

  //@{
  /**
   * Get/Set the time step interval to extract.  This is the N in 'extract every
   * Nth timestep in this range'.  Default to 1 or 'extract all timesteps in this range.
   */
  vtkGetMacro(TimeStepInterval, int);
  vtkSetClampMacro(TimeStepInterval, int, 1, VTK_INT_MAX);
  //@}

  // What timestep to provide when the requested time is between the timesteps
  // the filter is set to extract
  enum {
    PREVIOUS_TIMESTEP, // floor the time to the previous timestep
    NEXT_TIMESTEP, // ceiling the time to the next timestep
    NEAREST_TIMESTEP // take the timestep whose absolute difference from the requested time is smallest
  } EstimationMode;
  //@{
  /**
   * Get/Set what to do when the requested time is not one of the timesteps this filter
   * is set to extract.  Should be one of the values of the enum vtkExtractTimeSteps::EstimationMode.
   * The default is PREVIOUS_TIMESTEP.
   */
  vtkGetMacro(TimeEstimationMode, int);
  vtkSetMacro(TimeEstimationMode, int);
  void SetTimeEstimationModeToPrevious() { this->SetTimeEstimationMode(PREVIOUS_TIMESTEP); }
  void SetTimeEstimationModeToNext() { this->SetTimeEstimationMode(NEXT_TIMESTEP); }
  void SetTimeEstimationModeToNearest() { this->SetTimeEstimationMode(NEAREST_TIMESTEP); }
  //@}

protected:
  vtkExtractTimeSteps();
  ~vtkExtractTimeSteps() override {};

  int RequestData(vtkInformation *, vtkInformationVector **,
                          vtkInformationVector *) override;
  int RequestInformation(vtkInformation *, vtkInformationVector **,
                                 vtkInformationVector *) override;
  int RequestUpdateExtent(vtkInformation *, vtkInformationVector **,
                                 vtkInformationVector *) override;

  std::set<int> TimeStepIndices;
  bool UseRange;
  int Range[2];
  int TimeStepInterval;
  int TimeEstimationMode;

private:
  vtkExtractTimeSteps(const vtkExtractTimeSteps&) = delete;
  void operator=(const vtkExtractTimeSteps&) = delete;
};

#endif // vtkExtractTimeSteps_h
