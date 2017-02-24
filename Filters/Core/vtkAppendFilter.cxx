/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAppendFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkAppendFilter.h"

#include "vtkBoundingBox.h"
#include "vtkCellData.h"
#include "vtkCell.h"
#include "vtkDataSetAttributes.h"
#include "vtkDataSetCollection.h"
#include "vtkExecutive.h"
#include "vtkIncrementalOctreePointLocator.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkSmartPointer.h"
#include "vtkUnstructuredGrid.h"
#include "vtkStreamingDemandDrivenPipeline.h"

#include <set>
#include <string>

vtkStandardNewMacro(vtkAppendFilter);

//----------------------------------------------------------------------------
vtkAppendFilter::vtkAppendFilter()
{
  this->InputList = NULL;
  this->MergePoints = 0;
  this->OutputPointsPrecision = DEFAULT_PRECISION;
}

//----------------------------------------------------------------------------
vtkAppendFilter::~vtkAppendFilter()
{
  if (this->InputList != NULL)
  {
    this->InputList->Delete();
    this->InputList = NULL;
  }
}

//----------------------------------------------------------------------------
vtkDataSet *vtkAppendFilter::GetInput(int idx)
{
  if (idx >= this->GetNumberOfInputConnections(0) || idx < 0)
  {
    return NULL;
  }

  return vtkDataSet::SafeDownCast(
    this->GetExecutive()->GetInputData(0, idx));
}

//----------------------------------------------------------------------------
// Remove a dataset from the list of data to append.
void vtkAppendFilter::RemoveInputData(vtkDataSet *ds)
{
  if (!ds)
  {
    return;
  }
  int numCons = this->GetNumberOfInputConnections(0);
  for(int i=0; i<numCons; i++)
  {
    if (this->GetInput(i) == ds)
    {
      this->RemoveInputConnection(0,
        this->GetInputConnection(0, i));
    }
  }
}

//----------------------------------------------------------------------------
vtkDataSetCollection *vtkAppendFilter::GetInputList()
{
  if (this->InputList)
  {
    this->InputList->Delete();
  }
  this->InputList = vtkDataSetCollection::New();

  for (int idx = 0; idx < this->GetNumberOfInputConnections(0); ++idx)
  {
    if (this->GetInput(idx))
    {
      this->InputList->AddItem(this->GetInput(idx));
    }
  }

  return this->InputList;
}

