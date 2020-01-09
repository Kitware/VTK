/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVeraOutReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkVeraOutReader - File reader for VERA OUT HDF5 format.

#include "vtkVeraOutReader.h"

// vtkCommonCore
#include "vtkDataArray.h"
#include "vtkDataArraySelection.h"
#include "vtkDoubleArray.h"
#include "vtkFloatArray.h"
#include "vtkInformation.h"
#include "vtkInformationDoubleVectorKey.h"
#include "vtkInformationVector.h"
#include "vtkIntArray.h"
#include "vtkLongArray.h"
#include "vtkLongLongArray.h"
#include "vtkObjectFactory.h"
#include "vtkShortArray.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnsignedIntArray.h"
#include "vtkUnsignedShortArray.h"

// vtkCommonExecutionModel
#include "vtkStreamingDemandDrivenPipeline.h"

// vtkCommonDataModel
#include "vtkCellData.h"
#include "vtkFieldData.h"
#include "vtkRectilinearGrid.h"

// vtkhdf5
#define H5_USE_16_API
#include <vtk_hdf5.h>

#include <numeric>
#include <sstream>
#include <vector>

using namespace std;

#define VERA_MAX_DIMENSION 6
#define DATASET_NAME_MAX_SIZE 1024

//*****************************************************************************
class vtkVeraOutReader::Internals
{
public:
  Internals(vtkObject* owner)
  {
    this->Owner = owner;

    this->FileId = -1;

    this->NumberOfDimensions = 0;

    this->NeedCoreProcessing = true;

    this->APITCH = 20; // FIXME
    this->NASSX = 4;
    this->NASSY = 4;
    this->NPIN = 17;
    this->NAX = 4;
    this->APITCH = 20;
    this->SYMMETRY = 0;
    this->NUMBER_OF_STATES = 0;
  }
  // --------------------------------------------------------------------------
  virtual ~Internals() { this->CloseFile(); }

  // --------------------------------------------------------------------------

  void SetFileName(const char* filename)
  {
    std::string newFileName(filename ? filename : "");
    ;
    if (newFileName != this->FileName)
    {
      this->FileName = filename;
      this->CloseFile();

      // Reset any cache
      this->NUMBER_OF_STATES = 0;
      this->NeedCoreProcessing = true;
      this->CoreCellData.clear();
      this->CellDataArraySelection->RemoveAllArrays();
    }
  }

  // --------------------------------------------------------------------------

  bool OpenFile()
  {
    if (this->FileId > -1)
    {
      // Already open, skip...
      return true;
    }

    hid_t fileAccessPropListID = H5Pcreate(H5P_FILE_ACCESS);
    if (fileAccessPropListID < 0)
    {
      vtkErrorWithObjectMacro(this->Owner, "Couldn't H5Pcreate");
      return false;
    }
    herr_t err = H5Pset_fclose_degree(fileAccessPropListID, H5F_CLOSE_SEMI);
    if (err < 0)
    {
      vtkErrorWithObjectMacro(this->Owner, "Couldn't set file close access");
      return false;
    }
    if ((this->FileId = H5Fopen(this->FileName.c_str(), H5F_ACC_RDONLY, fileAccessPropListID)) < 0)
    {
      vtkErrorWithObjectMacro(
        this->Owner, "Cannot be a VERA file (" << this->FileName.c_str() << ")");
      return false;
    }
    H5Pclose(fileAccessPropListID);
    return true;
  }

  //----------------------------------------------------------------------------

  void CloseFile()
  {
    if (this->FileId > -1)
    {
      H5Fclose(this->FileId);
      this->FileId = -1;
    }
  }

  //----------------------------------------------------------------------------

