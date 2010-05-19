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
// .NAME vtkMergeFilter - extract separate components of data from different datasets
// .SECTION Description
// vtkMergeFilter is a filter that extracts separate components of data from
// different datasets and merges them into a single dataset. The output from
// this filter is of the same type as the input (i.e., vtkDataSet.) It treats 
// both cell and point data set attributes.

#ifndef __vtkMergeFilter_h
#define __vtkMergeFilter_h

#include "vtkDataSetAlgorithm.h"

class vtkFieldList;

class VTK_GRAPHICS_EXPORT vtkMergeFilter : public vtkDataSetAlgorithm
{
public:
  static vtkMergeFilter *New();
  vtkTypeMacro(vtkMergeFilter,vtkDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Specify object from which to extract geometry information.
  // Old style. Use SetGeometryConnection() instead.
  void SetGeometry(vtkDataSet *input) {this->SetInput(input);};
  vtkDataSet *GetGeometry();

  // Description:
  // Specify object from which to extract geometry information.
  // Equivalent to SetInputConnection(0, algOutput)
  void SetGeometryConnection(vtkAlgorithmOutput* algOutput) 
    {
      this->SetInputConnection(algOutput);
    }

  // Description:
  // Specify object from which to extract scalar information.
  // Old style. Use SetScalarsConnection() instead.
  void SetScalars(vtkDataSet *);
  vtkDataSet *GetScalars();

  // Description:
  // Specify object from which to extract scalar information.
  // Equivalent to SetInputConnection(1, algOutput)
  void SetScalarsConnection(vtkAlgorithmOutput* algOutput)
    {
      this->SetInputConnection(1, algOutput);
    }

  // Description:
  // Set / get the object from which to extract vector information.
  // Old style. Use SetVectorsConnection() instead.
  void SetVectors(vtkDataSet *);
  vtkDataSet *GetVectors();

  // Description:
  // Set the connection from which to extract vector information.
  // Equivalent to SetInputConnection(2, algOutput)
  void SetVectorsConnection(vtkAlgorithmOutput* algOutput)
    {
      this->SetInputConnection(2, algOutput);
    }

  // Description:
  // Set / get the object from which to extract normal information.
  // Old style. Use SetNormalsConnection() instead.
  void SetNormals(vtkDataSet *);
  vtkDataSet *GetNormals();

  // Description:
  // Set  the connection from which to extract normal information.
  // Equivalent to SetInputConnection(3, algOutput)
  void SetNormalsConnection(vtkAlgorithmOutput* algOutput)
    {
      this->SetInputConnection(3, algOutput);
    }

  // Description:
  // Set / get the object from which to extract texture coordinates
  // information.
  // Old style. Use SetTCoordsConnection() instead.
  void SetTCoords(vtkDataSet *);
  vtkDataSet *GetTCoords();

  // Description:
  // Set the connection from which to extract texture coordinates
  // information.
  // Equivalent to SetInputConnection(4, algOutput)
  void SetTCoordsConnection(vtkAlgorithmOutput* algOutput)
    {
      this->SetInputConnection(4, algOutput);
    }

  // Description:
  // Set / get the object from which to extract tensor data.
  // Old style. Use SetTensorsConnection() instead.
  void SetTensors(vtkDataSet *);
  vtkDataSet *GetTensors();

  // Description:
  // Set the connection from which to extract tensor data.
  // Equivalent to SetInputConnection(5, algOutput)
  void SetTensorsConnection(vtkAlgorithmOutput* algOutput)
    {
      this->SetInputConnection(5, algOutput);
    }

  // Description:
  // Set the object from which to extract a field and the name
  // of the field. Note that this does not create pipeline
  // connectivity.
  void AddField(const char* name, vtkDataSet* input);

protected:
  vtkMergeFilter();
  ~vtkMergeFilter();

  // Usual data generation method
  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  int RequestUpdateExtent(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  int FillInputPortInformation(int port, vtkInformation *info);

  vtkFieldList* FieldList;
private:
  vtkMergeFilter(const vtkMergeFilter&);  // Not implemented.
  void operator=(const vtkMergeFilter&);  // Not implemented.
  };

#endif


