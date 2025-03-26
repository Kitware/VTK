// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkCONVERGECFDCGNSReader.h"

#include "vtkArrayDispatch.h"
#include "vtkCGNSReader.h"
#include "vtkCellArray.h"
#include "vtkCommand.h"
#include "vtkConvertToPartitionedDataSetCollection.h"
#include "vtkDataArray.h"
#include "vtkDataArrayRange.h"
#include "vtkDataArraySelection.h"
#include "vtkDataAssembly.h"
#include "vtkDoubleArray.h"
#include "vtkFloatArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPartitionedDataSetCollection.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkPolyVertex.h"
#include "vtkSMPTools.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkTypeInt32Array.h"
#include "vtkTypeInt64Array.h"

#include "cgio_helpers.h"

#include <vector>

VTK_ABI_NAMESPACE_BEGIN
namespace
{
//------------------------------------------------------------------------------
template <vtkIdType NbComps>
struct CreateDataArray
{
  template <typename OutArray, typename ValueType = vtk::GetAPIType<OutArray>>
  void operator()(OutArray* output, const std::vector<std::vector<ValueType>>& data)
  {
    if (data.size() < NbComps)
    {
      vtkErrorWithObjectMacro(nullptr, "Not enough data to create an array.");
      return;
    }

    output->SetNumberOfComponents(NbComps);
    output->SetNumberOfTuples(data[0].size());
    auto range = vtk::DataArrayValueRange<NbComps>(output);

    vtkSMPTools::For(
      0, static_cast<vtkIdType>(data[0].size()), [&](vtkIdType begin, vtkIdType end) {
        for (vtkIdType idx = begin; idx < end; idx++)
        {
          for (vtkIdType comp = 0; comp < NbComps; comp++)
          {
            range[NbComps * idx + comp] = data[comp][idx];
          }
        }
      });
  }
};
}

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkCONVERGECFDCGNSReader);

//----------------------------------------------------------------------------
vtkCONVERGECFDCGNSReader::vtkCONVERGECFDCGNSReader()
{
  this->SetNumberOfInputPorts(0);

  // Make sure to read boundary conditions
  this->CGNSReader->SetLoadBndPatch(true);

  this->PointDataArraySelection->AddObserver(
    vtkCommand::ModifiedEvent, this, &vtkCONVERGECFDCGNSReader::Modified);
  this->CellDataArraySelection->AddObserver(
    vtkCommand::ModifiedEvent, this, &vtkCONVERGECFDCGNSReader::Modified);
  this->ParcelDataArraySelection->AddObserver(
    vtkCommand::ModifiedEvent, this, &vtkCONVERGECFDCGNSReader::Modified);
}

