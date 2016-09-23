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
 * The timesteps to be extracted are specified by their indices. If no
 * time step is specified, all of the input time steps are extracted.
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
  void PrintSelf(ostream& os, vtkIndent indent);

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

protected:
  vtkExtractTimeSteps() {};
  ~vtkExtractTimeSteps() {};

  virtual int RequestData(vtkInformation *, vtkInformationVector **,
                          vtkInformationVector *);
  virtual int RequestInformation(vtkInformation *, vtkInformationVector **,
                                 vtkInformationVector *);

  std::set<int> TimeStepIndices;

private:
  vtkExtractTimeSteps(const vtkExtractTimeSteps&) VTK_DELETE_FUNCTION;
  void operator=(const vtkExtractTimeSteps&) VTK_DELETE_FUNCTION;
};

#endif // vtkExtractTimeSteps_h
