/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLDataReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkXMLDataReader.h"

#include "vtkArrayIteratorIncludes.h"
#include "vtkCallbackCommand.h"
#include "vtkCellData.h"
#include "vtkDataArray.h"
#include "vtkDataArraySelection.h"
#include "vtkDataSet.h"
#include "vtkPointData.h"
#include "vtkXMLDataElement.h"
#include "vtkXMLDataParser.h"
#include "vtkInformationVector.h"
#include "vtkInformation.h"
#include "vtkStreamingDemandDrivenPipeline.h"

#include "assert.h"


//----------------------------------------------------------------------------
vtkXMLDataReader::vtkXMLDataReader()
{
  this->NumberOfPieces = 0;
  this->PointDataElements = 0;
  this->CellDataElements = 0;
  this->Piece = 0;
  this->NumberOfPointArrays = 0;
  this->NumberOfCellArrays = 0;
  this->InReadData = 0;
  
  // Setup a callback for when the XMLParser's data reading routines
  // report progress.
  this->DataProgressObserver = vtkCallbackCommand::New();
  this->DataProgressObserver->SetCallback(&vtkXMLDataReader::DataProgressCallbackFunction);
  this->DataProgressObserver->SetClientData(this);

  this->PointDataTimeStep = NULL;
  this->PointDataOffset = NULL;
  this->CellDataTimeStep = NULL;
  this->CellDataOffset = NULL;
}

//----------------------------------------------------------------------------
vtkXMLDataReader::~vtkXMLDataReader()
{
  if(this->XMLParser)
    {
    this->DestroyXMLParser();
    }
  if(this->NumberOfPieces)
    {
    this->DestroyPieces();
    }
  this->DataProgressObserver->Delete();
  if( this->NumberOfPointArrays )
    {
    delete[] this->PointDataTimeStep;
    delete[] this->PointDataOffset;
    }
  if( this->NumberOfCellArrays )
    {
    delete[] this->CellDataTimeStep;
    delete[] this->CellDataOffset;
    }
}

//----------------------------------------------------------------------------
void vtkXMLDataReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkXMLDataReader::CreateXMLParser()
{
  this->Superclass::CreateXMLParser();
  this->XMLParser->AddObserver(vtkCommand::ProgressEvent,
                               this->DataProgressObserver);
}

//----------------------------------------------------------------------------
void vtkXMLDataReader::DestroyXMLParser()
{
  if(this->XMLParser)
    {
    this->XMLParser->RemoveObserver(this->DataProgressObserver);
    }
  this->Superclass::DestroyXMLParser();
}



//----------------------------------------------------------------------------
// Note that any changes (add or removing information) made to this method
// should be replicated in CopyOutputInformation
void vtkXMLDataReader::SetupOutputInformation(vtkInformation *outInfo)
{
  if (this->InformationError)
    {
    vtkErrorMacro("Should not still be processing output information if have set InformationError");
    return;
    }

  // Initialize DataArraySelections to enable all that are present
  this->SetDataArraySelections(this->PointDataElements[0], 
                               this->PointDataArraySelection);
  this->SetDataArraySelections(this->CellDataElements[0], 
                               this->CellDataArraySelection);

  // Setup the Field Information for PointData.  We only need the
  // information from one piece because all pieces have the same set of arrays.
  vtkInformationVector *infoVector = NULL;
  if (!this->SetFieldDataInfo(this->PointDataElements[0],
                              vtkDataObject::FIELD_ASSOCIATION_POINTS, 
                              this->GetNumberOfPoints(), infoVector))
    {
    return;
    }
  if (infoVector)
    {
    outInfo->Set(vtkDataObject::POINT_DATA_VECTOR(), infoVector);
    infoVector->Delete();
    }

  // now the Cell data
  infoVector = NULL;
  if (!this->SetFieldDataInfo(this->CellDataElements[0],
                              vtkDataObject::FIELD_ASSOCIATION_CELLS, 
                              this->GetNumberOfCells(), infoVector))
    {
    return;
    }
  if (infoVector)
    {
    outInfo->Set(vtkDataObject::CELL_DATA_VECTOR(), infoVector);
    infoVector->Delete();
    }
}

