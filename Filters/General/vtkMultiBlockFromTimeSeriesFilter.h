/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMultiBlockFromTimeSeriesFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class vtkMultiBlockFromTimeSeriesFilter
 * @brief converts a temporal dataset into multiblock.
 *
 * @deprecated Use vtkGroupTimeStepsFilter instead. vtkGroupTimeStepsFilter can
 * handle vtkPartitionedDataSetCollection and other input types better.
 */

#ifndef vtkMultiBlockFromTimeSeriesFilter_h
#define vtkMultiBlockFromTimeSeriesFilter_h

#include "vtkFiltersGeneralModule.h" // For export macro
#include "vtkMultiBlockDataSetAlgorithm.h"
#include "vtkSmartPointer.h" // Smart pointer

#include <vector> // Vector to hold timesteps

class vtkMultiBlockDataSet;
VTK_DEPRECATED_IN_9_1_0("Use vtkGroupTimeStepsFilter instead")
class VTKFILTERSGENERAL_EXPORT vtkMultiBlockFromTimeSeriesFilter
  : public vtkMultiBlockDataSetAlgorithm
{
public:
  vtkTypeMacro(vtkMultiBlockFromTimeSeriesFilter, vtkMultiBlockDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  static vtkMultiBlockFromTimeSeriesFilter* New();

protected:
  vtkMultiBlockFromTimeSeriesFilter();
  ~vtkMultiBlockFromTimeSeriesFilter() override;

  int FillInputPortInformation(int, vtkInformation*) override;
  int RequestInformation(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int RequestUpdateExtent(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

private:
  vtkMultiBlockFromTimeSeriesFilter(const vtkMultiBlockFromTimeSeriesFilter&) = delete;
  void operator=(const vtkMultiBlockFromTimeSeriesFilter&) = delete;

  int UpdateTimeIndex;
  std::vector<double> TimeSteps;
  vtkSmartPointer<vtkMultiBlockDataSet> TempDataset;
};

#endif