  void LoadMetaData()
  {
    if (this->FileId == -1)
    {
      return;
    }

    this->ReadCore();

    // Register state fields
    if (this->GetNumberOfTimeSteps())
    {
      // Open group
      hid_t groupId = -1;
      if ((groupId = H5Gopen(this->FileId, "/STATE_0001")) < 0)
      {
        vtkErrorWithObjectMacro(this->Owner, "Can't open Group /STATE_0001");
        return;
      }

      H5G_info_t groupInfo;
      int status = H5Gget_info(groupId, &groupInfo);
      if (status < 0)
      {
        vtkErrorWithObjectMacro(
          this->Owner, "Can't get group info for /STATE_0001 (status: " << status << ")");
        H5Gclose(groupId);
        return;
      }

      // Extract dataset names
      std::vector<std::string> datasetNames;
      char datasetName[DATASET_NAME_MAX_SIZE];
      for (hsize_t idx = 0; idx < groupInfo.nlinks; idx++)
      {
        H5Lget_name_by_idx(groupId, ".", H5_INDEX_NAME, H5_ITER_INC, idx, datasetName,
          DATASET_NAME_MAX_SIZE, H5P_DEFAULT);
        datasetNames.push_back(datasetName);
      }
      H5Gclose(groupId);

      // Start processing datasets
      for (const std::string& dsName : datasetNames)
      {
        this->ReadDataSetDimensions("/STATE_0001", dsName.c_str());
        if (this->NumberOfDimensions == 4 && this->Dimensions[0] == this->NPIN &&
          this->Dimensions[1] == this->NPIN && this->Dimensions[2] == this->NAX &&
          this->Dimensions[3] == this->NASS)
        {
          this->CellDataArraySelection->AddArray(dsName.c_str());
        }
        else if (this->NumberOfDimensions == 1 && this->Dimensions[0] == 1)
        {
          this->FieldDataArraySelection->AddArray(dsName.c_str());
        }
      }
    }
  }

  //----------------------------------------------------------------------------

  int GetNumberOfTimeSteps()
  {
    if (this->NUMBER_OF_STATES)
    {
      return this->NUMBER_OF_STATES;
    }

    if (this->FileId == -1)
    {
      return 0;
    }

    // ----------------------------------
    // Find out how many state we have
    // ----------------------------------
    int count = 0;
    int status = 0;
    while (status >= 0)
    {
      count++;
      std::ostringstream groupName;
      groupName << "/STATE_" << std::setw(4) << std::setfill('0') << count;
      H5Eset_auto(nullptr, nullptr);
      status = H5Gget_objinfo(this->FileId, groupName.str().c_str(), 0, nullptr);
    }
    // H5Eset_auto(nullptr, nullptr);
    this->NUMBER_OF_STATES = count ? count - 1 : 0;
    // ----------------------------------

    return this->NUMBER_OF_STATES;
  }

  //----------------------------------------------------------------------------

  bool ReadDataSetDimensions(const char* groupName, const char* datasetName)
  {
    if (this->FileId == -1)
    {
      return false;
    }

    // Open group
    hid_t groupId = -1;
    if ((groupId = H5Gopen(this->FileId, groupName)) < 0)
    {
      vtkErrorWithObjectMacro(this->Owner, "Can't open group " << groupName);
      return false;
    }

    // Open dataset
    hid_t datasetId;
    if ((datasetId = H5Dopen(groupId, datasetName)) < 0)
    {
      H5Gclose(groupId);
      vtkErrorWithObjectMacro(this->Owner,
        "DataSet " << datasetName << " in group " << groupName << " don't want to open.");
      return false;
    }

    // Extract dataset dimensions
    hid_t spaceId = H5Dget_space(datasetId);
    H5Sget_simple_extent_dims(spaceId, this->Dimensions, nullptr);
    this->NumberOfDimensions = H5Sget_simple_extent_ndims(spaceId);

    // Close resources
    H5Sclose(spaceId);
    H5Dclose(datasetId);
    H5Gclose(groupId);

    return true;
  }

  //----------------------------------------------------------------------------