void vtkXMLDataReader::SetupUpdateExtentInformation(vtkInformation *outInfo)
{
  // get the current piece being requested
  int piece = 
    outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
  int npieces = 
    outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());
  
  // Setup the Field Information for PointData. 
  vtkInformationVector *infoVector = 
    outInfo->Get(vtkDataObject::POINT_DATA_VECTOR());
  if (!this->SetUpdateExtentInfo(this->PointDataElements[piece],
                                 infoVector, piece, npieces))
    {
    return;
    }

  // now the Cell data
  infoVector = outInfo->Get(vtkDataObject::CELL_DATA_VECTOR());
  if (!this->SetUpdateExtentInfo(this->CellDataElements[piece],
                                 infoVector, piece, npieces))
    {
    return;
    }
}

//----------------------------------------------------------------------------
int vtkXMLDataReader::SetUpdateExtentInfo(vtkXMLDataElement *eDSA, 
                                          vtkInformationVector *infoVector,
                                          int piece, int numPieces)
{
  if (!eDSA)
    {
    return 1;
    }

  int i;

  // Cycle through each data array
  for(i = 0; i < eDSA->GetNumberOfNestedElements(); i++)
    {
    vtkXMLDataElement* eNested = eDSA->GetNestedElement(i);
    vtkInformation *info = infoVector->GetInformationObject(i);
    
    double range[2];
    if (eNested->GetScalarAttribute("RangeMin", range[0]) &&
        eNested->GetScalarAttribute("RangeMax", range[1]))
      {
      info->Set(vtkDataObject::PIECE_FIELD_RANGE(), range, 2);
      if (piece == 0 && numPieces == 1)
        {
        info->Set(vtkDataObject::FIELD_RANGE(), range, 2);
        }
      }
    }
  
  return 1;
}

//----------------------------------------------------------------------------
void vtkXMLDataReader::CopyOutputInformation(vtkInformation *outInfo,
                                             int port)
{
  vtkInformation *localInfo = 
    this->GetExecutive()->GetOutputInformation( port );

  if ( localInfo->Has(vtkDataObject::POINT_DATA_VECTOR()) )
    {
    outInfo->CopyEntry( localInfo, vtkDataObject::POINT_DATA_VECTOR() );
    }
  if ( localInfo->Has(vtkDataObject::CELL_DATA_VECTOR()) )
    {
    outInfo->CopyEntry( localInfo, vtkDataObject::CELL_DATA_VECTOR() );
    }
}


//----------------------------------------------------------------------------
int vtkXMLDataReader::ReadPrimaryElement(vtkXMLDataElement* ePrimary)
{
  if(!this->Superclass::ReadPrimaryElement(ePrimary)) 
    { 
    return 0; 
    }
  
  // Count the number of pieces in the file.
  int numNested = ePrimary->GetNumberOfNestedElements();
  int numPieces = 0;
  int i;
  for(i=0; i < numNested; ++i)
    {
    vtkXMLDataElement* eNested = ePrimary->GetNestedElement(i);
    if(strcmp(eNested->GetName(), "Piece") == 0) 
      { 
      ++numPieces; 
      }
    }
  
  // Now read each piece.  If no "Piece" elements were found, assume
  // the primary element itself is a single piece.
  if(numPieces)
    {
    this->SetupPieces(numPieces);
    int piece = 0;
    for(i=0;i < numNested; ++i)
      {
      vtkXMLDataElement* eNested = ePrimary->GetNestedElement(i);
      if(strcmp(eNested->GetName(), "Piece") == 0)
        {
        if(!this->ReadPiece(eNested, piece++)) 
          { 
          return 0; 
          }
        }
      }
    }
  else
    {
    this->SetupPieces(1);
    if(!this->ReadPiece(ePrimary, 0)) 
      { 
      return 0; 
      }
    }
  return 1;
}

//----------------------------------------------------------------------------
void vtkXMLDataReader::SetupPieces(int numPieces)
{
  if(this->NumberOfPieces) 
    { 
    this->DestroyPieces(); 
    }
  this->NumberOfPieces = numPieces;
  if(numPieces > 0)
    {
    this->PointDataElements = new vtkXMLDataElement*[numPieces];
    this->CellDataElements = new vtkXMLDataElement*[numPieces];
    }
  int i;
  for(i=0;i < this->NumberOfPieces;++i)
    {
    this->PointDataElements[i] = 0;
    this->CellDataElements[i] = 0;
    }
}

