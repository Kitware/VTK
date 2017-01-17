/*=========================================================================

 Program:   Visualization Toolkit
 Module:    vtkAMRToMultiBlockFilter.h

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/
/**
 * @class   vtkAMRToMultiBlockFilter
 *
 *
 * A filter that accepts as input an AMR dataset and produces a corresponding
 * vtkMultiBlockDataset as output.
 *
 * @sa
 * vtkOverlappingAMR vtkMultiBlockDataSet
*/

#ifndef vtkAMRToMultiBlockFilter_h
#define vtkAMRToMultiBlockFilter_h

#include "vtkFiltersAMRModule.h" // For export macro
#include "vtkMultiBlockDataSetAlgorithm.h"

class vtkInformation;
class vtkInformationVector;
class vtkIndent;
class vtkMultiProcessController;
class vtkOverlappingAMR;
class vtkMultiBlockDataSet;

class VTKFILTERSAMR_EXPORT vtkAMRToMultiBlockFilter :
  public vtkMultiBlockDataSetAlgorithm
{
public:
  static vtkAMRToMultiBlockFilter* New();
  vtkTypeMacro(vtkAMRToMultiBlockFilter, vtkMultiBlockDataSetAlgorithm );
  void PrintSelf(ostream &oss, vtkIndent indent ) VTK_OVERRIDE;

  //@{
  /**
   * Set/Get a multiprocess controller for paralle processing.
   * By default this parameter is set to NULL by the constructor.
   */
  vtkSetMacro( Controller, vtkMultiProcessController* );
  vtkGetMacro( Controller, vtkMultiProcessController* );
  //@}

  // Standard pipeline routines

  int FillInputPortInformation(int port, vtkInformation *info) VTK_OVERRIDE;
  int FillOutputPortInformation(int port, vtkInformation *info) VTK_OVERRIDE;
  int RequestData(
      vtkInformation*, vtkInformationVector**, vtkInformationVector* ) VTK_OVERRIDE;

protected:
  vtkAMRToMultiBlockFilter();
  ~vtkAMRToMultiBlockFilter() VTK_OVERRIDE;

  //@{
  /**
   * Copies the AMR data to the output multi-block datastructure.
   */
  void CopyAMRToMultiBlock(
      vtkOverlappingAMR *amr, vtkMultiBlockDataSet *mbds);
  vtkMultiProcessController *Controller;
  //@}

private:
  vtkAMRToMultiBlockFilter(const vtkAMRToMultiBlockFilter& ) VTK_DELETE_FUNCTION;
  void operator=(const vtkAMRToMultiBlockFilter& ) VTK_DELETE_FUNCTION;
};

#endif /* vtkAMRToMultiBlockFilter_h */