//----------------------------------------------------------------------------
int vtkCONVERGECFDCGNSReader::RequestInformation(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  if (this->FileName.empty())
  {
    vtkWarningMacro("Filename is empty.");
    return 1;
  }

  if (this->DataArraysInitialized)
  {
    return 1;
  }

  this->CGNSReader->SetFileName(this->FileName.c_str());

  // Get point and cell data arrays from CGNS reader
  this->CGNSReader->ProcessRequest(request, inputVector, outputVector);
  this->PointDataArraySelection->DeepCopy(this->CGNSReader->GetPointDataArraySelection());
  this->CellDataArraySelection->DeepCopy(this->CGNSReader->GetCellDataArraySelection());
  this->PointDataArraySelection->EnableAllArrays();
  this->CellDataArraySelection->EnableAllArrays();

  // Go over all parcels and search for data arrays at the correct level
  // Since arrays can be present in several parcels but different between parcels,
  // check for existence before adding to selection

  // Use CGIO routine to open the file and find the zones
  int cgioId = 0;
  if (cgio_open_file(this->FileName.c_str(), CGIO_MODE_READ, CG_FILE_NONE, &cgioId) != CG_OK)
  {
    vtkWarningMacro("Could not open CGNS file with CGIO. Parcels will be ignored.");
    return 1;
  }

  double rootId = 0.0;
  if (cgio_get_root_id(cgioId, &rootId) != CG_OK)
  {
    vtkWarningMacro("Could not get root ID for the CGNS file. Parcels will be ignored.");
    return 1;
  }

  std::vector<double> baseIds;
  if (CGNSRead::readBaseIds(cgioId, rootId, baseIds) != 0)
  {
    vtkWarningMacro("Could not find base IDs for the CGNS file. Parcels will be ignored.");
    return 1;
  }

  // Loop over bases
  for (double baseId : baseIds)
  {
    // Search for zones under bases
    std::vector<double> baseChildIds;
    CGNSRead::getNodeChildrenId(cgioId, baseId, baseChildIds);

    for (double baseChildId : baseChildIds)
    {
      char nodeLabel[CGIO_MAX_NAME_LENGTH + 1];
      if (cgio_get_label(cgioId, baseChildId, nodeLabel) != CG_OK)
      {
        vtkWarningMacro("Could not get label for node " << baseChildId << ". Ignoring.");
        continue;
      }

      if (strcmp(nodeLabel, "Zone_t") != 0)
      {
        cgio_release_id(cgioId, baseChildId);
        continue;
      }

      // Search for UserDefinedData_t child nodes named "PARCEL_DATA" in current zone
      std::vector<double> zoneChildIds;
      CGNSRead::getNodeChildrenId(cgioId, baseChildId, zoneChildIds);

      for (double zoneChildId : zoneChildIds)
      {
        if (cgio_get_label(cgioId, zoneChildId, nodeLabel) != CG_OK)
        {
          vtkWarningMacro("Could not get label for node " << zoneChildId << ". Ignoring.");
          continue;
        }

        if (strcmp(nodeLabel, "UserDefinedData_t") != 0)
        {
          cgio_release_id(cgioId, zoneChildId);
          continue;
        }

        char nodeName[CGIO_MAX_NAME_LENGTH + 1];
        if (cgio_get_name(cgioId, zoneChildId, nodeName) != CG_OK)
        {
          vtkWarningMacro("Could not get name for node " << zoneChildId << ". Ignoring.");
          continue;
        }

        if (strcmp(nodeName, "PARCEL_DATA") != 0)
        {
          cgio_release_id(cgioId, zoneChildId);
          continue;
        }

        // Go down three levels and check for UserDefinedData_t nodes with names
        // different from "PARCEL_X", "PARCEL_Y" and "PARCEL_Z" (these are coordinates).
        // Ignore them if they do not have children DataArray_t nodes themselves.
        // Names ending with "_X", "_Y" and , "_Z" correspond to vectors.

        // First level is the type of parcel (liquid, solid, gas)
        std::vector<double> parcelTypesIds;
        CGNSRead::getNodeChildrenId(cgioId, zoneChildId, parcelTypesIds);

        for (double parcelTypesId : parcelTypesIds)
        {
          // Second level is the parcel name
          std::vector<double> parcelNamesIds;
          CGNSRead::getNodeChildrenId(cgioId, parcelTypesId, parcelNamesIds);

          for (double parcelNamesId : parcelNamesIds)
          {
            // Third level is the array name
            std::vector<double> arrayIds;
            CGNSRead::getNodeChildrenId(cgioId, parcelNamesId, arrayIds);

            for (double arrayId : arrayIds)
            {
              // Check whether a child DataArray_t node with the actual values exists
              // If it does not, it means that no values are available for this file (timestep)
              std::vector<double> dataIds;
              CGNSRead::getNodeChildrenId(cgioId, arrayId, dataIds);

              if (dataIds.empty())
              {
                continue;
              }

              if (cgio_get_name(cgioId, arrayId, nodeName) != CG_OK)
              {
                vtkWarningMacro("Could not get name for node " << arrayId << ". Ignoring.");
                continue;
              }

              std::string fullName = nodeName;
              std::string nameOnly = fullName.substr(0, fullName.size() - 2);
              std::string nameSuffix = fullName.substr(fullName.size() - 2, 2);

              // "PARCEL" nodes correspond to particle coordinates
              if (nameOnly == "PARCEL")
              {
                continue;
              }
              else if (nameSuffix == "_X")
              {
                if (!this->ParcelDataArraySelection->ArrayExists(nameOnly.c_str()))
                {
                  this->ParcelDataArraySelection->AddArray(nameOnly.c_str());
                }
              }
              else if (nameSuffix == "_Y" || nameSuffix == "_Z")
              {
                cgio_release_id(cgioId, dataIds[0]);
                continue;
              }
              else
              {
                if (!this->ParcelDataArraySelection->ArrayExists(fullName.c_str()))
                {
                  this->ParcelDataArraySelection->AddArray(fullName.c_str());
                }
              }
            }
          }
        }

        // There is only one "PARCEL_DATA" node per zone
        break;
      }
    }
  }

  cgio_close_file(cgioId);
  this->DataArraysInitialized = true;
  return 1;
}

