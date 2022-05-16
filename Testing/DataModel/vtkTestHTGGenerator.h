/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTestHTGGenerator.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyrgight notice for more information.

=========================================================================*/

/**
 * @class vtkTestHTGGenerator
 * @brief Helper class for generating a curated set of HyperTree Grids (HTGs) for testing purposes
 *
 * Provides a set of public methods for generating some commonly used HTG setups.
 */

#include "vtkObject.h"
#include "vtkTestingDataModelModule.h" //for export macro

#ifndef vtkTestHTGGenerator_h
#define vtkTestHTGGenerator_h

class vtkHyperTreeGrid;
class vtkHyperTreeGridNonOrientedCursor;
class vtkDoubleArray;

class VTKTESTINGDATAMODEL_EXPORT vtkTestHTGGenerator : public vtkObject
{
public:
  /**
   * Standard object factory setup
   */
  vtkTypeMacro(vtkTestHTGGenerator, vtkObject);
  void PrintSelf(std::ostream& os, vtkIndent indent) override;
  static vtkTestHTGGenerator* New();

  /**
   * Getter method for the HTG variable
   */
  vtkHyperTreeGrid* GetHTG() const { return this->HTG; };

  /**
   * Helper methods for generating HTGs
   */
  template <int dim, int factor, int depth>
  void generateUnbalanced(
    const std::array<double, 2 * dim>& extent, const std::array<int, dim>& subdivisions);

  template <int dim, int factor, int depth>
  void generateBalanced(
    const std::array<double, 2 * dim>& extent, const std::array<int, dim>& subdivisions);

  /**
   * Specializations
   */
  void generateUnbalanced3DepthQuadTree2x3();

  void generateBalanced3DepthQuadTree2x3();

  void generateUnbalanced2Depth3BranchTree3x3();

  void generateBalanced4Depth3BranchTree2x2();

  void generateUnbalanced3DepthOctTree3x2x3();

  void generateBalanced2Depth3BranchTree3x3x2();

  void clear() { this->HTG = nullptr; };

protected:
  /**
   * Contructor setup
   */
  vtkTestHTGGenerator() { this->HTG = nullptr; };
  ~vtkTestHTGGenerator() { this->clear(); };
  vtkTestHTGGenerator(const vtkTestHTGGenerator&) = delete;

  void operator=(const vtkTestHTGGenerator&) = delete;

  template <int dim, int factor>
  void preprocess(
    const std::array<double, 2 * dim>& extent, const std::array<int, dim>& subdivisions);

  void recurseBalanced(
    vtkHyperTreeGridNonOrientedCursor* cursor, vtkDoubleArray* levels, const int maxDepth);

  /**
   * Internal HTG
   */
  vtkHyperTreeGrid* HTG;

}; // vtkTestHTGGenerator

#endif // vtkTestHTGGenerator_h
