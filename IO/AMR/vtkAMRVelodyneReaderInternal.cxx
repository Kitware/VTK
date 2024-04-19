// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkAMRVelodyneReaderInternal.h"

#include <algorithm>
#include <fstream>
#include <iostream>
#define pv_int 1
#define pv_double 2
#define pv_scalar 1
#define pv_vector 3
#define pv_tensor6 6
#define pv_tensor 9
#define amrNode 1
#define amrLeaf 2
#define amrFullLeaf 3
VTK_ABI_NAMESPACE_BEGIN
vtkAMRVelodyneReaderInternal::vtkAMRVelodyneReaderInternal()
{
  this->Init();
}

vtkAMRVelodyneReaderInternal::~vtkAMRVelodyneReaderInternal()
{
  this->Init();
}

void vtkAMRVelodyneReaderInternal::Init()
{
  this->nLevels = 0;
  this->nBlocks = 0;
  this->nLeaves = 0;
  this->nFullLeaves = 0;
  this->Blocks.clear();
  this->blocksPerLevel.clear();
  this->blockDims.clear();
  this->globalOrigin.clear();
  this->rootDX.clear();
  this->file_id = -1;
  this->AttributeNames.clear();
  this->typeMap.clear();
  this->arrayMap.clear();
}

void vtkAMRVelodyneReaderInternal::SetFileName(VTK_FUTURE_CONST char* fileName)
{
  this->FileName = fileName ? fileName : "";
  if (this->file_id > 0)
  {
    herr_t ierr = this->CloseFile(this->file_id);
    if (ierr < 0)
    {
      vtkGenericWarningMacro("Failed to close previous file" << endl);
      return;
    }
  }
}