//----------------------------------------------------------------------------
int vtkCONVERGECFDCGNSReader::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector), vtkInformationVector* outputVector)
{
  if (this->FileName.empty())
  {
    vtkWarningMacro("Empty filename.");
    return 1;
  }

  vtkPartitionedDataSetCollection* output =
    vtkPartitionedDataSetCollection::GetData(outputVector, 0);

  if (!output)
  {
    vtkErrorMacro("Missing output.");
    return 0;
  }

  // Transfer information to the CGNS reader regarding selected arrays
  this->CGNSReader->GetPointDataArraySelection()->CopySelections(this->PointDataArraySelection);
  this->CGNSReader->GetCellDataArraySelection()->CopySelections(this->CellDataArraySelection);

  // Convert to partitioned dataset collection
  vtkNew<vtkConvertToPartitionedDataSetCollection> converter;
  converter->SetInputConnection(this->CGNSReader->GetOutputPort());
  converter->Update();

  vtkPartitionedDataSetCollection* cgnsOutput = converter->GetOutput();

  if (!cgnsOutput)
  {
    vtkErrorMacro("CGNS reader output is invalid.");
    return 0;
  }

  // Retrieve collection assembly so parcels can be added
  vtkDataAssembly* hierarchy = cgnsOutput->GetDataAssembly();

  // change root node name to "assembly"
  hierarchy->SetRootNodeName("assembly");
  hierarchy->SetAttribute(vtkDataAssembly::GetRootNode(), "label", "assembly");

  // Use CGIO routine to find the zones
  int cgioId = 0;
  if (cgio_open_file(this->FileName.c_str(), CGIO_MODE_READ, CG_FILE_NONE, &cgioId) != CG_OK)
  {
    vtkWarningMacro("Could not open CGNS file with CGIO. Parcels will be ignored.");
    return 1;
  }

  double rootId = 0.0;
  if (cgio_get_root_id(cgioId, &rootId) != CG_OK)
  {
    vtkWarningMacro("Could not get root ID for the CGNS file. Parcels will be ignored.");
    return 1;
  }

  std::vector<double> baseIds;
  if (CGNSRead::readBaseIds(cgioId, rootId, baseIds) != 0)
  {
    vtkWarningMacro("Could not find base IDs for the CGNS file. Parcels will be ignored.");
    return 1;
  }

  // Loop over bases to find parcel nodes
  // Parcel points are first created as vertex cells
  // Data arrays on parcels are then read and created
  for (double baseId : baseIds)
  {
    // Retrieve base name and assembly index
    char nodeName[CGIO_MAX_NAME_LENGTH + 1];
    if (cgio_get_name(cgioId, baseId, nodeName) != CG_OK)
    {
      vtkWarningMacro("Could not get name for node " << baseId << ". Ignoring.");
      continue;
    }

    int baseAssemblyId = hierarchy->FindFirstNodeWithName(nodeName);
    if (baseAssemblyId == -1)
    {
      vtkWarningMacro("Could not find assembly node '" << nodeName << "'. Ignoring.");
      continue;
    }

    std::vector<int> baseChildAssemblyIds = hierarchy->GetChildNodes(baseAssemblyId);

    // Search for zones under bases
    std::vector<double> baseChildIds;
    CGNSRead::getNodeChildrenId(cgioId, baseId, baseChildIds);

    for (double baseChildId : baseChildIds)
    {
      char nodeLabel[CGIO_MAX_NAME_LENGTH + 1];
      if (cgio_get_label(cgioId, baseChildId, nodeLabel) != CG_OK)
      {
        vtkWarningMacro("Could not get label for node " << baseChildId << ". Ignoring.");
        continue;
      }

      if (strcmp(nodeLabel, "Zone_t") != 0)
      {
        cgio_release_id(cgioId, baseChildId);
        continue;
      }

      // Retrieve zone name and assembly index
      if (cgio_get_name(cgioId, baseChildId, nodeName) != CG_OK)
      {
        vtkWarningMacro("Could not get name for node " << baseChildId << ". Ignoring.");
        continue;
      }

      // Since zones in different bases can have the same name, search among
      // the base child nodes
      int zoneAssemblyId = -1;

      for (int id : baseChildAssemblyIds)
      {
        if (strcmp(hierarchy->GetNodeName(id), nodeName) == 0)
        {
          zoneAssemblyId = id;
          break;
        }
      }

      if (zoneAssemblyId == -1)
      {
        vtkWarningMacro("Could not find assembly node '" << nodeName << "'. Ignoring.");
        continue;
      }

      const auto zoneChildren = hierarchy->GetChildNodes(zoneAssemblyId);
      for (const auto zoneChild : zoneChildren)
      {
        const auto childName = hierarchy->GetNodeName(zoneChild);
        // 1) change the name of the "Internal" node to "Mesh"
        if (strcmp(childName, "Internal") == 0)
        {
          hierarchy->SetNodeName(zoneChild, "Mesh");
          hierarchy->SetAttribute(zoneChild, "label", "Mesh");
          // also change the vtkCompositeDataSet::NAME() metadata
          unsigned int partitionedDataSetId = hierarchy->GetDataSetIndices(zoneChild).front();
          if (cgnsOutput->HasMetaData(partitionedDataSetId) &&
            cgnsOutput->GetMetaData(partitionedDataSetId)->Has(vtkCompositeDataSet::NAME()))
          {
            cgnsOutput->GetMetaData(partitionedDataSetId)->Set(vtkCompositeDataSet::NAME(), "Mesh");
          }
        }
        // 2) change the name of the "Patches" node to "Surfaces"
        else if (strcmp(childName, "Patches") == 0)
        {
          hierarchy->SetNodeName(zoneChild, "Surfaces");
          hierarchy->SetAttribute(zoneChild, "label", "Surfaces");
        }
      }

      // Search for UserDefinedData_t child nodes named "PARCEL_DATA" in current zone
      std::vector<double> zoneChildIds;
      CGNSRead::getNodeChildrenId(cgioId, baseChildId, zoneChildIds);

      for (double zoneChildId : zoneChildIds)
      {
        if (cgio_get_label(cgioId, zoneChildId, nodeLabel) != CG_OK)
        {
          vtkWarningMacro("Could not get label for node " << zoneChildId << ". Ignoring.");
          continue;
        }

        if (strcmp(nodeLabel, "UserDefinedData_t") != 0)
        {
          cgio_release_id(cgioId, zoneChildId);
          continue;
        }

        if (cgio_get_name(cgioId, zoneChildId, nodeName) != CG_OK)
        {
          vtkWarningMacro("Could not get name for node " << zoneChildId << ". Ignoring.");
          continue;
        }

        if (strcmp(nodeName, "PARCEL_DATA") != 0)
        {
          cgio_release_id(cgioId, zoneChildId);
          continue;
        }

        // Add a "Parcels" node under the current zone
        int parcelDataAssemblyId = hierarchy->AddNode("Parcels", zoneAssemblyId);

        // Go down three levels and check for UserDefinedData_t nodes.
        // Parcel points coordinates are described in "PARCEL_X", "PARCEL_Y" and
        // "PARCEL_Z" nodes.
        // Other nodes are data arrays defined on these points.
        // Ignore them if they do not have children DataArray_t nodes themselves.
        // Names ending with "_X", "_Y" and , "_Z" correspond to vectors.

        // First level is the type of parcel (liquid, solid, gas)
        std::vector<double> parcelTypesIds;
        CGNSRead::getNodeChildrenId(cgioId, zoneChildId, parcelTypesIds);

        for (double parcelTypesId : parcelTypesIds)
        {
          // Retrieve parcel type name
          if (cgio_get_name(cgioId, parcelTypesId, nodeName) != CG_OK)
          {
            vtkWarningMacro("Could not get name for node " << parcelTypesId << ". Ignoring.");
            continue;
          }

          int parcelTypeAssemblyId = hierarchy->AddNode(nodeName, parcelDataAssemblyId);

          // Second level is the parcel name
          std::vector<double> parcelNamesIds;
          CGNSRead::getNodeChildrenId(cgioId, parcelTypesId, parcelNamesIds);

          for (double parcelNamesId : parcelNamesIds)
          {
            // Retrieve actual parcel name
            if (cgio_get_name(cgioId, parcelNamesId, nodeName) != CG_OK)
            {
              vtkWarningMacro("Could not get name for node " << parcelNamesId << ". Ignoring.");
              continue;
            }

            int parcelAssemblyId = hierarchy->AddNode(nodeName, parcelTypeAssemblyId);

            // Add poly data
            vtkNew<vtkPolyData> parcel;
            int nbPDS = cgnsOutput->GetNumberOfPartitionedDataSets();
            cgnsOutput->SetNumberOfPartitionedDataSets(nbPDS + 1);
            cgnsOutput->SetPartition(nbPDS, 0, parcel);
            cgnsOutput->GetMetaData(nbPDS)->Set(vtkCompositeDataSet::NAME(), nodeName);
            hierarchy->AddDataSetIndex(parcelAssemblyId, nbPDS);

            // Third level is the array name
            std::vector<double> arrayIds;
            CGNSRead::getNodeChildrenId(cgioId, parcelNamesId, arrayIds);

            // Search for "PARCEL_X", "PARCEL_Y" and "PARCEL_Z" to create parcel points
            double parcelXId = -1.0, parcelYId = -1.0, parcelZId = -1.0;
            this->FindVectorNodeIds(cgioId, arrayIds, "PARCEL", parcelXId, parcelYId, parcelZId);

            if (parcelXId == -1.0 || parcelYId == -1.0 || parcelZId == -1.0)
            {
              vtkWarningMacro("One of the coordinates nodes for parcels is missing. Ignoring.");
              continue;
            }

            if (!this->CreateParcelPoints(cgioId, parcelXId, parcelYId, parcelZId, parcel))
            {
              vtkErrorMacro("Could not create parcels for node '" << nodeName << "'.");
              continue;
            }

            // Loop over remaining nodes to check if parcel data arrays are available
            for (double arrayId : arrayIds)
            {
              // Check whether a child DataArray_t node with the actual values exists
              // If it does not, it means that no values are available for this file (timestep)
              std::vector<double> dataIds;
              CGNSRead::getNodeChildrenId(cgioId, arrayId, dataIds);

              if (dataIds.empty())
              {
                continue;
              }

              if (cgio_get_name(cgioId, arrayId, nodeName) != CG_OK)
              {
                vtkWarningMacro("Could not get name for node " << arrayId << ". Ignoring.");
                continue;
              }

              std::string fullName = nodeName;
              std::string nameOnly = fullName.substr(0, fullName.size() - 2);
              std::string nameSuffix = fullName.substr(fullName.size() - 2, 2);

              // Ignore "PARCEL_X/Y/Z" coordinate nodes
              if (nameOnly == "PARCEL")
              {
                continue;
              }

              // Check that the array has been selected
              bool isVector = (nameSuffix == "_X" || nameSuffix == "_Y" || nameSuffix == "_Z");
              std::string arrayName = isVector ? nameOnly : fullName;

              if (this->ParcelDataArraySelection->ArrayIsEnabled(arrayName.c_str()) == 0)
              {
                continue;
              }

              // Create parcel data array
              vtkSmartPointer<vtkDataArray> array =
                this->ReadParcelDataArray(cgioId, dataIds[0], arrayName, arrayIds, isVector);

              if (array)
              {
                parcel->GetPointData()->AddArray(array);
              }
            }
          }
        }

        // There is only one "PARCEL_DATA" node per zone
        break;
      }
    }
  }

  output->ShallowCopy(cgnsOutput);
  cgio_close_file(cgioId);

  return 1;
}

