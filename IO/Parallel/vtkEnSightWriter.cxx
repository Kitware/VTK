// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
/* TODO
 *
 *
 * check that data was written
 *
 */

#include "vtkEnSightWriter.h"

#include <vtksys/SystemTools.hxx>

#include "vtkBitArray.h"
#include "vtkByteSwap.h"
#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkCharArray.h"
#include "vtkCommand.h"
#include "vtkDataSet.h"
#include "vtkDoubleArray.h"
#include "vtkErrorCode.h"
#include "vtkFieldData.h"
#include "vtkFloatArray.h"
#include "vtkInformation.h"
#include "vtkIntArray.h"
#include "vtkLogger.h"
#include "vtkLongArray.h"
#include "vtkLookupTable.h"
#include "vtkMath.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkShortArray.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnsignedIntArray.h"
#include "vtkUnsignedLongArray.h"
#include "vtkUnsignedShortArray.h"

#include "vtkPriorityQueue.h"

#include "vtkUnstructuredGrid.h"

#include <cctype>
#include <cerrno>
#include <cmath>

#include <algorithm>
#include <list>
#include <map>
#include <vector>

// If we are building against a slightly older VTK version,
// these cell types are not defined, and won't occur in the input

#ifndef VTK_QUADRATIC_WEDGE
#define VTK_QUADRATIC_WEDGE 26
#define VTK_QUADRATIC_PYRAMID 27
#endif

//------------------------------------------------------------------------------
VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkEnSightWriter);

//------------------------------------------------------------------------------
// Created object with no filename and timestep 0
vtkEnSightWriter::vtkEnSightWriter()
{

  this->BaseName = nullptr;
  this->FileName = nullptr;
  this->TimeStep = 0;
  this->Path = nullptr;
  this->GhostLevelMultiplier = 10000;
  this->GhostLevel = 0;
  this->WriteNodeIDs = true;
  this->WriteElementIDs = true;
  this->TransientGeometry = false;
  this->ProcessNumber = 0;
  this->NumberOfProcesses = 1;
  this->NumberOfBlocks = 0;
  this->BlockIDs = nullptr;
  this->TmpInput = nullptr;
}

//------------------------------------------------------------------------------
vtkEnSightWriter::~vtkEnSightWriter()
{
  this->SetBaseName(nullptr);
  this->SetFileName(nullptr);
  this->SetPath(nullptr);
}

//------------------------------------------------------------------------------
void vtkEnSightWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "FileName: " << (this->FileName ? this->FileName : "(none)") << "\n";
  os << indent << "Path: " << (this->Path ? this->Path : "(none)") << "\n";
  os << indent << "BaseName: " << (this->BaseName ? this->BaseName : "(none)") << "\n";

  os << indent << "TimeStep: " << this->TimeStep << "\n";
  os << indent << "TransientGeometry: " << this->TransientGeometry << "\n";
  os << indent << "ProcessNumber: " << this->ProcessNumber << endl;
  os << indent << "NumberOfProcesses: " << this->NumberOfProcesses << endl;
  os << indent << "NumberOfBlocks: " << this->NumberOfBlocks << endl;
  os << indent << "BlockIDs: " << this->BlockIDs << endl;
  os << indent << "GhostLevel: " << this->GhostLevel << endl;
  os << indent << "WriteNodeIDs: " << this->WriteNodeIDs << endl;
  os << indent << "WriteElementIDs: " << this->WriteElementIDs << endl;
}

int vtkEnSightWriter::FillInputPortInformation(int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkUnstructuredGrid");
  return 1;
}

//------------------------------------------------------------------------------
int vtkEnSightWriter::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector), vtkInformationVector* vtkNotUsed(outputVector))
{
  this->SetErrorCode(vtkErrorCode::NoError);

  vtkDataObject* input = this->GetInput();

  // make sure input is available
  if (!input)
  {
    vtkErrorMacro(<< "No input!");
    return 0;
  }

  this->InvokeEvent(vtkCommand::StartEvent, nullptr);

  this->WriteData();      // write geometry and variable files
  this->WriteCaseFile(1); // write .case file with one timestep (0)

  if (this->NumberOfProcesses > 1 && this->ProcessNumber == 0)
  {
    // write .sos file that contains paths to the .case pieces
    this->WriteSOSCaseFile(this->NumberOfProcesses);
  }

  this->InvokeEvent(vtkCommand::EndEvent, nullptr);

  this->WriteTime.Modified();

  return 1;
}

//------------------------------------------------------------------------------
// Specify the input data or filter.
void vtkEnSightWriter::SetInputData(vtkUnstructuredGrid* input)
{
  this->SetInputDataInternal(0, input);
}

//------------------------------------------------------------------------------
// Specify the input data or filter.
vtkUnstructuredGrid* vtkEnSightWriter::GetInput()
{
  if (this->GetNumberOfInputConnections(0) < 1)
  {
    return nullptr;
  }
  else if (this->TmpInput)
  {
    return this->TmpInput;
  }
  else
  {
    return (vtkUnstructuredGrid*)(this->Superclass::GetInput());
  }
}