void vtkAMRVelodyneReaderInternal::ReadMetaData()
{
  if (this->FileName.empty())
  {
    return;
  }
  if (this->file_id >= 0)
  {
    return;
  }
  this->file_id = H5Fopen(this->FileName.c_str(), H5F_ACC_RDONLY, H5P_DEFAULT);
  if (this->file_id < 0)
  {
    vtkGenericWarningMacro("Failed to open file " << this->FileName << endl);
    return;
  }
  hid_t att_id;
  hid_t grp_id = H5Gopen(file_id, "AMR");
  if (grp_id < 0)
  {
    vtkGenericWarningMacro("Failed to open AMR group " << endl);
    return;
  }
  // Read Time Step
  att_id = H5Aopen(grp_id, "SimTime", H5P_DEFAULT);
  herr_t ierr = H5Aread(att_id, H5T_NATIVE_DOUBLE, &this->dataTime);
  if (ierr < 0)
  {
    vtkGenericWarningMacro("Failed to open SimTime Attribute " << endl);
    return;
  }
  ierr = H5Aclose(att_id);
  // Read Max Levels
  att_id = H5Aopen(grp_id, "MaxLevel", H5P_DEFAULT);
  ierr = H5Aread(att_id, H5T_NATIVE_INT, &nLevels);
  if (ierr < 0)
  {
    vtkGenericWarningMacro("Failed to open MaxLevels Attribute " << endl);
    return;
  }
  ierr = H5Aclose(att_id);

  // Read Level Count
  blocksPerLevel.resize(nLevels, 0);

  att_id = H5Aopen(grp_id, "LevelCount", H5P_DEFAULT);
  ierr = H5Aread(att_id, H5T_NATIVE_INT, blocksPerLevel.data());
  if (ierr < 0)
  {
    vtkGenericWarningMacro("Failed to open LevelCount Attribute " << endl);
    return;
  }

  H5Aclose(att_id);

  att_id = H5Aopen(grp_id, "NumberOfNodes", H5P_DEFAULT);
  ierr = H5Aread(att_id, H5T_NATIVE_INT, &(this->nBlocks));
  if (ierr < 0)
  {
    vtkGenericWarningMacro("Failed to open Number of Nodes\n");
    return;
  }
  H5Aclose(att_id);

  this->Blocks.resize(this->nBlocks);

  // Read Block Dims

  att_id = H5Aopen(grp_id, "BlockDims", H5P_DEFAULT);
  blockDims.resize(3);
  ierr = H5Aread(att_id, H5T_NATIVE_INT, blockDims.data());
  if (ierr < 0)
  {
    vtkGenericWarningMacro("Failed to open BlockDims Attribute\n");
    return;
  }
  H5Aclose(att_id);

  att_id = H5Aopen(grp_id, "RootXS", H5P_DEFAULT);
  globalOrigin.resize(3);
  ierr = H5Aread(att_id, H5T_NATIVE_DOUBLE, globalOrigin.data());
  if (ierr < 0)
  {
    vtkGenericWarningMacro("Failed to open RootXS Attribute\n");
    return;
  }
  H5Aclose(att_id);

  att_id = H5Aopen(grp_id, "RootDX", H5P_DEFAULT);
  rootDX.resize(3);
  ierr = H5Aread(att_id, H5T_NATIVE_DOUBLE, rootDX.data());
  if (ierr < 0)
  {
    vtkGenericWarningMacro("Failed to open RootDX Attribute\n");
    return;
  }
  H5Aclose(att_id);

  att_id = H5Aopen(grp_id, "NumberOfFieldVariables", H5P_DEFAULT);
  int nVars;
  ierr = H5Aread(att_id, H5T_NATIVE_INT, &nVars);
  if (ierr < 0)
  {
    vtkGenericWarningMacro("Failed to open NumberOfFieldVariables Attribute\n");
    return;
  }
  H5Aclose(att_id);

  this->AttributeNames.resize(nVars);
  att_id = H5Aopen(grp_id, "VariableList", H5P_DEFAULT);
  hid_t atype = H5Aget_type(att_id);
  size_t att_size = H5Tget_size(atype);
  size_t total_size = att_size * nVars;
  char* stringout = nullptr;
  stringout = new char[total_size + 1];
  ierr = H5Aread(att_id, atype, stringout);
  if (ierr < 0)
  {
    vtkGenericWarningMacro("Cannot read Variable List Attribute\n");
  }
  stringout[total_size] = '\0';
  std::string tmp(stringout);
  delete[] stringout;
  stringout = nullptr;
  for (int i = 0; i < nVars; i++)
  {
    this->AttributeNames[i] = (tmp.substr(i * att_size, att_size));
    std::string::iterator end_pos =
      std::remove(this->AttributeNames[i].begin(), this->AttributeNames[i].end(), ' ');
    this->AttributeNames[i].erase(end_pos, this->AttributeNames[i].end());
  }
  H5Aclose(att_id);
  H5Tclose(atype);

  std::vector<int> arrtype(nVars);
  att_id = H5Aopen(grp_id, "FieldVariableDataType", H5P_DEFAULT);
  ierr = H5Aread(att_id, H5T_NATIVE_INT, arrtype.data());
  if (ierr < 0)
  {
    vtkGenericWarningMacro("Cannot read Variable Data Type Attribute\n");
  }
  for (int i = 0; i < nVars; i++)
  {
    typeMap[this->AttributeNames[i]] = arrtype[i];
  }
  H5Aclose(att_id);

  att_id = H5Aopen(grp_id, "FieldVariableArrayType", H5P_DEFAULT);
  ierr = H5Aread(att_id, H5T_NATIVE_INT, arrtype.data());
  if (ierr < 0)
  {
    vtkGenericWarningMacro("Cannot read Variable Array Type Attribute\n");
  }
  for (int i = 0; i < nVars; i++)
  {
    arrayMap[this->AttributeNames[i]] = arrtype[i];
  }
  H5Aclose(att_id);

  ierr = H5Gclose(grp_id);

  ReadBlocks();
}