  vtkDataArray* ReadDataSet(const char* groupName, const char* datasetName)
  {
    if (!this->ReadDataSetDimensions(groupName, datasetName))
    {
      return nullptr;
    }

    vtkDataArray* arrayToReturn = nullptr;

    // Compute total size
    vtkIdType nbTuples = 1;
    for (hsize_t idx = 0; idx < this->NumberOfDimensions; idx++)
    {
      nbTuples *= this->Dimensions[idx];
    }

    // Open group
    hid_t groupId = -1;
    if ((groupId = H5Gopen(this->FileId, groupName)) < 0)
    {
      vtkErrorWithObjectMacro(this->Owner, "Can't open group " << groupName);
      return nullptr;
    }

    // Open dataset
    hid_t datasetId;
    if ((datasetId = H5Dopen(groupId, datasetName)) < 0)
    {
      vtkErrorWithObjectMacro(this->Owner,
        "DataSet " << datasetName << " in group " << groupName << " don't want to open.");
      H5Gclose(groupId);
      return nullptr;
    }

    // Extract data type
    hid_t tRawType = H5Dget_type(datasetId);
    hid_t dataType = H5Tget_native_type(tRawType, H5T_DIR_ASCEND);
    if (H5Tequal(dataType, H5T_NATIVE_FLOAT))
    {
      arrayToReturn = vtkFloatArray::New();
      arrayToReturn->SetNumberOfTuples(nbTuples);
      float* arrayPtr =
        static_cast<float*>(vtkArrayDownCast<vtkFloatArray>(arrayToReturn)->GetPointer(0));
      H5Dread(datasetId, dataType, H5S_ALL, H5S_ALL, H5P_DEFAULT, arrayPtr);
      arrayPtr = nullptr;
    }
    else if (H5Tequal(dataType, H5T_NATIVE_DOUBLE))
    {
      arrayToReturn = vtkDoubleArray::New();
      arrayToReturn->SetNumberOfTuples(nbTuples);
      double* arrayPtr =
        static_cast<double*>(vtkArrayDownCast<vtkDoubleArray>(arrayToReturn)->GetPointer(0));
      H5Dread(datasetId, dataType, H5S_ALL, H5S_ALL, H5P_DEFAULT, arrayPtr);
      arrayPtr = nullptr;
    }
    else if (H5Tequal(dataType, H5T_NATIVE_INT))
    {
      arrayToReturn = vtkIntArray::New();
      arrayToReturn->SetNumberOfTuples(nbTuples);
      int* arrayPtr =
        static_cast<int*>(vtkArrayDownCast<vtkIntArray>(arrayToReturn)->GetPointer(0));
      H5Dread(datasetId, dataType, H5S_ALL, H5S_ALL, H5P_DEFAULT, arrayPtr);
      arrayPtr = nullptr;
    }
    else if (H5Tequal(dataType, H5T_NATIVE_UINT))
    {
      arrayToReturn = vtkUnsignedIntArray::New();
      arrayToReturn->SetNumberOfTuples(nbTuples);
      unsigned int* arrayPtr = static_cast<unsigned int*>(
        vtkArrayDownCast<vtkUnsignedIntArray>(arrayToReturn)->GetPointer(0));
      H5Dread(datasetId, dataType, H5S_ALL, H5S_ALL, H5P_DEFAULT, arrayPtr);
      arrayPtr = nullptr;
    }
    else if (H5Tequal(dataType, H5T_NATIVE_SHORT))
    {
      arrayToReturn = vtkShortArray::New();
      arrayToReturn->SetNumberOfTuples(nbTuples);
      short* arrayPtr =
        static_cast<short*>(vtkArrayDownCast<vtkShortArray>(arrayToReturn)->GetPointer(0));
      H5Dread(datasetId, dataType, H5S_ALL, H5S_ALL, H5P_DEFAULT, arrayPtr);
      arrayPtr = nullptr;
    }
    else if (H5Tequal(dataType, H5T_NATIVE_USHORT))
    {
      arrayToReturn = vtkUnsignedShortArray::New();
      arrayToReturn->SetNumberOfTuples(nbTuples);
      unsigned short* arrayPtr = static_cast<unsigned short*>(
        vtkArrayDownCast<vtkUnsignedShortArray>(arrayToReturn)->GetPointer(0));
      H5Dread(datasetId, dataType, H5S_ALL, H5S_ALL, H5P_DEFAULT, arrayPtr);
      arrayPtr = nullptr;
    }
    else if (H5Tequal(dataType, H5T_NATIVE_UCHAR))
    {

      arrayToReturn = vtkUnsignedCharArray::New();
      arrayToReturn->SetNumberOfTuples(nbTuples);
      unsigned char* arrayPtr = static_cast<unsigned char*>(
        vtkArrayDownCast<vtkUnsignedCharArray>(arrayToReturn)->GetPointer(0));
      H5Dread(datasetId, dataType, H5S_ALL, H5S_ALL, H5P_DEFAULT, arrayPtr);
      arrayPtr = nullptr;
    }
    else if (H5Tequal(dataType, H5T_NATIVE_LONG))
    {
      arrayToReturn = vtkLongArray::New();
      arrayToReturn->SetNumberOfTuples(nbTuples);
      long* arrayPtr =
        static_cast<long*>(vtkArrayDownCast<vtkLongArray>(arrayToReturn)->GetPointer(0));
      H5Dread(datasetId, dataType, H5S_ALL, H5S_ALL, H5P_DEFAULT, arrayPtr);
      arrayPtr = nullptr;
    }
    else if (H5Tequal(dataType, H5T_NATIVE_LLONG))
    {
      arrayToReturn = vtkLongLongArray::New();
      arrayToReturn->SetNumberOfTuples(nbTuples);
      long long* arrayPtr =
        static_cast<long long*>(vtkArrayDownCast<vtkLongLongArray>(arrayToReturn)->GetPointer(0));
      H5Dread(datasetId, dataType, H5S_ALL, H5S_ALL, H5P_DEFAULT, arrayPtr);
      arrayPtr = nullptr;
    }
    else
    {
      vtkErrorWithObjectMacro(this->Owner,
        "Unknown HDF5 data type --- it is not FLOAT, "
          << "DOUBLE, INT, UNSIGNED INT, SHORT, UNSIGNED SHORT, "
          << "UNSIGNED CHAR, LONG, or LONG LONG.");
    }

    arrayToReturn->SetName(datasetName);

    // Close what we opened
    H5Tclose(dataType);
    // H5Tclose( tRawType );
    H5Dclose(datasetId);
    H5Gclose(groupId);

    return arrayToReturn;
  }

