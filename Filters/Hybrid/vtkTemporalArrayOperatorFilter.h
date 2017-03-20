/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTemporalArrayOperatorFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

    This software is distributed WITHOUT ANY WARRANTY; without even
    the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
    PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkTemporalArrayOperatorFilter
 * @brief   perform simple mathematical operation on a data array at different time
 *
 * This filter computes a simple operation between two time steps of one
 * data array.
 *
 * @sa
 * vtkArrayCalulator
 */

#ifndef vtkTemporalArrayOperatorFilter_h
#define vtkTemporalArrayOperatorFilter_h

#include "vtkFiltersHybridModule.h" // For export macro
#include "vtkMultiTimeStepAlgorithm.h"

class VTKFILTERSHYBRID_EXPORT vtkTemporalArrayOperatorFilter :
  public vtkMultiTimeStepAlgorithm
{
public:
  static vtkTemporalArrayOperatorFilter* New();
  vtkTypeMacro(vtkTemporalArrayOperatorFilter, vtkMultiTimeStepAlgorithm);
  virtual void PrintSelf(ostream &OS, vtkIndent indent) VTK_OVERRIDE;

  enum OperatorType
  {
    ADD = 0,
    SUB = 1,
    MUL = 2,
    DIV = 3
  };

  //@{
  /**
   * @brief Set/Get the operator to apply. Default is ADD (0).
   */
  vtkSetMacro(Operator, int);
  vtkGetMacro(Operator, int);
  //@}

  //@{
  /**
   * @brief Set/Get the first time step.
   */
  vtkSetMacro(FirstTimeStepIndex, int);
  vtkGetMacro(FirstTimeStepIndex, int);
  //@}

  //@{
  /**
   * @brief Set/Get the second time step.
   */
  vtkSetMacro(SecondTimeStepIndex, int);
  vtkGetMacro(SecondTimeStepIndex, int);
  //@}

  //@{
  /**
   * @brief Set/Get the suffix to be append to the output array name.
   * If not specified, ouput will be suffixed with '_' and the operation
   * type (eg. myarrayname_add).
   */
  vtkSetStringMacro(OutputArrayNameSuffix);
  vtkGetStringMacro(OutputArrayNameSuffix);
  //@}

protected:
  vtkTemporalArrayOperatorFilter();
  virtual ~vtkTemporalArrayOperatorFilter() VTK_OVERRIDE;

  virtual int FillInputPortInformation(int, vtkInformation*) VTK_OVERRIDE;
  virtual int FillOutputPortInformation(int, vtkInformation*) VTK_OVERRIDE;

  virtual int RequestDataObject(vtkInformation*, vtkInformationVector**,
                                vtkInformationVector*) VTK_OVERRIDE;
  virtual int RequestInformation(vtkInformation*, vtkInformationVector**,
                                 vtkInformationVector*) VTK_OVERRIDE;
  virtual int RequestUpdateExtent(vtkInformation*, vtkInformationVector**,
                                  vtkInformationVector*) VTK_OVERRIDE;
  virtual int RequestData(vtkInformation*, vtkInformationVector**,
                          vtkInformationVector*) VTK_OVERRIDE;

  int GetInputArrayAssociation();
  virtual vtkDataObject* Process(vtkDataObject*, vtkDataObject*);
  virtual vtkDataObject* ProcessDataObject(vtkDataObject*, vtkDataObject*);
  virtual vtkDataArray* ProcessDataArray(vtkDataArray*, vtkDataArray*);

  int Operator;
  int FirstTimeStepIndex;
  int SecondTimeStepIndex;
  int NumberTimeSteps;
  char* OutputArrayNameSuffix;

private:
  vtkTemporalArrayOperatorFilter(const vtkTemporalArrayOperatorFilter &) VTK_DELETE_FUNCTION;
  void operator=(const vtkTemporalArrayOperatorFilter &) VTK_DELETE_FUNCTION;
};

#endif