//------------------------------------------------------------------------------
void vtkEnSightWriter::WriteData()
{
  int i;
  unsigned int ui;
  int blockCount = 0;
  std::list<int>::iterator iter;

  if (this->TmpInput)
  {
    this->TmpInput->Delete();
    this->TmpInput = nullptr;
  }

  // figure out process ID

  this->ProcessNumber = 0;
  this->NumberOfProcesses = 1;

  vtkMultiProcessController* c = vtkMultiProcessController::GetGlobalController();

  if (c != nullptr)
  {
    this->ProcessNumber = c->GetLocalProcessId();
    this->NumberOfProcesses = c->GetNumberOfProcesses();
  }

  vtkUnstructuredGrid* input = this->GetInput();
  vtkInformation* inInfo = this->GetInputInformation();

  if (this->GhostLevel >
    inInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS()))
  {
    // re-execute pipeline if necessary to obtain ghost cells

    this->GetInputAlgorithm()->UpdateInformation();
    inInfo->Set(
      vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(), this->GhostLevel);
    this->GetInputAlgorithm()->Update();
  }

  // get the BlockID Cell Array
  vtkDataArray* BlockData = input->GetCellData()->GetScalars("BlockId");

  if (BlockData == nullptr || strcmp(BlockData->GetName(), "BlockId") != 0)
  {
    BlockData = nullptr;
    vtkLog(WARNING, "No BlockID was found");
  }

  this->ComputeNames();

  if (!this->BaseName)
  {
    vtkErrorMacro("A FileName or Path/BaseName must be specified.");
    return;
  }

  this->SanitizeFileName(this->BaseName);

  char** blockNames = nullptr;
  int* elementIDs = nullptr;
  char charBuffer[1024];
  char fileBuffer[512];
  snprintf(charBuffer, sizeof(charBuffer), "%s/%s.%d.%05d.geo", this->Path, this->BaseName,
    this->ProcessNumber, this->TimeStep);

  // open the geometry file
  // only if timestep 0 and not transient geometry or transient geometry
  FILE* fd = nullptr;
  if (this->ShouldWriteGeometry())
  {
    if (!(fd = OpenFile(charBuffer)))
      return;
  }

  // Get the FILE's for Point Data Fields
  std::vector<FILE*> pointArrayFiles;
  int NumPointArrays = input->GetPointData()->GetNumberOfArrays();
  for (i = 0; i < NumPointArrays; i++)
  {
    strcpy(fileBuffer, input->GetPointData()->GetArray(i)->GetName());
    this->SanitizeFileName(fileBuffer);
    snprintf(charBuffer, sizeof(charBuffer), "%s/%s.%d.%05d_n.%s", this->Path, this->BaseName,
      this->ProcessNumber, this->TimeStep, fileBuffer);
    FILE* ftemp = OpenFile(charBuffer);
    if (!ftemp)
    {
      fclose(fd);
      return;
    }
    pointArrayFiles.push_back(ftemp);

    // write the description line to the file
    WriteStringToFile(fileBuffer, ftemp);
  }

  // Get the FILE's for Cell Data Fields
  std::vector<FILE*> cellArrayFiles;
  int NumCellArrays = input->GetCellData()->GetNumberOfArrays();
  for (i = 0; i < NumCellArrays; i++)
  {

    strcpy(fileBuffer, input->GetCellData()->GetArray(i)->GetName());
    this->SanitizeFileName(fileBuffer);
    snprintf(charBuffer, sizeof(charBuffer), "%s/%s.%d.%05d_c.%s", this->Path, this->BaseName,
      this->ProcessNumber, this->TimeStep, fileBuffer);
    FILE* ftemp = OpenFile(charBuffer);
    if (!ftemp)
    {
      fclose(fd);
      return;
    }
    cellArrayFiles.push_back(ftemp);

    // write the description line to the file
    WriteStringToFile(fileBuffer, ftemp);
  }

  // write the header information
  if (this->ShouldWriteGeometry())
  {
    this->WriteStringToFile("C Binary", fd);
    this->WriteStringToFile("Written by VTK EnSight Writer", fd);
    // if (this->Title)
    // this->WriteStringToFile(this->Title,fd);
    // else
    this->WriteStringToFile("No Title was Specified", fd);
    if (this->WriteNodeIDs)
    {
      this->WriteStringToFile("node id given\n", fd);
    }
    else
    {
      this->WriteStringToFile("node id off\n", fd);
    }
    if (this->WriteElementIDs)
    {
      this->WriteStringToFile("element id given\n", fd);
    }
    else
    {
      this->WriteStringToFile("element id off\n", fd);
    }
  }

  // get the Ghost Cell Array if it exists
  vtkDataArray* GhostData =
    input->GetCellData()->GetScalars(vtkDataSetAttributes::GhostArrayName());
  // if the strings are not the same then we did not get the ghostData array
  if (GhostData == nullptr ||
    strcmp(GhostData->GetName(), vtkDataSetAttributes::GhostArrayName()) != 0)
  {
    GhostData = nullptr;
  }

  // data structure to get all the cells for a certain part
  // basically sort by part# and cell type
  std::map<int, std::vector<int>> CellsByPart;

  // just a list of part numbers
  std::list<int> partNumbers;

  // get all the part numbers in the unstructured grid and sort the cells
  // by part number
  for (i = 0; i < input->GetNumberOfCells(); i++)
  {
    int key = 1;
    if (BlockData)
    {
      key = (int)(BlockData->GetTuple(i)[0]);
    }
    if (CellsByPart.count(key) == 0)
    {
      CellsByPart[key] = std::vector<int>();
    }
    CellsByPart[key].push_back(i);
    partNumbers.push_back(key);
  }

  // remove the duplicates from the partNumbers
  partNumbers.sort();
  partNumbers.unique();

  // write out each part
  for (iter = partNumbers.begin(); iter != partNumbers.end(); ++iter)
  {
    unsigned int j;
    std::list<int>::iterator iter2;
    int part = *iter;

    // write the part Header
    if (this->ShouldWriteGeometry())
    {
      blockCount += 1;
      this->WriteStringToFile("part", fd);
      this->WriteIntToFile(part, fd);
      // cout << "part is " << part << endl;
      this->WriteStringToFile("VTK Part", fd);
      this->WriteStringToFile("coordinates", fd);
    }

    // write the part header for data files
    for (j = 0; j < pointArrayFiles.size(); j++)
    {
      this->WriteStringToFile("part", pointArrayFiles[j]);
      this->WriteIntToFile(part, pointArrayFiles[j]);
      this->WriteStringToFile("coordinates", pointArrayFiles[j]);
    }
    for (j = 0; j < cellArrayFiles.size(); j++)
    {
      this->WriteStringToFile("part", cellArrayFiles[j]);
      this->WriteIntToFile(part, cellArrayFiles[j]);
    }

    // list of VTK Node Indices per part
    std::list<int> NodesPerPart;

    // map that goes from NodeID to the order, used for element connectivity
    std::map<int, int> NodeIdToOrder;

    // get a list of all the nodes used for a particular part
    for (j = 0; j < CellsByPart[part].size(); j++)
    {
      vtkCell* cell = input->GetCell(CellsByPart[part][j]);
      vtkIdList* points = cell->GetPointIds();
      for (int k = 0; k < points->GetNumberOfIds(); k++)
      {
        NodesPerPart.push_back(points->GetId(k));
      }
    }

    // remove the duplicate Node ID's
    NodesPerPart.sort();
    NodesPerPart.unique();

    // write the number of nodes
    if (this->ShouldWriteGeometry())
    {
      this->WriteIntToFile(static_cast<int>(NodesPerPart.size()), fd);

      // write the Node ID's to the file
      // also set up the NodeID->order map
      int NodeCount = 0;
      for (iter2 = NodesPerPart.begin(); iter2 != NodesPerPart.end(); ++iter2)
      {
        if (this->WriteNodeIDs)
        {
          this->WriteIntToFile(*iter2, fd);
        }

        NodeIdToOrder[*iter2] = NodeCount + 1;
        NodeCount++;
      }

      // EnSight format requires all the X's, then all the Y's, then all the Z's
      // write the X Coordinates

      vtkPoints* inputPoints = input->GetPoints();
      for (iter2 = NodesPerPart.begin(); iter2 != NodesPerPart.end(); ++iter2)
      {
        this->WriteFloatToFile((float)(inputPoints->GetPoint(*iter2)[0]), fd);
      }
      for (iter2 = NodesPerPart.begin(); iter2 != NodesPerPart.end(); ++iter2)
      {
        this->WriteFloatToFile((float)(inputPoints->GetPoint(*iter2)[1]), fd);
      }
      for (iter2 = NodesPerPart.begin(); iter2 != NodesPerPart.end(); ++iter2)
      {
        this->WriteFloatToFile((float)(inputPoints->GetPoint(*iter2)[2]), fd);
      }
    }

    // write the Node Data for this part
    for (j = 0; j < pointArrayFiles.size(); j++)
    {
      vtkDataArray* DataArray = input->GetPointData()->GetArray(j);
      // figure out what type of data it is
      int DataSize = DataArray->GetNumberOfComponents();

      for (int CurrentDimension = 0; CurrentDimension < DataSize; CurrentDimension++)
      {
        int OutputComponent = this->GetDestinationComponent(CurrentDimension, DataSize);
        for (std::list<int>::iterator k = NodesPerPart.begin(); k != NodesPerPart.end(); ++k)
        {
          this->WriteFloatToFile(
            (float)(DataArray->GetTuple(*k)[OutputComponent]), pointArrayFiles[j]);
        }
      }
    }

    // now we need to sort the cell list by element type
    // map is indexed by cell type has a vector of cell ID's
    std::map<int, std::vector<int>> CellsByElement;
    for (j = 0; j < CellsByPart[part].size(); j++)
    {
      int CellType = input->GetCell(CellsByPart[part][j])->GetCellType();
      int ghostLevel = 0;
      if (GhostData)
      {
        ghostLevel = (int)(GhostData->GetTuple(CellsByPart[part][j])[0]);
        if (ghostLevel & vtkDataSetAttributes::DUPLICATECELL)
        {
          ghostLevel = 1;
        }
      }
      // we want to sort out the ghost cells from the normal cells
      // so the element type will be ghostMultiplier*ghostLevel+elementType
      CellType += ghostLevel * GhostLevelMultiplier;
      if (CellsByElement.count(CellType) == 0)
      {
        CellsByElement[CellType] = std::vector<int>();
      }
      CellsByElement[CellType].push_back(CellsByPart[part][j]);
    }

    // now we need to go through each element type that EnSight understands
    std::vector<int> elementTypes;

    // list the types that EnSight understands
    elementTypes.push_back(VTK_VERTEX);
    elementTypes.push_back(VTK_LINE);
    elementTypes.push_back(VTK_TRIANGLE);
    elementTypes.push_back(VTK_QUAD);
    elementTypes.push_back(VTK_POLYGON);
    elementTypes.push_back(VTK_TETRA);
    elementTypes.push_back(VTK_HEXAHEDRON);
    elementTypes.push_back(VTK_WEDGE);
    elementTypes.push_back(VTK_PYRAMID);
    elementTypes.push_back(VTK_CONVEX_POINT_SET);
    elementTypes.push_back(VTK_POLYHEDRON);
    elementTypes.push_back(VTK_QUADRATIC_EDGE);
    elementTypes.push_back(VTK_QUADRATIC_TRIANGLE);
    elementTypes.push_back(VTK_QUADRATIC_QUAD);
    elementTypes.push_back(VTK_QUADRATIC_TETRA);
    elementTypes.push_back(VTK_QUADRATIC_HEXAHEDRON);
    elementTypes.push_back(VTK_QUADRATIC_WEDGE);
    elementTypes.push_back(VTK_QUADRATIC_PYRAMID);
    elementTypes.push_back(this->GhostLevelMultiplier + VTK_VERTEX);
    elementTypes.push_back(this->GhostLevelMultiplier + VTK_LINE);
    elementTypes.push_back(this->GhostLevelMultiplier + VTK_TRIANGLE);
    elementTypes.push_back(this->GhostLevelMultiplier + VTK_QUAD);
    elementTypes.push_back(this->GhostLevelMultiplier + VTK_POLYGON);
    elementTypes.push_back(this->GhostLevelMultiplier + VTK_TETRA);
    elementTypes.push_back(this->GhostLevelMultiplier + VTK_HEXAHEDRON);
    elementTypes.push_back(this->GhostLevelMultiplier + VTK_WEDGE);
    elementTypes.push_back(this->GhostLevelMultiplier + VTK_PYRAMID);
    elementTypes.push_back(this->GhostLevelMultiplier + VTK_CONVEX_POINT_SET);
    elementTypes.push_back(this->GhostLevelMultiplier + VTK_POLYHEDRON);
    elementTypes.push_back(this->GhostLevelMultiplier + VTK_QUADRATIC_EDGE);
    elementTypes.push_back(this->GhostLevelMultiplier + VTK_QUADRATIC_TRIANGLE);
    elementTypes.push_back(this->GhostLevelMultiplier + VTK_QUADRATIC_QUAD);
    elementTypes.push_back(this->GhostLevelMultiplier + VTK_QUADRATIC_TETRA);
    elementTypes.push_back(this->GhostLevelMultiplier + VTK_QUADRATIC_HEXAHEDRON);
    elementTypes.push_back(this->GhostLevelMultiplier + VTK_QUADRATIC_WEDGE);
    elementTypes.push_back(this->GhostLevelMultiplier + VTK_QUADRATIC_PYRAMID);

    // EnSight Gold unstructured grid element block has the following general structure:
    //
    //     element-type         <--  80*char, eg. "tria3"
    //     number-of-elements   <--  1*int
    //     (element-ids)        <--  number-of-elements*int, optional
    //     connectivity         <--  depends on element-type
    //
    // There are three variants of connectivity definition based on element type.
    //
    // Ghost cells have separate element types prefixed by "g_" but use the same representation
    // as corresponding non-ghost types.
    //
    // For element types with fixed number of nodes, it simply lists nodes for the first element,
    // for second element, etc.:
    //
    //     tetra4               <--  80*char (element-type)
    //     2                    <--  1*int (number-of-elements)
    //     100 101              <--  number-of-elements*int (element IDs)
    //     1 2 3 4              <--  number-of-elements*nodes-per-element*int       [first tetra]
    //     2 3 4 5                                                                  [second tetra]
    //
    // For "nsided" elements (ie. polygons), first the number of nodes for each polygon is given,
    // followed by nodes for the first polygon, second polygon, etc.:
    //
    //     nsided               <--  80*char (element-type)
    //     2                    <--  1*int (number-of-elements)
    //     100 101              <--  number-of-elements*int (element IDs)
    //     5 6                  <--  number-of-elements*int (nodes-per-polygon)
    //     1 2 3 4 5            <--  sum(nodes-per-polygon)*int                     [first polygon]
    //     4 5 6 7 8 9                                                              [second polygon]
    //
    // For "nfaced" elements (ie. polyhedra), definition is similar to "nsided" but it starts with
    // defining the number of faces for each element, followed by number of nodes for each face, and
    // then nodes for the faces. The following example defines the same two tetrahedrons as "tetra4"
    // example above, but using "nfaced" elements.
    //
    //     nfaced               <--  80*char (element-type)
    //     2                    <--  1*int (number-of-elements)
    //     100 101              <--  number-of-elements*int (element IDs)
    //     4 4                  <--  number-of-elements*int (faces-per-polyhedron)
    //     3 3 3 3              <--  sum(faces-per-polyhedron)*int (nodes-per-face) [1st polyhedron]
    //     3 3 3 3                                                                  [2nd polyhedron]
    //     1 2 3                <--  sum(nodes-per-face)*int                        [cell 1, face 1]
    //     1 2 4                                                                    [cell 1, face 2]
    //     2 3 4                                                                    [cell 1, face 3]
    //     1 3 4                                                                    [cell 1, face 4]
    //     2 3 4                                                                    [cell 2, face 1]
    //     2 3 5                                                                    [cell 2, face 2]
    //     3 4 5                                                                    [cell 2, face 3]
    //     2 4 5                                                                    [cell 2, face 4]

    // write out each type of element
    if (this->ShouldWriteGeometry())
    {
      for (j = 0; j < elementTypes.size(); j++)
      {
        unsigned int k;
        int elementType = elementTypes[j];
        int elementTypeWithoutGhostLevel = elementType % GhostLevelMultiplier;
        if (CellsByElement.count(elementType) > 0)
        {
          // switch on element type to write correct type to file
          this->WriteElementTypeToFile(elementType, fd);

          // number of elements
          this->WriteIntToFile(static_cast<int>(CellsByElement[elementType].size()), fd);

          // element ID's
          if (this->WriteElementIDs)
          {
            for (k = 0; k < CellsByElement[elementType].size(); k++)
            {
              int CellId = CellsByElement[elementType][k];
              this->WriteIntToFile(CellId, fd);
            }
          }

          // element connectivity information
          if (elementTypeWithoutGhostLevel == VTK_POLYGON)
          {
            // VTK_POLYGON is represented as "nsided" EnSight element (which has special
            // representation)

            // write number of nodes per polygon
            for (k = 0; k < CellsByElement[elementType].size(); k++)
            {
              int CellId = CellsByElement[elementType][k];
              int NumberOfNodes = input->GetCellSize(CellId);
              this->WriteIntToFile(NumberOfNodes, fd);
            }

            // write nodes for each polygon
            for (k = 0; k < CellsByElement[elementType].size(); k++)
            {
              int CellId = CellsByElement[elementType][k];
              vtkIdList* PointIds = input->GetCell(CellId)->GetPointIds();
              for (int m = 0; m < PointIds->GetNumberOfIds(); m++)
              {
                int PointId = PointIds->GetId(m);
                this->WriteIntToFile(NodeIdToOrder[PointId], fd);
              }
            }
          }
          else if (elementTypeWithoutGhostLevel == VTK_POLYHEDRON)
          {
            // VTK_POLYHEDRON is represented as "nfaced" EnSight element (which has special
            // representation), we will use vtkUnstructuredGrid Faces and FaceLocations arrays to
            // write the connectivity.

            vtkIdTypeArray* Faces = input->GetFaces();
            vtkIdTypeArray* FaceLocations = input->GetFaceLocations();

            // write number of faces per polyhedron
            for (k = 0; k < CellsByElement[elementType].size(); k++)
            {
              int CellId = CellsByElement[elementType][k];
              int FacesIdx = FaceLocations->GetValue(CellId);
              assert(FacesIdx >= 0);
              int NumberOfFaces = Faces->GetValue(FacesIdx++);
              this->WriteIntToFile(NumberOfFaces, fd);
            }

            // write number of nodes per face
            for (k = 0; k < CellsByElement[elementType].size(); k++)
            {
              int CellId = CellsByElement[elementType][k];
              int FacesIdx = FaceLocations->GetValue(CellId);
              int NumberOfFaces = Faces->GetValue(FacesIdx++);
              for (int m = 0; m < NumberOfFaces; m++)
              {
                int NumberOfNodes = Faces->GetValue(FacesIdx++);
                FacesIdx += NumberOfNodes; // skip point IDs for the face
                this->WriteIntToFile(NumberOfNodes, fd);
              }
            }

            // write nodes for each face
            for (k = 0; k < CellsByElement[elementType].size(); k++)
            {
              int CellId = CellsByElement[elementType][k];
              int FacesIdx = FaceLocations->GetValue(CellId);
              int NumberOfFaces = Faces->GetValue(FacesIdx++);
              for (int m = 0; m < NumberOfFaces; m++)
              {
                int NumberOfNodes = Faces->GetValue(FacesIdx++);
                for (int n = 0; n < NumberOfNodes; n++)
                {
                  int PointId = Faces->GetValue(FacesIdx++);
                  this->WriteIntToFile(NodeIdToOrder[PointId], fd);
                }
              }
            }
          }
          else if (elementTypeWithoutGhostLevel == VTK_CONVEX_POINT_SET)
          {
            // VTK_CONVEX_POINT_SET is represented as "nfaced" EnSight element (which has special
            // representation) and unlike VTK_POLYHEDRON we have to compute its boundary faces since
            // they are implicit.

            std::vector<int> faceCountPerPolyhedron;
            std::vector<int> nodeCountPerFace;
            std::vector<int> pointIds;

            for (k = 0; k < CellsByElement[elementType].size(); k++)
            {
              int CellId = CellsByElement[elementType][k];
              vtkCell* cell = input->GetCell(CellId);
              int NumberOfFaces = cell->GetNumberOfFaces();
              faceCountPerPolyhedron.push_back(NumberOfFaces);

              for (int m = 0; m < NumberOfFaces; m++)
              {
                vtkCell* cellFace = cell->GetFace(m);
                int NumberOfNodes = cellFace->GetNumberOfPoints();
                nodeCountPerFace.push_back(NumberOfNodes);
                for (int n = 0; n < NumberOfNodes; n++)
                {
                  int PointId = cellFace->GetPointId(n);
                  pointIds.push_back(PointId);
                }
              }
            }

            // write number of faces per polyhedron
            for (int NumberOfFaces : faceCountPerPolyhedron)
            {
              this->WriteIntToFile(NumberOfFaces, fd);
            }

            // write number of nodes per face
            for (int NumberOfNodes : nodeCountPerFace)
            {
              this->WriteIntToFile(NumberOfNodes, fd);
            }

            // write nodes for each face
            for (int PointId : pointIds)
            {
              this->WriteIntToFile(NodeIdToOrder[PointId], fd);
            }
          }
          else
          {
            // VTK cell types with fixed number of nodes are represented with corresponding
            // EnSight element types which all use the simple representation. VTK and EnSight
            // mostly agree on implicit ordering of nodes, except for the following:
            // - "bar3" (VTK_QUADRATIC_EDGE)
            // - "penta6" (VTK_WEDGE)
            // - "penta15" (VTK_QUADRATIC_WEDGE)
            // See the code in vtkEnSightGoldBinaryReader::CreateUnstructuredGridOutput.
            for (k = 0; k < CellsByElement[elementType].size(); k++)
            {
              int CellId = CellsByElement[elementType][k];
              vtkIdList* PointIds = input->GetCell(CellId)->GetPointIds();

              const unsigned char bar3Map[3] = { 0, 2, 1 };
              const unsigned char penta6Map[6] = { 0, 2, 1, 3, 5, 4 };
              const unsigned char penta15Map[15] = { 0, 2, 1, 3, 5, 4, 8, 7, 6, 11, 10, 9, 12, 14,
                13 };

              // write nodes for each cell, converting to EnSight ordering where necessary
              for (int m = 0; m < PointIds->GetNumberOfIds(); m++)
              {
                int n = m;
                switch (elementType)
                {
                  case VTK_QUADRATIC_EDGE:
                    n = bar3Map[m];
                    break;
                  case VTK_WEDGE:
                    n = penta6Map[m];
                    break;
                  case VTK_QUADRATIC_WEDGE:
                    n = penta15Map[m];
                    break;
                  default:
                    break;
                }
                int PointId = PointIds->GetId(n);
                this->WriteIntToFile(NodeIdToOrder[PointId], fd);
              }
            }
          }
        }
      }
    }

    // write the Cell Data for this part
    for (j = 0; j < cellArrayFiles.size(); j++)
    {
      vtkDataArray* DataArray = input->GetCellData()->GetArray(j);
      // figure out what type of data it is
      int DataSize = DataArray->GetNumberOfComponents();

      for (unsigned int k = 0; k < elementTypes.size(); k++)
      {
        if (!CellsByElement[elementTypes[k]].empty())
        {
          this->WriteElementTypeToFile(elementTypes[k], cellArrayFiles[j]);
          for (int CurrentDimension = 0; CurrentDimension < DataSize; CurrentDimension++)
          {
            int OutputComponent = this->GetDestinationComponent(CurrentDimension, DataSize);
            for (unsigned int m = 0; m < CellsByElement[elementTypes[k]].size(); m++)
            {
              this->WriteFloatToFile(
                (float)(DataArray->GetTuple(CellsByElement[elementTypes[k]][m])[OutputComponent]),
                cellArrayFiles[j]);
            }
          }
        }
      }
    }
  }

  // now write the empty blocks
  // use the block list in the exodus model if it exists, otherwise
  // use the BlockID list if that exists.

  if (this->BlockIDs)
  {
    elementIDs = this->BlockIDs;
  }

  if (elementIDs)
  {
    // cout << "have " << this->NumberOfBlocks << " blocks " << endl;
    for (i = 0; i < this->NumberOfBlocks; i++)
    {
      unsigned int j;
      // figure out if the part was already written
      int part = elementIDs[i];
      if (std::find(partNumbers.begin(), partNumbers.end(), part) == partNumbers.end())
      {
        // no information about the part was written to the output files
        // so write some empty information
        if (this->ShouldWriteGeometry())
        {
          blockCount += 1;
          this->WriteStringToFile("part", fd);
          this->WriteIntToFile(part, fd);

          int exodusIndex = this->GetExodusModelIndex(elementIDs, this->NumberOfBlocks, part);

          if (exodusIndex != -1 && blockNames)
          {
            snprintf(charBuffer, sizeof(charBuffer), "Exodus-%s-%d", blockNames[exodusIndex], part);
            this->WriteStringToFile(charBuffer, fd);
          }
          else
          {
            this->WriteStringToFile("VTK Part", fd);
          }
        }

        // write the part header for data files
        for (j = 0; j < pointArrayFiles.size(); j++)
        {
          this->WriteStringToFile("part", pointArrayFiles[j]);
          this->WriteIntToFile(part, pointArrayFiles[j]);
        }
        for (j = 0; j < cellArrayFiles.size(); j++)
        {
          this->WriteStringToFile("part", cellArrayFiles[j]);
          this->WriteIntToFile(part, cellArrayFiles[j]);
        }
      }
    }
  }
  vtkLog(TRACE, "wrote " << blockCount << "parts\n");

  if (this->TmpInput)
  {
    this->TmpInput->Delete();
    this->TmpInput = nullptr;
  }

  // close all the files
  if (fd)
  {
    fclose(fd);
  }

  for (ui = 0; ui < cellArrayFiles.size(); ui++)
  {
    fclose(cellArrayFiles[ui]);
  }
  for (ui = 0; ui < pointArrayFiles.size(); ui++)
  {
    fclose(pointArrayFiles[ui]);
  }
}

