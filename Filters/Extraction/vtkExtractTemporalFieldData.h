/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExtractTemporalFieldData.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkExtractTemporalFieldData
 * @brief   Extract temporal arrays from input field data
 *
 * vtkExtractTemporalFieldData extracts arrays from the input vtkFieldData.
 * These arrays are assumed to contain temporal data, where the nth tuple
 * contains the value for the nth timestep.
 *
 * For composite datasets, the filter has two modes, it can treat each block in
 * the dataset individually (default) or just look at the first non-empty field data
 * (common for readers vtkExodusIIReader). For latter, set
 * HandleCompositeDataBlocksIndividually to false.
 *
 * The output is a vtkTable (or a multiblock of vtkTables) based of whether
 * HandleCompositeDataBlocksIndividually is true and input is a composite
 * dataset.
 *
 * This algorithm does not produce a TIME_STEPS or TIME_RANGE information
 * because it works across time.
 *
 * @par Caveat:
 * This algorithm works only with source that produce TIME_STEPS().
 * Continuous time range is not yet supported.
*/

#ifndef vtkExtractTemporalFieldData_h
#define vtkExtractTemporalFieldData_h

#include "vtkFiltersExtractionModule.h" // For export macro
#include "vtkDataObjectAlgorithm.h"

class vtkDataSet;
class vtkTable;
class vtkDataSetAttributes;

class VTKFILTERSEXTRACTION_EXPORT vtkExtractTemporalFieldData : public vtkDataObjectAlgorithm
{
public:
  static vtkExtractTemporalFieldData *New();
  vtkTypeMacro(vtkExtractTemporalFieldData,vtkDataObjectAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Get the number of time steps
   */
  int GetNumberOfTimeSteps();

  //@{
  /**
   * When set to true (default), if the input is a vtkCompositeDataSet, then
   * each block in the input dataset in processed separately. If false, then the first
   * non-empty FieldData is considered.
   */
  vtkSetMacro(HandleCompositeDataBlocksIndividually, bool);
  vtkGetMacro(HandleCompositeDataBlocksIndividually, bool);
  vtkBooleanMacro(HandleCompositeDataBlocksIndividually, bool);
  //@}

protected:
  vtkExtractTemporalFieldData();
  ~vtkExtractTemporalFieldData() override;

  int RequestDataObject(vtkInformation*,
                        vtkInformationVector**,
                        vtkInformationVector*) override;
  int RequestInformation(vtkInformation* request,
                         vtkInformationVector** inputVector,
                         vtkInformationVector* outputVector) override;
  int RequestData(vtkInformation* request,
                  vtkInformationVector** inputVector,
                  vtkInformationVector* outputVector) override;
  int FillInputPortInformation(int port, vtkInformation* info) override;

  /**
   * This looks at the arrays in the vtkFieldData of input and copies them
   * to the output point data.
   * Returns true if the input had an "appropriate" field data.
   */
  bool CopyDataToOutput(vtkDataSet *input, vtkTable *output);

  bool HandleCompositeDataBlocksIndividually;
private:
  vtkExtractTemporalFieldData(const vtkExtractTemporalFieldData&) = delete;
  void operator=(const vtkExtractTemporalFieldData&) = delete;

  class vtkInternals;
  vtkInternals* Internals;
};

#endif
