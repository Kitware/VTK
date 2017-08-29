//=============================================================================
//
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2012 Sandia Corporation.
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//=============================================================================
/**
 * @class   vtkmExternalFaces
 * @brief   generate External Faces of a DataSet
 *
 * vtkmExternalFaces is a filter that extracts all external faces from a
 * data set. An external face is defined is defined as a face/side of a cell
 * that belongs only to one cell in the entire mesh.
 * @warning
 * This filter is currently only supports propagation of point properties
 *
*/

#ifndef vtkmExternalFaces_h
#define vtkmExternalFaces_h

#include "vtkAlgorithm.h"
#include "vtkAcceleratorsVTKmModule.h" //required for correct implementation

class vtkUnstructuredGrid;

class VTKACCELERATORSVTKM_EXPORT vtkmExternalFaces : public vtkAlgorithm
{
public:
  vtkTypeMacro(vtkmExternalFaces, vtkAlgorithm)
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;
  static vtkmExternalFaces* New();

  /**
   * Set the input DataSet
   */
  void SetInputData(vtkUnstructuredGrid *ds);

  /**
   * Get the resulr DataSet
   */
  vtkUnstructuredGrid* GetOutput();

  //@{
  /**
   * Get/Set if the points from the input that are unused in the output should
   * be removed. This will take extra time but the result dataset may use
   * less memory. Off by default.
   */
  vtkSetMacro(CompactPoints, bool);
  vtkGetMacro(CompactPoints, bool);
  vtkBooleanMacro(CompactPoints, bool);
  //@}

protected:
  vtkmExternalFaces();
  ~vtkmExternalFaces();

  int FillInputPortInformation(int, vtkInformation *) VTK_OVERRIDE;
  int FillOutputPortInformation(int, vtkInformation *) VTK_OVERRIDE;

  int ProcessRequest(vtkInformation*, vtkInformationVector**,
                     vtkInformationVector*) VTK_OVERRIDE;
  virtual int RequestData(vtkInformation *, vtkInformationVector **,
                          vtkInformationVector *);

  bool CompactPoints;

private:
  vtkmExternalFaces(const vtkmExternalFaces&) VTK_DELETE_FUNCTION;
  void operator=(const vtkmExternalFaces&) VTK_DELETE_FUNCTION;
};

#endif // vtkmExternalFaces_h
// VTK-HeaderTest-Exclude: vtkmExternalFaces.h