//------------------------------------------------------------------------------
void vtkEnSightWriter::WriteCaseFile(int TotalTimeSteps)
{

  vtkUnstructuredGrid* input = this->GetInput();
  int i;

  this->ComputeNames();

  if (!this->BaseName)
  {
    vtkErrorMacro("A FileName or Path/BaseName must be specified.");
    return;
  }

  char charBuffer[1024];
  snprintf(charBuffer, sizeof(charBuffer), "%s/%s.%d.case", this->Path, this->BaseName,
    this->ProcessNumber);

  // open the geometry file
  FILE* fd = nullptr;
  if (!(fd = OpenFile(charBuffer)))
  {
    return;
  }

  this->WriteTerminatedStringToFile("FORMAT\n", fd);
  this->WriteTerminatedStringToFile("type: ensight gold\n\n", fd);
  this->WriteTerminatedStringToFile("\nGEOMETRY\n", fd);

  // write the geometry file
  if (!this->TransientGeometry)
  {
    snprintf(charBuffer, sizeof(charBuffer), "model: %s.%d.00000.geo\n", this->BaseName,
      this->ProcessNumber);
    this->WriteTerminatedStringToFile(charBuffer, fd);
  }
  else
  {
    snprintf(charBuffer, sizeof(charBuffer), "model: 1 %s.%d.*****.geo\n", this->BaseName,
      this->ProcessNumber);
    this->WriteTerminatedStringToFile(charBuffer, fd);
  }

  this->WriteTerminatedStringToFile("\nVARIABLE\n", fd);

  char fileBuffer[256];

  // write the Node variable files
  for (i = 0; i < input->GetPointData()->GetNumberOfArrays(); i++)
  {

    strcpy(fileBuffer, input->GetPointData()->GetArray(i)->GetName());
    // skip arrays that were not written
    if (strcmp(fileBuffer, "GlobalElementId") == 0)
    {
      continue;
    }
    if (strcmp(fileBuffer, "GlobalNodeId") == 0)
    {
      continue;
    }
    if (strcmp(fileBuffer, "BlockId") == 0)
    {
      continue;
    }
    this->SanitizeFileName(fileBuffer);
    // figure out what kind of data it is
    char SmallBuffer[16];
    switch (input->GetPointData()->GetArray(i)->GetNumberOfComponents())
    {
      case (1):
        strcpy(SmallBuffer, "scalar");
        break;
      case (3):
        strcpy(SmallBuffer, "vector");
        break;
      case (6):
        strcpy(SmallBuffer, "tensor symm");
        break;
      case (9):
        strcpy(SmallBuffer, "tensor asym");
        break;
    }
    if (TotalTimeSteps <= 1)
    {
      snprintf(charBuffer, sizeof(charBuffer), "%s per node: %s_n %s.%d.00000_n.%s\n", SmallBuffer,
        fileBuffer, this->BaseName, this->ProcessNumber, fileBuffer);
    }
    else
    {
      snprintf(charBuffer, sizeof(charBuffer), "%s per node: 1 %s_n %s.%d.*****_n.%s\n",
        SmallBuffer, fileBuffer, this->BaseName, this->ProcessNumber, fileBuffer);
    }
    this->WriteTerminatedStringToFile(charBuffer, fd);
  }

  // write the cell variable files
  for (i = 0; i < input->GetCellData()->GetNumberOfArrays(); i++)
  {
    // figure out what kind of data it is
    char SmallBuffer[16];

    strcpy(fileBuffer, input->GetCellData()->GetArray(i)->GetName());
    // skip arrays that were not written
    if (strcmp(fileBuffer, "GlobalElementId") == 0)
    {
      continue;
    }
    if (strcmp(fileBuffer, "GlobalNodeId") == 0)
    {
      continue;
    }
    if (strcmp(fileBuffer, "BlockId") == 0)
    {
      continue;
    }
    this->SanitizeFileName(fileBuffer);
    switch (input->GetCellData()->GetArray(i)->GetNumberOfComponents())
    {
      case (1):
        strcpy(SmallBuffer, "scalar");
        break;
      case (3):
        strcpy(SmallBuffer, "vector");
        break;
      case (6):
        strcpy(SmallBuffer, "tensor symm");
        break;
      case (9):
        strcpy(SmallBuffer, "tensor asym");
        break;
    }
    if (TotalTimeSteps <= 1)
    {
      snprintf(charBuffer, sizeof(charBuffer), "%s per element: %s_c %s.%d.00000_c.%s\n",
        SmallBuffer, fileBuffer, this->BaseName, this->ProcessNumber, fileBuffer);
    }
    else
    {
      snprintf(charBuffer, sizeof(charBuffer), "%s per element: 1 %s_c %s.%d.*****_c.%s\n",
        SmallBuffer, fileBuffer, this->BaseName, this->ProcessNumber, fileBuffer);
    }
    this->WriteTerminatedStringToFile(charBuffer, fd);
  }

  // write time information if we have multiple timesteps
  if (TotalTimeSteps > 1)
  {
    this->WriteTerminatedStringToFile("\nTIME\n", fd);
    this->WriteTerminatedStringToFile("time set: 1\n", fd);
    snprintf(charBuffer, sizeof(charBuffer), "number of steps: %d\n", TotalTimeSteps);
    this->WriteTerminatedStringToFile(charBuffer, fd);
    this->WriteTerminatedStringToFile("filename start number: 00000\n", fd);
    this->WriteTerminatedStringToFile("filename increment: 00001\n", fd);
    this->WriteTerminatedStringToFile("time values: \n", fd);
    for (i = 0; i < TotalTimeSteps; i++)
    {
      double timestep = i;

      snprintf(charBuffer, sizeof(charBuffer), "%f ", timestep);
      this->WriteTerminatedStringToFile(charBuffer, fd);
      if (i % 6 == 0 && i > 0)
      {
        this->WriteTerminatedStringToFile("\n", fd);
      }
    }
  }

  if (fd)
  {
    fclose(fd);
  }
}