  //----------------------------------------------------------------------------

  vtkDataArray* CreatePinFieldArray(vtkDataArray* dataSource)
  {
    if (this->CoreMap == nullptr || dataSource == nullptr)
    {
      return nullptr;
    }

    vtkDataArray* outputField = dataSource->NewInstance();
    outputField->SetNumberOfTuples(this->NASSX * this->NPIN * this->NASSY * this->NPIN * this->NAX);
    vtkIdType srcIndex = 0;

    for (hsize_t sj = 0; sj < this->NASSY; sj++)
    {
      for (hsize_t si = 0; si < this->NASSX; si++)
      {
        vtkIdType assembyId = this->CoreMap->GetTuple1(si * this->NASSX + sj) - 1;
        vtkIdType dstOffset = si * this->NPIN + sj * this->NASSX * this->NPIN * this->NPIN;
        for (hsize_t dk = 0; dk < this->NAX; dk++)
        {
          for (hsize_t dj = 0; dj < this->NPIN; dj++)
          {
            for (hsize_t di = 0; di < this->NPIN; di++)
            {
              if (assembyId < 0)
              {
                outputField->SetTuple1(dstOffset + di + (dj * this->NASSX * this->NPIN) +
                    (dk * this->NASSX * this->NPIN * this->NASSY * this->NPIN),
                  0);
              }
              else
              {
                // Fortran ordering
                srcIndex = assembyId + dk * this->NASS + dj * this->NASS * this->NAX +
                  di * this->NASS * this->NAX * this->NPIN;
                if (SYMMETRY == 4)
                {
                  srcIndex = assembyId + dk * this->NASS;
                  if (2 * si > this->NASSX)
                  {
                    srcIndex += di * this->NASS * this->NAX * this->NPIN;
                  }
                  else
                  {
                    srcIndex += (this->NPIN - di - 1) * this->NASS * this->NAX * this->NPIN;
                  }
                  if (2 * sj > this->NASSY)
                  {
                    srcIndex += dj * this->NASS * this->NAX;
                  }
                  else
                  {
                    srcIndex += (this->NPIN - dj - 1) * this->NASS * this->NAX;
                  }
                }
                outputField->SetTuple1(dstOffset + di + (dj * this->NASSX * this->NPIN) +
                    (dk * this->NASSX * this->NPIN * this->NASSY * this->NPIN),
                  dataSource->GetTuple1(srcIndex));
              }
            }
          }
        }
      }
    }
    return outputField;
  }

  //----------------------------------------------------------------------------

