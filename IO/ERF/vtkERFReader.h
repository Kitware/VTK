// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#ifndef vtkERFReader_h
#define vtkERFReader_h

#include "vtkIOERFModule.h" // For export macro
#include "vtkNew.h"         // For vtkNew
#include "vtkPartitionedDataSetCollectionAlgorithm.h"
#include "vtkSmartPointer.h" // For smart pointer

#include <map>    // for std::map
#include <vector> // for std::vector

VTK_ABI_NAMESPACE_BEGIN

class vtkDataAssembly;
class vtkDataArraySelection;
class vtkIntArray;
class vtkUnsignedCharArray;
class vtkCellArray;
class vtkUnstructuredGrid;

/**
 * @class vtkERFReader
 * @brief  Read ERF-HDF5 files.
 *
 * Based on HDF5, ERF format will contain multiple stages and a special group named 'post' which
 * indicates the default stage.
 *
 * A stage is a group with multiple subgroups which could be mandatory or not:
 *   - erfheader   (mandatory): defines the version of the format / simulation.
 *   - constant    (mandatory): defines all data which is not varying.
 *   - singlestate (opt)      : defines multiple states with varying data.
 *   - multistate  (opt)      : defines data which can depend on multiple varying parameters like
 *                              time series and load cases.
 *
 * Here is an example of a valid ERF HDF5 file hierarchy:
 * /
 *  CSMEXPL/
 *   constant/
 *    connectivities/
 *     SHELL/
 *      erfblock/
 *       cell indices
 *       connecitvity indices
 *       offsets
 *    entityresults/
 *     NODE/
 *      erfblock/
 *       indices
 *       points
 *       offsets
 *    systems/
 *      erfblock/
 *   erfheader/
 *   multistate/                (currently not supported yet)
 *   singlestate/
 *    state00001/
 *     ...
 *    state00004/
 *     ...
 *  post/
 *
 * Each subgroup of stage, like 'constant', will also contains multiple groups (e.g.
 * 'connectivities') and so on. The deeper subgroup, for each possible path, named `erfblock` has
 * several handy attributes to generate the data in VTK like: dimension, cell type, associated
 * groups, ... Also 'erfblock' contains several datasets (which is a file in HDF5) which will be the
 * raw data like points, indices, ...
 *
 * This reader will output a vtkPartitionedDataSetCollection as it should be read in distributed
 * fashion later and it represents a collection of dataSet which can vary depending on the time
 * (singlestate, multistate) or not (constant).
 *
 *
 * @warning Multistate isn't supported for now.
 *
 * @note for more details, the spec about ERF HDF5 implemented can be found here:
 * https://myesi.esi-group.com/ERF-HDF5/doc/ERF_HDF5_Specs_1.2.pdf.
 *
 * @note there is also a complementary spec about PAM-CSM here:
 * https://myesi.esi-group.com/ERF-HDF5/doc/ERF_CSM_RESULT_Specs_1.2.pdf.
 */