void vtkAMRVelodyneReaderInternal::ReadBlocks()
{
  hid_t grp_amr;
  hid_t grp_sub;
  hid_t ds_id;
  hid_t dspace_id;
  hid_t mspace_id;

  hsize_t mem_dims[2];
  hsize_t data_dims[2];
  hsize_t max_dims[2];

  std::string amrName = "AMR";
  std::string nodeName = "NonLeafNodes";
  std::string leafName = "Leaves";
  std::string fullLeafName = "FullLeaves";
  std::vector<int> readMap(this->nBlocks * 2, 0);
  grp_amr = H5Gopen(this->file_id, amrName.c_str());

  //*****************Open the Morton Order Read Map***********************
  ds_id = H5Dopen(grp_amr, "ReadMap");
  dspace_id = H5Dget_space(ds_id);
  int nDims = H5Sget_simple_extent_dims(dspace_id, data_dims, max_dims);
  if (nDims != 2 || data_dims[0] != static_cast<unsigned int>(this->nBlocks))
  {
    vtkGenericWarningMacro("Wrong number of blocks in the Morton Map\n");
    return;
  }
  mem_dims[0] = this->nBlocks;
  mem_dims[1] = 2;
  mspace_id = H5Screate_simple(2, mem_dims, mem_dims);
  herr_t ierr = H5Dread(ds_id, H5T_NATIVE_INT, mspace_id, dspace_id, H5P_DEFAULT, readMap.data());
  if (ierr < 0)
  {
    vtkGenericWarningMacro("Cannot Read the Morton Order Map\n");
  }
  ierr = H5Sclose(dspace_id);
  ierr = H5Sclose(mspace_id);
  ierr = H5Dclose(ds_id);
  //*************Done with Morton Order Read**********************************
  std::vector<int> nodeLevels;
  std::vector<double> nodeX0;
  herr_t status;
  status = H5Eset_auto(nullptr, nullptr);
  status = H5Gget_objinfo(grp_amr, "NonLeafNodes", false, nullptr);
  if (status == 0)
  {
    grp_sub = H5Gopen(grp_amr, nodeName.c_str());
    this->nNodes = ReadLevelsAndX0(grp_sub, nodeLevels, nodeX0);
    ierr = H5Gclose(grp_sub);
  }

  std::vector<int> leafLevels;
  std::vector<double> leafX0;
  status = H5Eset_auto(nullptr, nullptr);
  status = H5Gget_objinfo(grp_amr, "Leaves", false, nullptr);
  if (status == 0)
  {
    grp_sub = H5Gopen(grp_amr, leafName.c_str());
    this->nLeaves = ReadLevelsAndX0(grp_sub, leafLevels, leafX0);
    ierr = H5Gclose(grp_sub);
  }

  std::vector<int> fullLeafLevels;
  std::vector<double> fullLeafX0;
  status = H5Eset_auto(nullptr, nullptr);
  status = H5Gget_objinfo(grp_amr, "FullLeaves", false, nullptr);
  if (status == 0)
  {
    grp_sub = H5Gopen(grp_amr, fullLeafName.c_str());
    this->nFullLeaves = ReadLevelsAndX0(grp_sub, fullLeafLevels, fullLeafX0);
    ierr = H5Gclose(grp_sub);
  }

  int nodeType;
  int loc;
  int ind;
  std::vector<int> levelV(this->nLevels, 0);
  int cLevel;
  for (int i = 0; i < this->nBlocks; i++)
  {
    ind = 2 * i;
    nodeType = readMap[ind];
    ind = 2 * i + 1;
    loc = readMap[ind];

    switch (nodeType)
    {
      case amrNode:
        Blocks[i].isLeaf = false;
        Blocks[i].isFull = false;
        cLevel = nodeLevels[loc] - 1;
        Blocks[i].Level = cLevel;
        for (int k = 0; k < 3; k++)
        {
          Blocks[i].Origin[k] = nodeX0[3 * loc + k];
        }
        break;
      case amrLeaf:
        Blocks[i].isLeaf = true;
        Blocks[i].isFull = false;
        cLevel = leafLevels[loc] - 1;
        Blocks[i].Level = cLevel;
        for (int k = 0; k < 3; k++)
        {
          Blocks[i].Origin[k] = leafX0[3 * loc + k];
        }
        break;
      case amrFullLeaf:
        Blocks[i].isLeaf = true;
        Blocks[i].isFull = true;
        cLevel = fullLeafLevels[loc] - 1;
        Blocks[i].Level = cLevel;
        for (int k = 0; k < 3; k++)
        {
          Blocks[i].Origin[k] = fullLeafX0[3 * loc + k];
        }
        break;
      default:
        vtkGenericWarningMacro("Unrecognized node type\n");
        return;
    }
    Blocks[i].dSetLoc = loc;
    Blocks[i].Index = levelV[cLevel];
    levelV[cLevel]++;
  }
  levelV.clear();
  nodeLevels.clear();
  leafLevels.clear();
  fullLeafLevels.clear();
  nodeX0.clear();
  leafX0.clear();
  fullLeafX0.clear();
  ierr = H5Gclose(grp_amr);
}

void vtkAMRVelodyneReaderInternal::GetBlockAttribute(
  const char* attribute, int blockIdx, vtkUniformGrid* pDataSet)
{
  this->ReadMetaData();
  if (attribute == nullptr || blockIdx < 0 || pDataSet == nullptr || blockIdx >= this->nBlocks)
  {
    return;
  }
  std::string dName(attribute);
  auto it = arrayMap.find(dName);
  auto it2 = typeMap.find(dName);
  switch (it->second)
  {
    case pv_scalar:
      this->AttachScalarToGrid(it2->second, attribute, blockIdx, pDataSet);
      break;
    case pv_vector:
      this->AttachVectorToGrid(it2->second, attribute, blockIdx, pDataSet);
      break;
    case pv_tensor6:
      this->AttachTensor6ToGrid(it2->second, attribute, blockIdx, pDataSet);
      break;
    case pv_tensor:
      this->AttachTensorToGrid(it2->second, attribute, blockIdx, pDataSet);
      break;
  }
}