  void AddDataSetNamesWithDimension(
    const char* groupName, hsize_t dimension, std::vector<std::string>& names)
  {
    // Open group
    hid_t groupId = -1;
    if ((groupId = H5Gopen(this->FileId, groupName)) < 0)
    {
      vtkErrorWithObjectMacro(this->Owner, "Can't open group " << groupName);
      return;
    }

    H5G_info_t groupInfo;
    int status = H5Gget_info(groupId, &groupInfo);
    if (status < 0)
    {
      vtkErrorWithObjectMacro(this->Owner, "Can't get group info for " << groupName);
      H5Gclose(groupId);
      return;
    }

    // Extract dataset names
    std::vector<std::string> datasetNames;
    char datasetName[DATASET_NAME_MAX_SIZE];
    for (hsize_t idx = 0; idx < groupInfo.nlinks; idx++)
    {
      H5Lget_name_by_idx(groupId, ".", H5_INDEX_NAME, H5_ITER_INC, idx, datasetName,
        DATASET_NAME_MAX_SIZE, H5P_DEFAULT);
      datasetNames.push_back(datasetName);
    }

    // Start processing datasets
    for (const std::string& dsName : datasetNames)
    {
      // Open dataset
      hid_t datasetId;
      if ((datasetId = H5Dopen(groupId, dsName.c_str())) < 0)
      {
        vtkErrorWithObjectMacro(this->Owner,
          "DataSet " << dsName.c_str() << " in group " << groupName << " don't want to open.");
        continue;
      }

      // Extract dataset dimensions
      hid_t spaceId = H5Dget_space(datasetId);
      H5Sget_simple_extent_dims(spaceId, this->Dimensions, nullptr);
      this->NumberOfDimensions = H5Sget_simple_extent_ndims(spaceId);

      // Add name if condition is valid
      if (this->NumberOfDimensions == dimension)
      {
        names.push_back(dsName);
      }
      H5Sclose(spaceId);
      H5Dclose(datasetId);
    }
    H5Gclose(groupId);
  }

  //----------------------------------------------------------------------------

  void ReadCore()
  {
    if (!this->NeedCoreProcessing)
    {
      return;
    }

    // Guard further reading if file name does not change..
    this->NeedCoreProcessing = false;
    this->CoreCellData.clear();

    // Local vars
    vtkDataArray* dataSource;
    vtkDataArray* outputCellArray;

    // --------------------------------------------------------------------------
    // Global variables
    // --------------------------------------------------------------------------
    // * NASSX – Maximum number of assemblies across the core horizontally in full symmetry
    // * NASSY – Maximum number of assemblies down the core vertically in full symmetry
    // * NPIN – Maximum number of fuel pins across a fuel assembly in the core. Assemblies are
    // assumed to be symmetric.
    // * NAX – Number of axial levels edited in the fuel
    // * NASS – Total number of fuel assemblies in the problem considering the symmetry of the
    // calculation.
    // --------------------------------------------------------------------------

    this->ZCoordinates = this->ReadDataSet("/CORE", "axial_mesh");
    this->NAX = this->Dimensions[0] - 1;
    this->ZCoordinates->Delete(); // Let the smart pointer hold the only ref

    this->CoreMap = this->ReadDataSet("/CORE", "core_map");
    this->NASSX = this->Dimensions[0];
    this->NASSY = this->Dimensions[1];
    this->CoreMap->Delete(); // Let the smart pointer hold the only ref

    dataSource = this->ReadDataSet("/CORE", "core_sym");
    this->SYMMETRY = dataSource->GetTuple1(0);
    dataSource->Delete(); // Just needed to extract the single value

    // ------------------------------------------
    // Extract pin information
    // ------------------------------------------
    std::vector<std::string> names;
    this->AddDataSetNamesWithDimension("/CORE", 4, names);
    for (const std::string& name : names)
    {
      dataSource = this->ReadDataSet("/CORE", name.c_str());
      this->NPIN = this->Dimensions[0];
      this->NASS = this->Dimensions[3];

      outputCellArray = this->CreatePinFieldArray(dataSource);
      outputCellArray->SetName(dataSource->GetName());
      this->CoreCellData.push_back(outputCellArray);
      outputCellArray->Delete();
      dataSource->Delete();
    }
    // ------------------------------------------

    // DEBUG: std::cout << "============================" << std::endl;
    // DEBUG: std::cout << "NASSX    " << this->NASSX << std::endl;
    // DEBUG: std::cout << "NASSY    " << this->NASSY << std::endl;
    // DEBUG: std::cout << "NPIN     " << this->NPIN << std::endl;
    // DEBUG: std::cout << "NAX      " << this->NAX << std::endl;
    // DEBUG: std::cout << "NASS     " << this->NASS << std::endl;
    // DEBUG: std::cout << "SYMMETRY " << this->SYMMETRY << std::endl;
    // DEBUG: std::cout << "============================" << std::endl;

    // ------------------------------------------
    // X/Y Coordinates
    // ------------------------------------------

    float axialStep = this->APITCH / (double)NPIN;
    this->XCoordinates->SetNumberOfTuples(NASSX * NPIN + 1);
    for (vtkIdType idx = 0; idx < this->XCoordinates->GetNumberOfTuples(); idx++)
    {
      this->XCoordinates->SetTuple1(idx, ((float)idx) * axialStep);
    }

    this->YCoordinates->SetNumberOfTuples(NASSY * NPIN + 1);
    for (vtkIdType idx = 0; idx < this->YCoordinates->GetNumberOfTuples(); idx++)
    {
      this->YCoordinates->SetTuple1(idx, ((float)idx) * axialStep);
    }

    // ------------------------------------------
    // Fill cellData from core information
    // ------------------------------------------

    outputCellArray = this->CoreMap->NewInstance();
    outputCellArray->SetNumberOfTuples(
      this->NASSX * this->NPIN * this->NASSY * this->NPIN * this->NAX);
    outputCellArray->SetName("AssemblyID");

    for (hsize_t sj = 0; sj < this->NASSY; sj++)
    {
      for (hsize_t si = 0; si < this->NASSX; si++)
      {
        vtkIdType srcIdx = si * this->NASSX + sj; // Fortran ordering
        vtkIdType dstOffset = si * this->NPIN + sj * this->NASSX * this->NPIN * this->NPIN;
        for (hsize_t dk = 0; dk < this->NAX; dk++)
        {
          for (hsize_t dj = 0; dj < this->NPIN; dj++)
          {
            for (hsize_t di = 0; di < this->NPIN; di++)
            {
              outputCellArray->SetTuple1(dstOffset + di + (dj * this->NASSX * this->NPIN) +
                  (dk * this->NASSX * this->NPIN * this->NASSY * this->NPIN),
                this->CoreMap->GetTuple1(srcIdx));
            }
          }
        }
      }
    }
    this->CoreCellData.push_back(outputCellArray);
    outputCellArray->Delete();
  }

