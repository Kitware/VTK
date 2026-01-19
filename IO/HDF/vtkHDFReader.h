// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkHDFReader
 * @brief   Read VTK HDF files.
 *
 * Reader for data saved using the VTKHDF format, supporting
 * image data, poly data, unstructured grid, overlapping AMR, hyper tree grid, partitioned dataset
 * collection and multiblock.
 *
 * This reader supports reading any vtkResourceStream, but is more efficient with a vtkMemoryStream.
 *
 * Serial and parallel reading are supported, with the possibility of piece selection.
 *
 * This reader uses an internal cache which avoid reading and modifying data when not needed, which
 * increase reading performance for temporal datasets when the geometry is constant between time
 * steps, with a constant MeshMTime. Please note this cache is not used when reading
 * vtkOverlappingAMR and vtkHyperTreeGrid.
 *
 * Major version of the specification should be incremented when older readers can no
 * longer read files written for this reader. Minor versions are
 * for added functionality that can be safely ignored by older
 * readers.
 *
 * @note vtkHDF file format is defined here :
 * https://docs.vtk.org/en/latest/design_documents/VTKFileFormats.html#hdf-file-formats
 */

#ifndef vtkHDFReader_h
#define vtkHDFReader_h

#include "vtkDataAssembly.h" // For vtkDataAssembly
#include "vtkDataObjectAlgorithm.h"
#include "vtkDeprecation.h"  // For VTK_DEPRECATED_IN_9_7_0
#include "vtkIOHDFModule.h"  // For export macro
#include "vtkSmartPointer.h" // For vtkSmartPointer

#include <array>  // For storing the time range
#include <memory> // For std::unique_ptr
#include <vector> // For storing list of values

VTK_ABI_NAMESPACE_BEGIN
class vtkAbstractArray;
class vtkCallbackCommand;
class vtkCellData;
class vtkCommand;
class vtkDataArraySelection;
class vtkDataSet;
class vtkDataSetAttributes;
class vtkHyperTreeGrid;
class vtkImageData;
class vtkInformation;
class vtkInformationVector;
class vtkMultiBlockDataSet;
class vtkOverlappingAMR;
class vtkPartitionedDataSet;
class vtkPartitionedDataSetCollection;
class vtkPointData;
class vtkPolyData;
class vtkResourceStream;
class vtkUnstructuredGrid;

namespace vtkHDFUtilities
{
struct TemporalHyperTreeGridOffsets;
struct TemporalGeometryOffsets;
}

