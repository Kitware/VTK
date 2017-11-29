/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSubdivisionFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkSubdivisionFilter
 * @brief   base class for subvision filters
 *
 * vtkSubdivisionFilter is an abstract class that defines
 * the protocol for subdivision surface filters.
 *
*/

#ifndef vtkSubdivisionFilter_h
#define vtkSubdivisionFilter_h

#include "vtkFiltersGeneralModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

class vtkCellArray;
class vtkCellData;
class vtkIdList;
class vtkIntArray;
class vtkPoints;
class vtkPointData;

class VTKFILTERSGENERAL_EXPORT vtkSubdivisionFilter : public vtkPolyDataAlgorithm
{
public:
  vtkTypeMacro(vtkSubdivisionFilter,vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Set/get the number of subdivisions.
   * Default is 1.
   */
  vtkSetMacro(NumberOfSubdivisions,int);
  vtkGetMacro(NumberOfSubdivisions,int);
  //@}

  //@{
  /**
   * Set/get CheckForTriangles
   * Should subdivision check that the dataset only contains triangles?
   * Default is On (1).
   */
  vtkSetClampMacro(CheckForTriangles, vtkTypeBool, 0, 1);
  vtkGetMacro(CheckForTriangles, vtkTypeBool);
  vtkBooleanMacro(CheckForTriangles, vtkTypeBool);
  //@}

protected:
  vtkSubdivisionFilter();
  ~vtkSubdivisionFilter() override {}

  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *) override;

  int NumberOfSubdivisions;
  vtkTypeBool CheckForTriangles;
private:
  vtkSubdivisionFilter(const vtkSubdivisionFilter&) = delete;
  void operator=(const vtkSubdivisionFilter&) = delete;
};

#endif
