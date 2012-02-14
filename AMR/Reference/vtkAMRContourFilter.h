/*=========================================================================

 Program:   Visualization Toolkit
 Module:    vtkAMRContourFilter.h

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/
// .NAME vtkAMRContourFilter.h -- Contour for 2D and 3D AMR datasets.
//
// .SECTION Description
// This filter is a concrete instance of vktMultiBlockDataSetAlgorithm which
// accepts as input an AMR dataset, represented as a vtkHierarchicalBoxDataSet,
// and generates a corresponding multiblock dataset of polydata to represent the
// contours.
//
// .SECTION See Also
// vtkHierarchicalBoxDataSet

#ifndef VTKAMRCONTOURFILTER_H_
#define VTKAMRCONTOURFILTER_H_

#include "vtkMultiBlockDataSetAlgorithm.h"

class vtkInformation;
class vtkInformationVector;

class VTK_AMR_EXPORT vtkAMRContourFilter : public vtkMultiBlockDataSetAlgorithm
{
  public:
    static vtkAMRContourFilter *New();
    vtkTypeMacro(vtkAMRContourFilter,vtkMultiBlockDataSetAlgorithm);
    void PrintSelf(std::ostream& os, vtkIndent indent);

  protected:
    vtkAMRContourFilter();
    virtual ~vtkAMRContourFilter();

    virtual int RequestData(
     vtkInformation*,vtkInformationVector**,vtkInformationVector*);
    virtual int FillInputPortInformation(int port, vtkInformation *info);
    virtual int FillOutputPortInformation(int port, vtkInformation *info);

  private:
    vtkAMRContourFilter(const vtkAMRContourFilter&); // Not implemented
    void operator=(const vtkAMRContourFilter&); // Not implemented
};

#endif /* VTKAMRCONTOURFILTER_H_ */