//----------------------------------------------------------------------------
void vtkXMLDataReader::DestroyPieces()
{
  delete [] this->PointDataElements;
  delete [] this->CellDataElements;
  this->PointDataElements = 0;
  this->CellDataElements = 0;
  this->NumberOfPieces = 0;
}


//----------------------------------------------------------------------------
void vtkXMLDataReader::SetupOutputData()
{
  this->Superclass::SetupOutputData();
  
  vtkDataSet* output = vtkDataSet::SafeDownCast(this->GetCurrentOutput());
  vtkPointData* pointData = output->GetPointData();
  vtkCellData* cellData = output->GetCellData();
  
  // Get the size of the output arrays.
  unsigned long pointTuples = this->GetNumberOfPoints();
  unsigned long cellTuples = this->GetNumberOfCells();
  
  // Allocate the arrays in the output.  We only need the information
  // from one piece because all pieces have the same set of arrays.
  vtkXMLDataElement* ePointData = this->PointDataElements[0];
  vtkXMLDataElement* eCellData = this->CellDataElements[0];
  this->NumberOfPointArrays = 0;
  if (ePointData)
    {
    for (int i = 0; i < ePointData->GetNumberOfNestedElements(); i++)
      {
      vtkXMLDataElement* eNested = ePointData->GetNestedElement(i);
      if (this->PointDataArrayIsEnabled(eNested) && 
        !pointData->HasArray(eNested->GetAttribute("Name")))
        {
        this->NumberOfPointArrays++;
        vtkAbstractArray* array = this->CreateArray(eNested);
        if (array)
          {
          array->SetNumberOfTuples(pointTuples);
          pointData->AddArray(array);
          array->Delete();
          }
        else
          {
          this->DataError = 1;
          }
        }
      }
    }
  assert( this->NumberOfPointArrays == this->PointDataArraySelection->GetNumberOfArraysEnabled());

  this->NumberOfCellArrays = 0;
  if (eCellData)
    {
    for (int i = 0; i < eCellData->GetNumberOfNestedElements(); i++)
      {
      vtkXMLDataElement* eNested = eCellData->GetNestedElement(i);
      if (this->CellDataArrayIsEnabled(eNested) && 
        !cellData->HasArray(eNested->GetAttribute("Name")))
        {
        this->NumberOfCellArrays++;
        vtkAbstractArray* array = this->CreateArray(eNested);
        if (array)
          {
          array->SetNumberOfTuples(cellTuples);
          cellData->AddArray(array);
          array->Delete();
          }
        else
          {
          this->DataError = 1;
          }
        }
      }
    }
  assert( this->NumberOfCellArrays == this->CellDataArraySelection->GetNumberOfArraysEnabled());

  // Setup attribute indices for the point data and cell data.
  this->ReadAttributeIndices(ePointData, pointData);
  this->ReadAttributeIndices(eCellData, cellData);

  // Since NumberOfCellArrays and NumberOfPointArrays are valid
  // lets allocate PointDataTimeStep, CellDataTimeStep, PointDataOffset
  // CellDataOffset
  if( this->NumberOfPointArrays )
    {
    if (this->PointDataTimeStep)
      {
      delete [] this->PointDataTimeStep;
      }
    if (this->PointDataOffset)
      {
      delete [] this->PointDataOffset;
      }
    this->PointDataTimeStep = new int[this->NumberOfPointArrays];
    this->PointDataOffset = new unsigned long[this->NumberOfPointArrays];
    for(int i=0; i<this->NumberOfPointArrays;i++)
      {
      this->PointDataTimeStep[i] = -1;
      this->PointDataOffset[i] = static_cast<unsigned long>(-1);
      }
    }
  if( this->NumberOfCellArrays )
    {
    if( this->CellDataTimeStep != NULL )
      {
      delete[] this->CellDataTimeStep;
      }
    if( this->CellDataOffset != NULL )
      {
      delete[] this->CellDataOffset;
      }
    this->CellDataTimeStep = new int[this->NumberOfCellArrays];
    this->CellDataOffset = new unsigned long[this->NumberOfCellArrays];
    for(int i=0; i<this->NumberOfCellArrays;i++)
      {
      this->CellDataTimeStep[i] = -1;
      this->CellDataOffset[i]   = static_cast<unsigned long>(-1);
      }
    }
}

//----------------------------------------------------------------------------
int vtkXMLDataReader::ReadPiece(vtkXMLDataElement* ePiece, int piece)
{
  this->Piece = piece;
  return this->ReadPiece(ePiece);
}

