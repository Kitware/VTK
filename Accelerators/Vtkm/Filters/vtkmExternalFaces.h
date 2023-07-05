// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) Kitware, Inc.
// SPDX-FileCopyrightText: Copyright 2012 Sandia Corporation.
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov
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

#include "vtkAcceleratorsVTKmFiltersModule.h" //required for correct implementation
#include "vtkAlgorithm.h"
#include "vtkmlib/vtkmInitializer.h" // Need for initializing vtk-m

VTK_ABI_NAMESPACE_BEGIN
class vtkUnstructuredGrid;

class VTKACCELERATORSVTKMFILTERS_EXPORT vtkmExternalFaces : public vtkAlgorithm
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

  ///@{
  /**
   * Get/Set if the points from the input that are unused in the output should
   * be removed. This will take extra time but the result dataset may use
   * less memory. Off by default.
   */
  vtkSetMacro(CompactPoints, bool);
  vtkGetMacro(CompactPoints, bool);
  vtkBooleanMacro(CompactPoints, bool);
  ///@}

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
  vtkmInitializer Initializer;
};

VTK_ABI_NAMESPACE_END
#endif // vtkmExternalFaces_h
