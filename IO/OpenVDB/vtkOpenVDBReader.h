/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenVDBReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkOpenVDBReader
 * @brief   OpenVDB reader
 * Reader for OpenVDB files. An OpenVDB file is a collection of grids. There are two types of
 * grids: image volumes, and point clouds. The reader returns a vtkPartitionedDataSetCollection
 * which reflects the grids of the file. An image volume will give a vtkImageData block, and
 * a point cloud will give a vtkPolyData (with only vertices).
 * It is also possible to merge all image volumes into a single vtkImageData, and independently
 * merge all point clouds into a single vtkPolyData (cf vtkOpenVDBReader::SetMergeImageVolumes
 * and vtkOpenVDBReader::SetMergePointSets).
 */

#ifndef vtkOpenVDBReader_h
#define vtkOpenVDBReader_h

#include "vtkDataArraySelection.h" // needed for vtkDataArraySelection
#include "vtkIOOpenVDBModule.h"    //needed for exports
#include "vtkPartitionedDataSetCollectionAlgorithm.h"
#include "vtkSmartPointer.h" // needed for smart pointers

class vtkOpenVDBReaderInternals;

class VTKIOOPENVDB_EXPORT vtkOpenVDBReader : public vtkPartitionedDataSetCollectionAlgorithm
{
public:
  static vtkOpenVDBReader* New();
  vtkTypeMacro(vtkOpenVDBReader, vtkPartitionedDataSetCollectionAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * @brief Returns whether a file can be read by the reader or not.
   * The parameter is the path to the file.
   */
  bool CanReadFile(VTK_FILEPATH const char*);

  /**
   * Get the file extensions for this format.
   * Returns a string with a space separated list of extensions in
   * the format .extension
   */
  const char* GetFileExtensions();

  ///@{
  /**
   * Set/get the file name to be opened by the reader.
   */
  vtkSetFilePathMacro(FileName);
  vtkGetFilePathMacro(FileName);
  ///@}

  /**
   * Return a descriptive name for the file format that might be useful in a GUI.
   */
  const char* GetDescriptiveName();

  ///@{
  /**
   * Get/Set the downsampling factor used to convert
   * VDB data to image data. Should be between 0.05 (very downsampled)
   * and 1.0 (not downsampled).
   * It is only considered for image volume grids.
   * Default value is 1.0.
   */
  vtkSetClampMacro(DownsamplingFactor, float, 0.01, 1.0);
  vtkGetMacro(DownsamplingFactor, float);
  ///@}

  ///@{
  /**
   * When enabled, the reader will merge all the requested image grids
   * into a single vtkImageData. This vtkImageData will have several arrays,
   * one for each requested image volume grid.
   * When disabled, there will be one vtkImageData per requested image grid,
   * each containing one data array.
   * It is disabled by default.
   */
  vtkSetMacro(MergeImageVolumes, bool);
  vtkGetMacro(MergeImageVolumes, bool);
  vtkBooleanMacro(MergeImageVolumes, bool);
  ///@}

  ///@{
  /**
   * When enabled, the reader will all the requested points cloud grids into a
   * single vtkPolyData. This vtkPolyData will contain the points of every merged
   * grid.
   * When disabled, there will be one vtkPolyData per requested points cloud grid.
   * It is disabled by default.
   */
  vtkGetMacro(MergePointSets, bool);
  vtkSetMacro(MergePointSets, bool);
  vtkBooleanMacro(MergePointSets, bool);
  ///@}

  ///@{
  /**
   * Standard interface to a vtkDataArraySelection object,
   * allowing the user to choose the grids they want.
   * They can get the number of available grids,
   * get the descriptive name of a grid, and get/set the status
   * (selected or not) of a grid.
   */
  vtkGetObjectMacro(GridSelection, vtkDataArraySelection);
  int GetNumberOfGridsSelectionArrays();
  const char* GetGridsSelectionArrayName(int index);
  int GetGridsSelectionArrayStatus(const char* name);
  void SetGridsSelectionArrayStatus(const char* name, int status);
  ///@}

  /**
   * Get the name of the grid with the given index in
   * the input.
   */
  const char* GetGridArrayName(int index);

  /**
   * Get the VTK object type corresponding to a given grid index.
   * This is VTK_POLY_DATA for a point cloud, VTK_IMAGE_DATA for an image volume, VTK_DATA_OBJECT
   * when unsupported and -1 when out of range.
   */
  int GetGridArrayType(int index);

  /**
   * Get the total number of grids available in the file
   */
  int NumberOfGrids();

protected:
  vtkOpenVDBReader();
  ~vtkOpenVDBReader() override = default;

  bool LoadFile();
  void InitializeData();

  float DownsamplingFactor = 1.0;

  char* FileName = nullptr;

  bool MergeImageVolumes = false;
  bool MergePointSets = false;

  bool DataCorrect = true;

  vtkNew<vtkDataArraySelection> GridSelection;

  constexpr static const char* FILE_EXTENSIONS = ".vdb";
  constexpr static const char* DESCRIPTIVE_NAME = "OpenVDB volumetric data file format";

  int RequestInformation(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

  int RequestData(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

  int RequestDataObject(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

private:
  vtkOpenVDBReader(const vtkOpenVDBReader&) = delete;
  void operator=(const vtkOpenVDBReader&) = delete;

  std::unique_ptr<vtkOpenVDBReaderInternals> Internals;
};

#endif