void vtkAMRVelodyneReaderInternal::AttachScalarToGrid(
  int type, const char* attribute, int blockIdx, vtkUniformGrid* pDataSet)
{
  if (!this->Blocks[blockIdx].isLeaf)
  {
    return;
  }

  std::vector<int> iData;
  std::vector<double> fData;

  hid_t dType;
  vtkDataArray* dataArray = this->GetTypeAndArray(type, dType);

  dataArray->SetName(attribute);
  dataArray->SetNumberOfComponents(pv_scalar);
  bool isFullLeaf = this->Blocks[blockIdx].isFull;
  std::string groupName;
  std::vector<int> cBlockDims(3);
  if (isFullLeaf)
  {
    groupName = "AMR/FullLeaves";
  }
  else
  {
    groupName = "AMR/Leaves";
  }
  int bOff = this->Blocks[blockIdx].dSetLoc;
  hid_t grp_id = H5Gopen(this->file_id, groupName.c_str());
  hid_t ds_id = H5Dopen(grp_id, attribute);
  hsize_t data_dims[5];
  hsize_t mem_dims[5];
  hsize_t max_dims[5];
  hsize_t block_dims[5];
  hid_t dspace_id = H5Dget_space(ds_id);
  H5Sget_simple_extent_dims(dspace_id, data_dims, max_dims);
  cBlockDims[0] = data_dims[1];
  cBlockDims[1] = data_dims[2];
  cBlockDims[2] = data_dims[3];

  block_dims[0] = 1;
  block_dims[1] = cBlockDims[0];
  block_dims[2] = cBlockDims[1];
  block_dims[3] = cBlockDims[2];
  block_dims[4] = pv_scalar;

  hsize_t data_off[5];
  data_off[0] = bOff;
  data_off[1] = 0;
  data_off[2] = 0;
  data_off[3] = 0;
  data_off[4] = 0;
  hsize_t stride[5];
  stride[0] = 1;
  stride[1] = 1;
  stride[2] = 1;
  stride[3] = 1;
  stride[4] = 1;
  hsize_t count[5];
  count[0] = 1;
  count[1] = 1;
  count[2] = 1;
  count[3] = 1;
  count[4] = 1;
  H5Sselect_hyperslab(dspace_id, H5S_SELECT_SET, data_off, stride, count, block_dims);

  int nTuples = cBlockDims[0] * cBlockDims[1] * cBlockDims[2];

  mem_dims[0] = 1;
  mem_dims[1] = cBlockDims[0];
  mem_dims[2] = cBlockDims[1];
  mem_dims[3] = cBlockDims[2];
  mem_dims[4] = pv_scalar;
  hid_t mspace_id = H5Screate_simple(5, mem_dims, mem_dims);
  herr_t ierr;
  switch (type)
  {
    case pv_int:
    {
      iData.resize(nTuples);
      ierr = H5Dread(ds_id, dType, mspace_id, dspace_id, H5P_DEFAULT, iData.data());
      if (ierr < 0)
      {
        vtkGenericWarningMacro("Cannot read " << std::string(attribute) << endl);
      }
    }
    break;
    case pv_double:
    {
      fData.resize(nTuples);
      ierr = H5Dread(ds_id, dType, mspace_id, dspace_id, H5P_DEFAULT, fData.data());
      if (ierr < 0)
      {
        vtkGenericWarningMacro("Cannot read " << std::string(attribute) << endl);
      }
    }
    break;
  }
  H5Dclose(ds_id);
  H5Sclose(dspace_id);
  H5Sclose(mspace_id);
  H5Gclose(grp_id);
  dataArray->SetNumberOfTuples(nTuples);
  if (type == pv_double)
  {
    for (int k = 0; k < cBlockDims[0]; k++)
    {
      for (int j = 0; j < cBlockDims[1]; j++)
      {
        for (int i = 0; i < cBlockDims[2]; i++)
        {
          int ind = k * cBlockDims[2] * cBlockDims[1] + j * cBlockDims[2] + i;
          dataArray->SetTuple1(ind, fData[ind]);
        }
      }
    }
  }
  else
  {
    for (int k = 0; k < cBlockDims[0]; k++)
    {
      for (int j = 0; j < cBlockDims[1]; j++)
      {
        for (int i = 0; i < cBlockDims[2]; i++)
        {
          int ind = k * cBlockDims[2] * cBlockDims[1] + j * cBlockDims[2] + i;
          dataArray->SetTuple1(ind, iData[ind]);
        }
      }
    }
  }

  pDataSet->GetCellData()->AddArray(dataArray);
  dataArray->Delete();
  dataArray = nullptr;
}