  //----------------------------------------------------------------------------

  void InitializeWithCoreData(vtkRectilinearGrid* output)
  {
    // NoOp if already loaded
    this->ReadCore();

    output->SetDimensions(
      this->NASSX * this->NPIN + 1, this->NASSY * this->NPIN + 1, this->NAX + 1);
    output->SetXCoordinates(this->XCoordinates);
    output->SetYCoordinates(this->YCoordinates);
    output->SetZCoordinates(this->ZCoordinates);

    for (const vtkSmartPointer<vtkDataArray>& cellArray : this->CoreCellData)
    {
      output->GetCellData()->AddArray(cellArray);
    }
  }

  //----------------------------------------------------------------------------

  void AddStateData(vtkRectilinearGrid* output, vtkIdType timestep)
  {
    if (this->FileId == -1)
    {
      return;
    }

    // --------------------------------------------------------------------------
    // STATE_0001                    Group
    // STATE_0001/crit_boron         Dataset {1}
    // STATE_0001/detector_response  Dataset {49, 18} <------ SKIP/FIXME
    // STATE_0001/exposure           Dataset {1}
    // STATE_0001/keff               Dataset {1}
    // STATE_0001/pin_cladtemps      Dataset {17, 17, 49, 56}
    // STATE_0001/pin_fueltemps      Dataset {17, 17, 49, 56}
    // STATE_0001/pin_moddens        Dataset {17, 17, 49, 56}
    // STATE_0001/pin_modtemps       Dataset {17, 17, 49, 56}
    // STATE_0001/pin_powers         Dataset {17, 17, 49, 56}
    // --------------------------------------------------------------------------
    std::ostringstream stateGroupName;
    stateGroupName << "/STATE_" << std::setw(4) << std::setfill('0') << timestep;
    vtkDataArray* dataSource;
    vtkDataArray* outputCellArray;

    // Open group
    hid_t groupId = -1;
    if ((groupId = H5Gopen(this->FileId, stateGroupName.str().c_str())) < 0)
    {
      vtkErrorWithObjectMacro(this->Owner, "Can't open Group " << stateGroupName.str().c_str());
      return;
    }

    H5G_info_t groupInfo;
    int status = H5Gget_info(groupId, &groupInfo);
    if (status < 0)
    {
      vtkErrorWithObjectMacro(
        this->Owner, "Can't get group info for " << stateGroupName.str().c_str());
      return;
    }

    // Extract dataset names
    std::vector<std::string> datasetNames;
    char datasetName[DATASET_NAME_MAX_SIZE];
    for (hsize_t idx = 0; idx < groupInfo.nlinks; idx++)
    {
      H5Lget_name_by_idx(groupId, ".", H5_INDEX_NAME, H5_ITER_INC, idx, datasetName,
        DATASET_NAME_MAX_SIZE, H5P_DEFAULT);
      datasetNames.push_back(datasetName);
    }
    H5Gclose(groupId);

    // Start processing datasets
    for (const std::string& dsName : datasetNames)
    {
      if (!(this->CellDataArraySelection->ArrayExists(dsName.c_str()) ||
            this->FieldDataArraySelection->ArrayExists(dsName.c_str())) ||
        !(this->CellDataArraySelection->ArrayIsEnabled(dsName.c_str()) ||
          this->FieldDataArraySelection->ArrayIsEnabled(dsName.c_str())))
      {
        continue;
      }
      dataSource = this->ReadDataSet(stateGroupName.str().c_str(), dsName.c_str());
      if (dataSource)
      {
        if (this->NumberOfDimensions == 4 && this->Dimensions[0] == this->NPIN &&
          this->Dimensions[1] == this->NPIN && this->Dimensions[2] == this->NAX &&
          this->Dimensions[3] == this->NASS)
        {
          outputCellArray = this->CreatePinFieldArray(dataSource);
          outputCellArray->SetName(dsName.c_str());
          output->GetCellData()->AddArray(outputCellArray);
          outputCellArray->Delete();
        }
        else if (this->NumberOfDimensions == 1 && this->Dimensions[0] == 1)
        {
          // Add fields add FieldData
          output->GetFieldData()->AddArray(dataSource);
        }
        else
        {
          std::ostringstream message;
          message << "Invalid dimensions: ";
          for (hsize_t i = 0; i < this->NumberOfDimensions; i++)
          {
            message << this->Dimensions[i] << " ";
          }
          vtkDebugWithObjectMacro(this->Owner, << message.str());
        }
        dataSource->Delete();
      }
    }
  }