//------------------------------------------------------------------------------
void vtkEnSightWriter::WriteSOSCaseFile(int numProcs)
{
  this->ComputeNames();

  if (!this->BaseName)
  {
    vtkErrorMacro("A FileName or Path/BaseName must be specified.");
    return;
  }

  this->SanitizeFileName(this->BaseName);

  char charBuffer[512];
  snprintf(charBuffer, sizeof(charBuffer), "%s/%s.case.sos", this->Path, this->BaseName);

  FILE* fd = nullptr;
  if (!(fd = OpenFile(charBuffer)))
    return;

  this->WriteTerminatedStringToFile("FORMAT\n", fd);
  this->WriteTerminatedStringToFile("type: master_server gold\n\n", fd);

  this->WriteTerminatedStringToFile("SERVERS\n", fd);
  snprintf(charBuffer, sizeof(charBuffer), "number of servers: %d\n\n", numProcs);
  this->WriteTerminatedStringToFile(charBuffer, fd);

  // write the servers section with placeholders for the ensight server
  // location and server name
  int i = 0;
  for (i = 0; i < numProcs; i++)
  {
    snprintf(charBuffer, sizeof(charBuffer), "#Server %d\n", i);
    this->WriteTerminatedStringToFile(charBuffer, fd);
    this->WriteTerminatedStringToFile("#-------\n", fd);
    snprintf(charBuffer, sizeof(charBuffer), "machine id: MID%05d\n", i);
    this->WriteTerminatedStringToFile(charBuffer, fd);

    this->WriteTerminatedStringToFile("executable: MEX\n", fd);
    snprintf(charBuffer, sizeof(charBuffer), "data_path: %s\n", this->Path);
    this->WriteTerminatedStringToFile(charBuffer, fd);
    snprintf(charBuffer, sizeof(charBuffer), "casefile: %s.%d.case\n\n", this->BaseName, i);
    this->WriteTerminatedStringToFile(charBuffer, fd);
  }

  if (fd)
  {
    fclose(fd);
  }
}

