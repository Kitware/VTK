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
 * @class   vtkmProbe
 * @brief   Sample data at specified point locations
 *
 * vtkmProbe is a filter that computes point attributes(e.g., scalars, vectors,
 * etc.) at specific point positions using the probe filter in VTK-m. The
 * filter has two inputs: the Input and Source.
 * The Input geometric structure is passed through the filter. The point
 * attributes are computed at the Input point positions by interpolating into
 * the source data. For example, we can compute data values on a plane(plane
 * specified as Input from a volume(Source). The source geometry must have cellSet
 * defined otherwise the vtkm filter won't work. The cell data of the source data
 * is copied to the output based on in which source cell each input point is. If
 * an array of the same name exists both in source's point and cell data, only
 * the one from the point data is probed. The valid point result is stored as
 * a field array whose default name is "vtkValidPointMask" in the point data and
 * the valid cell result(Invalid cells are the cells with at least one invalid
 * point) is stored as a field array whose default name is "vtkValidCellMask" in
 * the cell data.
 *
 * This filter can be used to resample data, or convert one dataset form into
 * another. For example, an unstructured grid (vtkUnstructuredGrid) can be
 * probed with a volume (three-dimensional vtkImageData), and then volume
 * rendering techniques can be used to visualize the results. Another example:
 * a line or curve can be used to probe data to produce x-y plots along
 * that line or curve.
 */

#ifndef vtkmProbe_h
#define vtkmProbe_h

#include <string> // for std::string

#include "vtkAcceleratorsVTKmModule.h" //required for export
#include "vtkDataSetAlgorithm.h"

class VTKACCELERATORSVTKM_EXPORT vtkmProbe : public vtkDataSetAlgorithm
{
public:
  vtkTypeMacro(vtkmProbe, vtkDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  static vtkmProbe* New();

  //@{
  /**
   * Specify the data set that will be probed at the input points.
   * The Input gives the geometry (the points and cells) for the output,
   * while the Source is probed (interpolated) to generate the scalars,
   * vectors, etc. for the output points based on the point locations.
   */
  void SetSourceData(vtkDataObject* source);
  vtkDataObject* GetSource();
  //@}

  //@}
  /**
   * Specify the data set that will be probed at the input points.
   * The Input gives the geometry (the points and cells) for the output,
   * while the Source is probed (interpolated) to generate the scalars,
   * vectors, etc. for the output points based on the point locations.
   */
  void SetSourceConnection(vtkAlgorithmOutput* algOutput);
  //@}

  //@{
  /**
   * Shallow copy the input cell data arrays to the output.
   * Off by default.
   */
  vtkSetMacro(PassCellArrays, vtkTypeBool);
  vtkBooleanMacro(PassCellArrays, vtkTypeBool);
  vtkGetMacro(PassCellArrays, vtkTypeBool);
  //@}
  //@{
  /**
   * Shallow copy the input point data arrays to the output.
   * Off by default.
   */
  vtkSetMacro(PassPointArrays, vtkTypeBool);
  vtkBooleanMacro(PassPointArrays, vtkTypeBool);
  vtkGetMacro(PassPointArrays, vtkTypeBool);
  //@}

  //@{
  /**
   * Set whether to pass the field-data arrays from the Input i.e. the input
   * providing the geometry to the output. On by default.
   */
  vtkSetMacro(PassFieldArrays, vtkTypeBool);
  vtkBooleanMacro(PassFieldArrays, vtkTypeBool);
  vtkGetMacro(PassFieldArrays, vtkTypeBool);
  //@}

  //@{
  /**
   * Returns the name of the valid point array added to the output with values 2 for
   * hidden points and 0 for valid points.
   * Set to "vtkValidPointMask" by default.
   */
  vtkSetMacro(ValidPointMaskArrayName, std::string);
  vtkGetMacro(ValidPointMaskArrayName, std::string);
  //@}

  //@{
  /**
   * Returns the name of the valid cell array added to the output with values 2 for
   * hidden points and 0 for valid points.
   * Set to "vtkValidCellMask" by default.
   */
  vtkSetMacro(ValidCellMaskArrayName, std::string);
  vtkGetMacro(ValidCellMaskArrayName, std::string);
  //@}

protected:
  vtkmProbe();
  ~vtkmProbe() override = default;

  vtkTypeBool PassCellArrays;
  vtkTypeBool PassPointArrays;
  vtkTypeBool PassFieldArrays;
  std::string ValidPointMaskArrayName;
  std::string ValidCellMaskArrayName;

  virtual int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  virtual int RequestUpdateExtent(
    vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  virtual int RequestInformation(
    vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  /**
   * Call at the end of RequestData() to pass attribute dat a respecting the
   * PassCellArrays, PassPointArrays and PassFieldArrays flag
   */
  void PassAttributeData(vtkDataSet* input, vtkDataObject* source, vtkDataSet* output);

private:
  vtkmProbe(const vtkmProbe&) = delete;
  void operator=(const vtkmProbe&) = delete;
};

#endif // vtkmProbe_h
// VTK-HeaderTest-Exclude: vtkmProbe.h
