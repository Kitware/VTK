/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMergeFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkMergeFilter
 * @brief   extract separate components of data from different datasets
 *
 * vtkMergeFilter is a filter that extracts separate components of data from
 * different datasets and merges them into a single dataset. The output from
 * this filter is of the same type as the input (i.e., vtkDataSet.) It treats
 * both cell and point data set attributes.
*/

#ifndef vtkMergeFilter_h
#define vtkMergeFilter_h

#include "vtkFiltersCoreModule.h" // For export macro
#include "vtkDataSetAlgorithm.h"

class vtkFieldList;

class VTKFILTERSCORE_EXPORT vtkMergeFilter : public vtkDataSetAlgorithm
{
public:
  static vtkMergeFilter *New();
  vtkTypeMacro(vtkMergeFilter,vtkDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Specify object from which to extract geometry information.
   * Note that this method does not connect the pipeline. The algorithm will
   * work on the input data as it is without updating the producer of the data.
   * See SetGeometryConnection for connecting the pipeline.
   */
  void SetGeometryInputData(vtkDataSet *input) {this->SetInputData(input);};
  vtkDataSet *GetGeometry();
  //@}

  /**
   * Specify object from which to extract geometry information.
   * Equivalent to SetInputConnection(0, algOutput)
   */
  void SetGeometryConnection(vtkAlgorithmOutput* algOutput)
  {
      this->SetInputConnection(algOutput);
  }

  //@{
  /**
   * Specify object from which to extract scalar information.
   * Note that this method does not connect the pipeline. The algorithm will
   * work on the input data as it is without updating the producer of the data.
   * See SetScalarConnection for connecting the pipeline.
   */
  void SetScalarsData(vtkDataSet *);
  vtkDataSet *GetScalars();
  //@}

  /**
   * Specify object from which to extract scalar information.
   * Equivalent to SetInputConnection(1, algOutput)
   */
  void SetScalarsConnection(vtkAlgorithmOutput* algOutput)
  {
      this->SetInputConnection(1, algOutput);
  }

  //@{
  /**
   * Set / get the object from which to extract vector information.
   * Note that this method does not connect the pipeline. The algorithm will
   * work on the input data as it is without updating the producer of the data.
   * See SetVectorsConnection for connecting the pipeline.
   */
  void SetVectorsData(vtkDataSet *);
  vtkDataSet *GetVectors();
  //@}

  /**
   * Set the connection from which to extract vector information.
   * Equivalent to SetInputConnection(2, algOutput)
   */
  void SetVectorsConnection(vtkAlgorithmOutput* algOutput)
  {
      this->SetInputConnection(2, algOutput);
  }

  //@{
  /**
   * Set / get the object from which to extract normal information.
   * Note that this method does not connect the pipeline. The algorithm will
   * work on the input data as it is without updating the producer of the data.
   * See SetNormalsConnection for connecting the pipeline.
   */
  void SetNormalsData(vtkDataSet *);
  vtkDataSet *GetNormals();
  //@}

  /**
   * Set  the connection from which to extract normal information.
   * Equivalent to SetInputConnection(3, algOutput)
   */
  void SetNormalsConnection(vtkAlgorithmOutput* algOutput)
  {
      this->SetInputConnection(3, algOutput);
  }

  //@{
  /**
   * Set / get the object from which to extract texture coordinates
   * information.
   * Note that this method does not connect the pipeline. The algorithm will
   * work on the input data as it is without updating the producer of the data.
   * See SetTCoordsConnection for connecting the pipeline.
   */
  void SetTCoordsData(vtkDataSet *);
  vtkDataSet *GetTCoords();
  //@}

  /**
   * Set the connection from which to extract texture coordinates
   * information.
   * Equivalent to SetInputConnection(4, algOutput)
   */
  void SetTCoordsConnection(vtkAlgorithmOutput* algOutput)
  {
      this->SetInputConnection(4, algOutput);
  }

  //@{
  /**
   * Set / get the object from which to extract tensor data.
   * Note that this method does not connect the pipeline. The algorithm will
   * work on the input data as it is without updating the producer of the data.
   * See SetTensorsConnection for connecting the pipeline.
   */
  void SetTensorsData(vtkDataSet *);
  vtkDataSet *GetTensors();
  //@}

  /**
   * Set the connection from which to extract tensor data.
   * Equivalent to SetInputConnection(5, algOutput)
   */
  void SetTensorsConnection(vtkAlgorithmOutput* algOutput)
  {
      this->SetInputConnection(5, algOutput);
  }

  /**
   * Set the object from which to extract a field and the name
   * of the field. Note that this does not create pipeline
   * connectivity.
   */
  void AddField(const char* name, vtkDataSet* input);

protected:
  vtkMergeFilter();
  ~vtkMergeFilter() override;

  // Usual data generation method
  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *) override;
  int RequestUpdateExtent(vtkInformation *, vtkInformationVector **, vtkInformationVector *) override;
  int FillInputPortInformation(int port, vtkInformation *info) override;

  vtkFieldList* FieldList;
private:
  vtkMergeFilter(const vtkMergeFilter&) = delete;
  void operator=(const vtkMergeFilter&) = delete;
};

#endif


