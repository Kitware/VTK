/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkIdFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkIdFilter
 * @brief   generate scalars or field data from point and cell ids
 *
 * vtkIdFilter is a filter to that generates scalars or field data
 * using cell and point ids. That is, the point attribute data scalars
 * or field data are generated from the point ids, and the cell
 * attribute data scalars or field data are generated from the the
 * cell ids.
 *
 * Typically this filter is used with vtkLabeledDataMapper (and possibly
 * vtkSelectVisiblePoints) to create labels for points and cells, or labels
 * for the point or cell data scalar values.
*/

#ifndef vtkIdFilter_h
#define vtkIdFilter_h

#include "vtkFiltersCoreModule.h" // For export macro
#include "vtkDataSetAlgorithm.h"

class VTKFILTERSCORE_EXPORT vtkIdFilter : public vtkDataSetAlgorithm
{
public:
  vtkTypeMacro(vtkIdFilter,vtkDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Construct object with PointIds and CellIds on; and ids being generated
   * as scalars.
   */
  static vtkIdFilter *New();

  //@{
  /**
   * Enable/disable the generation of point ids. Default is on.
   */
  vtkSetMacro(PointIds,int);
  vtkGetMacro(PointIds,int);
  vtkBooleanMacro(PointIds,int);
  //@}

  //@{
  /**
   * Enable/disable the generation of point ids. Default is on.
   */
  vtkSetMacro(CellIds,int);
  vtkGetMacro(CellIds,int);
  vtkBooleanMacro(CellIds,int);
  //@}

  //@{
  /**
   * Set/Get the flag which controls whether to generate scalar data
   * or field data. If this flag is off, scalar data is generated.
   * Otherwise, field data is generated. Default is off.
   */
  vtkSetMacro(FieldData,int);
  vtkGetMacro(FieldData,int);
  vtkBooleanMacro(FieldData,int);
  //@}

  //@{
  /**
   * Set/Get the name of the Ids array if generated. By default the Ids
   * are named "vtkIdFilter_Ids", but this can be changed with this function.
   */
  vtkSetStringMacro(IdsArrayName);
  vtkGetStringMacro(IdsArrayName);
  //@}

protected:
  vtkIdFilter();
  ~vtkIdFilter() VTK_OVERRIDE;

  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *) VTK_OVERRIDE;

  int PointIds;
  int CellIds;
  int FieldData;
  char *IdsArrayName;

private:
  vtkIdFilter(const vtkIdFilter&) VTK_DELETE_FUNCTION;
  void operator=(const vtkIdFilter&) VTK_DELETE_FUNCTION;
};

#endif


