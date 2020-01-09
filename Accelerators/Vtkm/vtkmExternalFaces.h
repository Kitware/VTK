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

#include "vtkAcceleratorsVTKmModule.h" //required for correct implementation
#include "vtkAlgorithm.h"

class vtkUnstructuredGrid;

class VTKACCELERATORSVTKM_EXPORT vtkmExternalFaces : public vtkAlgorithm
{
public:
  vtkTypeMacro(vtkmExternalFaces, vtkAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  static vtkmExternalFaces* New();

  /**
   * Set the input DataSet
   */
  void SetInputData(vtkUnstructuredGrid* ds);

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
  ~vtkmExternalFaces() override;

  int FillInputPortInformation(int, vtkInformation*) override;
  int FillOutputPortInformation(int, vtkInformation*) override;

  vtkTypeBool ProcessRequest(
    vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  virtual int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*);

  bool CompactPoints;

private:
  vtkmExternalFaces(const vtkmExternalFaces&) = delete;
  void operator=(const vtkmExternalFaces&) = delete;
};

#endif // vtkmExternalFaces_h
// VTK-HeaderTest-Exclude: vtkmExternalFaces.h