//----------------------------------------------------------------------------
int vtkXMLDataReader::ReadPiece(vtkXMLDataElement* ePiece)
{
  // Find the PointData and CellData in the piece.
  int i;
  for(i=0; i < ePiece->GetNumberOfNestedElements(); ++i)
    {
    vtkXMLDataElement* eNested = ePiece->GetNestedElement(i);
    if(strcmp(eNested->GetName(), "PointData") == 0)
      {
      this->PointDataElements[this->Piece] = eNested;
      }
    else if(strcmp(eNested->GetName(), "CellData") == 0)
      {
      this->CellDataElements[this->Piece] = eNested;
      }
    }
  return 1;
}

//----------------------------------------------------------------------------
int vtkXMLDataReader::ReadPieceData(int piece)
{
  this->Piece = piece;
  return this->ReadPieceData();
}

//----------------------------------------------------------------------------
int vtkXMLDataReader::ReadPieceData()
{
  vtkDataSet* output = vtkDataSet::SafeDownCast(this->GetCurrentOutput());

  vtkPointData* pointData = output->GetPointData();
  vtkCellData* cellData = output->GetCellData();
  vtkXMLDataElement* ePointData = this->PointDataElements[this->Piece];
  vtkXMLDataElement* eCellData = this->CellDataElements[this->Piece];
  
  // Split current progress range over number of arrays.  This assumes
  // that each array contributes approximately the same amount of data
  // within this piece.
  float progressRange[2] = {0,0};
  int currentArray=0;
  int numArrays = this->NumberOfPointArrays + this->NumberOfCellArrays;
  this->GetProgressRange(progressRange);
  
  // Read the data for this piece from each array.
  int i;
  if(ePointData)
    {
    int a=0;
    for(i=0;(i < ePointData->GetNumberOfNestedElements() &&
             !this->AbortExecute);++i)
      {
      vtkXMLDataElement* eNested = ePointData->GetNestedElement(i);
      if (this->PointDataArrayIsEnabled(eNested))
        {
        if( strcmp(eNested->GetName(), "DataArray") != 0  && 
          strcmp(eNested->GetName(),"Array") != 0 )
          {
          vtkErrorMacro("Invalid Array.");
          this->DataError = 1;
          return 0;
          }
        int needToRead = this->PointDataNeedToReadTimeStep(eNested);
        if( needToRead )
          {
          // Set the range of progress for this array.
          this->SetProgressRange(progressRange, currentArray++, numArrays);

          // Read the array.
          if(!this->ReadArrayForPoints(eNested, pointData->GetAbstractArray(a++)))
            {
            vtkErrorMacro("Cannot read point data array \""
              << pointData->GetArray(a-1)->GetName() << "\" from "
              << ePointData->GetName() << " in piece " << this->Piece
              << ".  The data array in the element may be too short.");
            return 0;
            }
          }
        }
      }
    }
  if(eCellData)
    {
    int a=0;
    for(i=0;(i < eCellData->GetNumberOfNestedElements() &&
             !this->AbortExecute);++i)
      {
      vtkXMLDataElement* eNested = eCellData->GetNestedElement(i);
      if (this->CellDataArrayIsEnabled(eNested))
        {
        if( strcmp(eNested->GetName(), "DataArray") != 0 && 
          strcmp(eNested->GetName(),"Array") != 0 )
          {
          this->DataError = 1;
          vtkErrorMacro("Invalid Array" );
          return 0;
          }
        int needToRead = this->CellDataNeedToReadTimeStep(eNested);
        if( needToRead )
          {
          // Set the range of progress for this array.
          this->SetProgressRange(progressRange, currentArray++, numArrays);

          // Read the array.
          if(!this->ReadArrayForCells(eNested, cellData->GetAbstractArray(a++)))
            {
            vtkErrorMacro("Cannot read cell data array \""
              << cellData->GetAbstractArray(a-1)->GetName() << "\" from "
              << ePointData->GetName() << " in piece " << this->Piece
              << ".  The data array in the element may be too short.");
            return 0;
            }
          }
        }
      }
    }
  
  if(this->AbortExecute)
    {
    return 0;
    }
  
  return 1;
}

