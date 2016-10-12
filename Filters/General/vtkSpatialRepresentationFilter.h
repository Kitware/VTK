/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSpatialRepresentationFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkSpatialRepresentationFilter
 * @brief   generate polygonal model of spatial search object (i.e., a vtkLocator)
 *
 * vtkSpatialRepresentationFilter generates an polygonal representation of a
 * spatial search (vtkLocator) object. The representation varies depending
 * upon the nature of the spatial search object. For example, the
 * representation for vtkOBBTree is a collection of oriented bounding
 * boxes. This input to this filter is a dataset of any type, and the output
 * is polygonal data. You must also specify the spatial search object to
 * use.
 *
 * Generally spatial search objects are used for collision detection and
 * other geometric operations, but in this filter one or more levels of
 * spatial searchers can be generated to form a geometric approximation to
 * the input data. This is a form of data simplification, generally used to
 * accelerate the rendering process. Or, this filter can be used as a
 * debugging/ visualization aid for spatial search objects.
 *
 * This filter can generate one or more  vtkPolyData blocks corresponding to
 * different levels in the spatial search tree. The block ids range from 0
 * (root level) to MaximumLevel. Note that the block for level "id" is not computed
 * unless a AddLevel(id) method is issued. Thus, if you desire three levels of output
 * (say 2,4,7), you would have to invoke AddLevel(2), AddLevel(4), and
 * AddLevel(7). If GenerateLeaves is set to true (off by default), all leaf nodes
 * of the locator (which may be at different levels) are computed and stored in
 * block with id MaximumLevel + 1.
 *
 * @sa
 * vtkLocator vtkPointLocator vtkCellLocator vtkOBBTree
*/

#ifndef vtkSpatialRepresentationFilter_h
#define vtkSpatialRepresentationFilter_h

#include "vtkFiltersGeneralModule.h" // For export macro
#include "vtkMultiBlockDataSetAlgorithm.h"

class vtkLocator;
class vtkDataSet;
class vtkSpatialRepresentationFilterInternal;

class VTKFILTERSGENERAL_EXPORT vtkSpatialRepresentationFilter : public vtkMultiBlockDataSetAlgorithm
{
public:
  static vtkSpatialRepresentationFilter *New();
  vtkTypeMacro(vtkSpatialRepresentationFilter,vtkMultiBlockDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  //@{
  /**
   * Set/Get the locator that will be used to generate the representation.
   */
  virtual void SetSpatialRepresentation(vtkLocator*);
  vtkGetObjectMacro(SpatialRepresentation,vtkLocator);
  //@}

  //@{
  /**
   * Get the maximum level that is available. Populated during
   * RequestData().
   */
  vtkGetMacro(MaximumLevel,int);
  //@}

  /**
   * Add a level to be computed.
   */
  void AddLevel(int level);

  /**
   * Remove all levels.
   */
  void ResetLevels();

  //@{
  /**
   * Turn on/off the generation of leaf nodes. Off by default.
   */
  vtkSetMacro(GenerateLeaves, bool);
  vtkGetMacro(GenerateLeaves, bool);
  vtkBooleanMacro(GenerateLeaves, bool);
  //@}

protected:
  vtkSpatialRepresentationFilter();
  ~vtkSpatialRepresentationFilter() VTK_OVERRIDE;

  int RequestData(vtkInformation*,
                  vtkInformationVector**,
                  vtkInformationVector*) VTK_OVERRIDE;

  int MaximumLevel;
  bool GenerateLeaves;

  vtkLocator *SpatialRepresentation;

  void ReportReferences(vtkGarbageCollector*) VTK_OVERRIDE;
  int FillInputPortInformation(int, vtkInformation*) VTK_OVERRIDE;
private:
  vtkSpatialRepresentationFilter(const vtkSpatialRepresentationFilter&) VTK_DELETE_FUNCTION;
  void operator=(const vtkSpatialRepresentationFilter&) VTK_DELETE_FUNCTION;

  vtkSpatialRepresentationFilterInternal* Internal;
};

#endif
