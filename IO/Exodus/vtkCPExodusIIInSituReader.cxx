/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCPExodusInSituReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkCPExodusIIInSituReader.h"

#include "vtkCellData.h"
#include "vtkCPExodusIIElementBlock.h"
#include "vtkCPExodusIINodalCoordinatesTemplate.h"
#include "vtkCPExodusIIResultsArrayTemplate.h"
#include "vtkDemandDrivenPipeline.h"
#include "vtkDoubleArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkMultiBlockDataSet.h"

#include "vtk_exodusII.h"

vtkStandardNewMacro(vtkCPExodusIIInSituReader)

//------------------------------------------------------------------------------
void vtkCPExodusIIInSituReader::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "FileName: "
     << (this->FileName ? this->FileName : "(none)") << endl;
}

//------------------------------------------------------------------------------
vtkCPExodusIIInSituReader::vtkCPExodusIIInSituReader()
  : FileName(NULL),
    FileId(-1),
    NumberOfDimensions(0),
    NumberOfNodes(0),
    NumberOfElementBlocks(0),
    CurrentTimeStep(0)
{
  this->TimeStepRange[0] = 0;
  this->TimeStepRange[1] = 0;
  this->SetNumberOfInputPorts(0);
}

//------------------------------------------------------------------------------
vtkCPExodusIIInSituReader::~vtkCPExodusIIInSituReader()
{
  this->SetFileName(NULL);
}

//------------------------------------------------------------------------------
int vtkCPExodusIIInSituReader::ProcessRequest(
    vtkInformation *request, vtkInformationVector **inputVector,
    vtkInformationVector *outputVector)
{
  if(request->Has(vtkDemandDrivenPipeline::REQUEST_DATA()))
  {
    return this->RequestData(request, inputVector, outputVector);
  }

  if(request->Has(vtkDemandDrivenPipeline::REQUEST_INFORMATION()))
  {
    return this->RequestInformation(request, inputVector, outputVector);
  }

  return this->Superclass::ProcessRequest(request, inputVector, outputVector);
}

//------------------------------------------------------------------------------
int vtkCPExodusIIInSituReader::RequestData(vtkInformation *,
                                         vtkInformationVector **,
                                         vtkInformationVector *outputVector)
{
  // Get output object:
  vtkInformation* outInfo(outputVector->GetInformationObject(0));
  vtkMultiBlockDataSet *output(vtkMultiBlockDataSet::SafeDownCast(
                                 outInfo->Get( vtkDataObject::DATA_OBJECT())));

  // Prepare high-level structure:
  //
  // output                             vtkMultiBlockDataSet
  //   - Block 0: this->ElementBlocks   vtkMultiBlockDataSet
  //     - Block N: Element blocks      vtkCPExodusIIElementBlock
  output->SetNumberOfBlocks(1);
  output->SetBlock(0, this->ElementBlocks.GetPointer());

  bool success = false;

  if (!this->ExOpen())
  {
    return 0;
  }

  for (;;) // Used to skip reading rest of file and close handle if error occurs
  {
    if (!this->ExGetMetaData())
    {
      break;
    }

    if (!this->ExGetCoords())
    {
      break;
    }

    if (!this->ExGetNodalVars())
    {
      break;
    }

    if (!this->ExGetElemBlocks())
    {
      break;
    }

    success = true;
    break;
  }

  this->ExClose();

  if (!success)
  {
    output->Initialize();
  }

  return success ? 1 : 0;
}

//------------------------------------------------------------------------------
int vtkCPExodusIIInSituReader::RequestInformation(
    vtkInformation *, vtkInformationVector **, vtkInformationVector *)
{
  if (!this->ExOpen())
  {
    return 0;
  }

  bool success(this->ExGetMetaData());

  this->ExClose();

  return success ? 1 : 0;
}

//------------------------------------------------------------------------------
bool vtkCPExodusIIInSituReader::ExOpen()
{
  int doubleSize = sizeof(double);
  int fileRealSize = 0;
  float exodusVersion;

  this->FileId = ex_open(this->FileName, EX_READ, &doubleSize, &fileRealSize,
                         &exodusVersion);

  if (this->FileId < 0)
  {
    vtkErrorMacro("Cannot open file: " << this->FileName);
    return false;
  }
  return true;
}