void vtkAMRVelodyneReaderInternal::AttachVectorToGrid(
  int type, const char* attribute, int blockIdx, vtkUniformGrid* pDataSet)
{
  if (!this->Blocks[blockIdx].isLeaf)
  {
    return;
  }

  std::vector<int> iData;
  std::vector<double> fData;

  hid_t dType;
  vtkDataArray* dataArray = this->GetTypeAndArray(type, dType);

  dataArray->SetName(attribute);
  dataArray->SetNumberOfComponents(pv_vector);
  bool isFullLeaf = this->Blocks[blockIdx].isFull;
  std::string groupName;
  std::vector<int> cBlockDims(3);
  if (isFullLeaf)
  {
    groupName = "AMR/FullLeaves";
  }
  else
  {
    groupName = "AMR/Leaves";
  }
  int bOff = this->Blocks[blockIdx].dSetLoc;
  hid_t grp_id = H5Gopen(this->file_id, groupName.c_str());
  hid_t ds_id = H5Dopen(grp_id, attribute);
  hsize_t data_dims[5];
  hsize_t mem_dims[5];
  hsize_t max_dims[5];
  hsize_t block_dims[5];
  hid_t dspace_id = H5Dget_space(ds_id);
  H5Sget_simple_extent_dims(dspace_id, data_dims, max_dims);
  cBlockDims[0] = data_dims[1];
  cBlockDims[1] = data_dims[2];
  cBlockDims[2] = data_dims[3];

  block_dims[0] = 1;
  block_dims[1] = cBlockDims[0];
  block_dims[2] = cBlockDims[1];
  block_dims[3] = cBlockDims[2];
  block_dims[4] = pv_vector;

  hsize_t data_off[5];
  data_off[0] = bOff;
  data_off[1] = 0;
  data_off[2] = 0;
  data_off[3] = 0;
  data_off[4] = 0;
  hsize_t stride[5];
  stride[0] = 1;
  stride[1] = 1;
  stride[2] = 1;
  stride[3] = 1;
  stride[4] = 1;
  hsize_t count[5];
  count[0] = 1;
  count[1] = 1;
  count[2] = 1;
  count[3] = 1;
  count[4] = 1;
  H5Sselect_hyperslab(dspace_id, H5S_SELECT_SET, data_off, stride, count, block_dims);

  int nTuples = cBlockDims[0] * cBlockDims[1] * cBlockDims[2];

  mem_dims[0] = 1;
  mem_dims[1] = cBlockDims[0];
  mem_dims[2] = cBlockDims[1];
  mem_dims[3] = cBlockDims[2];
  mem_dims[4] = pv_vector;
  hid_t mspace_id = H5Screate_simple(5, mem_dims, mem_dims);
  herr_t ierr;
  switch (type)
  {
    case pv_int:
    {
      iData.resize(pv_vector * nTuples);
      ierr = H5Dread(ds_id, dType, mspace_id, dspace_id, H5P_DEFAULT, iData.data());
      if (ierr < 0)
      {
        vtkGenericWarningMacro("Cannot read " << std::string(attribute) << endl);
      }
    }
    break;
    case pv_double:
    {
      fData.resize(pv_vector * nTuples);
      ierr = H5Dread(ds_id, dType, mspace_id, dspace_id, H5P_DEFAULT, fData.data());
      if (ierr < 0)
      {
        vtkGenericWarningMacro("Cannot read " << std::string(attribute) << endl);
      }
    }
    break;
  }
  H5Dclose(ds_id);
  H5Sclose(dspace_id);
  H5Sclose(mspace_id);
  H5Gclose(grp_id);

  dataArray->SetNumberOfTuples(nTuples);
  int cnt = 0;
  if (type == pv_double)
  {
    for (int k = 0; k < cBlockDims[0]; k++)
    {
      for (int j = 0; j < cBlockDims[1]; j++)
      {
        for (int i = 0; i < cBlockDims[2]; i++)
        {
          int ind = pv_vector * (k * cBlockDims[2] * cBlockDims[1] + j * cBlockDims[2] + i);
          dataArray->SetTuple3(cnt++, fData[ind], fData[ind + 1], fData[ind + 2]);
        }
      }
    }
  }
  else
  {
    for (int k = 0; k < cBlockDims[0]; k++)
    {
      for (int j = 0; j < cBlockDims[1]; j++)
      {
        for (int i = 0; i < cBlockDims[2]; i++)
        {
          int ind = pv_vector * (k * cBlockDims[2] * cBlockDims[1] + j * cBlockDims[2] + i);
          dataArray->SetTuple3(cnt++, iData[ind], iData[ind + 1], iData[ind + 2]);
        }
      }
    }
  }

  pDataSet->GetCellData()->AddArray(dataArray);
  dataArray->Delete();
  dataArray = nullptr;
}