class VTKIOERF_EXPORT vtkERFReader : public vtkPartitionedDataSetCollectionAlgorithm
{
public:
  static vtkERFReader* New();
  vtkTypeMacro(vtkERFReader, vtkPartitionedDataSetCollectionAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  typedef int64_t hid_t;

  ///@{
  /**
   * Get/Set the name of the input file.
   */
  vtkSetMacro(FileName, std::string);
  vtkGetMacro(FileName, std::string);
  ///@}

  ///@{
  /**
   * Get the data array selections used to configure which data
   * arrays are loaded by the reader.
   */
  vtkGetNewMacro(StagesSelection, vtkDataArraySelection);
  vtkGetNewMacro(VariablesSelection, vtkDataArraySelection);
  vtkGetNewMacro(BlocksSelection, vtkDataArraySelection);
  ///@}

  ///@{
  /**
   * Enable all arrays to be read.
   * Call forwarded to vtkDataArraySelection.
   */
  void EnableAllVariables();
  void EnableAllBlocks();
  ///@}

  ///@{
  /**
   * Get/Set whether the point or cell array with the given name is enabled.
   * Call forwarded to vtkDataArraySelection.
   */
  void SetVariablesStatus(const char* name, int status);
  void SetStagesStatus(const char* name, int status);
  void SetBlocksStatus(const char* name, int status);
  ///@}

  /**
   * Return the current stage.
   */
  std::string GetStage() const;

protected:
  vtkERFReader();
  ~vtkERFReader() override;

  /**
   * Standard method herited from the superclass.
   *
   * It will be responsible to:
   * - verify if the input is a valid ERF HDF5 file.
   * - extract and let the user choose between each stages.
   * - extract temporal information.
   * - for the selected stages, display all possible data arrays.
   */
  int RequestInformation(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

  /**
   * Standard method herited from the superclass.
   *
   * It will be responsible to:
   * - extract and generate the mesh from the 'constant' block
   * - retrieve for the current timestep the associated state file and generate additional mesh,
   *   data array described in states group.
   */
  int RequestData(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

private:
  vtkERFReader(const vtkERFReader&) = delete;
  void operator=(const vtkERFReader&) = delete;

  /**
   * Return false if at least one of the mandatory blocks is missing, true otherwise.
   *
   * @note mandatory block: Constant, ERF Header, System.
   * @note Follow the version 1.2 of the ERF file format.
   */
  bool IsValidERFDataset(const hid_t& fileId) const;

  /**
   * Extract time values and time step (named state in this format) from the singlestate group and
   * populate States and TimeValues array.
   *
   * Return false if no temporal data can be extracted, true otherwise.
   */
  bool ExtractTemporalData(const hid_t& rootIdx);

  /**
   * Fill vtkInformation with temporal data filled in TimeSteps.
   */
  void AddTemporalInformation(vtkInformation* info);

  /**
   * Add temporal data as field data.
   *
   * Useful to retrieve the state associated to the time value.
   */
  void AddTemporalInformationAsFieldData(vtkPartitionedDataSetCollection* pdc);

  /**
   * Iterate through the selected stage and populate the pdc.
   */
  int TraverseStage(vtkPartitionedDataSetCollection* pdc);

  /**
   * Append all data from the 'constant' group
   */
  void AppendConstantGroupData(vtkPartitionedDataSetCollection* pdc, hid_t fileId);

  /**
   * Append all data from the 'singlestate' group
   */
  void AppendSinglestateGroupData(hid_t fileId);

  ///@{
  /**
   * Parse and append mandatory block (erf header, system) as field data in the output.
   */
  void AppendMandatoryBlock(vtkPartitionedDataSetCollection* output, const hid_t& fileId);
  void AppendSystemBlock(vtkPartitionedDataSetCollection* output, const hid_t& fileId);
  void AppendERFHeaderBlock(vtkPartitionedDataSetCollection* output, const hid_t& fileId);
  ///@}

  /**
   * Parse and append mandatory block (erf header, system) as field data in the output.
   */
  void AppendSinglestateBlock(const hid_t& fileId);

  /**
   * Try to append a field data based on the id and the name given as parameters.
   */
  void AppendFieldDataByName(
    vtkPartitionedDataSetCollection* pdc, const hid_t& id, const std::string& name);

  ///@{
  /**
   * Recreate the mesh.
   */
  void BuildMesh(const hid_t& fileId);
  void AppendPoints(
    vtkUnstructuredGrid* output, const std::string& nodeAttributePath, const hid_t& fileId);
  void AppendCells(
    vtkUnstructuredGrid* output, const std::string& shellAttributePath, const hid_t& fileId);
  ///@}

  /**
   * Append all mesh containing in the Meshs into the pdc.
   */
  void AppendMeshs(vtkPartitionedDataSetCollection* pdc, vtkDataAssembly* hierarchy,
    int& streamNodeId, int& meshNodeId, int& meshStartId);

  /**
   * Get the index of the current time values selected by the user in the TimeValues.
   */
  int GetTimeValuesIndex();

  ///@{
  /**
   * Get the value of an attribute on a ERF Block.
   */
  std::string GetAttributeValueAsStr(const hid_t& erfIdx, const std::string& attributeName) const;
  int GetAttributeValueAsInt(const hid_t& erfIdx, const std::string& attributeName) const;
  ///@}

  ///@{
  /**
   * Return true if the cell type is supported depending of the number of dimensions and the number
   * of nodes.
   *
   * Supported cell type:
   *  - Vertex
   *  - Line
   *  - Triangle
   *  - Tetra
   *  - Pyramid
   *  - Penta
   *  - Hexa
   *
   * @note High order cells aren't supported.
   */
  bool IsCellSupported(int ndim, int npelem);
  bool Is0DCellSupported(int npelem);
  bool Is1DCellSupported(int npelem);
  bool Is2DCellSupported(int npelem);
  bool Is3DCellSupported(int npelem);
  ///@}

  /**
   * Fill the cell array to the right cell type based on the dimension.
   */
  void FillCellsByType(vtkCellArray* cellArray, vtkUnsignedCharArray* cellType, hid_t shellDataType,
    hid_t arrayId, vtkIntArray* entid, int numberOfDimensions, int numberOfIndicePerCell,
    int numberOfCell);

  ///@{
  /**
   * Fill the cell array with the correct cell type
   */
  void Fill0DCellType(vtkCellArray* cellArray, vtkUnsignedCharArray* cellTypes, vtkIntArray* entid,
    const std::vector<int>& resData, int numberOfIndicePerCell, int numberOfCell);
  void Fill1DCellType(vtkCellArray* cellArray, vtkUnsignedCharArray* cellTypes, vtkIntArray* entid,
    const std::vector<int>& resData, int numberOfIndicePerCell, int numberOfCell);
  void Fill2DCellType(vtkCellArray* cellArray, vtkUnsignedCharArray* cellTypes, vtkIntArray* entid,
    const std::vector<int>& resData, int numberOfIndicePerCell, int numberOfCell);
  void Fill3DCellType(vtkCellArray* cellArray, vtkUnsignedCharArray* cellTypes, vtkIntArray* entid,
    const std::vector<int>& resData, int numberOfIndicePerCell, int numberOfCell);
  ///@}

  std::string FileName;
  std::string CurrentName;

  vtkNew<vtkDataArraySelection> StagesSelection;
  vtkNew<vtkDataArraySelection> VariablesSelection;
  vtkNew<vtkDataArraySelection> BlocksSelection;
  vtkNew<vtkDataArraySelection> ConstantGroupSelection;
  vtkNew<vtkDataArraySelection> MultistateGroupSelection;
  vtkNew<vtkDataArraySelection> SinglestateGroupSelection;

  std::vector<int> States;
  std::vector<double> TimeValues;
  double TimeRanges[2] = { 0.0, 0.0 };
  double CurrentTimeValue = 0.0;

  std::map<std::string, vtkSmartPointer<vtkUnstructuredGrid>> MeshPoints;
  std::map<std::string, vtkSmartPointer<vtkUnstructuredGrid>> Meshs;
};

VTK_ABI_NAMESPACE_END
#endif