//------------------------------------------------------------------------------
bool vtkCPExodusIIInSituReader::ExGetMetaData()
{
  // Generic metadata:
  int numElem, numNodeSets, numSideSets;
  std::string title(MAX_LINE_LENGTH + 1, '\0');

  int error = ex_get_init(this->FileId,
                          &title[0],
                          &this->NumberOfDimensions,
                          &this->NumberOfNodes,
                          &numElem,
                          &NumberOfElementBlocks,
                          &numNodeSets, &numSideSets);

  // Trim excess null characters from string:
  title.resize(strlen(title.c_str()));

  if (error < 0)
  {
    vtkErrorMacro("Error retrieving file metadata.");
    return false;
  }

  // Number of nodal variables
  int numNodalVars;

  error = ex_get_var_param(this->FileId, "n", &numNodalVars);

  if (error < 0)
  {
    vtkErrorMacro("Error retrieving number of nodal variables.");
    return false;
  }

  // Names of nodal variables
  this->NodalVariableNames = std::vector<std::string>(
        numNodalVars, std::string(MAX_STR_LENGTH+1, '\0'));

  for (int i = 0; i < numNodalVars; ++i)
  {
    error = ex_get_var_name(this->FileId, "n", i + 1,
                            &(this->NodalVariableNames[i][0]));
    if (error < 0)
    {
      vtkErrorMacro("Error retrieving nodal variable name at index" << i);
      return false;
    }
    // Trim excess null chars from the strings:
    this->NodalVariableNames[i].resize(
          strlen(this->NodalVariableNames[i].c_str()));
  }

  // Number of element variables
  int numElemVars;

  error = ex_get_var_param(this->FileId, "e", &numElemVars);

  if (error < 0)
  {
    vtkErrorMacro("Error retrieving number of element variables.");
    return false;
  }

  // Names of element variables
  this->ElementVariableNames = std::vector<std::string>(
        numElemVars, std::string(MAX_STR_LENGTH+1, '\0'));

  for (int i = 0; i < numElemVars; ++i)
  {
    error = ex_get_var_name(this->FileId, "e", i + 1,
                            &(this->ElementVariableNames[i][0]));
    if (error < 0)
    {
      vtkErrorMacro("Error retrieving element variable name at index" << i);
      return false;
    }
    // Trim excess null chars from the strings:
    this->ElementVariableNames[i].resize(
          strlen(this->ElementVariableNames[i].c_str()));
  }

  // Element block ids:
  this->ElementBlockIds.resize(this->NumberOfElementBlocks);

  error = ex_get_elem_blk_ids(this->FileId, &(this->ElementBlockIds[0]));

  if (error < 0)
  {
    vtkErrorMacro("Failed to get the element block ids.");
    return false;
  }

  // Timesteps
  int numTimeSteps;

  error = ex_inquire(this->FileId, EX_INQ_TIME,
                     &numTimeSteps, NULL, NULL);
  if (error < 0)
  {
    vtkErrorMacro("Error retrieving the number of timesteps.");
    return false;
  }

  this->TimeStepRange[0] = 0;
  this->TimeStepRange[1] = numTimeSteps - 1;
  this->TimeSteps.resize(numTimeSteps);

  if (numTimeSteps > 0)
  {
    error = ex_get_all_times(this->FileId, &(this->TimeSteps[0]));

    if (error < 0)
    {
      vtkErrorMacro("Error retrieving timestep array.");
      return false;
    }
  }
  return true;
}

//------------------------------------------------------------------------------
bool vtkCPExodusIIInSituReader::ExGetCoords()
{
  this->Points->Reset();
  vtkNew<vtkCPExodusIINodalCoordinatesTemplate<double> > nodeCoords;

  // Get coordinates
  double *x(new double[this->NumberOfNodes]);
  double *y(new double[this->NumberOfNodes]);
  double *z(this->NumberOfDimensions >= 3
            ? new double[this->NumberOfNodes]
            : NULL);

  int error = ex_get_coord(this->FileId, x, y, z);

  if (error < 0)
  {
    delete [] x;
    delete [] y;
    delete [] z;
    vtkErrorMacro("Error retrieving coordinates.");
    return false;
  }

  // NodalCoordinates takes ownership of the arrays.
  nodeCoords->SetExodusScalarArrays(x, y, z, this->NumberOfNodes);
  this->Points->SetData(nodeCoords.GetPointer());
  return true;
}