//----------------------------------------------------------------------------
void vtkCONVERGECFDCGNSReader::FindVectorNodeIds(int cgioId, const std::vector<double>& arrayIds,
  const std::string& prefix, double& vectorXId, double& vectorYId, double& vectorZId) const
{
  const std::string xName = prefix + "_X";
  const std::string yName = prefix + "_Y";
  const std::string zName = prefix + "_Z";

  for (double arrayId : arrayIds)
  {
    char arrayName[CGIO_MAX_NAME_LENGTH + 1];

    if (cgio_get_name(cgioId, arrayId, arrayName) != CG_OK)
    {
      vtkWarningMacro("Could not get name for node " << arrayId << ". Ignoring.");
      continue;
    }

    if (strcmp(arrayName, xName.c_str()) == 0)
    {
      vectorXId = arrayId;
      continue;
    }
    else if (strcmp(arrayName, yName.c_str()) == 0)
    {
      vectorYId = arrayId;
      continue;
    }
    else if (strcmp(arrayName, zName.c_str()) == 0)
    {
      vectorZId = arrayId;
      continue;
    }
  }
}

//----------------------------------------------------------------------------
bool vtkCONVERGECFDCGNSReader::CreateParcelPoints(
  int cgioId, double parcelXId, double parcelYId, double parcelZId, vtkPolyData* parcel) const
{
  // Check whether a non empty child DataArray_t node exists
  // If it does not, it means that the coordinates are defined in another node
  std::vector<double> dataIdsX;
  std::vector<double> dataIdsY;
  std::vector<double> dataIdsZ;
  CGNSRead::getNodeChildrenId(cgioId, parcelXId, dataIdsX);
  CGNSRead::getNodeChildrenId(cgioId, parcelYId, dataIdsY);
  CGNSRead::getNodeChildrenId(cgioId, parcelZId, dataIdsZ);

  if (dataIdsX.empty() && dataIdsY.empty() && dataIdsZ.empty())
  {
    return true;
  }

  // Determine data type
  CGNSRead::char_33 dataType;

  if (cgio_get_data_type(cgioId, dataIdsX[0], dataType) != CG_OK)
  {
    vtkErrorMacro("Could not read node data type.");
    return false;
  }

  // Create coordinates array based on type
  using SupportedTypes = vtkTypeList::Create<vtkFloatArray, vtkDoubleArray>;
  using Dispatcher = vtkArrayDispatch::DispatchByArray<SupportedTypes>;
  vtkSmartPointer<vtkDataArray> array;
  CreateDataArray<3> worker;

  if (strcmp(dataType, "R4") == 0)
  {
    std::vector<float> dataX;
    std::vector<float> dataY;
    std::vector<float> dataZ;
    CGNSRead::readNodeData<float>(cgioId, dataIdsX[0], dataX);
    CGNSRead::readNodeData<float>(cgioId, dataIdsY[0], dataY);
    CGNSRead::readNodeData<float>(cgioId, dataIdsZ[0], dataZ);
    std::vector<std::vector<float>> data{ dataX, dataY, dataZ };
    array.TakeReference(vtkDataArray::FastDownCast(vtkAbstractArray::CreateArray(VTK_FLOAT)));
    Dispatcher::Execute(array, worker, data);
  }
  else if (strcmp(dataType, "R8") == 0)
  {
    std::vector<double> dataX;
    std::vector<double> dataY;
    std::vector<double> dataZ;
    CGNSRead::readNodeData<double>(cgioId, dataIdsX[0], dataX);
    CGNSRead::readNodeData<double>(cgioId, dataIdsY[0], dataY);
    CGNSRead::readNodeData<double>(cgioId, dataIdsZ[0], dataZ);
    std::vector<std::vector<double>> data{ dataX, dataY, dataZ };
    array.TakeReference(vtkDataArray::FastDownCast(vtkAbstractArray::CreateArray(VTK_DOUBLE)));
    Dispatcher::Execute(array, worker, data);
  }
  else
  {
    vtkWarningMacro(
      "Encountered data type different from float or double for the parcel coordinates.");
    return false;
  }

  // Set up points
  vtkNew<vtkPoints> points;
  points->SetData(array);
  parcel->SetPoints(points);
  vtkIdType nbPoints = points->GetNumberOfPoints();

  // Define vertex cells for each point
  vtkNew<vtkCellArray> cells;
  vtkNew<vtkPolyVertex> polyVertex;
  cells->AllocateExact(1, nbPoints);
  polyVertex->GetPointIds()->SetNumberOfIds(nbPoints);

  for (vtkIdType id = 0; id < nbPoints; id++)
  {
    polyVertex->GetPointIds()->SetId(id, id);
  }

  cells->InsertNextCell(polyVertex);
  parcel->SetVerts(cells);

  return true;
}