//----------------------------------------------------------------------------
// Append data sets into single unstructured grid
int vtkAppendFilter::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  bool reallyMergePoints = false;
  if (this->MergePoints == 1 &&
      inputVector[0]->GetNumberOfInformationObjects() > 0 )
  {
    reallyMergePoints = true;

    // ensure that none of the inputs has ghost-cells.
    // (originally the code was checking for ghost cells only on 1st input,
    // that's not sufficient).
    for (int cc = 0; cc < inputVector[0]->GetNumberOfInformationObjects(); cc++)
    {
      vtkDataSet * tempData = vtkDataSet::GetData(inputVector[0], cc);
      if (tempData->HasAnyGhostCells())
      {
        vtkDebugMacro(<< "Ghost cells present, so points will not be merged");
        reallyMergePoints = false;
        break;
      }
    }
  }

  // get the output info object
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the ouptut
  vtkUnstructuredGrid *output = vtkUnstructuredGrid::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkDebugMacro(<<"Appending data together");

  // Loop over all data sets, checking to see what data is common to
  // all inputs. Note that data is common if 1) it is the same attribute
  // type (scalar, vector, etc.), 2) it is the same native type (int,
  // float, etc.), and 3) if a data array in a field, if it has the same name.
  vtkIdType totalNumPts = 0;
  vtkIdType totalNumCells = 0;

  vtkSmartPointer<vtkDataSetCollection> inputs;
  inputs.TakeReference(this->GetNonEmptyInputs(inputVector));

  vtkCollectionSimpleIterator iter;
  inputs->InitTraversal(iter);
  vtkDataSet* dataSet = 0;
  while ((dataSet = inputs->GetNextDataSet(iter)))
  {
    totalNumPts += dataSet->GetNumberOfPoints();
    totalNumCells += dataSet->GetNumberOfCells();
  }

  if ( totalNumPts < 1)
  {
    vtkDebugMacro(<<"No data to append!");
    return 1;
  }

  // Now we can allocate memory
  output->Allocate(totalNumCells);

  vtkSmartPointer<vtkPoints> newPts = vtkSmartPointer<vtkPoints>::New();

  // set precision for the points in the output
  if (this->OutputPointsPrecision == vtkAlgorithm::DEFAULT_PRECISION)
  {
    // take the precision of the first pointset
    int datatype = VTK_FLOAT;
    const int numInputs = inputVector[0]->GetNumberOfInformationObjects();
    for (int inputIndex = 0; inputIndex < numInputs; ++inputIndex)
    {
      vtkInformation* inInfo = inputVector[0]->GetInformationObject(inputIndex);
      vtkPointSet* ps = 0;
      if (inInfo)
      {
        ps = vtkPointSet::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
      }
      if ( ps != NULL && ps->GetNumberOfPoints() > 0)
      {
        datatype = ps->GetPoints()->GetDataType();
        break;
      }
    }
    newPts->SetDataType(datatype);
  }
  else if(this->OutputPointsPrecision == vtkAlgorithm::SINGLE_PRECISION)
  {
    newPts->SetDataType(VTK_FLOAT);
  }
  else if(this->OutputPointsPrecision == vtkAlgorithm::DOUBLE_PRECISION)
  {
    newPts->SetDataType(VTK_DOUBLE);
  }

  // If we aren't merging points, we need to allocate the points here.
  if (!reallyMergePoints)
  {
    newPts->SetNumberOfPoints(totalNumPts);
  }

  vtkSmartPointer<vtkIdList> ptIds = vtkSmartPointer<vtkIdList>::New();
  ptIds->Allocate(VTK_CELL_SIZE);
  vtkSmartPointer<vtkIdList> newPtIds = vtkSmartPointer<vtkIdList>::New();
  newPtIds->Allocate(VTK_CELL_SIZE);

  vtkIdType twentieth = (totalNumPts + totalNumCells)/20 + 1;

  // For optionally merging duplicate points
  vtkIdType* globalIndices = new vtkIdType[totalNumPts];
  vtkSmartPointer<vtkIncrementalOctreePointLocator> ptInserter;
  if (reallyMergePoints)
  {
    vtkBoundingBox outputBB;

    inputs->InitTraversal(iter);
    while ((dataSet = inputs->GetNextDataSet(iter)))
    {

      // Union of bounding boxes
      double localBox[6];
      dataSet->GetBounds(localBox);
      outputBB.AddBounds(localBox);
    }

    double outputBounds[6];
    outputBB.GetBounds(outputBounds);

    ptInserter = vtkSmartPointer<vtkIncrementalOctreePointLocator>::New();
    ptInserter->SetTolerance(0.0);
    ptInserter->InitPointInsertion(newPts, outputBounds);
  }

  // append the blocks / pieces in terms of the geoemetry and topology
  vtkIdType count = 0;
  vtkIdType ptOffset = 0;
  float decimal = 0.0;
  inputs->InitTraversal(iter);
  int abort = 0;
  while (!abort && (dataSet = inputs->GetNextDataSet(iter)))
  {
    vtkIdType dataSetNumPts = dataSet->GetNumberOfPoints();
    vtkIdType dataSetNumCells = dataSet->GetNumberOfCells();

    // copy points
    for (vtkIdType ptId = 0; ptId < dataSetNumPts && !abort; ++ptId)
    {
      if (reallyMergePoints)
      {
        vtkIdType globalPtId = 0;
        ptInserter->InsertUniquePoint(dataSet->GetPoint(ptId), globalPtId);
        globalIndices[ptId + ptOffset] = globalPtId;
        // The point inserter puts the point into newPts, so we don't have to do that here.
      }
      else
      {
        globalIndices[ptId + ptOffset] = ptId + ptOffset;
        newPts->SetPoint(ptId + ptOffset, dataSet->GetPoint(ptId));
      }

      // Update progress
      count++;
      if ( !(count % twentieth) )
      {
        decimal += 0.05;
        this->UpdateProgress(decimal);
        abort = this->GetAbortExecute();
      }
    }

    // copy cell
    vtkUnstructuredGrid *ug = vtkUnstructuredGrid::SafeDownCast(dataSet);
    for (vtkIdType cellId = 0; cellId < dataSetNumCells && !abort; ++cellId)
    {
      newPtIds->Reset ();
      if (ug && dataSet->GetCellType(cellId) == VTK_POLYHEDRON )
      {
        vtkIdType nfaces, *facePtIds;
        ug->GetFaceStream(cellId,nfaces,facePtIds);
        for(vtkIdType id=0; id < nfaces; ++id)
        {
          vtkIdType nPoints = facePtIds[0];
          newPtIds->InsertNextId(nPoints);
          for (vtkIdType j = 1; j <= nPoints; ++j)
          {
            newPtIds->InsertNextId(globalIndices[facePtIds[j] + ptOffset]);
          }
          facePtIds += nPoints + 1;
        }
        output->InsertNextCell(VTK_POLYHEDRON, nfaces, newPtIds->GetPointer(0));
      }
      else
      {
        dataSet->GetCellPoints(cellId, ptIds);
        for (vtkIdType id = 0; id < ptIds->GetNumberOfIds(); ++id)
        {
          newPtIds->InsertId(id, globalIndices[ptIds->GetId(id) + ptOffset]);
        }
        output->InsertNextCell(dataSet->GetCellType(cellId),newPtIds);
      }

      // Update progress
      count++;
      if ( !(count % twentieth) )
      {
        decimal += 0.05;
        this->UpdateProgress(decimal);
        abort = this->GetAbortExecute();
      }
    }
    ptOffset += dataSetNumPts;
  }


  // Now copy the array data
  this->AppendArrays(
    vtkDataObject::POINT, inputVector, globalIndices, output, newPts->GetNumberOfPoints());
  this->UpdateProgress(0.75);
  this->AppendArrays(vtkDataObject::CELL, inputVector, NULL, output, output->GetNumberOfCells());
  this->UpdateProgress(1.0);

  // Update ourselves and release memory
  output->SetPoints(newPts);
  output->Squeeze();

  delete[] globalIndices;

  return 1;
}

