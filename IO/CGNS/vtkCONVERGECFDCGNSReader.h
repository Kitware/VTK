// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @class    vtkCONVERGECFDCGNSReader
 * @brief    Reader for CONVERGECFD CGNS post files.
 *
 * This class reads CONVERGECFD post files using the CGNS standard. Meshes,
 * surfaces, and parcels are read. Parcels are defined inside "PARCEL_DATA"
 * UserDefinedData_t nodes.
 *
 * Cell data arrays associated with mesh cells can be individually
 * selected for reading using the CellArrayStatus API.
 *
 * Regular point data arrays associated with mesh points can be individually
 * selected for reading using the PointArrayStatus API.
 *
 * Point data arrays associated with parcels can be individually selected
 * for reading using the ParcelArrayStatus API.
 */

#ifndef vtkCONVERGECFDCGNSReader_h
#define vtkCONVERGECFDCGNSReader_h

#include "vtkIOCGNSReaderModule.h" // For export macro

#include "vtkNew.h" // for vtkNew
#include "vtkPartitionedDataSetCollectionAlgorithm.h"
#include "vtkSmartPointer.h" // for vtkSmartPointer

#include <string> // for std::string

VTK_ABI_NAMESPACE_BEGIN
class vtkCGNSReader;
class vtkDataArraySelection;
class vtkPolyData;

class VTKIOCGNSREADER_EXPORT vtkCONVERGECFDCGNSReader
  : public vtkPartitionedDataSetCollectionAlgorithm
{
public:
  static vtkCONVERGECFDCGNSReader* New();
  vtkTypeMacro(vtkCONVERGECFDCGNSReader, vtkPartitionedDataSetCollectionAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Access the point data array selection to specify which point data arrays
   * should be read. Only the specified arrays will be read from the file.
   */
  vtkGetMacro(PointDataArraySelection, vtkDataArraySelection*);

  /**
   * Access the cell data array selection to specify which cell data arrays
   * should be read. Only the specified arrays will be read from the file.
   */
  vtkGetMacro(CellDataArraySelection, vtkDataArraySelection*);

  /**
   * Access the parcel data array selection to specify which parcel data arrays
   * should be read. Only the specified arrays will be read from the file.
   * Note that parcels are defined as points separate from the main mesh.
   */
  vtkGetMacro(ParcelDataArraySelection, vtkDataArraySelection*);

  /**
   * Return whether the file can be read with this reader.
   * Forwarded to the CGNS reader which does the non parcel part of the reading.
   */
  virtual int CanReadFile(VTK_FILEPATH const std::string& filename);

  ///@{
  /**
   * Get/set the CGNS file name.
   */
  vtkSetMacro(FileName, std::string);
  vtkGetMacro(FileName, std::string);
  ///@}

protected:
  vtkCONVERGECFDCGNSReader();
  ~vtkCONVERGECFDCGNSReader() override = default;

  int RequestInformation(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

private:
  vtkCONVERGECFDCGNSReader(const vtkCONVERGECFDCGNSReader&) = delete;
  void operator=(const vtkCONVERGECFDCGNSReader&) = delete;

  /**
   * Find the IDs of the nodes among the given vector that use the names "prefix_X",
   * "prefix_Y" and "prefix_Z", with "prefix" being given as input.
   */
  void FindVectorNodeIds(int cgioId, const std::vector<double>& arrayIds, const std::string& prefix,
    double& vectorXId, double& vectorYId, double& vectorZId) const;

  /**
   * Define parcel points and assign them to the given polydata.
   */
  bool CreateParcelPoints(
    int cgioId, double parcelXId, double parcelYId, double parcelZId, vtkPolyData* parcel) const;

  /**
   * Retrieve data values for the given array name, then create the corresponding
   * VTK array.
   */
  vtkSmartPointer<vtkDataArray> ReadParcelDataArray(int cgioId, double dataNodeId,
    const std::string& name, const std::vector<double>& arrayIds, bool isVector) const;

  std::string FileName;
  bool DataArraysInitialized = false;
  vtkNew<vtkDataArraySelection> PointDataArraySelection;
  vtkNew<vtkDataArraySelection> CellDataArraySelection;
  vtkNew<vtkDataArraySelection> ParcelDataArraySelection;
  vtkNew<vtkCGNSReader> CGNSReader;
};

VTK_ABI_NAMESPACE_END
#endif // vtkCONVERGECFDCGNSReader_h
