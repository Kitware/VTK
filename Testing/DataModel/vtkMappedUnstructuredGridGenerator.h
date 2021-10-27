/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMappedUnstructuredGridGenerator.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyrgight notice for more information.

=========================================================================*/

/**
 * @class   vtkMappedUnstructuredGridGenerator
 * @brief   A generator for mapped unstructured grids for testing
 *
 * Provides a static GenerateMappedUnstructuredGrid() method to
 * generate a mapped unstructured grid.
 */

#include "vtkObject.h"
#include "vtkTestingDataModelModule.h" // For export macro

#ifndef vtkMappedUnstructuredGridGenerator_h
#define vtkMappedUnstructuredGridGenerator_h

class vtkUnstructuredGridBase;
class vtkUnstructuredGrid;

class VTKTESTINGDATAMODEL_EXPORT vtkMappedUnstructuredGridGenerator : public vtkObject
{
public:
  /**
   * Standard object factory instantiation method.
   */
  static vtkMappedUnstructuredGridGenerator* New();
  void PrintSelf(ostream& os, vtkIndent indent) override;

  vtkTypeMacro(vtkMappedUnstructuredGridGenerator, vtkObject);

  /**
   * Generate a mapped unstructured grid. The user is responsible
   * for deleting the generated grid after use.
   */
  static void GenerateMappedUnstructuredGrid(vtkUnstructuredGridBase** grid);

  /**
   * Generate an unstructured grid. The user is responsible
   * for deleting the generated grid after use.
   */
  static void GenerateUnstructuredGrid(vtkUnstructuredGrid** grid);

protected:
  vtkMappedUnstructuredGridGenerator() {}
  vtkMappedUnstructuredGridGenerator(const vtkMappedUnstructuredGridGenerator&) = delete;

  void operator=(const vtkMappedUnstructuredGridGenerator&) = delete;
};

#endif