void vtkAMRVelodyneReaderInternal::AttachTensor6ToGrid(
  int type, const char* attribute, int blockIdx, vtkUniformGrid* pDataSet)
{
  if (!this->Blocks[blockIdx].isLeaf)
  {
    return;
  }

  std::vector<int> iData;
  std::vector<double> fData;

  hid_t dType;
  vtkDataArray* dataArray = this->GetTypeAndArray(type, dType);

  dataArray->SetName(attribute);
  dataArray->SetNumberOfComponents(pv_tensor6);
  bool isFullLeaf = this->Blocks[blockIdx].isFull;
  std::string groupName;
  std::vector<int> cBlockDims(3);
  if (isFullLeaf)
  {
    groupName = "AMR/FullLeaves";
  }
  else
  {
    groupName = "AMR/Leaves";
  }
  int bOff = this->Blocks[blockIdx].dSetLoc;
  hid_t grp_id = H5Gopen(this->file_id, groupName.c_str());
  hid_t ds_id = H5Dopen(grp_id, attribute);
  hsize_t data_dims[5];
  hsize_t mem_dims[5];
  hsize_t max_dims[5];
  hsize_t block_dims[5];
  hid_t dspace_id = H5Dget_space(ds_id);
  H5Sget_simple_extent_dims(dspace_id, data_dims, max_dims);
  cBlockDims[0] = data_dims[1];
  cBlockDims[1] = data_dims[2];
  cBlockDims[2] = data_dims[3];

  block_dims[0] = 1;
  block_dims[1] = cBlockDims[0];
  block_dims[2] = cBlockDims[1];
  block_dims[3] = cBlockDims[2];
  block_dims[4] = pv_tensor6;

  hsize_t data_off[5];
  data_off[0] = bOff;
  data_off[1] = 0;
  data_off[2] = 0;
  data_off[3] = 0;
  data_off[4] = 0;
  hsize_t stride[5];
  stride[0] = 1;
  stride[1] = 1;
  stride[2] = 1;
  stride[3] = 1;
  stride[4] = 1;
  hsize_t count[5];
  count[0] = 1;
  count[1] = 1;
  count[2] = 1;
  count[3] = 1;
  count[4] = 1;
  H5Sselect_hyperslab(dspace_id, H5S_SELECT_SET, data_off, stride, count, block_dims);

  int nTuples = cBlockDims[0] * cBlockDims[1] * cBlockDims[2];

  mem_dims[0] = 1;
  mem_dims[1] = cBlockDims[0];
  mem_dims[2] = cBlockDims[1];
  mem_dims[3] = cBlockDims[2];
  mem_dims[4] = pv_tensor6;
  hid_t mspace_id = H5Screate_simple(5, mem_dims, mem_dims);

  herr_t ierr;
  switch (type)
  {
    case pv_int:
    {
      iData.resize(pv_tensor6 * nTuples);
      ierr = H5Dread(ds_id, dType, mspace_id, dspace_id, H5P_DEFAULT, iData.data());
      if (ierr < 0)
      {
        vtkGenericWarningMacro("Cannot read " << std::string(attribute) << endl);
      }
    }
    break;
    case pv_double:
    {
      fData.resize(pv_tensor6 * nTuples);
      ierr = H5Dread(ds_id, dType, mspace_id, dspace_id, H5P_DEFAULT, fData.data());
      if (ierr < 0)
      {
        vtkGenericWarningMacro("Cannot read " << std::string(attribute) << endl);
      }
    }
    break;
  }

  H5Dclose(ds_id);
  H5Sclose(dspace_id);
  H5Sclose(mspace_id);
  H5Gclose(grp_id);

  dataArray->SetNumberOfTuples(nTuples);
  int cnt = 0;
  if (type == pv_double)
  {
    for (int k = 0; k < cBlockDims[0]; k++)
    {
      for (int j = 0; j < cBlockDims[1]; j++)
      {
        for (int i = 0; i < cBlockDims[2]; i++)
        {
          int ind = pv_tensor6 * (k * cBlockDims[2] * cBlockDims[1] + j * cBlockDims[2] + i);
          dataArray->SetTuple6(cnt++, fData[ind], fData[ind + 1], fData[ind + 2], fData[ind + 3],
            fData[ind + 4], fData[ind + 5]);
        }
      }
    }
  }
  else
  {
    for (int k = 0; k < cBlockDims[0]; k++)
    {
      for (int j = 0; j < cBlockDims[1]; j++)
      {
        for (int i = 0; i < cBlockDims[2]; i++)
        {
          int ind = pv_tensor6 * (k * cBlockDims[2] * cBlockDims[1] + j * cBlockDims[2] + i);
          dataArray->SetTuple6(cnt++, iData[ind], iData[ind + 1], iData[ind + 2], iData[ind + 3],
            iData[ind + 4], iData[ind + 5]);
        }
      }
    }
  }
  pDataSet->GetCellData()->AddArray(dataArray);
  dataArray->Delete();
  dataArray = nullptr;
}

