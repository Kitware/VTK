/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkTransferAttributes.h

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
 * @class   vtkTransferAttributes
 * @brief   transfer data from a graph representation
 * to a tree representation using direct mapping or pedigree ids.
 *
 *
 * The filter requires
 * both a vtkGraph and vtkTree as input.  The tree vertices must be a
 * superset of the graph vertices.  A common example is when the graph vertices
 * correspond to the leaves of the tree, but the internal vertices of the tree
 * represent groupings of graph vertices.  The algorithm matches the vertices
 * using the array "PedigreeId".  The user may alternately set the
 * DirectMapping flag to indicate that the two structures must have directly
 * corresponding offsets (i.e. node i in the graph must correspond to node i in
 * the tree).
 *
*/

#ifndef vtkTransferAttributes_h
#define vtkTransferAttributes_h

#include "vtkInfovisCoreModule.h" // For export macro
#include "vtkPassInputTypeAlgorithm.h"
#include "vtkVariant.h" //For vtkVariant method arguments

class VTKINFOVISCORE_EXPORT vtkTransferAttributes : public vtkPassInputTypeAlgorithm
{
public:
  /**
   * Create a vtkTransferAttributes object.
   * Initial values are DirectMapping = false, DefaultValue = 1,
   * SourceArrayName=0, TargetArrayName = 0,
   * SourceFieldType=vtkDataObject::FIELD_ASSOCIATION_POINTS,
   * TargetFieldType=vtkDataObject::FIELD_ASSOCIATION_POINTS
   */
  static vtkTransferAttributes *New();

  vtkTypeMacro(vtkTransferAttributes,vtkPassInputTypeAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * If on, uses direct mapping from tree to graph vertices.
   * If off, both the graph and tree must contain PedigreeId arrays
   * which are used to match graph and tree vertices.
   * Default is off.
   */
  vtkSetMacro(DirectMapping, bool);
  vtkGetMacro(DirectMapping, bool);
  vtkBooleanMacro(DirectMapping, bool);
  //@}

  //@{
  /**
   * The field name to use for storing the source array.
   */
  vtkGetStringMacro(SourceArrayName);
  vtkSetStringMacro(SourceArrayName);
  //@}

  //@{
  /**
   * The field name to use for storing the source array.
   */
  vtkGetStringMacro(TargetArrayName);
  vtkSetStringMacro(TargetArrayName);
  //@}

  //@{
  /**
   * The source field type for accessing the source array. Valid values are
   * those from enum vtkDataObject::FieldAssociations.
   */
  vtkGetMacro(SourceFieldType, int);
  vtkSetMacro(SourceFieldType, int);
  //@}

  //@{
  /**
   * The target field type for accessing the target array. Valid values are
   * those from enum vtkDataObject::FieldAssociations.
   */
  vtkGetMacro(TargetFieldType, int);
  vtkSetMacro(TargetFieldType, int);
  //@}

  //@{
  /**
   * Method to get/set the default value.
   */
  vtkVariant GetDefaultValue();
  void SetDefaultValue(vtkVariant value);
  //@}

  /**
   * Set the input type of the algorithm to vtkGraph.
   */
  int FillInputPortInformation(int port, vtkInformation* info) override;

protected:
  vtkTransferAttributes();
  ~vtkTransferAttributes() override;

  bool DirectMapping;
  char* SourceArrayName;
  char* TargetArrayName;
  int SourceFieldType;
  int TargetFieldType;

  vtkVariant DefaultValue;

  /**
   * Convert the vtkGraph into vtkPolyData.
   */
  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *) override;

private:
  vtkTransferAttributes(const vtkTransferAttributes&) = delete;
  void operator=(const vtkTransferAttributes&) = delete;
};

#endif
