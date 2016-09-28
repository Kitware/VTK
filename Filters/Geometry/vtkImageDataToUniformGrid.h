/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageDataToUniformGrid.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkImageDataToUniformGrid
 * @brief   convert vtkImageData to vtkUniformGrid
 *
 * Convert a vtkImageData to vtkUniformGrid and set blanking based on
 * specified by named arrays. By default, values of 0 in the named
 * array will result in the point or cell being blanked. Set Reverse
 * to 1 to indicate that values of 0 will result in the point or
 * cell to not be blanked.
*/

#ifndef vtkImageDataToUniformGrid_h
#define vtkImageDataToUniformGrid_h

#include "vtkFiltersGeometryModule.h" // For export macro
#include "vtkDataObjectAlgorithm.h"

class vtkDataArray;
class vtkFieldData;
class vtkImageData;
class vtkUniformGrid;

class VTKFILTERSGEOMETRY_EXPORT vtkImageDataToUniformGrid
: public vtkDataObjectAlgorithm
{
 public:
  static vtkImageDataToUniformGrid *New();
  vtkTypeMacro(vtkImageDataToUniformGrid,vtkDataObjectAlgorithm);
  void PrintSelf(ostream &os, vtkIndent indent) VTK_OVERRIDE;

  //@{
  /**
   * By default, values of 0 (i.e. Reverse = 0) in the array will
   * result in that point or cell to be blanked. Set Reverse to
   * 1 to make points or cells to not be blanked for array values
   * of 0.
   */
  vtkSetClampMacro(Reverse, int, 0, 1);
  vtkGetMacro(Reverse, int);
  vtkBooleanMacro(Reverse, int);
  //@}

protected:
  vtkImageDataToUniformGrid();
  ~vtkImageDataToUniformGrid() VTK_OVERRIDE;

  int RequestData(vtkInformation *req,
                  vtkInformationVector **inV,
                  vtkInformationVector *outV) VTK_OVERRIDE;
  int RequestDataObject(vtkInformation *req,
                        vtkInformationVector **inV,
                        vtkInformationVector *outV) VTK_OVERRIDE;

  int FillInputPortInformation(int port, vtkInformation* info) VTK_OVERRIDE;
  int FillOutputPortInformation(int port, vtkInformation* info) VTK_OVERRIDE;

  virtual int Process(vtkImageData* input, int association, const char* arrayName,
                      vtkUniformGrid* output);

private:
  vtkImageDataToUniformGrid(const vtkImageDataToUniformGrid&) VTK_DELETE_FUNCTION;
  void operator=(const vtkImageDataToUniformGrid&) VTK_DELETE_FUNCTION;

  int Reverse;
};

#endif