//----------------------------------------------------------------------------
void vtkXMLDataReader::ReadXMLData()
{
  // Let superclasses read data.  This also allocates output data.
  this->Superclass::ReadXMLData();

  if (this->FieldDataElement) // read the field data information
    {
    int i, numTuples;
    vtkFieldData *fieldData = this->GetCurrentOutput()->GetFieldData();
    for(i=0; i < this->FieldDataElement->GetNumberOfNestedElements() &&
             !this->AbortExecute; i++)
      {
      vtkXMLDataElement* eNested = this->FieldDataElement->GetNestedElement(i);
      vtkAbstractArray* array = this->CreateArray(eNested);
      if (array)
        {
        if(eNested->GetScalarAttribute("NumberOfTuples", numTuples))
          {
          array->SetNumberOfTuples(numTuples);
          }
        else
          {
          numTuples = 0;
          }
        fieldData->AddArray(array);
        array->Delete(); 
        if (!this->ReadArrayValues(eNested, 0, array, 0, numTuples*array->GetNumberOfComponents()))
          {
          this->DataError = 1;
          }
        }
      }
    }
}


//----------------------------------------------------------------------------
int vtkXMLDataReader::ReadArrayForPoints(vtkXMLDataElement* da,
                                         vtkAbstractArray* outArray)
{
  vtkIdType components = outArray->GetNumberOfComponents();
  vtkIdType numberOfTuples = this->GetNumberOfPoints();
  return this->ReadArrayValues(da, 0, outArray,0, numberOfTuples*components);
}

//----------------------------------------------------------------------------
int vtkXMLDataReader::ReadArrayForCells(vtkXMLDataElement* da,
                                        vtkAbstractArray* outArray)
{
  vtkIdType components = outArray->GetNumberOfComponents();
  vtkIdType numberOfTuples = this->GetNumberOfCells();
  return this->ReadArrayValues(da, 0, outArray,0, numberOfTuples*components);
}

//----------------------------------------------------------------------------
template <class iterT>
int vtkXMLDataReaderReadArrayValues(vtkXMLDataElement* da,
  vtkXMLDataParser* xmlparser, vtkIdType arrayIndex,
  iterT* iter, vtkIdType startIndex, vtkIdType numValues)
{
  if (!iter)
    {
    return 0;
    }
  vtkAbstractArray* array = iter->GetArray();
  // For all contiguous arrays (except vtkBitArray).
  vtkIdType num = numValues;
  int result;
  void* data = array->GetVoidPointer(arrayIndex);
  if(da->GetAttribute("offset"))
    {
    unsigned long offset = 0;
    da->GetScalarAttribute("offset", offset);
    result = (xmlparser->ReadAppendedData(offset, data, startIndex,
        numValues, array->GetDataType()) == num);
    }
  else
    {
    int isAscii = 1;
    const char* format = da->GetAttribute("format");
    if(format && (strcmp(format, "binary") == 0)) 
      { 
      isAscii = 0; 
      }
    result = (xmlparser->ReadInlineData(da, isAscii, data,
        startIndex, numValues, array->GetDataType()) == num);
    }
  return result;
}

