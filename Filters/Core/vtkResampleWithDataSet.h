/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkResampleWithDataSet.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkResampleWithDataset
 * @brief   sample point and cell data of a dataset on
 * points from another dataset.
 *
 * Similar to vtkCompositeDataProbeFilter, vtkResampleWithDataset takes two
 * inputs - Input and Source, and samples the point and cell values of Source
 * on to the point locations of Input. The output has the same structure as
 * Input but its point data have the resampled values from Source. Unlike
 * vtkCompositeDataProbeFilter, this filter support composite datasets for both
 * Input and Source.
 * @sa
 * vtkCompositeDataProbeFilter vtkResampleToImage
*/

#ifndef vtkResampleWithDataSet_h
#define vtkResampleWithDataSet_h


#include "vtkFiltersCoreModule.h" // For export macro
#include "vtkNew.h" // For vtkCompositeDataProbeFilter member variable
#include "vtkPassInputTypeAlgorithm.h"

class vtkCompositeDataProbeFilter;
class vtkDataSet;

class VTKFILTERSCORE_EXPORT vtkResampleWithDataSet : public vtkPassInputTypeAlgorithm
{
public:
  vtkTypeMacro(vtkResampleWithDataSet, vtkPassInputTypeAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  static vtkResampleWithDataSet *New();

  /**
   * Specify the data set that will be probed at the input points.
   * The Input gives the geometry (the points and cells) for the output,
   * while the Source is probed (interpolated) to generate the scalars,
   * vectors, etc. for the output points based on the point locations.
   */
  void SetSourceData(vtkDataObject *source);

  /**
   * Specify the data set that will be probed at the input points.
   * The Input gives the geometry (the points and cells) for the output,
   * while the Source is probed (interpolated) to generate the scalars,
   * vectors, etc. for the output points based on the point locations.
   */
  void SetSourceConnection(vtkAlgorithmOutput* algOutput);

  //@{
  /**
   * Shallow copy the input cell data arrays to the output.
   * Off by default.
   */
  void SetPassCellArrays(bool arg);
  bool GetPassCellArrays();
  vtkBooleanMacro(PassCellArrays, bool);
  //@}

  //@{
  /**
   * Shallow copy the input point data arrays to the output
   * Off by default.
   */
  void SetPassPointArrays(bool arg);
  bool GetPassPointArrays();
  vtkBooleanMacro(PassPointArrays, bool);
  //@}

  //@{
  /**
   * Set whether to pass the field-data arrays from the Input i.e. the input
   * providing the geometry to the output. On by default.
   */
  void SetPassFieldArrays(bool arg);
  bool GetPassFieldArrays();
  vtkBooleanMacro(PassFieldArrays, bool);
  //@}

  vtkMTimeType GetMTime() VTK_OVERRIDE;

protected:
  vtkResampleWithDataSet();
  ~vtkResampleWithDataSet() VTK_OVERRIDE;

  // Usual data generation method
  int RequestData(vtkInformation *, vtkInformationVector **,
                  vtkInformationVector *) VTK_OVERRIDE;
  //virtual int RequestInformation(vtkInformation *, vtkInformationVector **,
  //                               vtkInformationVector *);
  int RequestUpdateExtent(vtkInformation *, vtkInformationVector **,
                          vtkInformationVector *) VTK_OVERRIDE;
  int FillInputPortInformation(int, vtkInformation *) VTK_OVERRIDE;
  int FillOutputPortInformation(int, vtkInformation *) VTK_OVERRIDE;

  /**
   * Get the name of the valid-points mask array.
   */
  const char* GetMaskArrayName() const;

  /**
   * Mark invalid points and cells of output DataSet as hidden
   */
  void SetBlankPointsAndCells(vtkDataSet *data);

  vtkNew<vtkCompositeDataProbeFilter> Prober;

private:
  vtkResampleWithDataSet(const vtkResampleWithDataSet&) VTK_DELETE_FUNCTION;
  void operator=(const vtkResampleWithDataSet&) VTK_DELETE_FUNCTION;
};

#endif // vtkResampleWithDataSet_h
