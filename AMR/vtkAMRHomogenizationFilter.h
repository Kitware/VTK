/*=========================================================================

 Program:   Visualization Toolkit
 Module:    vtkAMRHomogenizationFilter.h

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/
// .NAME vtkAMRHomogenizationFilter.h -- Creates a non-overlapping AMR dataset
//
// .SECTION Description
// This filter accepts as input an AMR dataset in vtkHierarchicalBoxDataSet
// instance and generates a corresponding homogenized dataset instance of
// non-overlapping AMR pathces.
//
// .SECTION See Also
// vtkHierarchicalBoxDataSet, vtkAMRBox

#ifndef VTKAMRHOMOGENIZATIONFILTER_H_
#define VTKAMRHOMOGENIZATIONFILTER_H_

//#include "vtkHierarchicalBoxDataSetAlgorithm.h"

#include "vtkMultiBlockDataSetAlgorithm.h"

#include <set>

class vtkInformation;
class vtkInformationVector;
class vtkHierarchicalBoxDataSet;
class vtkUniformGrid;

class VTK_AMR_EXPORT vtkAMRHomogenizationFilter :
  public vtkMultiBlockDataSetAlgorithm
{
  public:
    static vtkAMRHomogenizationFilter* New();
    vtkTypeMacro(vtkAMRHomogenizationFilter,vtkMultiBlockDataSetAlgorithm);
    void PrintSelf( std::ostream &oss, vtkIndent indent);

  protected:
    vtkAMRHomogenizationFilter();
    virtual ~vtkAMRHomogenizationFilter();

    // Description:
    // Given the patch extent and the input grid, the patch
    // corresponding to the given extent is extracted.
    // NOTE: the patch is cell-based.
    vtkUniformGrid* ExtractPatch(
        vtkUniformGrid *ug, int patchExtent[6] );

    // Description:
    // Creates a non-overlapping AMR for the given grid starting at the given
    // cellijk. Upon return, the patch extent holds the extents of the patch to
    // be extracted and cellHistory is updated accordingly.
    void GetPatchExtent(
       vtkUniformGrid *ug, int celldims[3], int cellijk[3], int pExtent[6],
       std::set<vtkIdType> &cellHistory );

    // Description:
    // Given an AMR patch at a given level, a set of subset patches are
    // extracted s.t. they are not overlapping based on the pre-computed
    // visibility.
    void ExtractNonOverlappingPatches(
        vtkUniformGrid *ug, const unsigned int level,
        vtkMultiBlockDataSet *out );

    // Description:
    // Homogenizes the input AMR grids into a new instance.
    void HomogenizeGrids(
      vtkHierarchicalBoxDataSet* in, vtkMultiBlockDataSet* out );

    // Standard pipeline routines
    virtual int RequestData(
        vtkInformation*,vtkInformationVector**,vtkInformationVector*);
    virtual int FillInputPortInformation(int port, vtkInformation *info);
    virtual int FillOutputPortInformation(int port, vtkInformation *info);

  private:
    vtkAMRHomogenizationFilter( const vtkAMRHomogenizationFilter& ); // Not implemented
    void operator=( const vtkAMRHomogenizationFilter& ); // Not implemented
};

#endif /* VTKAMRHOMOGENIZATIONFILTER_H_ */
