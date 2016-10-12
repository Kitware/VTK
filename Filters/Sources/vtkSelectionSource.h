/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSelectionSource.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkSelectionSource
 * @brief   Generate selection from given set of ids
 * vtkSelectionSource generates a vtkSelection from a set of
 * (piece id, cell id) pairs. It will only generate the selection values
 * that match UPDATE_PIECE_NUMBER (i.e. piece == UPDATE_PIECE_NUMBER).
*/

#ifndef vtkSelectionSource_h
#define vtkSelectionSource_h

#include "vtkFiltersSourcesModule.h" // For export macro
#include "vtkSelectionAlgorithm.h"

class vtkSelectionSourceInternals;

class VTKFILTERSSOURCES_EXPORT vtkSelectionSource : public vtkSelectionAlgorithm
{
public:
  static vtkSelectionSource *New();
  vtkTypeMacro(vtkSelectionSource,vtkSelectionAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  //@{
  /**
   * Add a (piece, id) to the selection set. The source will generate
   * only the ids for which piece == UPDATE_PIECE_NUMBER.
   * If piece == -1, the id applies to all pieces.
   */
  void AddID(vtkIdType piece, vtkIdType id);
  void AddStringID(vtkIdType piece, const char* id);
  //@}

  /**
   * Add a point in world space to probe at.
   */
  void AddLocation(double x, double y, double z);

  /**
   * Add a value range to threshold within.
   */
  void AddThreshold(double min, double max);

  /**
   * Set a frustum to choose within.
   */
  void SetFrustum(double *vertices);

  /**
   * Add the flat-index/composite index for a block.
   */
  void AddBlock(vtkIdType blockno);

  //@{
  /**
   * Removes all IDs.
   */
  void RemoveAllIDs();
  void RemoveAllStringIDs();
  //@}

  /**
   * Remove all thresholds added with AddThreshold.
   */
  void RemoveAllThresholds();

  /**
   * Remove all locations added with AddLocation.
   */
  void RemoveAllLocations();

  /**
   * Remove all blocks added with AddBlock.
   */
  void RemoveAllBlocks();

  //@{
  /**
   * Set the content type for the generated selection.
   * Possible values are as defined by
   * vtkSelection::SelectionContent.
   */
  vtkSetMacro(ContentType, int);
  vtkGetMacro(ContentType, int);
  //@}

  //@{
  /**
   * Set the field type for the generated selection.
   * Possible values are as defined by
   * vtkSelection::SelectionField.
   */
  vtkSetMacro(FieldType, int);
  vtkGetMacro(FieldType, int);
  //@}

  //@{
  /**
   * When extracting by points, extract the cells that contain the
   * passing points.
   */
  vtkSetMacro(ContainingCells, int);
  vtkGetMacro(ContainingCells, int);
  //@}

  //@{
  /**
   * Determines whether the selection describes what to include or exclude.
   * Default is 0, meaning include.
   */
  vtkSetMacro(Inverse, int);
  vtkGetMacro(Inverse, int);
  //@}

  //@{
  /**
   * Access to the name of the selection's subset description array.
   */
  vtkSetStringMacro(ArrayName);
  vtkGetStringMacro(ArrayName);
  //@}

  //@{
  /**
   * Access to the component number for the array specified by ArrayName.
   * Default is component 0. Use -1 for magnitude.
   */
  vtkSetMacro(ArrayComponent, int);
  vtkGetMacro(ArrayComponent, int);
  //@}

  //@{
  /**
   * If CompositeIndex < 0 then COMPOSITE_INDEX() is not added to the output.
   */
  vtkSetMacro(CompositeIndex, int);
  vtkGetMacro(CompositeIndex, int);
  //@}

  //@{
  /**
   * If HierarchicalLevel or HierarchicalIndex < 0 , then HIERARCHICAL_LEVEL()
   * and HIERARCHICAL_INDEX() keys are not added to the output.
   */
  vtkSetMacro(HierarchicalLevel, int);
  vtkGetMacro(HierarchicalLevel, int);
  vtkSetMacro(HierarchicalIndex, int);
  vtkGetMacro(HierarchicalIndex, int);
  //@}

  //@{
  /**
   * Set/Get the query expression string.
   */
  vtkSetStringMacro(QueryString);
  vtkGetStringMacro(QueryString);
  //@}

protected:
  vtkSelectionSource();
  ~vtkSelectionSource() VTK_OVERRIDE;

  int RequestInformation(vtkInformation* request,
                          vtkInformationVector** inputVector,
                          vtkInformationVector* outputVector) VTK_OVERRIDE;
  int RequestData(vtkInformation* request,
                          vtkInformationVector** inputVector,
                          vtkInformationVector* outputVector) VTK_OVERRIDE;

  vtkSelectionSourceInternals* Internal;

  int ContentType;
  int FieldType;
  int ContainingCells;
  int PreserveTopology;
  int Inverse;
  int CompositeIndex;
  int HierarchicalLevel;
  int HierarchicalIndex;
  char *ArrayName;
  int ArrayComponent;
  char *QueryString;

private:
  vtkSelectionSource(const vtkSelectionSource&) VTK_DELETE_FUNCTION;
  void operator=(const vtkSelectionSource&) VTK_DELETE_FUNCTION;
};

#endif