  //----------------------------------------------------------------------------

  vtkNew<vtkDataArraySelection> CellDataArraySelection;
  vtkNew<vtkDataArraySelection> FieldDataArraySelection;

private:
  hid_t FileId;
  std::string FileName;

  hsize_t NumberOfDimensions;
  hsize_t Dimensions[VERA_MAX_DIMENSION];

  bool NeedCoreProcessing;

  double APITCH;
  hsize_t NASSX;
  hsize_t NASSY;
  hsize_t NAX;
  hsize_t NPIN;
  hsize_t NASS;
  vtkIdType SYMMETRY;
  vtkIdType NUMBER_OF_STATES;

  vtkNew<vtkFloatArray> XCoordinates;
  vtkNew<vtkFloatArray> YCoordinates;

  vtkObject* Owner;
  vtkSmartPointer<vtkDataArray> ZCoordinates;
  vtkSmartPointer<vtkDataArray> CoreMap;

  std::vector<vtkSmartPointer<vtkDataArray> > CoreCellData;
};
//*****************************************************************************

vtkStandardNewMacro(vtkVeraOutReader);

//----------------------------------------------------------------------------
// Constructor for vtkVeraOutReader
//----------------------------------------------------------------------------
vtkVeraOutReader::vtkVeraOutReader()
{
  this->FileName = nullptr;
  this->NumberOfTimeSteps = 0;
  this->TimeSteps.clear();
  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(1);
  this->Internal = new Internals(this);
}

//----------------------------------------------------------------------------
// Destructor for vtkVeraOutReader
//----------------------------------------------------------------------------
vtkVeraOutReader::~vtkVeraOutReader()
{
  this->SetFileName(nullptr);
  delete this->Internal;
  this->Internal = nullptr;
}