//----------------------------------------------------------------------------
VTK_TEMPLATE_SPECIALIZE
int vtkXMLDataReaderReadArrayValues(
  vtkXMLDataElement* da,
  vtkXMLDataParser* xmlparser, vtkIdType arrayIndex,
  vtkArrayIteratorTemplate<vtkStdString>* iter, vtkIdType startIndex, vtkIdType numValues)
{
  // now, for strings, we have to read from the start, as we don't have 
  // support for index array yet.
  // So this specialization will read all strings starting from the beginning,
  // start putting the strings at the requested indices into the array 
  // until the request numValues are put into the array.
  vtkIdType bufstart = 0;
  vtkIdType actualNumValues = startIndex + numValues;
  
  int size = 1024;
  char* buffer = new char[size+1 + 7]; // +7 is leeway.
  buffer[1024] = 0; // to avoid string reads beyond buffer size.
  
  int inline_data = (da->GetAttribute("offset") == NULL);
  
  unsigned long offset = 0;
  if (inline_data == 0)
    {
    da->GetScalarAttribute("offset", offset);
    }

  int isAscii = 1;
  const char* format = da->GetAttribute("format");
  if(format && (strcmp(format, "binary") == 0)) 
    { 
    isAscii = 0; 
    }

  // Now read a buffer full of data, 
  // create strings out of it.
  int result = 1;
  vtkIdType inIndex = 0;
  vtkIdType outIndex = arrayIndex;
  vtkStdString prev_string;
  while (result && inIndex < actualNumValues)
    {
    int chars_read = 0;
    if (inline_data)
      {
      chars_read = xmlparser->ReadInlineData(da, isAscii, buffer,
          bufstart, size, VTK_CHAR);
      }
    else
      {
      chars_read = xmlparser->ReadAppendedData(offset, buffer, bufstart,
          size, VTK_CHAR);
      }
    if (!chars_read)
      {
      // failed.
      result = 0;
      break;
      }
    bufstart += chars_read;
    // now read strings 
    const char* ptr = buffer;
    const char* end_ptr = &buffer[chars_read];
    buffer[chars_read] = 0;
    
    while (ptr < end_ptr)
      {
      vtkStdString temp_string = ptr; // will read in string until 0x0;
      ptr += temp_string.size() + 1;
      if (prev_string.size() > 0)
        {
        temp_string = prev_string + temp_string;
        prev_string = "";
        }
      // now decide if the string terminated or buffer was full.
      if (ptr > end_ptr)
        {
        // buffer ended -- string is incomplete.
        // keep the prefix in temp_string.
        prev_string = temp_string;
        }
      else
        {
        // string read fully.
        if (inIndex >= startIndex)
          {
          // add string to the array.
          iter->GetValue(outIndex) = temp_string; // copy the value.
          outIndex++;
          }
        inIndex++;
        }
      }
    
    }
  delete [] buffer;
  return result;
}

//----------------------------------------------------------------------------
int vtkXMLDataReader::ReadArrayValues(vtkXMLDataElement* da, vtkIdType arrayIndex,
  vtkAbstractArray* array, vtkIdType startIndex,
  vtkIdType numValues)
{
  // Skip real read if aborting.
  if (this->AbortExecute)
    {
    return 0;
    }
  this->InReadData = 1;
  int result;
  // All arrays types except vtkBitArray.
  vtkArrayIterator* iter = array->NewIterator();
  switch (array->GetDataType())
    {
    vtkArrayIteratorTemplateMacro(
      result = vtkXMLDataReaderReadArrayValues(da, this->XMLParser,
        arrayIndex, static_cast<VTK_TT*>(iter), startIndex, numValues));
  default:
    result = 0;
    }
  if (iter)
    {
    iter->Delete();
    }
  this->InReadData = 0;
  return result;
}

//----------------------------------------------------------------------------
void vtkXMLDataReader::DataProgressCallbackFunction(vtkObject*, unsigned long,
                                                    void* clientdata, void*)
{
  reinterpret_cast<vtkXMLDataReader*>(clientdata)->DataProgressCallback();
}

//----------------------------------------------------------------------------
void vtkXMLDataReader::DataProgressCallback()
{
  if(this->InReadData)
    {
    float width = this->ProgressRange[1]-this->ProgressRange[0];
    float dataProgress = this->XMLParser->GetProgress();
    float progress = this->ProgressRange[0] + dataProgress*width;
    this->UpdateProgressDiscrete(progress);
    if(this->AbortExecute)
      {
      this->XMLParser->SetAbort(1);
      }
    }
}