class VTKIOHDF_EXPORT vtkHDFReader : public vtkDataObjectAlgorithm
{
public:
  static vtkHDFReader* New();
  vtkTypeMacro(vtkHDFReader, vtkDataObjectAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Get/Set the name of the input file.
   */
  vtkSetFilePathMacro(FileName);
  vtkGetFilePathMacro(FileName);
  ///@}

  ///@{
  /**
   * Specify stream to read from
   * When both `Stream` and `Filename` are set, stream is used.
   * Please note that when using virtual dataset (eg. when using the vtkHDFWriter::UseExternal*
   * properties) these files are still read from disk.
   */
  void SetStream(vtkResourceStream* stream);
  vtkResourceStream* GetStream();
  ///@}

  ///@{
  /**
   * Return true if, after a quick check of file header, it looks like the provided file or stream
   * can be read. Return false if it is sure it cannot be read. The stream version may move the
   * stream cursor. The filename version should be static but is not for backward compatibility
   * reasons. It does not modify nor uses members. If the file has a newer version than the reader,
   * this still returns true and the reader will fail on reading. This enables clients (ParaView) to
   * distinguish between failures when we need to look for another reader and failures when we
   * don't.
   */
  virtual vtkTypeBool CanReadFile(VTK_FILEPATH const char* name);
  static bool CanReadFile(vtkResourceStream* stream);
  ///@}

  ///@{
  /**
   * Get the output as a vtkDataSet pointer.
   */
  vtkDataSet* GetOutputAsDataSet();
  vtkDataSet* GetOutputAsDataSet(int index);
  ///@}

  ///@{
  /**
   * Get the data array selection tables used to configure which data
   * arrays are loaded by the reader.
   */
  virtual vtkDataArraySelection* GetPointDataArraySelection();
  virtual vtkDataArraySelection* GetCellDataArraySelection();
  virtual vtkDataArraySelection* GetFieldDataArraySelection();
  ///@}

  ///@{
  /**
   * Get the number of point or cell arrays available in the input.
   */
  int GetNumberOfPointArrays();
  int GetNumberOfCellArrays();
  ///@}

  ///@{
  /**
   * Get the name of the point or cell array with the given index in
   * the input.
   */
  const char* GetPointArrayName(int index);
  const char* GetCellArrayName(int index);
  ///@}

  ///@{
  /**
   * Getters and setters for temporal data
   * - HasTemporalData is a boolean that flags whether the file has temporal data
   * - NumberOfSteps is the number of time steps contained in the file
   * - Step is the time step to be read or last read by the reader
   * - TimeValue is the value corresponding to the Step property
   * - TimeRange is an array with the {min, max} values of time for the data
   */
  bool GetHasTemporalData();
  vtkGetMacro(NumberOfSteps, vtkIdType);
  vtkGetMacro(Step, vtkIdType);
  vtkSetMacro(Step, vtkIdType);
  vtkGetMacro(TimeValue, double);
  const std::array<double, 2>& GetTimeRange() const { return this->TimeRange; }
  ///@}

  ///@{
  /**
   * Boolean property determining whether to use the internal cache or not (default is true).
   */
  VTK_DEPRECATED_IN_9_7_0("Do not use, cache is on by default and should not be disabled.")
  virtual bool GetUseCache();
  VTK_DEPRECATED_IN_9_7_0("Do not use, cache is on by default and should not be disabled.")
  virtual void SetUseCache(bool use);
  VTK_DEPRECATED_IN_9_7_0("Do not use, cache is on by default and should not be disabled.")
  virtual void UseCacheOn();
  VTK_DEPRECATED_IN_9_7_0("Do not use, cache is on by default and should not be disabled.")
  virtual void UseCacheOff();
  ///@}

  ///@{
  /**
   * Choose the maximum level to read for AMR structures.
   * This only applies if LimitAMRLevelsToRead is active.
   * The value 0 indicates that the level read is not limited.
   * Default is 0.
   */
  vtkSetMacro(MaximumLevelsToReadByDefaultForAMR, unsigned int);
  vtkGetMacro(MaximumLevelsToReadByDefaultForAMR, unsigned int);
  ///@}

  ///@{
  /**
   * Get or Set the Original id name of an attribute (POINT, CELL, FIELD...)
   */
  VTK_DEPRECATED_IN_9_7_0("Do not use, will be removed.")
  std::string GetAttributeOriginalIdName(vtkIdType attribute);
  VTK_DEPRECATED_IN_9_7_0("Do not use, will be removed.")
  void SetAttributeOriginalIdName(vtkIdType attribute, const std::string& name);
  ///@}

  /**
   * Overridden to take into account mtime from the internal vtkResourceStream.
   */
  vtkMTimeType GetMTime() override;

protected:
  vtkHDFReader();
  ~vtkHDFReader() override;

  /**
   * Test if the reader can read a file with the given version number.
   */
  int CanReadFileVersion(int major, int minor);

  ///@{
  /**
   * Reads the 'data' requested in 'outInfo' (through extents or
   * pieces) for a specialized data type.
   * Returns 1 if successful, 0 otherwise.
   */
  int Read(vtkInformation* outInfo, vtkImageData* data);
  int Read(vtkInformation* outInfo, vtkUnstructuredGrid* data, vtkPartitionedDataSet* pData);
  int Read(vtkInformation* outInfo, vtkPolyData* data, vtkPartitionedDataSet* pData);
  int Read(vtkInformation* outInfo, vtkHyperTreeGrid* data, vtkPartitionedDataSet* pData);
  int Read(vtkInformation* outInfo, vtkOverlappingAMR* data);
  int Read(vtkInformation* outInfo, vtkPartitionedDataSetCollection* data);
  int Read(vtkInformation* outInfo, vtkMultiBlockDataSet* data);
  int ReadRecursively(vtkInformation* outInfo, vtkMultiBlockDataSet* data, const std::string& path);
  ///@}

  VTK_DEPRECATED_IN_9_6_0("This method is deprecated, do not use")
  int Read(const std::vector<vtkIdType>& numberOfPoints,
    const std::vector<vtkIdType>& numberOfCells,
    const std::vector<vtkIdType>& numberOfConnectivityIds, vtkIdType partOffset,
    vtkIdType startingPointOffset, vtkIdType startingCellOffset,
    vtkIdType startingConnectctivityIdOffset, int filePiece, vtkUnstructuredGrid* pieceData);

  /**
   * Read the field arrays from the file and add them to the dataset.
   */
  int AddFieldArrays(vtkDataObject* data);

  /**
   * Modify this object when an array selection is changed.
   */
  static void SelectionModifiedCallback(
    vtkObject* caller, unsigned long eid, void* clientdata, void* calldata);

  ///@{
  /**
   * Standard functions to specify the type, information and read the data from
   * the file.
   */
  int RequestDataObject(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;
  int RequestInformation(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;
  int RequestData(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;
  ///@}

  /**
   * Print update number of pieces, piece number and ghost levels.
   */
  void PrintPieceInformation(vtkInformation* outInfo);

  /**
   * Setup the information pass in parameter based on current vtkHDF file loaded.
   */
  int SetupInformation(vtkInformation* outInfo);

  /**
   * The input file's name.
   */
  char* FileName;

  /**
   * The input stream.
   */
  vtkSmartPointer<vtkResourceStream> Stream;

  /**
   * The array selections.
   * in the same order as vtkDataObject::AttributeTypes: POINT, CELL, FIELD
   */
  vtkDataArraySelection* DataArraySelection[3];

  /**
   * The observer to modify this object when the array selections are
   * modified.
   */
  vtkCallbackCommand* SelectionObserver;
  ///@{

  /**
   * Assembly used for PartitionedDataSetCollection
   */
  vtkSmartPointer<vtkDataAssembly> Assembly;

  ///@{
  /**
   * Temporal data properties
   */
  vtkIdType Step = 0;
  vtkIdType NumberOfSteps = 1;
  double TimeValue = 0.0;
  std::array<double, 2> TimeRange;
  ///@}

  unsigned int MaximumLevelsToReadByDefaultForAMR = 0;

  bool UseCache = true;
  struct DataCache;
  std::shared_ptr<DataCache> Cache;

private:
  vtkHDFReader(const vtkHDFReader&) = delete;
  void operator=(const vtkHDFReader&) = delete;

  class Implementation;
  Implementation* Impl;

  /**
   * Read data requested in 'outInfo', dispatching to the right specialized method
   * following the type of 'data'.
   */
  bool ReadData(vtkInformation* outInfo, vtkDataObject* data);

  /**
   * Read 'pieceData' specified by 'filePiece' where
   * number of points, cells and connectivity ids
   * store those numbers for all pieces.
   */
  int Read(const std::vector<vtkIdType>& numberOfPoints,
    const std::vector<vtkIdType>& numberOfCells,
    const std::vector<vtkIdType>& numberOfConnectivityIds,
    const std::vector<vtkIdType>& numberOfFaces,
    const std::vector<vtkIdType>& numberOfPolyhedronToFaceIds,
    const std::vector<vtkIdType>& numberOfFaceConnectivityIds,
    vtkHDFUtilities::TemporalGeometryOffsets& geoOffsets, int filePiece,
    vtkUnstructuredGrid* pieceData);

  /**
   * Read a single HyperTreeGrid piece from the file.
   * `htgTemporalOffsets` gives the information about the offsets for the current time step.
   * Returns 1 if successful, 0 otherwise.
   */
  int Read(const std::vector<vtkIdType>& numberOfTrees, const std::vector<vtkIdType>& numberOfCells,
    const std::vector<vtkIdType>& numberOfDepths, const std::vector<vtkIdType>& descriptorSizes,
    const vtkHDFUtilities::TemporalHyperTreeGridOffsets& htgTemporalOffsets, int filePiece,
    vtkHyperTreeGrid* pieceData);

  /**
   * Setter for UseTemporalData.
   */
  void SetHasTemporalData(bool useTemporalData);

  /**
   * Generate the vtkDataAssembly used for vtkPartitionedDataSetCollection and store it in Assembly.
   */
  void GenerateAssembly();

  /**
   * Retrieve the number of steps in each composite element of the dataset.
   * Return false if the number of steps is inconsistent across components, true otherwise.
   */
  bool RetrieveStepsFromAssembly();

  /**
   * Add array names from all composite elements to DataArraySelection array.
   * Return true on success
   */
  bool RetrieveDataArraysFromAssembly();

  vtkSmartPointer<vtkDataObject> OutputCache;

  std::map<vtkIdType, std::string> AttributesOriginalIdName{
    { vtkDataObject::POINT, "__pointsOriginalIds__" },
    { vtkDataObject::CELL, "__cellOriginalIds__" }, { vtkDataObject::FIELD, "__fieldOriginalIds__" }
  };

  bool HasTemporalData = false;
  std::string CompositeCachePath; // Identifier for the current composite piece
};

VTK_ABI_NAMESPACE_END
#endif