void vtkAMRVelodyneReaderInternal::AttachTensorToGrid(
  int type, const char* attribute, int blockIdx, vtkUniformGrid* pDataSet)
{
  if (!this->Blocks[blockIdx].isLeaf)
  {
    return;
  }

  std::vector<int> iData;
  std::vector<double> fData;

  hid_t dType;
  vtkDataArray* dataArray = this->GetTypeAndArray(type, dType);

  dataArray->SetName(attribute);
  dataArray->SetNumberOfComponents(pv_tensor);
  bool isFullLeaf = this->Blocks[blockIdx].isFull;
  std::string groupName;
  std::vector<int> cBlockDims(3);
  if (isFullLeaf)
  {
    groupName = "AMR/FullLeaves";
  }
  else
  {
    groupName = "AMR/Leaves";
  }
  int bOff = this->Blocks[blockIdx].dSetLoc;
  hid_t grp_id = H5Gopen(this->file_id, groupName.c_str());
  hid_t ds_id = H5Dopen(grp_id, attribute);
  hsize_t data_dims[5];
  hsize_t mem_dims[5];
  hsize_t max_dims[5];
  hsize_t block_dims[5];
  hid_t dspace_id = H5Dget_space(ds_id);
  H5Sget_simple_extent_dims(dspace_id, data_dims, max_dims);
  cBlockDims[0] = data_dims[1];
  cBlockDims[1] = data_dims[2];
  cBlockDims[2] = data_dims[3];

  block_dims[0] = 1;
  block_dims[1] = cBlockDims[0];
  block_dims[2] = cBlockDims[1];
  block_dims[3] = cBlockDims[2];
  block_dims[4] = pv_tensor;

  hsize_t data_off[5];
  data_off[0] = bOff;
  data_off[1] = 0;
  data_off[2] = 0;
  data_off[3] = 0;
  data_off[4] = 0;
  hsize_t stride[5];
  stride[0] = 1;
  stride[1] = 1;
  stride[2] = 1;
  stride[3] = 1;
  stride[4] = 1;
  hsize_t count[5];
  count[0] = 1;
  count[1] = 1;
  count[2] = 1;
  count[3] = 1;
  count[4] = 1;
  H5Sselect_hyperslab(dspace_id, H5S_SELECT_SET, data_off, stride, count, block_dims);

  int nTuples = cBlockDims[0] * cBlockDims[1] * cBlockDims[2];

  mem_dims[0] = 1;
  mem_dims[1] = cBlockDims[0];
  mem_dims[2] = cBlockDims[1];
  mem_dims[3] = cBlockDims[2];
  mem_dims[4] = pv_tensor;
  hid_t mspace_id = H5Screate_simple(5, mem_dims, mem_dims);

  herr_t ierr;
  switch (type)
  {
    case pv_int:
    {
      iData.resize(pv_tensor * nTuples);
      ierr = H5Dread(ds_id, dType, mspace_id, dspace_id, H5P_DEFAULT, iData.data());
      if (ierr < 0)
      {
        vtkGenericWarningMacro("Cannot read " << std::string(attribute) << endl);
      }
    }
    break;
    case pv_double:
    {
      fData.resize(pv_tensor * nTuples);
      ierr = H5Dread(ds_id, dType, mspace_id, dspace_id, H5P_DEFAULT, fData.data());
      if (ierr < 0)
      {
        vtkGenericWarningMacro("Cannot read " << std::string(attribute) << endl);
      }
    }
    break;
  }
  H5Dclose(ds_id);
  H5Sclose(dspace_id);
  H5Sclose(mspace_id);
  H5Gclose(grp_id);

  dataArray->SetNumberOfTuples(nTuples);
  int cnt = 0;
  if (type == pv_double)
  {
    for (int k = 0; k < cBlockDims[0]; k++)
    {
      for (int j = 0; j < cBlockDims[1]; j++)
      {
        for (int i = 0; i < cBlockDims[2]; i++)
        {
          int ind = pv_tensor * (k * cBlockDims[2] * cBlockDims[1] + j * cBlockDims[2] + i);
          dataArray->SetTuple9(cnt++, fData[ind], fData[ind + 1], fData[ind + 2], fData[ind + 3],
            fData[ind + 4], fData[ind + 5], fData[ind + 6], fData[ind + 7], fData[ind + 8]);
        }
      }
    }
  }
  else
  {
    for (int k = 0; k < cBlockDims[0]; k++)
    {
      for (int j = 0; j < cBlockDims[1]; j++)
      {
        for (int i = 0; i < cBlockDims[2]; i++)
        {
          int ind = pv_tensor * (k * cBlockDims[2] * cBlockDims[1] + j * cBlockDims[2] + i);
          dataArray->SetTuple9(cnt++, iData[ind], iData[ind + 1], iData[ind + 2], iData[ind + 3],
            iData[ind + 4], iData[ind + 5], iData[ind + 6], iData[ind + 7], iData[ind + 8]);
        }
      }
    }
  }
  pDataSet->GetCellData()->AddArray(dataArray);
  dataArray->Delete();
  dataArray = nullptr;
}