//------------------------------------------------------------------------------
bool vtkCPExodusIIInSituReader::ExGetNodalVars()
{
  this->PointData->Reset();
  const int numNodalVars = static_cast<int>(this->NodalVariableNames.size());
  for (int nodalVarIndex = 0; nodalVarIndex < numNodalVars; ++nodalVarIndex)
  {
    double *nodalVars = new double[this->NumberOfNodes];
    int error = ex_get_nodal_var(this->FileId, this->CurrentTimeStep + 1,
                                 nodalVarIndex + 1, this->NumberOfNodes,
                                 nodalVars);
    std::vector<double*> varsVector(1, nodalVars);
    vtkNew<vtkCPExodusIIResultsArrayTemplate<double> > nodalVarArray;
    nodalVarArray->SetExodusScalarArrays(varsVector, this->NumberOfNodes);
    nodalVarArray->SetName(this->NodalVariableNames[nodalVarIndex].c_str());

    if (error < 0)
    {
      vtkErrorMacro("Failed to read nodal variable array '"
                    << this->NodalVariableNames[nodalVarIndex] << "'");
      return false;
    }

    this->PointData->AddArray(nodalVarArray.GetPointer());
  }
  return true;
}

//------------------------------------------------------------------------------
bool vtkCPExodusIIInSituReader::ExGetElemBlocks()
{
  const int numElemBlk = static_cast<int>(this->ElementBlockIds.size());
  const int numElemVars = static_cast<int>(this->ElementVariableNames.size());
  this->ElementBlocks->Initialize();
  this->ElementBlocks->SetNumberOfBlocks(numElemBlk);
  for (int blockInd = 0; blockInd < numElemBlk; ++blockInd)
  {
    std::string elemType(MAX_STR_LENGTH + 1, '\0');
    int numElem;
    int nodesPerElem;
    int numAttributes;

    int error = ex_get_elem_block(this->FileId,
                                  this->ElementBlockIds[blockInd],
                                  &(elemType[0]), &numElem, &nodesPerElem,
                                  &numAttributes);

    // Trim excess null chars from the type string:
    elemType.resize(strlen(elemType.c_str()));

    if (error < 0)
    {
      vtkErrorMacro("Failed to get the element block metadata for block "
                    << blockInd);
      return false;
    }

    // Get element block connectivity
    vtkNew<vtkCPExodusIIElementBlock> block;
    int *connect = new int[numElem * nodesPerElem];
    error = ex_get_elem_conn(this->FileId, this->ElementBlockIds[blockInd],
                             connect);
    if (!block->GetImplementation()->SetExodusConnectivityArray(
          connect, elemType, numElem, nodesPerElem))
    {
      delete [] connect;
      return false;
    }

    if (error < 0)
    {
      vtkErrorMacro("Failed to get the connectivity for block " << blockInd);
      return false;
    }

    // Use the mapped point container for the block points
    block->SetPoints(this->Points.GetPointer());

    // Add the point data arrays
    block->GetPointData()->ShallowCopy(this->PointData.GetPointer());

    // Read the element variables (cell data)
    for (int elemVarIndex = 0; elemVarIndex < numElemVars; ++elemVarIndex)
    {
      double *elemVars = new double[numElem];
      error = ex_get_elem_var(this->FileId, this->CurrentTimeStep + 1,
                              elemVarIndex + 1, this->ElementBlockIds[blockInd],
                              numElem, elemVars);
      std::vector<double*> varsVector(1, elemVars);
      vtkNew<vtkCPExodusIIResultsArrayTemplate<double> > elemVarArray;
      elemVarArray->SetExodusScalarArrays(varsVector, numElem);
      elemVarArray->SetName(this->ElementVariableNames[elemVarIndex].c_str());

      if (error < 0)
      {
        vtkErrorMacro("Failed to read element block variable array '"
                      << this->ElementVariableNames[elemVarIndex] << "'");
        return false;
      }

      block->GetCellData()->AddArray(elemVarArray.GetPointer());
    }

    // Add this element block to the multi-block data set
    this->ElementBlocks->SetBlock(blockInd, block.GetPointer());
  }

  return true;
}

//------------------------------------------------------------------------------
void vtkCPExodusIIInSituReader::ExClose()
{
  ex_close(this->FileId);
  this->FileId = -1;
}
