/*=========================================================================

 Program:   Visualization Toolkit
 Module:    vtkImageDataToStructuredGridFilter.h

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/
// .NAME vtkImageToStructuredGrid.h -- A filter to convert image data to
// a structured grid instance.
//
// .SECTION Description
// A concrete instance of vtkStructuredGridAlgorithm which provides
// functionality for converting instances of vtkImageData to vtkStructuredGrid.

#ifndef VTKIMAGEDATATOSTRUCTUREDGRIDFILTER_H_
#define VTKIMAGEDATATOSTRUCTUREDGRIDFILTER_H_

#include "vtkCommonExecutionModelModule.h" // For export macro
#include "vtkStructuredGridAlgorithm.h"

class vtkStructuredGrid;
class vtkImageData;
class vtkInformation;
class vtkInformationVector;

class VTKCOMMONEXECUTIONMODEL_EXPORT vtkImageToStructuredGrid:
  public vtkStructuredGridAlgorithm
{
  public:
    static vtkImageToStructuredGrid* New();
    vtkTypeMacro(vtkImageToStructuredGrid,vtkStructuredGridAlgorithm);
    void PrintSelf(ostream &oss, vtkIndent indent );

  protected:
    vtkImageToStructuredGrid();
    virtual ~vtkImageToStructuredGrid();

    virtual int RequestData(
        vtkInformation* request,
        vtkInformationVector** inputVector,
        vtkInformationVector* outputVector );

    // Description:
    // Helper function to copy point/cell data from image to grid
    void CopyPointData( vtkImageData*, vtkStructuredGrid* );
    void CopyCellData( vtkImageData*, vtkStructuredGrid*  );

    virtual int FillInputPortInformation(int, vtkInformation* info);
    virtual int FillOutputPortInformation(int, vtkInformation* info );

  private:
    vtkImageToStructuredGrid(
        const vtkImageToStructuredGrid& ); //Not implemented
    void operator=(const vtkImageToStructuredGrid&);//Not implemented


};

#endif /* VTKIMAGEDATATOSTRUCTUREDGRIDFILTER_H_ */
