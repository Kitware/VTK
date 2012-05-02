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
// .NAME vtkAMRToMultiBlockFilter.h -- Converts an AMR instance to multiblock
//
// .SECTION Description
// A filter that accepts as input an AMR dataset and produces a corresponding
// vtkMultiBlockDataset as output.
//
// .SECTION See Also
// vtkOverlappingAMR vtkMultiBlockDataSet

#ifndef VTKAMRTOMULTIBLOCKFILTER_H_
#define VTKAMRTOMULTIBLOCKFILTER_H_

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
  void PrintSelf(ostream &oss, vtkIndent indent );

  // Description:
  // Set/Get a multiprocess controller for paralle processing.
  // By default this parameter is set to NULL by the constructor.
  vtkSetMacro( Controller, vtkMultiProcessController* );
  vtkGetMacro( Controller, vtkMultiProcessController* );

  // Standard pipeline routines

  virtual int FillInputPortInformation(int port, vtkInformation *info);
  virtual int FillOutputPortInformation(int port, vtkInformation *info);
  virtual int RequestData(
      vtkInformation*, vtkInformationVector**, vtkInformationVector* );

protected:
  vtkAMRToMultiBlockFilter();
  virtual ~vtkAMRToMultiBlockFilter();

  // Description:
  // Copies the AMR data to the output multi-block datastructure.
  void CopyAMRToMultiBlock(
      vtkOverlappingAMR *amr, vtkMultiBlockDataSet *mbds);
  vtkMultiProcessController *Controller;

private:
  vtkAMRToMultiBlockFilter(const vtkAMRToMultiBlockFilter& ); // Not implemented
  void operator=(const vtkAMRToMultiBlockFilter& ); // Not implemented
};

#endif /* VTKAMRTOMULTIBLOCKFILTER_H_ */
