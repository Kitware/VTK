/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTreeMapToPolyData.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/
/**
 * @class   vtkTreeMapToPolyData
 * @brief   converts a tree to a polygonal data representing a tree map
 *
 *
 * This algorithm requires that the vtkTreeMapLayout filter has already applied to the
 * data in order to create the quadruple array (min x, max x, min y, max y) of
 * bounds for each vertex of the tree.
*/

#ifndef vtkTreeMapToPolyData_h
#define vtkTreeMapToPolyData_h

#include "vtkInfovisLayoutModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

class VTKINFOVISLAYOUT_EXPORT vtkTreeMapToPolyData : public vtkPolyDataAlgorithm
{
public:
  static vtkTreeMapToPolyData *New();
  vtkTypeMacro(vtkTreeMapToPolyData,vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  /**
   * The field containing quadruples of the form (min x, max x, min y, max y)
   * representing the bounds of the rectangles for each vertex.
   * This array may be added to the tree using vtkTreeMapLayout.
   */
  virtual void SetRectanglesArrayName(const char* name)
    { this->SetInputArrayToProcess(0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_VERTICES, name); }

  /**
   * The field containing the level of each tree node.
   * This can be added using vtkTreeLevelsFilter before this filter.
   * If this is not present, the filter simply calls tree->GetLevel(v) for
   * each vertex, which will produce the same result, but
   * may not be as efficient.
   */
  virtual void SetLevelArrayName(const char* name)
    { this->SetInputArrayToProcess(1, 0, 0, vtkDataObject::FIELD_ASSOCIATION_VERTICES, name); }

  //@{
  /**
   * The spacing along the z-axis between tree map levels.
   */
  vtkGetMacro(LevelDeltaZ, double);
  vtkSetMacro(LevelDeltaZ, double);
  //@}

  //@{
  /**
   * The spacing along the z-axis between tree map levels.
   */
  vtkGetMacro(AddNormals, bool);
  vtkSetMacro(AddNormals, bool);
  //@}

  int FillInputPortInformation(int port, vtkInformation* info);

protected:
  vtkTreeMapToPolyData();
  ~vtkTreeMapToPolyData();

  double LevelDeltaZ;
  bool AddNormals;

  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
private:
  vtkTreeMapToPolyData(const vtkTreeMapToPolyData&) VTK_DELETE_FUNCTION;
  void operator=(const vtkTreeMapToPolyData&) VTK_DELETE_FUNCTION;
};

#endif