int vtkAMRVelodyneReaderInternal::ReadLevelsAndX0(
  hid_t grp_id, std::vector<int>& levels, std::vector<double>& X0)
{
  hid_t dspace_id;
  hid_t mspace_id;
  hid_t ds_id;
  hid_t att_id;
  hsize_t mem_dims[2];
  hsize_t data_dims[2];
  hsize_t max_dims[2];
  att_id = H5Aopen(grp_id, "NBlocks", H5P_DEFAULT);
  int bSize;
  herr_t ierr = H5Aread(att_id, H5T_NATIVE_INT, &bSize);
  if (ierr < 0)
  {
    vtkGenericWarningMacro("Cannot Read NBlocks\n");
    return -1;
  }
  ierr = H5Aclose(att_id);
  levels.resize(bSize, 0);
  X0.resize(3 * bSize, 0);

  ds_id = H5Dopen(grp_id, "Level");
  dspace_id = H5Dget_space(ds_id);
  int nDims = H5Sget_simple_extent_dims(dspace_id, data_dims, max_dims);
  if (nDims != 1 || data_dims[0] != static_cast<unsigned int>(bSize))
  {
    vtkGenericWarningMacro("Wrong dimension for Level Array,expecting: 1X" << bSize << endl);
    return -1;
  }
  mem_dims[0] = bSize;
  mspace_id = H5Screate_simple(1, mem_dims, mem_dims);
  ierr = H5Dread(ds_id, H5T_NATIVE_INT, mspace_id, dspace_id, H5P_DEFAULT, levels.data());
  if (ierr < 0)
  {
    vtkGenericWarningMacro("Cannot Read Level Data\n");
    return -1;
  }
  ierr = H5Sclose(dspace_id);
  ierr = H5Sclose(mspace_id);
  ierr = H5Dclose(ds_id);
  // Close Levels
  // Open X0
  ds_id = H5Dopen(grp_id, "X0");
  dspace_id = H5Dget_space(ds_id);
  nDims = H5Sget_simple_extent_dims(dspace_id, data_dims, max_dims);
  if (nDims != 2 || data_dims[0] != static_cast<unsigned int>(bSize) || data_dims[1] != 3)
  {
    vtkGenericWarningMacro("Wrong dimension for X0 Array\n");
    return -1;
  }
  mem_dims[0] = 3 * bSize;
  mspace_id = H5Screate_simple(1, mem_dims, mem_dims);
  ierr = H5Dread(ds_id, H5T_NATIVE_DOUBLE, mspace_id, dspace_id, H5P_DEFAULT, X0.data());
  if (ierr < 0)
  {
    vtkGenericWarningMacro("Cannot Read X0 Data\n");
    return -1;
  }
  ierr = H5Sclose(dspace_id);
  ierr = H5Sclose(mspace_id);
  ierr = H5Dclose(ds_id);
  return bSize;
}

herr_t vtkAMRVelodyneReaderInternal::CloseFile(hid_t& fid)
{
  herr_t ierr = H5Fclose(fid);
  fid = -1;
  this->Blocks.clear();
  return ierr;
}

vtkDataArray* vtkAMRVelodyneReaderInternal::GetTypeAndArray(int type, hid_t& dType)
{
  vtkDataArray* dataArray;
  switch (type)
  {
    case pv_int:
    {
      dataArray = vtkIntArray::New();
      dType = H5T_NATIVE_INT;
      break;
    }
    case pv_double:
    {
      dataArray = vtkDoubleArray::New();
      dType = H5T_NATIVE_DOUBLE;
      break;
    }
    default:
      vtkGenericWarningMacro("Unknown Data Type Using Double\n");
      dataArray = vtkDoubleArray::New();
      dType = H5T_NATIVE_DOUBLE;
      break;
  }
  return dataArray;
}
VTK_ABI_NAMESPACE_END