//----------------------------------------------------------------------------
// Verify that the file exists, get dimension sizes and variables
//----------------------------------------------------------------------------
int vtkVeraOutReader::RequestInformation(
  vtkInformation* reqInfo, vtkInformationVector** inVector, vtkInformationVector* outVector)
{
  vtkDebugMacro(<< "In vtkVeraOutReader::RequestInformation" << endl);
  if (!this->Superclass::RequestInformation(reqInfo, inVector, outVector))
    return 0;

  if (!this->FileName || this->FileName[0] == '\0')
  {
    vtkErrorMacro("No filename specified");
    return 0;
  }

  vtkDebugMacro(<< "In vtkVeraOutReader::RequestInformation read filename okay" << endl);
  vtkInformation* outInfo = outVector->GetInformationObject(0);

  this->NumberOfTimeSteps = 0;
  this->Internal->SetFileName(this->FileName);
  if (this->Internal->OpenFile())
  {
    this->NumberOfTimeSteps = this->Internal->GetNumberOfTimeSteps();
    this->Internal->LoadMetaData();
    this->Internal->CloseFile();
  }

  this->TimeSteps.resize(this->NumberOfTimeSteps);
  std::iota(this->TimeSteps.begin(), this->TimeSteps.end(), 1.0);

  outInfo->Set(
    vtkStreamingDemandDrivenPipeline::TIME_STEPS(), &this->TimeSteps[0], this->NumberOfTimeSteps);

  double tRange[2];
  tRange[0] = this->NumberOfTimeSteps ? this->TimeSteps[0] : 0;
  tRange[1] = this->NumberOfTimeSteps ? this->TimeSteps[this->NumberOfTimeSteps - 1] : 0;
  outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_RANGE(), tRange, 2);

  return 1;
}

//----------------------------------------------------------------------------
// Default method: Data is read into a vtkUnstructuredGrid
//----------------------------------------------------------------------------
int vtkVeraOutReader::RequestData(vtkInformation* vtkNotUsed(reqInfo),
  vtkInformationVector** vtkNotUsed(inVector), vtkInformationVector* outVector)
{
  if (!this->FileName || this->FileName[0] == '\0')
  {
    vtkErrorMacro("No filename specified");
    return 0;
  }

  vtkDebugMacro(<< "In vtkVeraOutReader::RequestData" << endl);
  vtkInformation* outInfo = outVector->GetInformationObject(0);
  vtkRectilinearGrid* output =
    vtkRectilinearGrid::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  // --------------------------------------------------------------------------
  // Time / State handling
  // --------------------------------------------------------------------------

  vtkIdType requestedTimeStep = 0;
  auto timeKey = vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP();

  if (outInfo->Has(timeKey))
  {
    requestedTimeStep = outInfo->Get(timeKey);
  }

  // --------------------------------------------------------------------------
  // Data handling
  // --------------------------------------------------------------------------

  this->Internal->SetFileName(this->FileName);
  if (this->Internal->OpenFile())
  {
    this->Internal->InitializeWithCoreData(output);
    this->Internal->AddStateData(output, requestedTimeStep);
    this->Internal->CloseFile();
  }

  // --------------------------------------------------------------------------

  vtkDebugMacro(<< "Out vtkVeraOutReader::RequestData" << endl);

  return 1;
}

//----------------------------------------------------------------------------
//  Print self.
//----------------------------------------------------------------------------
void vtkVeraOutReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "FileName: " << (this->FileName ? this->FileName : "NULL") << "\n";
}

//----------------------------------------------------------------------------
// Cell Array Selection
//----------------------------------------------------------------------------

vtkDataArraySelection* vtkVeraOutReader::GetCellDataArraySelection() const
{
  return this->Internal->CellDataArraySelection;
}

//----------------------------------------------------------------------------
// Field Array Selection
//----------------------------------------------------------------------------

vtkDataArraySelection* vtkVeraOutReader::GetFieldDataArraySelection() const
{
  return this->Internal->FieldDataArraySelection;
}

//----------------------------------------------------------------------------
// MTime
//----------------------------------------------------------------------------

vtkMTimeType vtkVeraOutReader::GetMTime()
{
  vtkMTimeType mTime = this->Superclass::GetMTime();
  vtkMTimeType cellMTime = this->Internal->CellDataArraySelection->GetMTime();
  vtkMTimeType fieldMTime = this->Internal->FieldDataArraySelection->GetMTime();

  mTime = (cellMTime > mTime ? cellMTime : mTime);
  mTime = (fieldMTime > mTime ? fieldMTime : mTime);

  return mTime;
}