//------------------------------------------------------------------------------
void vtkEnSightWriter::WriteStringToFile(const char* cstring, FILE* file)
{
  char cbuffer[81];
  unsigned long cstringLength = static_cast<unsigned long>(strlen(cstring));
  memcpy(cbuffer, cstring, vtkMath::Min(cstringLength, 80ul));
  for (int i = cstringLength; i < 81; ++i)
  {
    cbuffer[i] = '\0';
  }

  // Write a constant 80 bytes to the file.
  fwrite(cbuffer, sizeof(char), 80, file);
}

//------------------------------------------------------------------------------
void vtkEnSightWriter::WriteTerminatedStringToFile(const char* cstring, FILE* file)
{
  fwrite(cstring, sizeof(char), std::min(strlen(cstring), static_cast<size_t>(512)), file);
}

//------------------------------------------------------------------------------
void vtkEnSightWriter::WriteIntToFile(int i, FILE* file)
{
  fwrite(&i, sizeof(int), 1, file);
}

//------------------------------------------------------------------------------
void vtkEnSightWriter::WriteFloatToFile(float f, FILE* file)
{
  fwrite(&f, sizeof(float), 1, file);
}

//------------------------------------------------------------------------------
void vtkEnSightWriter::WriteElementTypeToFile(int elementType, FILE* fd)
{
  int ghostLevel = elementType / GhostLevelMultiplier;
  elementType = elementType % GhostLevelMultiplier;
  if (ghostLevel == 0)
  {
    switch (elementType)
    {
      case (VTK_VERTEX):
        this->WriteStringToFile("point", fd);
        break;
      case (VTK_LINE):
        this->WriteStringToFile("bar2", fd);
        break;
      case (VTK_TRIANGLE):
        this->WriteStringToFile("tria3", fd);
        break;
      case (VTK_QUAD):
        this->WriteStringToFile("quad4", fd);
        break;
      case (VTK_POLYGON):
        this->WriteStringToFile("nsided", fd);
        break;
      case (VTK_TETRA):
        this->WriteStringToFile("tetra4", fd);
        break;
      case (VTK_HEXAHEDRON):
        this->WriteStringToFile("hexa8", fd);
        break;
      case (VTK_WEDGE):
        this->WriteStringToFile("penta6", fd);
        break;
      case (VTK_PYRAMID):
        this->WriteStringToFile("pyramid5", fd);
        break;
      case (VTK_CONVEX_POINT_SET):
      case (VTK_POLYHEDRON):
        this->WriteStringToFile("nfaced", fd);
        break;
      case (VTK_QUADRATIC_EDGE):
        this->WriteStringToFile("bar3", fd);
        break;
      case (VTK_QUADRATIC_TRIANGLE):
        this->WriteStringToFile("tria6", fd);
        break;
      case (VTK_QUADRATIC_QUAD):
        this->WriteStringToFile("quad8", fd);
        break;
      case (VTK_QUADRATIC_TETRA):
        this->WriteStringToFile("tetra10", fd);
        break;
      case (VTK_QUADRATIC_HEXAHEDRON):
        this->WriteStringToFile("hexa20", fd);
        break;
      case (VTK_QUADRATIC_WEDGE):
        this->WriteStringToFile("penta15", fd);
        break;
      case (VTK_QUADRATIC_PYRAMID):
        this->WriteStringToFile("pyramid13", fd);
        break;
    }
  }
  else
  {
    switch (elementType)
    {
      case (VTK_VERTEX):
        this->WriteStringToFile("g_point", fd);
        break;
      case (VTK_LINE):
        this->WriteStringToFile("g_bar2", fd);
        break;
      case (VTK_TRIANGLE):
        this->WriteStringToFile("g_tria3", fd);
        break;
      case (VTK_QUAD):
        this->WriteStringToFile("g_quad4", fd);
        break;
      case (VTK_POLYGON):
        this->WriteStringToFile("g_nsided", fd);
        break;
      case (VTK_TETRA):
        this->WriteStringToFile("g_tetra4", fd);
        break;
      case (VTK_HEXAHEDRON):
        this->WriteStringToFile("g_hexa8", fd);
        break;
      case (VTK_WEDGE):
        this->WriteStringToFile("g_penta6", fd);
        break;
      case (VTK_PYRAMID):
        this->WriteStringToFile("g_pyramid5", fd);
        break;
      case (VTK_CONVEX_POINT_SET):
      case (VTK_POLYHEDRON):
        this->WriteStringToFile("g_nfaced", fd);
        break;
      case (VTK_QUADRATIC_EDGE):
        this->WriteStringToFile("g_bar3", fd);
        break;
      case (VTK_QUADRATIC_TRIANGLE):
        this->WriteStringToFile("g_tria6", fd);
        break;
      case (VTK_QUADRATIC_QUAD):
        this->WriteStringToFile("g_quad8", fd);
        break;
      case (VTK_QUADRATIC_TETRA):
        this->WriteStringToFile("g_tetra10", fd);
        break;
      case (VTK_QUADRATIC_HEXAHEDRON):
        this->WriteStringToFile("g_hexa20", fd);
        break;
      case (VTK_QUADRATIC_WEDGE):
        this->WriteStringToFile("g_penta15", fd);
        break;
      case (VTK_QUADRATIC_PYRAMID):
        this->WriteStringToFile("g_pyramid13", fd);
        break;
    }
  }
}

