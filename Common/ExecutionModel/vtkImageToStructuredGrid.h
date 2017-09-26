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
/**
 * @class   vtkImageToStructuredGrid
 * a structured grid instance.
 *
 *
 * A concrete instance of vtkStructuredGridAlgorithm which provides
 * functionality for converting instances of vtkImageData to vtkStructuredGrid.
*/

#ifndef vtkImageToStructuredGrid_h
#define vtkImageToStructuredGrid_h

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
    void PrintSelf(ostream &oss, vtkIndent indent ) override;

  protected:
    vtkImageToStructuredGrid();
    ~vtkImageToStructuredGrid() override;

    int RequestData(
        vtkInformation* request,
        vtkInformationVector** inputVector,
        vtkInformationVector* outputVector ) override;

    //@{
    /**
     * Helper function to copy point/cell data from image to grid
     */
    void CopyPointData( vtkImageData*, vtkStructuredGrid* );
    void CopyCellData( vtkImageData*, vtkStructuredGrid*  );
    //@}

    int FillInputPortInformation(int, vtkInformation* info) override;
    int FillOutputPortInformation(int, vtkInformation* info ) override;

  private:
    vtkImageToStructuredGrid(
        const vtkImageToStructuredGrid& ) = delete;
    void operator=(const vtkImageToStructuredGrid&) = delete;


};

#endif /* VTKIMAGEDATATOSTRUCTUREDGRIDFILTER_H_ */