//----------------------------------------------------------------------------
vtkSmartPointer<vtkDataArray> vtkCONVERGECFDCGNSReader::ReadParcelDataArray(int cgioId,
  double dataNodeId, const std::string& name, const std::vector<double>& arrayIds,
  bool isVector) const
{
  // Determine data type
  CGNSRead::char_33 dataType;

  if (cgio_get_data_type(cgioId, dataNodeId, dataType) != CG_OK)
  {
    vtkErrorMacro("Could not read node data type.");
    return nullptr;
  }

  // If the array is a vectorial quantity, find all three nodes
  double vectorXId = -1.0, vectorYId = -1.0, vectorZId = -1.0;
  std::vector<double> dataIdsX;
  std::vector<double> dataIdsY;
  std::vector<double> dataIdsZ;

  if (isVector)
  {
    // Find node ID for each vector component
    this->FindVectorNodeIds(cgioId, arrayIds, name, vectorXId, vectorYId, vectorZId);

    if (vectorXId == -1.0 || vectorYId == -1.0 || vectorZId == -1.0)
    {
      vtkWarningMacro("One of the nodes for vector '" << name << "' is missing. Skipping.");
      return nullptr;
    }

    // Retrieve actual array values and check whether they are defined
    CGNSRead::getNodeChildrenId(cgioId, vectorXId, dataIdsX);
    CGNSRead::getNodeChildrenId(cgioId, vectorYId, dataIdsY);
    CGNSRead::getNodeChildrenId(cgioId, vectorZId, dataIdsZ);

    if (dataIdsX.empty() && dataIdsY.empty() && dataIdsZ.empty())
    {
      return nullptr;
    }
  }

  // Read data according to type
  using SupportedTypes =
    vtkTypeList::Create<vtkTypeInt32Array, vtkTypeInt64Array, vtkFloatArray, vtkDoubleArray>;
  using Dispatcher = vtkArrayDispatch::DispatchByArray<SupportedTypes>;
  vtkSmartPointer<vtkDataArray> array;
  CreateDataArray<1> scalarWorker;
  CreateDataArray<3> vectorWorker;

  if (strcmp(dataType, "I4") == 0)
  {
    array.TakeReference(vtkDataArray::FastDownCast(vtkAbstractArray::CreateArray(VTK_TYPE_INT32)));

    if (isVector)
    {
      std::vector<vtkTypeInt32> dataX;
      std::vector<vtkTypeInt32> dataY;
      std::vector<vtkTypeInt32> dataZ;
      CGNSRead::readNodeData<vtkTypeInt32>(cgioId, dataIdsX[0], dataX);
      CGNSRead::readNodeData<vtkTypeInt32>(cgioId, dataIdsY[0], dataY);
      CGNSRead::readNodeData<vtkTypeInt32>(cgioId, dataIdsZ[0], dataZ);
      std::vector<std::vector<vtkTypeInt32>> data{ dataX, dataY, dataZ };
      Dispatcher::Execute(array, vectorWorker, data);
    }
    else
    {
      std::vector<vtkTypeInt32> dataVec;
      CGNSRead::readNodeData<vtkTypeInt32>(cgioId, dataNodeId, dataVec);
      std::vector<std::vector<vtkTypeInt32>> data{ dataVec };
      Dispatcher::Execute(array, scalarWorker, data);
    }
  }
  else if (strcmp(dataType, "I8") == 0)
  {
    array.TakeReference(vtkDataArray::FastDownCast(vtkAbstractArray::CreateArray(VTK_TYPE_INT64)));

    if (isVector)
    {
      std::vector<vtkTypeInt64> dataX;
      std::vector<vtkTypeInt64> dataY;
      std::vector<vtkTypeInt64> dataZ;
      CGNSRead::readNodeData<vtkTypeInt64>(cgioId, dataIdsX[0], dataX);
      CGNSRead::readNodeData<vtkTypeInt64>(cgioId, dataIdsY[0], dataY);
      CGNSRead::readNodeData<vtkTypeInt64>(cgioId, dataIdsZ[0], dataZ);
      std::vector<std::vector<vtkTypeInt64>> data{ dataX, dataY, dataZ };
      Dispatcher::Execute(array, vectorWorker, data);
    }
    else
    {
      std::vector<vtkTypeInt64> dataVec;
      CGNSRead::readNodeData<vtkTypeInt64>(cgioId, dataNodeId, dataVec);
      std::vector<std::vector<vtkTypeInt64>> data{ dataVec };
      Dispatcher::Execute(array, scalarWorker, data);
    }
  }
  else if (strcmp(dataType, "R4") == 0)
  {
    array.TakeReference(vtkDataArray::FastDownCast(vtkAbstractArray::CreateArray(VTK_FLOAT)));

    if (isVector)
    {
      std::vector<float> dataX;
      std::vector<float> dataY;
      std::vector<float> dataZ;
      CGNSRead::readNodeData<float>(cgioId, dataIdsX[0], dataX);
      CGNSRead::readNodeData<float>(cgioId, dataIdsY[0], dataY);
      CGNSRead::readNodeData<float>(cgioId, dataIdsZ[0], dataZ);
      std::vector<std::vector<float>> data{ dataX, dataY, dataZ };
      Dispatcher::Execute(array, vectorWorker, data);
    }
    else
    {
      std::vector<float> dataVec;
      CGNSRead::readNodeData<float>(cgioId, dataNodeId, dataVec);
      std::vector<std::vector<float>> data{ dataVec };
      Dispatcher::Execute(array, scalarWorker, data);
    }
  }
  else if (strcmp(dataType, "R8") == 0)
  {
    array.TakeReference(vtkDataArray::FastDownCast(vtkAbstractArray::CreateArray(VTK_DOUBLE)));

    if (isVector)
    {
      std::vector<double> dataX;
      std::vector<double> dataY;
      std::vector<double> dataZ;
      CGNSRead::readNodeData<double>(cgioId, dataIdsX[0], dataX);
      CGNSRead::readNodeData<double>(cgioId, dataIdsY[0], dataY);
      CGNSRead::readNodeData<double>(cgioId, dataIdsZ[0], dataZ);
      std::vector<std::vector<double>> data{ dataX, dataY, dataZ };
      Dispatcher::Execute(array, vectorWorker, data);
    }
    else
    {
      std::vector<double> dataVec;
      CGNSRead::readNodeData<double>(cgioId, dataNodeId, dataVec);
      std::vector<std::vector<double>> data{ dataVec };
      Dispatcher::Execute(array, scalarWorker, data);
    }
  }
  else
  {
    vtkWarningMacro("Encountered data type different from int32, int64, float or double for parcel "
                    "data array. Skipping.");
    return nullptr;
  }

  array->SetName(name.c_str());
  return array;
}

//----------------------------------------------------------------------------
int vtkCONVERGECFDCGNSReader::CanReadFile(const std::string& filename)
{
  return this->CGNSReader->CanReadFile(filename.c_str());
}

//----------------------------------------------------------------------------
void vtkCONVERGECFDCGNSReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "FileName: " << this->FileName << endl;
  os << indent << "DataArraysInitialized: " << this->DataArraysInitialized << endl;

  this->PointDataArraySelection->PrintSelf(os, indent.GetNextIndent());
  this->PointDataArraySelection->PrintSelf(os, indent.GetNextIndent());
  this->ParcelDataArraySelection->PrintSelf(os, indent.GetNextIndent());
  this->CGNSReader->PrintSelf(os, indent.GetNextIndent());
}

VTK_ABI_NAMESPACE_END