//------------------------------------------------------------------------------
bool vtkEnSightWriter::ShouldWriteGeometry()
{
  return (this->TransientGeometry || (this->TimeStep == 0));
}

//------------------------------------------------------------------------------
void vtkEnSightWriter::SanitizeFileName(char* name)
{

  char buffer[512];
  unsigned int i;
  int BufferPosition = 0;
  for (i = 0; i < strlen(name); i++)
  {
    if (name[i] != '/')
    {
      buffer[BufferPosition] = name[i];
      BufferPosition++;
    }
  }
  buffer[BufferPosition] = 0;
  for (i = 0; i < strlen(buffer); i++)
  {
    name[i] = buffer[i];
  }
  name[strlen(buffer)] = 0;
}

//------------------------------------------------------------------------------
FILE* vtkEnSightWriter::OpenFile(char* name)
{
  FILE* fd = vtksys::SystemTools::Fopen(name, "wb");

  if (fd == nullptr)
  {
    vtkErrorMacro("Error opening " << name << ": " << strerror(errno));
    return nullptr;
  }
  return fd;
}

//------------------------------------------------------------------------------
int vtkEnSightWriter::GetExodusModelIndex(int* elementArray, int numberElements, int partID)
{
  int i;
  for (i = 0; i < numberElements; i++)
  {
    if (elementArray[i] == partID)
      return i;
  }
  return -1;
}