//----------------------------------------------------------------------------
int vtkXMLDataReader::PointDataNeedToReadTimeStep(vtkXMLDataElement *eNested)
{
  // First thing need to find the id of this dataarray from its name:
  const char* name = eNested->GetAttribute("Name");
  int idx = this->PointDataArraySelection->GetEnabledArrayIndex(name);

  // Easy case no timestep:
  int numTimeSteps = eNested->GetVectorAttribute("TimeStep", 
    this->NumberOfTimeSteps, this->TimeSteps);
  if( !(numTimeSteps <= this->NumberOfTimeSteps) )
    {
    vtkErrorMacro("Invalid TimeStep specification");
    this->DataError = 1;
    return 0;
    }
  if (!numTimeSteps && !this->NumberOfTimeSteps)
    {
    assert( this->PointDataTimeStep[idx] == -1 ); //No timestep in this file
    return 1;
    }
  // else TimeStep was specified but no TimeValues associated were found
  assert( this->NumberOfTimeSteps ); 

  // case numTimeSteps > 1
  int isCurrentTimeInArray = 
    vtkXMLReader::IsTimeStepInArray(this->CurrentTimeStep, this->TimeSteps, numTimeSteps);
  if( numTimeSteps && !isCurrentTimeInArray)
    {
    return 0;
    }
  // we know that time steps are specified and that CurrentTimeStep is in the array
  // we need to figure out if we need to read the array or if it was forwarded
  // Need to check the current 'offset'
  unsigned long offset;
  if( eNested->GetScalarAttribute("offset", offset) )
    {
    if( this->PointDataOffset[idx] != offset )
      {
      // save the pointsOffset
      assert( this->PointDataTimeStep[idx] == -1 ); //cannot have mixture of binary and appended
      this->PointDataOffset[idx] = offset;
      return 1;
      }
    }
  else
    {
    // No offset is specified this is a binary file
    // First thing to check if numTimeSteps == 0:
    if( !numTimeSteps && this->NumberOfTimeSteps && this->PointDataTimeStep[idx] == -1)
      {
      // Update last PointsTimeStep read
      this->PointDataTimeStep[idx] = this->CurrentTimeStep;
      return 1;
      }
    int isLastTimeInArray = vtkXMLReader::IsTimeStepInArray(
      this->PointDataTimeStep[idx], this->TimeSteps, numTimeSteps);
     // If no time is specified or if time is specified and match then read
    if (isCurrentTimeInArray && !isLastTimeInArray)
      {
      // CurrentTimeStep is in TimeSteps but Last is not := need to read
      // Update last PointsTimeStep read
      this->PointDataTimeStep[idx] = this->CurrentTimeStep;
      return 1;
      }
    }
  // all other cases we don't need to read:
  return 0;
}

//----------------------------------------------------------------------------
int vtkXMLDataReader::CellDataNeedToReadTimeStep(vtkXMLDataElement *eNested)
{
  // First thing need to find the id of this dataarray from its name:
  const char* name = eNested->GetAttribute("Name");
  int idx = this->CellDataArraySelection->GetEnabledArrayIndex(name);

  // Easy case no timestep:
  int numTimeSteps = eNested->GetVectorAttribute("TimeStep", 
    this->NumberOfTimeSteps, this->TimeSteps);
  if( !(numTimeSteps <= this->NumberOfTimeSteps) )
    {
    vtkErrorMacro( "Invalid TimeSteps specification");
    this->DataError = 1;
    return 0;
    }
  if (!numTimeSteps && !this->NumberOfTimeSteps)
    {
    assert( this->CellDataTimeStep[idx] == -1 ); //No timestep in this file
    return 1;
    }
  // else TimeStep was specified but no TimeValues associated were found
  assert( this->NumberOfTimeSteps ); 

  // case numTimeSteps > 1
  int isCurrentTimeInArray = 
    vtkXMLReader::IsTimeStepInArray(this->CurrentTimeStep, this->TimeSteps, numTimeSteps);
  if( numTimeSteps && !isCurrentTimeInArray)
    {
    return 0;
    }
  // we know that time steps are specified and that CurrentTimeStep is in the array
  // we need to figure out if we need to read the array or if it was forwarded
  // Need to check the current 'offset'
  unsigned long offset;
  if( eNested->GetScalarAttribute("offset", offset) )
    {
    if( this->CellDataOffset[idx] != offset )
      {
      // save the pointsOffset
      assert( this->CellDataTimeStep[idx] == -1 ); //cannot have mixture of binary and appended
      this->CellDataOffset[idx] = offset;
      return 1;
      }
    }
  else
    {
    // No offset is specified this is a binary file
    // First thing to check if numTimeSteps == 0:
    if( !numTimeSteps && this->NumberOfTimeSteps && this->CellDataTimeStep[idx] == -1)
      {
      // Update last CellDataTimeStep read
      this->CellDataTimeStep[idx] = this->CurrentTimeStep;
      return 1;
      }
    int isLastTimeInArray = vtkXMLReader::IsTimeStepInArray(
      this->CellDataTimeStep[idx], this->TimeSteps, numTimeSteps);
     // If no time is specified or if time is specified and match then read
    if (isCurrentTimeInArray && !isLastTimeInArray)
      {
      // CurrentTimeStep is in TimeSteps but Last is not := need to read
      // Update last CellsTimeStep read
      this->CellDataTimeStep[idx] = this->CurrentTimeStep;
      return 1;
      }
    }
  // all other cases we don't need to read:
  return 0;
}
