/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGDALVectorReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkGDALVectorReader - Read vector file formats using GDAL.
// .SECTION Description
// vtkGDALVectorReader is a source object that reads vector files and uses
// GDAL as the underlying library for the task. GDAL is required for this
// reader. The output of the reader is a vtkMultiBlockDataSet
//
// This filter uses the ActiveLayer member to only load entries from the
// specified layer (when ActiveLayer >= 0).
//
// .SECTION See Also
// vtkMultiBlockDataSet

#ifndef vtkGDALVectorReader_h
#define vtkGDALVectorReader_h

#include "vtkMultiBlockDataSetAlgorithm.h"
#include "vtkIOGDALModule.h" // For export macro

#include <map> // STL required.

class VTKIOGDAL_EXPORT vtkGDALVectorReader : public vtkMultiBlockDataSetAlgorithm
{
public:
  static vtkGDALVectorReader* New();
  virtual void PrintSelf( ostream& os, vtkIndent indent );
  vtkTypeMacro(vtkGDALVectorReader,vtkMultiBlockDataSetAlgorithm);

  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);

  // Description:
  // Return number of layers.
  int GetNumberOfLayers();

  // Description:
  // Given a index return layer type (eg point, line, polygon).
  int GetLayerType(int layerIndex=0);

  // Description:
  // Given a layer index return number of features (shapes).
  int GetFeatureCount(int layerIndex=0);

  // Description:
  // Return the active layer type (eg point, line, polygon).
  int GetActiveLayerType();

  // Description:
  // Return the number of features in the active layer (shapes).
  int GetActiveLayerFeatureCount();

  // Description:
  // Set and Get the active layer.
  // If ActiveLayer is less than 0 (the default is -1), then all
  // layers are read. Otherwise, only the specified layer is read.
  vtkSetMacro(ActiveLayer,int);
  vtkGetMacro(ActiveLayer,int);

  // Description:
  // Set and Get whether features are appended to a single
  // vtkPolyData. Turning the option on is useful when a shapefile has
  // a number of features which could otherwise lead to a huge
  // multiblock structure.
  vtkSetMacro(AppendFeatures, int);
  vtkGetMacro(AppendFeatures, int);
  vtkBooleanMacro(AppendFeatures, int);

  //BTX
  // Description:
  // Return projection string belong to each layer.
  std::map<int, std::string> GetLayersProjection();
  //ETX

  // Description:
  // Return projection string belong to a layer.
  const char* GetLayerProjection(int layerIndex);

  // Description:
  // Set/get whether feature IDs should be generated.
  // Some GDAL primitives (e.g., a polygon with a hole
  // in its interior) are represented by multiple VTK
  // cells. If you wish to identify the primitive
  // responsible for a VTK cell, turn this on. It is
  // off by default for backwards compatibility.
  // The array of feature IDs will be the active
  // cell-data pedigree IDs.
  vtkSetMacro(AddFeatureIds,int);
  vtkGetMacro(AddFeatureIds,int);
  vtkBooleanMacro(AddFeatureIds,int);

protected:
  vtkGDALVectorReader();
  virtual ~vtkGDALVectorReader();

  int RequestInformation( vtkInformation*, vtkInformationVector**, vtkInformationVector* );
  int RequestData( vtkInformation*, vtkInformationVector**, vtkInformationVector* );

  int InitializeInternal();

  /// The name of the file that will be opened on the next call to RequestData()
  char* FileName;

  int ActiveLayer;
  int AppendFeatures;
  int AddFeatureIds;

  //BTX
  class Internal;

  /// Private per-file metadata
  vtkGDALVectorReader::Internal* Implementation;

  /// Global variable indicating whether the OGR library has been registered yet or not.
  static int OGRRegistered;

  /// Mapping of layer to projection.
  std::map<int, std::string> LayersProjection;
  //ETX

private:
  vtkGDALVectorReader(const vtkGDALVectorReader&);  // Not implemented.
  void operator=(const vtkGDALVectorReader&);       // Not implemented.
};

#endif // vtkGDALVectorReader_h