//------------------------------------------------------------------------------
void vtkEnSightWriter::DefaultNames()
{
  char* path = new char[4];
  char* base = new char[20];
  strcpy(path, "./");
  strcpy(base, "EnSightWriter.out");

  this->SetPath(path);
  this->SetBaseName(base);
}

//------------------------------------------------------------------------------
void vtkEnSightWriter::ComputeNames()
{
  if (this->Path && this->BaseName)
  {
    return;
  }

  if (!this->FileName)
  {
    this->DefaultNames();
    return;
  }

  // FileName = Path/BaseName.digits.digits

  char* path = nullptr;
  char* base = nullptr;

  char* f = this->FileName;

  while (!isgraph(*f))
    f++; // find first printable character

  if (!*f)
  {
    // FileName is garbage
    DefaultNames();
    return;
  }

  char* buf = new char[strlen(f) + 1];
  strcpy(buf, f);

  char* slash = strrchr(buf, '/'); // final slash

  if (slash)
  {
    *slash = 0;
    path = new char[strlen(buf) + 1];
    strcpy(path, buf);
    f = slash + 1;
  }
  else
  {
    path = new char[4];
    strcpy(path, "./");

    f = buf;
  }

  char* firstChar = f;
  while (*f && (*f != '.'))
    f++;
  *f = 0;

  base = new char[strlen(firstChar) + 1];
  strcpy(base, firstChar);

  this->SetPath(path);
  this->SetBaseName(base);

  delete[] buf;
}

//------------------------------------------------------------------------------
// Copied from vtkEnSightGoldBinaryReader::vtkUtilities::GetDestinationComponent
int vtkEnSightWriter::GetDestinationComponent(int srcComponent, int numComponents)
{
  if (numComponents == 6)
  {
    // for 6 component tensors, the symmetric tensor components XZ and YZ are interchanged
    // see Paraview issue #10637.
    switch (srcComponent)
    {
      case 4:
        return 5;

      case 5:
        return 4;
    }
  }

  return srcComponent;
}

VTK_ABI_NAMESPACE_END
