/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkUnstructuredGridToReebGraphFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkUnstructuredGridToReebGraphFilter
 * @brief   generate a Reeb graph from a
 * scalar field defined on a vtkUnstructuredGrid.
 *
 * The filter will first try to pull as a scalar field the vtkDataArray with
 * Id 'fieldId' of the mesh's vtkPointData.
 * If this field does not exist, the filter will use the vtkElevationFilter to
 * generate a default scalar field.
 */

#ifndef vtkUnstructuredGridToReebGraphFilter_h
#define vtkUnstructuredGridToReebGraphFilter_h

#include "vtkDirectedGraphAlgorithm.h"
#include "vtkFiltersReebGraphModule.h" // For export macro

class vtkReebGraph;

class VTKFILTERSREEBGRAPH_EXPORT vtkUnstructuredGridToReebGraphFilter
  : public vtkDirectedGraphAlgorithm
{
public:
  static vtkUnstructuredGridToReebGraphFilter* New();
  vtkTypeMacro(vtkUnstructuredGridToReebGraphFilter, vtkDirectedGraphAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Set the scalar field id (default = 0).
   */
  vtkSetMacro(FieldId, int);
  vtkGetMacro(FieldId, int);
  //@}

  vtkReebGraph* GetOutput();

protected:
  vtkUnstructuredGridToReebGraphFilter();
  ~vtkUnstructuredGridToReebGraphFilter() override;

  int FieldId;

  int FillInputPortInformation(int portNumber, vtkInformation*) override;
  int FillOutputPortInformation(int, vtkInformation*) override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

private:
  vtkUnstructuredGridToReebGraphFilter(const vtkUnstructuredGridToReebGraphFilter&) = delete;
  void operator=(const vtkUnstructuredGridToReebGraphFilter&) = delete;
};

#endif
