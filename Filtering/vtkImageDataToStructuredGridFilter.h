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
// .NAME vtkImageDataToStructuredGridFilter.h -- A filter to convert image
//  data to structured grid.
//
// .SECTION Description
// A concrete instance of vtkStructuredGridAlgorithm which provides
// functionality for converting instances of vtkImageData to vtkStructuredGrid.

#ifndef VTKIMAGEDATATOSTRUCTUREDGRIDFILTER_H_
#define VTKIMAGEDATATOSTRUCTUREDGRIDFILTER_H_

#include "vtkStructuredGridAlgorithm.h"

class vtkStructuredGrid;
class vtkImageData;
class vtkInformation;
class vtkInformationVector;

class VTK_FILTERING_EXPORT vtkImageDataToStructuredGridFilter :
                                             public vtkStructuredGridAlgorithm
{
  public:
    static vtkImageDataToStructuredGridFilter* New();
    vtkTypeMacro(vtkImageDataToStructuredGridFilter,vtkStructuredGridAlgorithm);
    void PrintSelf( std::ostream &oss, vtkIndent indent );

  protected:
    vtkImageDataToStructuredGridFilter();
    virtual ~vtkImageDataToStructuredGridFilter();

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
    vtkImageDataToStructuredGridFilter(
        const vtkImageDataToStructuredGridFilter& ); //Not implemented
    void operator=(const vtkImageDataToStructuredGridFilter&);//Not implemented


};

#endif /* VTKIMAGEDATATOSTRUCTUREDGRIDFILTER_H_ */