//----------------------------------------------------------------------------
vtkDataSetCollection* vtkAppendFilter::GetNonEmptyInputs(vtkInformationVector ** inputVector)
{
  vtkDataSetCollection* collection = vtkDataSetCollection::New();
  int numInputs = inputVector[0]->GetNumberOfInformationObjects();
  for (int inputIndex = 0; inputIndex < numInputs; ++inputIndex)
  {
    vtkInformation* inInfo = inputVector[0]->GetInformationObject(inputIndex);
    vtkDataSet* dataSet = NULL;
    if (inInfo)
    {
      dataSet = vtkDataSet::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
    }
    if (dataSet != NULL)
    {
      if (dataSet->GetNumberOfPoints() <= 0 && dataSet->GetNumberOfCells() <= 0)
      {
        continue; //no input, just skip
      }
      collection->AddItem(dataSet);
    }
  }

  return collection;
}

//----------------------------------------------------------------------------
void vtkAppendFilter::AppendArrays(int attributesType,
                                   vtkInformationVector **inputVector,
                                   vtkIdType* globalIds,
                                   vtkUnstructuredGrid* output,
                                   vtkIdType totalNumberOfElements)
{
  // Check if attributesType is supported
  if (attributesType != vtkDataObject::POINT && attributesType != vtkDataObject::CELL)
  {
    vtkErrorMacro(<< "Unhandled attributes type " << attributesType << ", must be either "
                  << "vtkDataObject::POINT or vtkDataObject::CELL");
    return;
  }

  //////////////////////////////////////////////////////////////////
  // Phase 1 - Find arrays to append based on name
  //////////////////////////////////////////////////////////////////

  // Store the set of data arrays common to all inputs. This set is
  // initialized with the data arrays from the first input and is
  // updated to be the intersection of it with the arrays from
  // subsequent inputs.
  std::set<std::string> dataArrayNames;

  vtkDataSetAttributes* outputData = output->GetAttributes(attributesType);

  bool isFirstInputData = true;
  vtkDataSetAttributes* firstInputData = NULL;
  vtkSmartPointer<vtkDataSetCollection> inputs;
  inputs.TakeReference(this->GetNonEmptyInputs(inputVector));
  vtkCollectionSimpleIterator iter;
  inputs->InitTraversal(iter);
  vtkDataSet* dataSet = NULL;
  while ((dataSet = inputs->GetNextDataSet(iter)))
  {
    vtkDataSetAttributes* inputData = dataSet->GetAttributes(attributesType);

    if (isFirstInputData)
    {
      isFirstInputData = false;
      firstInputData = inputData;
      for (int arrayIndex = 0; arrayIndex < inputData->GetNumberOfArrays(); ++arrayIndex)
      {
        vtkAbstractArray* array = inputData->GetAbstractArray(arrayIndex);
        if (array && array->GetName())
        {
          // NOTE - it is possible for an array to not have a name,
          // but be an active attribute. We'll deal with that case
          // later on.
          dataArrayNames.insert(std::string(array->GetName()));
        }
      }
    }
    else
    {
      std::set<std::string>::iterator it = dataArrayNames.begin();
      while (it != dataArrayNames.end())
      {
        const char* arrayName = it->c_str();
        vtkAbstractArray* array = inputData->GetAbstractArray(arrayName);
        vtkAbstractArray* firstArray = firstInputData->GetAbstractArray(arrayName);
        if (!array ||
            array->GetDataType() != firstArray->GetDataType() ||
            array->GetNumberOfComponents() != firstArray->GetNumberOfComponents())
        {
          // Incompatible array in this input. We can't append it.
          dataArrayNames.erase(it++);
        }
        else
        {
          ++it;
        }
      }
    }
  }

  // Allocate arrays for the output
  for (std::set<std::string>::iterator it = dataArrayNames.begin(); it != dataArrayNames.end(); ++it)
  {
    vtkAbstractArray* srcArray = firstInputData->GetAbstractArray((*it).c_str());
    vtkAbstractArray* dstArray = vtkAbstractArray::CreateArray(srcArray->GetDataType());
    dstArray->CopyInformation(srcArray->GetInformation());
    dstArray->SetName(srcArray->GetName());
    dstArray->SetNumberOfComponents(srcArray->GetNumberOfComponents());
    for (int j = 0; j < srcArray->GetNumberOfComponents(); ++j)
    {
      if (srcArray->GetComponentName(j))
      {
        dstArray->SetComponentName(j, srcArray->GetComponentName(j));
      }
    }
    dstArray->SetNumberOfTuples(totalNumberOfElements);
    outputData->AddArray(dstArray);
    dstArray->Delete();
  }

  //////////////////////////////////////////////////////////////////
  // Phase 2 - Set up arrays as attributes
  //////////////////////////////////////////////////////////////////

  // Set active attributes in the outputs only if all the inputs have
  // the same active attributes name (or the name is NULL).
  vtkAbstractArray* attributeArrays[vtkDataSetAttributes::NUM_ATTRIBUTES];

  // Initialize with the active attribute from the first input
  for (int attribute = 0; attribute < vtkDataSetAttributes::NUM_ATTRIBUTES; ++attribute)
  {
    attributeArrays[attribute] = firstInputData->GetAbstractAttribute(attribute);
  }

  inputs->InitTraversal(iter);
  while ((dataSet = inputs->GetNextDataSet(iter)))
  {
    for (int attributeIndex = 0; attributeIndex < vtkDataSetAttributes::NUM_ATTRIBUTES; ++attributeIndex)
    {
      if (attributeArrays[attributeIndex])
      {
        vtkDataSetAttributes* inputData = dataSet->GetAttributes(attributesType);
        vtkAbstractArray* thisArray = inputData->GetAbstractAttribute(attributeIndex);
        bool matches = thisArray &&
          ((attributeArrays[attributeIndex]->GetName() == NULL && thisArray->GetName() == NULL) ||
           strcmp(attributeArrays[attributeIndex]->GetName(), thisArray->GetName()) == 0);
        if (!matches)
        {
          // This input doesn't agree on the active attribute, so unset it.
          attributeArrays[attributeIndex] = NULL;
        }
      }
    }
  }

  // Set the active attributes
  for (int attributeIndex = 0; attributeIndex < vtkDataSetAttributes::NUM_ATTRIBUTES; ++attributeIndex)
  {
    if (attributeArrays[attributeIndex])
    {
      const char* arrayName = attributeArrays[attributeIndex]->GetName();
      if (arrayName)
      {
        outputData->SetActiveAttribute(arrayName, attributeIndex);
      }
    }
  }

  //////////////////////////////////////////////////////////////////
  // Phase 3 - Handle attributes with no name
  //////////////////////////////////////////////////////////////////

  // Now check if we need NULL-named arrays for the special case where
  // the active attributes are set to an array with a NULL name.  It's
  // important to point out that vtkFieldData can have more than one
  // array with a NULL name. We append only those arrays with a NULL
  // name that are set as the active attribute because otherwise we
  // have no information about how to append NULL-named arrays.
  bool attributeNeedsNullArray[vtkDataSetAttributes::NUM_ATTRIBUTES];
  for (int attributeIndex = 0; attributeIndex < vtkDataSetAttributes::NUM_ATTRIBUTES; ++attributeIndex)
  {
    attributeNeedsNullArray[attributeIndex] = true;
  }
  inputs->InitTraversal(iter);
  while ((dataSet = inputs->GetNextDataSet(iter)))
  {
    for (int attributeIndex = 0; attributeIndex < vtkDataSetAttributes::NUM_ATTRIBUTES; ++attributeIndex)
    {
      // Check if the attribute array name is NULL. If attribute is
      // not set or the name is not NULL, we do not need a NULL
      // array.
      vtkDataSetAttributes *inputData = dataSet->GetAttributes(attributesType);
      vtkDataArray* attributeArray = inputData->GetAttribute(attributeIndex);
      vtkDataArray* firstAttributeArray = firstInputData->GetAttribute(attributeIndex);
      if (!attributeArray || attributeArray->GetName() ||
          (attributeArray->GetNumberOfComponents() != firstAttributeArray->GetNumberOfComponents()) ||
          (attributeArray->GetDataType() != firstAttributeArray->GetDataType()))
      {
        attributeNeedsNullArray[attributeIndex] = false;
      }
    }
  }

  // Now allocate the attribute arrays we need
  for (int attributeIndex = 0; attributeIndex < vtkDataSetAttributes::NUM_ATTRIBUTES; ++attributeIndex)
  {
    if (attributeNeedsNullArray[attributeIndex])
    {
      vtkAbstractArray* srcArray = firstInputData->GetAttribute(attributeIndex);
      vtkAbstractArray* dstArray = vtkAbstractArray::CreateArray(srcArray->GetDataType());
      dstArray->SetNumberOfComponents(srcArray->GetNumberOfComponents());
      for (int j = 0; j < srcArray->GetNumberOfComponents(); ++j)
      {
        if (srcArray->GetComponentName(j))
        {
          dstArray->SetComponentName(j, srcArray->GetComponentName(j));
        }
      }
      dstArray->SetNumberOfTuples(totalNumberOfElements);
      outputData->SetAttribute(dstArray, attributeIndex);
      dstArray->Delete();
    }
  }

  //////////////////////////////////////////////////////////////
  // Phase 4 - Copy data
  //////////////////////////////////////////////////////////////
  vtkIdType offset = 0;
  inputs->InitTraversal(iter);
  while ((dataSet = inputs->GetNextDataSet(iter)))
  {
    vtkDataSetAttributes* inputData = dataSet->GetAttributes(attributesType);
    for (std::set<std::string>::iterator it = dataArrayNames.begin(); it != dataArrayNames.end(); ++it)
    {
      const char* arrayName = it->c_str();
      vtkAbstractArray* srcArray = inputData->GetAbstractArray(arrayName);
      vtkAbstractArray* dstArray = outputData->GetAbstractArray(arrayName);

      for (vtkIdType id = 0; id < srcArray->GetNumberOfTuples(); ++id)
      {
        if (globalIds)
        {
          dstArray->SetTuple(globalIds[id + offset], id, srcArray);
        }
        else
        {
          dstArray->SetTuple(id + offset, id, srcArray);
        }
      }
    }

    // Copy attributes
    for (int attribute = 0; attribute < vtkDataSetAttributes::NUM_ATTRIBUTES; ++attribute)
    {
      vtkAbstractArray* srcArray = inputData->GetAbstractAttribute(attribute);
      vtkAbstractArray* dstArray = outputData->GetAbstractAttribute(attribute);

      // Copy if only the array name is NULL. If the array name is non-NULL, it will
      // have been copied in the loop above.
      if (srcArray && !srcArray->GetName() &&
          dstArray && !dstArray->GetName())
      {
        for (vtkIdType id = 0; id < srcArray->GetNumberOfTuples(); ++id)
        {
          if (globalIds)
          {
            dstArray->SetTuple(globalIds[id + offset], id, srcArray);
          }
          else
          {
            dstArray->SetTuple(id + offset, id, srcArray);
          }
        }
      }
    }

    if (attributesType == vtkDataObject::POINT)
    {
      offset += dataSet->GetNumberOfPoints();
    }
    else if (attributesType == vtkDataObject::CELL)
    {
      offset += dataSet->GetNumberOfCells();
    }
  }
}

//----------------------------------------------------------------------------
int vtkAppendFilter::RequestUpdateExtent(vtkInformation *vtkNotUsed(request),
                                         vtkInformationVector **inputVector,
                                         vtkInformationVector *vtkNotUsed(outputVector))
{
  int numInputConnections = this->GetNumberOfInputConnections(0);

  // Let downstream request a subset of connection 0, for connections >= 1
  // send their WHOLE_EXTENT as UPDATE_EXTENT.
  for (int idx = 1; idx < numInputConnections; ++idx)
  {
    vtkInformation * inputInfo = inputVector[0]->GetInformationObject(idx);
    if (inputInfo->Has(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()))
    {
      int ext[6];
      inputInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), ext);
      inputInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), ext, 6);
    }
  }

  return 1;
}

//----------------------------------------------------------------------------
int vtkAppendFilter::FillInputPortInformation(int, vtkInformation *info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  info->Set(vtkAlgorithm::INPUT_IS_REPEATABLE(), 1);
  return 1;
}

//----------------------------------------------------------------------------
void vtkAppendFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "MergePoints:" << (this->MergePoints?"On":"Off") << "\n";
  os << indent << "OutputPointsPrecision: "
     << this->OutputPointsPrecision << "\n";
}
