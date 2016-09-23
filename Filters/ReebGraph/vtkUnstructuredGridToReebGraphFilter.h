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

#include "vtkFiltersReebGraphModule.h" // For export macro
#include "vtkDirectedGraphAlgorithm.h"

class vtkReebGraph;

class VTKFILTERSREEBGRAPH_EXPORT vtkUnstructuredGridToReebGraphFilter :
  public vtkDirectedGraphAlgorithm
{
public:
  static vtkUnstructuredGridToReebGraphFilter* New();
  vtkTypeMacro(vtkUnstructuredGridToReebGraphFilter,
    vtkDirectedGraphAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

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
  ~vtkUnstructuredGridToReebGraphFilter();

  int FieldId;

  int FillInputPortInformation(int portNumber, vtkInformation *);
  int FillOutputPortInformation(int, vtkInformation *);

  int RequestData(vtkInformation*,
                  vtkInformationVector**,
                  vtkInformationVector*);

private:
  vtkUnstructuredGridToReebGraphFilter(const vtkUnstructuredGridToReebGraphFilter&) VTK_DELETE_FUNCTION;
  void operator=(const vtkUnstructuredGridToReebGraphFilter&) VTK_DELETE_FUNCTION;
};

#endif
