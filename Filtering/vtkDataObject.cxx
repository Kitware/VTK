/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkDataObject.cxx

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkDataObject.h"

#include "vtkAlgorithmOutput.h"
#include "vtkDataSetAttributes.h"
#include "vtkExtentTranslator.h"
#include "vtkFieldData.h"
#include "vtkGarbageCollector.h"
#include "vtkInformation.h"
#include "vtkObjectFactory.h"
#include "vtkSource.h"
#include "vtkTrivialProducer.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkInformationDataObjectKey.h"
#include "vtkInformationDoubleKey.h"
#include "vtkInformationDoubleVectorKey.h"
#include "vtkInformationExecutivePortKey.h"
#include "vtkInformationExecutivePortVectorKey.h"
#include "vtkInformationIntegerKey.h"
#include "vtkInformationIntegerPointerKey.h"
#include "vtkInformationIntegerVectorKey.h"
#include "vtkInformationInformationVectorKey.h"
#include "vtkInformationStringKey.h"
#include "vtkInformationVector.h"
#include "vtkDataSetAttributes.h"

vtkStandardNewMacro(vtkDataObject);

vtkCxxSetObjectMacro(vtkDataObject,Information,vtkInformation);
vtkCxxSetObjectMacro(vtkDataObject,FieldData,vtkFieldData);

vtkInformationKeyMacro(vtkDataObject, DATA_TYPE_NAME, String);
vtkInformationKeyMacro(vtkDataObject, DATA_OBJECT, DataObject);
vtkInformationKeyMacro(vtkDataObject, DATA_EXTENT_TYPE, Integer);
vtkInformationKeyMacro(vtkDataObject, DATA_PIECE_NUMBER, Integer);
vtkInformationKeyMacro(vtkDataObject, DATA_NUMBER_OF_PIECES, Integer);
vtkInformationKeyMacro(vtkDataObject, DATA_NUMBER_OF_GHOST_LEVELS, Integer);
vtkInformationKeyMacro(vtkDataObject, DATA_RESOLUTION, Double);
vtkInformationKeyMacro(vtkDataObject, DATA_TIME_STEPS, DoubleVector);
vtkInformationKeyMacro(vtkDataObject, POINT_DATA_VECTOR, InformationVector);
vtkInformationKeyMacro(vtkDataObject, CELL_DATA_VECTOR, InformationVector);
vtkInformationKeyMacro(vtkDataObject, VERTEX_DATA_VECTOR, InformationVector);
vtkInformationKeyMacro(vtkDataObject, EDGE_DATA_VECTOR, InformationVector);
vtkInformationKeyMacro(vtkDataObject, FIELD_ARRAY_TYPE, Integer);
vtkInformationKeyMacro(vtkDataObject, FIELD_ASSOCIATION, Integer);
vtkInformationKeyMacro(vtkDataObject, FIELD_ATTRIBUTE_TYPE, Integer);
vtkInformationKeyMacro(vtkDataObject, FIELD_ACTIVE_ATTRIBUTE, Integer);
vtkInformationKeyMacro(vtkDataObject, FIELD_NAME, String);
vtkInformationKeyMacro(vtkDataObject, FIELD_NUMBER_OF_COMPONENTS, Integer);
vtkInformationKeyMacro(vtkDataObject, FIELD_NUMBER_OF_TUPLES, Integer);
vtkInformationKeyRestrictedMacro(vtkDataObject, FIELD_RANGE, DoubleVector, 2);
vtkInformationKeyRestrictedMacro(vtkDataObject, PIECE_FIELD_RANGE, DoubleVector, 2);
vtkInformationKeyRestrictedMacro(vtkDataObject, PIECE_EXTENT, IntegerVector, 6);
vtkInformationKeyMacro(vtkDataObject, FIELD_OPERATION, Integer);
vtkInformationKeyRestrictedMacro(vtkDataObject, DATA_EXTENT, IntegerPointer, 6);
vtkInformationKeyRestrictedMacro(vtkDataObject, ORIGIN, DoubleVector, 3);
vtkInformationKeyRestrictedMacro(vtkDataObject, SPACING, DoubleVector, 3);
vtkInformationKeyMacro(vtkDataObject, DATA_GEOMETRY_UNMODIFIED, Integer);
vtkInformationKeyMacro(vtkDataObject, SIL, DataObject);

class vtkDataObjectToSourceFriendship
{
public:
  static void SetOutput(vtkSource* source, int i, vtkDataObject* newData)
    {
    if(source)
      {
      // Make sure there is room in the source for this output.
      if(i >= source->NumberOfOutputs)
        {
        source->SetNumberOfOutputs(i+1);
        }

      // Update the source's Outputs array.
      vtkDataObject* oldData = source->Outputs[i];
      if(newData)
        {
        newData->Register(source);
        }
      source->Outputs[i] = newData;
      if(oldData)
        {
        oldData->UnRegister(source);
        }
      }
    }
};

// Initialize static member that controls global data release 
// after use by filter
static int vtkDataObjectGlobalReleaseDataFlag = 0;

const char vtkDataObject
::AssociationNames[vtkDataObject::NUMBER_OF_ASSOCIATIONS][55] =
{
  "vtkDataObject::FIELD_ASSOCIATION_POINTS",
  "vtkDataObject::FIELD_ASSOCIATION_CELLS",
  "vtkDataObject::FIELD_ASSOCIATION_NONE",
  "vtkDataObject::FIELD_ASSOCIATION_POINTS_THEN_CELLS",
  "vtkDataObject::FIELD_ASSOCIATION_VERTICES",
  "vtkDataObject::FIELD_ASSOCIATION_EDGES",
  "vtkDataObject::FIELD_ASSOCIATION_ROWS"
};

//----------------------------------------------------------------------------
vtkDataObject::vtkDataObject()
{
  this->Source = NULL;
  this->PipelineInformation = 0;

  this->Information = vtkInformation::New();
  this->Information->Register(this);
  this->Information->Delete();

  // We have to assume that if a user is creating the data on their own,
  // then they will fill it with valid data.
  this->DataReleased = 0;

  this->FieldData = NULL;
  vtkFieldData *fd = vtkFieldData::New();
  this->SetFieldData(fd);
  fd->Delete();
}

//----------------------------------------------------------------------------
vtkDataObject::~vtkDataObject()
{
  this->SetPipelineInformation(0);
  this->SetInformation(0);
  this->SetFieldData(NULL);
}

//----------------------------------------------------------------------------
void vtkDataObject::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  if ( this->Source )
    {
    os << indent << "Source: " << this->Source << "\n";
    }
  else
    {
    os << indent << "Source: (none)\n";
    }
  
  if ( this->Information )
    {
    os << indent << "Information: " << this->Information << "\n";
    }
  else
    {
    os << indent << "Information: (none)\n";
    }

  os << indent << "Data Released: "
     << (this->DataReleased ? "True\n" : "False\n");
  os << indent << "Global Release Data: "
     << (vtkDataObjectGlobalReleaseDataFlag ? "On\n" : "Off\n");

  os << indent << "UpdateTime: " << this->UpdateTime << endl;

  if(vtkInformation* pInfo = this->GetPipelineInformation())
    {
    os << indent << "Release Data: "
       << (this->GetReleaseDataFlag() ? "On\n" : "Off\n");
    if(pInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT_INITIALIZED()))
      {
      os << indent << "UpdateExtent: Initialized\n";
      }
    else
      {
      os << indent << "UpdateExtent: Not Initialized\n";
      }
    if(pInfo->Has(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT()))
      {
      int updateExtent[6] = {0,-1,0,-1,0,-1};
      this->GetUpdateExtent(updateExtent);
      os << indent << "UpdateExtent: " << updateExtent[0] << ", "
         << updateExtent[1] << ", " << updateExtent[2] << ", "
         << updateExtent[3] << ", " << updateExtent[4] << ", "
         << updateExtent[5] << endl;
      }
    if(pInfo->Has(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES()))
      {
      os << indent << "Update Number Of Pieces: "
         << pInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES())
         << endl;
      }
    if(pInfo->Has(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER()))
      {
      os << indent << "Update Piece: "
         << pInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER())
         << endl;
      }
    if(pInfo->Has(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS()))
      {
      os << indent << "Update Ghost Level: "
         << pInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS())
         << endl;
      }
    if(pInfo->Has(vtkStreamingDemandDrivenPipeline::UPDATE_RESOLUTION()))
      {
      os << indent << "Update Resolution: "
         << pInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_RESOLUTION())
         << endl;
      }
    if(pInfo->Has(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()))
      {
      int wholeExtent[6] = {0,-1,0,-1,0,-1};
      this->GetWholeExtent(wholeExtent);
      os << indent << "WholeExtent: " << wholeExtent[0] << ", "
         << wholeExtent[1] << ", " << wholeExtent[2] << ", "
         << wholeExtent[3] << ", " << wholeExtent[4] << ", "
         << wholeExtent[5] << endl;
      }
    if(pInfo->Has(vtkStreamingDemandDrivenPipeline::MAXIMUM_NUMBER_OF_PIECES()))
      {
      os << indent << "MaximumNumberOfPieces: "
         << pInfo->Get(vtkStreamingDemandDrivenPipeline::MAXIMUM_NUMBER_OF_PIECES())
         << endl;
      }
    if(pInfo->Has(vtkStreamingDemandDrivenPipeline::EXTENT_TRANSLATOR()))
      {
      os << indent << "ExtentTranslator: ("
         << pInfo->Get(vtkStreamingDemandDrivenPipeline::EXTENT_TRANSLATOR())
         << ")\n";
      }
    if(pInfo->Get(vtkStreamingDemandDrivenPipeline::EXACT_EXTENT()))
      {
      os << indent << "RequestExactExtent: On\n ";
      }
    else
      {
      os << indent << "RequestExactExtent: Off\n ";
      }
    }

  os << indent << "Field Data:\n";
  this->FieldData->PrintSelf(os,indent.GetNextIndent());
}

//----------------------------------------------------------------------------
void vtkDataObject::SetUpdateExtent(int piece, int numPieces, int ghostLevel)
{
  if(SDDP* sddp = this->TrySDDP("SetUpdateExtent"))
    {
    // this code used to check the return value of the
    // SetUpdateExtent method which would indicate if the
    // UpdatePiece, UpdateNumberOfPieces, or UpdateGhostLevel
    // had changed, and call Modified(). We actually don't want
    // to do this - just a change in the update extent does not
    // make this object modified!
    sddp->SetUpdateExtent
      (sddp->GetOutputInformation()->GetInformationObject
       (this->GetPortNumber()), piece, numPieces, ghostLevel);
    }
}


//----------------------------------------------------------------------------
void vtkDataObject::SetPipelineInformation(vtkInformation* newInfo)
{
  vtkInformation* oldInfo = this->PipelineInformation;
  if(newInfo != oldInfo)
    {
    // Remove any existing compatibility link to a source.
    this->Source = 0;

    if(newInfo)
      {
      // Reference the new information.
      newInfo->Register(this);

      // Detach the output that used to be held by the new information.
      if(vtkDataObject* oldData = newInfo->Get(vtkDataObject::DATA_OBJECT()))
        {
        oldData->Register(this);
        oldData->SetPipelineInformation(0);
        oldData->UnRegister(this);
        }

      // Tell the new information about this object.
      newInfo->Set(vtkDataObject::DATA_OBJECT(), this);

      // If the new producer is a vtkSource then setup the backward
      // compatibility link.
      vtkExecutive* newExec = vtkExecutive::PRODUCER()->GetExecutive(newInfo);
      int newPort = vtkExecutive::PRODUCER()->GetPort(newInfo);
      if(newExec)
        {
        vtkSource* newSource = vtkSource::SafeDownCast(newExec->GetAlgorithm());
        if(newSource)
          {
          vtkDataObjectToSourceFriendship::SetOutput(newSource, newPort, this);
          this->Source = newSource;
          }
        }
      }

    // Save the pointer to the new information.
    this->PipelineInformation = newInfo;

    if(oldInfo)
      {
      // If the old producer was a vtkSource then remove the backward
      // compatibility link.
      vtkExecutive* oldExec = vtkExecutive::PRODUCER()->GetExecutive(oldInfo);
      int oldPort = vtkExecutive::PRODUCER()->GetPort(oldInfo);
      if(oldExec)
        {
        vtkSource* oldSource = vtkSource::SafeDownCast(oldExec->GetAlgorithm());
        if(oldSource)
          {
          vtkDataObjectToSourceFriendship::SetOutput(oldSource, oldPort, 0);
          }
        }

      // Remove the old information's reference to us.
      oldInfo->Set(vtkDataObject::DATA_OBJECT(), 0);

      // Remove our reference to the old information.
      oldInfo->UnRegister(this);
      }
    }
}

//----------------------------------------------------------------------------
vtkAlgorithmOutput* vtkDataObject::GetProducerPort()
{  
  // Make sure there is an executive.
  if(!this->GetExecutive())
    {
    vtkTrivialProducer* tp = vtkTrivialProducer::New();
    tp->SetOutput(this);
    tp->Delete();
    }

  // Get the port from the executive.
  return this->GetExecutive()->GetProducerPort(this);
}

//----------------------------------------------------------------------------
// Determine the modified time of this object
unsigned long int vtkDataObject::GetMTime()
{
  unsigned long result;

  result = vtkObject::GetMTime();
  if ( this->FieldData )
    {
    unsigned long mtime = this->FieldData->GetMTime();
    result = ( mtime > result ? mtime : result);
    }
  
  return result;
}

//----------------------------------------------------------------------------
void vtkDataObject::Initialize()
{
  if (this->FieldData)
    {
    this->FieldData->Initialize();
    }

  if (this->Information)
    {
    // Make sure the information is cleared.
    this->Information->Remove(DATA_PIECE_NUMBER());
    this->Information->Remove(DATA_NUMBER_OF_PIECES());
    this->Information->Remove(DATA_NUMBER_OF_GHOST_LEVELS());
    this->Information->Remove(DATA_TIME_STEPS());
    this->Information->Remove(DATA_RESOLUTION());
    }

  this->Modified();
}

//----------------------------------------------------------------------------
void vtkDataObject::SetGlobalReleaseDataFlag(int val)
{
  if (val == vtkDataObjectGlobalReleaseDataFlag)
    {
    return;
    }
  vtkDataObjectGlobalReleaseDataFlag = val;
}

//----------------------------------------------------------------------------
void vtkDataObject::CopyInformationToPipeline(vtkInformation *request, 
                                              vtkInformation *input,
                                              vtkInformation *output,
                                              int vtkNotUsed(forceCopy))
{
  // Set default pipeline information during a request for information.
  if(request->Has(vtkDemandDrivenPipeline::REQUEST_INFORMATION()))
    {
    // Copy point and cell data from the input if available.  Otherwise use our
    // current settings.

    if (input)
      {
      // copy point data.
      if (input->Has(POINT_DATA_VECTOR()))
        {
        output->CopyEntry(input, POINT_DATA_VECTOR(), 1);
        }
      // copy cell data.
      if (input && input->Has(CELL_DATA_VECTOR()))
        {
        output->CopyEntry(input, CELL_DATA_VECTOR(), 1);
        }
      // copy vertex data.
      if (input->Has(VERTEX_DATA_VECTOR()))
        {
        output->CopyEntry(input, VERTEX_DATA_VECTOR(), 1);
        }
      // copy edge data.
      if (input && input->Has(EDGE_DATA_VECTOR()))
        {
        output->CopyEntry(input, EDGE_DATA_VECTOR(), 1);
        }

      // copy the actual time
      if (input->Has(DATA_TIME_STEPS()))
        {
        output->CopyEntry(input, DATA_TIME_STEPS());
        }
      }
    }
  if(request->Has(vtkDemandDrivenPipeline::REQUEST_DATA()))
    {
    if (input)
      {
      if (input->Has(DATA_RESOLUTION()))
        {
        output->CopyEntry(input, DATA_RESOLUTION());
        }
      }
    }
}

//----------------------------------------------------------------------------
void vtkDataObject::CopyInformationFromPipeline(vtkInformation*)
{
  // Copy nothing by default.
}

//----------------------------------------------------------------------------
vtkInformation *vtkDataObject::GetActiveFieldInformation(vtkInformation *info, 
    int fieldAssociation, int attributeType)
{
  int i;
  vtkInformation *fieldDataInfo;
  vtkInformationVector *fieldDataInfoVector;
  
  if (fieldAssociation == FIELD_ASSOCIATION_POINTS)
    {
    fieldDataInfoVector = info->Get(POINT_DATA_VECTOR());
    }
  else if (fieldAssociation == FIELD_ASSOCIATION_CELLS)
    {
    fieldDataInfoVector = info->Get(CELL_DATA_VECTOR());
    }
  else if (fieldAssociation == FIELD_ASSOCIATION_VERTICES)
    {
    fieldDataInfoVector = info->Get(VERTEX_DATA_VECTOR());
    }
  else if (fieldAssociation == FIELD_ASSOCIATION_EDGES)
    {
    fieldDataInfoVector = info->Get(EDGE_DATA_VECTOR());
    }
  else
    {
    vtkGenericWarningMacro("Unrecognized field association!");
    return NULL;
    }

  if (!fieldDataInfoVector)
    {
    return NULL;
    }

  for (i = 0; i < fieldDataInfoVector->GetNumberOfInformationObjects(); i++)
    {
    fieldDataInfo = fieldDataInfoVector->GetInformationObject(i);
    if ( fieldDataInfo->Has(FIELD_ACTIVE_ATTRIBUTE()) &&
      (fieldDataInfo->Get(FIELD_ACTIVE_ATTRIBUTE()) & (1 << attributeType )) )
      {
      return fieldDataInfo;
      }
    }
  return NULL;
}

//----------------------------------------------------------------------------
vtkInformation *vtkDataObject::GetNamedFieldInformation(vtkInformation *info, 
                                                        int fieldAssociation, 
                                                        const char *name)
{
  int i;
  vtkInformation *fieldDataInfo;
  vtkInformationVector *fieldDataInfoVector;
  
  if (fieldAssociation == FIELD_ASSOCIATION_POINTS)
    {
    fieldDataInfoVector = info->Get(POINT_DATA_VECTOR());
    }
  else if (fieldAssociation == FIELD_ASSOCIATION_CELLS)
    {
    fieldDataInfoVector = info->Get(CELL_DATA_VECTOR());
    }
  else if (fieldAssociation == FIELD_ASSOCIATION_VERTICES)
    {
    fieldDataInfoVector = info->Get(VERTEX_DATA_VECTOR());
    }
  else if (fieldAssociation == FIELD_ASSOCIATION_EDGES)
    {
    fieldDataInfoVector = info->Get(EDGE_DATA_VECTOR());
    }
  else
    {
    vtkGenericWarningMacro("Unrecognized field association!");
    return NULL;
    }
  
  if (!fieldDataInfoVector)
    {
    return NULL;
    }
  
  for (i = 0; i < fieldDataInfoVector->GetNumberOfInformationObjects(); i++)
    {
    fieldDataInfo = fieldDataInfoVector->GetInformationObject(i);
    if ( fieldDataInfo->Has(FIELD_NAME()) &&
         !strcmp(fieldDataInfo->Get(FIELD_NAME()),name))
      {
      return fieldDataInfo;
      }
    }
  return NULL;
}

//----------------------------------------------------------------------------
void vtkDataObject::RemoveNamedFieldInformation(vtkInformation *info, 
                                                int fieldAssociation, 
                                                const char *name)
{
  int i;
  vtkInformation *fieldDataInfo;
  vtkInformationVector *fieldDataInfoVector;
  
  if (fieldAssociation == FIELD_ASSOCIATION_POINTS)
    {
    fieldDataInfoVector = info->Get(POINT_DATA_VECTOR());
    }
  else if (fieldAssociation == FIELD_ASSOCIATION_CELLS)
    {
    fieldDataInfoVector = info->Get(CELL_DATA_VECTOR());
    }
  else if (fieldAssociation == FIELD_ASSOCIATION_VERTICES)
    {
    fieldDataInfoVector = info->Get(VERTEX_DATA_VECTOR());
    }
  else if (fieldAssociation == FIELD_ASSOCIATION_EDGES)
    {
    fieldDataInfoVector = info->Get(EDGE_DATA_VECTOR());
    }
  else
    {
    vtkGenericWarningMacro("Unrecognized field association!");
    return;
    }
  
  if (!fieldDataInfoVector)
    {
    return;
    }
  
  for (i = 0; i < fieldDataInfoVector->GetNumberOfInformationObjects(); i++)
    {
    fieldDataInfo = fieldDataInfoVector->GetInformationObject(i);
    if ( fieldDataInfo->Has(FIELD_NAME()) &&
         !strcmp(fieldDataInfo->Get(FIELD_NAME()),name))
      {
      fieldDataInfoVector->Remove(fieldDataInfo);
      return;
      }
    }
  return;
}

//----------------------------------------------------------------------------
vtkInformation *vtkDataObject::SetActiveAttribute(vtkInformation *info, 
                                                  int fieldAssociation,
                                                  const char *attributeName,
                                                  int attributeType)
{
  int i;
  vtkInformation *fieldDataInfo;
  vtkInformationVector *fieldDataInfoVector;
  
  if (fieldAssociation == FIELD_ASSOCIATION_POINTS)
    {
    fieldDataInfoVector = info->Get(POINT_DATA_VECTOR());
    }
  else if (fieldAssociation == FIELD_ASSOCIATION_CELLS)
    {
    fieldDataInfoVector = info->Get(CELL_DATA_VECTOR());
    }
  else if (fieldAssociation == FIELD_ASSOCIATION_VERTICES)
    {
    fieldDataInfoVector = info->Get(VERTEX_DATA_VECTOR());
    }
  else if (fieldAssociation == FIELD_ASSOCIATION_EDGES)
    {
    fieldDataInfoVector = info->Get(EDGE_DATA_VECTOR());
    }
  else
    {
    vtkGenericWarningMacro("Unrecognized field association!");
    return NULL;
    }
  if (!fieldDataInfoVector)
    {
    fieldDataInfoVector = vtkInformationVector::New();
    if (fieldAssociation == FIELD_ASSOCIATION_POINTS)
      {
      info->Set(POINT_DATA_VECTOR(), fieldDataInfoVector);
      }
    else if (fieldAssociation == FIELD_ASSOCIATION_CELLS)
      {
      info->Set(CELL_DATA_VECTOR(), fieldDataInfoVector);
      }
    else if (fieldAssociation == FIELD_ASSOCIATION_VERTICES)
      {
      info->Set(VERTEX_DATA_VECTOR(), fieldDataInfoVector);
      }
    else // if (fieldAssociation == FIELD_ASSOCIATION_EDGES)
      {
      info->Set(EDGE_DATA_VECTOR(), fieldDataInfoVector);
      }
    fieldDataInfoVector->Delete();
    }

  // if we find a matching field, turn it on (active);  if another field of same
  // attribute type was active, turn it off (not active)
  vtkInformation *activeField = NULL;
  int activeAttribute;
  const char *fieldName;
  for (i = 0; i < fieldDataInfoVector->GetNumberOfInformationObjects(); i++)
    {
    fieldDataInfo = fieldDataInfoVector->GetInformationObject(i);
    activeAttribute = fieldDataInfo->Get( FIELD_ACTIVE_ATTRIBUTE() );
    fieldName = fieldDataInfo->Get(FIELD_NAME());
    // if names match (or both empty... no field name), then set active
    if ( (attributeName && fieldName && !strcmp(attributeName, fieldName)) ||
      (!attributeName && !fieldName) )
      {
      activeAttribute |= 1 << attributeType;
      fieldDataInfo->Set( FIELD_ACTIVE_ATTRIBUTE(), activeAttribute );
      activeField = fieldDataInfo;
      }
    else if ( activeAttribute & (1 << attributeType) )
      {
      activeAttribute &= ~(1 << attributeType);
      fieldDataInfo->Set( FIELD_ACTIVE_ATTRIBUTE(), activeAttribute );
      }
    }
  
  // if we didn't find a matching field, create one
  if (!activeField) 
    {
    activeField = vtkInformation::New();
    activeField->Set( FIELD_ACTIVE_ATTRIBUTE(), 1 << attributeType);
    activeField->Set(FIELD_ASSOCIATION(), fieldAssociation);
    if (attributeName)
      {
      activeField->Set( FIELD_NAME(), attributeName );
      }
    fieldDataInfoVector->Append(activeField);
    activeField->Delete();
    }

  return activeField;
}

//----------------------------------------------------------------------------
void vtkDataObject::SetActiveAttributeInfo(vtkInformation *info, 
                                          int fieldAssociation,
                                          int attributeType,
                                          const char *name,
                                          int arrayType,
                                          int numComponents,
                                          int numTuples)
{
  vtkInformation *attrInfo = vtkDataObject::GetActiveFieldInformation(info, 
    fieldAssociation, attributeType);
  if (!attrInfo)
    {
    // create an entry and set it as active
    attrInfo = SetActiveAttribute(info, fieldAssociation, name, attributeType);
    }

  if (name)
    {
    attrInfo->Set(FIELD_NAME(), name);
    }

  // Set the scalar type if it was given.  If it was not given and
  // there is no current scalar type set the default to VTK_DOUBLE.
  if (arrayType != -1)
    {
    attrInfo->Set(FIELD_ARRAY_TYPE(), arrayType);
    }
  else if(!attrInfo->Has(FIELD_ARRAY_TYPE()))
    {
    attrInfo->Set(FIELD_ARRAY_TYPE(), VTK_DOUBLE);
    }

  // Set the number of components if it was given.  If it was not
  // given and there is no current number of components set the
  // default to 1.
  if (numComponents != -1)
    {
    attrInfo->Set(FIELD_NUMBER_OF_COMPONENTS(), numComponents);
    }
  else if(!attrInfo->Has(FIELD_NUMBER_OF_COMPONENTS()))
    {
    attrInfo->Set(FIELD_NUMBER_OF_COMPONENTS(), 1);
    }

  if (numTuples != -1)
    {
    attrInfo->Set(FIELD_NUMBER_OF_TUPLES(), numTuples);
    }
}

//----------------------------------------------------------------------------
void vtkDataObject::SetPointDataActiveScalarInfo(vtkInformation *info,
                                        int arrayType, int numComponents)
  {
  vtkDataObject::SetActiveAttributeInfo(info, FIELD_ASSOCIATION_POINTS, 
    vtkDataSetAttributes::SCALARS, NULL, arrayType, numComponents, -1);
  }

//----------------------------------------------------------------------------
void vtkDataObject::DataHasBeenGenerated()
{
  this->DataReleased = 0;
  this->UpdateTime.Modified();
}

//----------------------------------------------------------------------------
int vtkDataObject::GetGlobalReleaseDataFlag()
{
  return vtkDataObjectGlobalReleaseDataFlag;
}

//----------------------------------------------------------------------------
void vtkDataObject::ReleaseData()
{
  this->Initialize();
  this->DataReleased = 1;
}

//----------------------------------------------------------------------------
int vtkDataObject::ShouldIReleaseData()
{
  return vtkDataObjectGlobalReleaseDataFlag || this->GetReleaseDataFlag();
}

//----------------------------------------------------------------------------
void vtkDataObject::SetSource(vtkSource* newSource)
{
  vtkDebugMacro( << this->GetClassName() << " ("
                 << this << "): setting Source to " << newSource );
  if(newSource)
    {
    // Find the output index on the source producing this data object.
    int index = newSource->GetOutputIndex(this);
    if(index >= 0)
      {
      newSource->GetExecutive()->SetOutputData(index, this);
      }
    else
      {
      vtkErrorMacro("SetSource cannot find the output index of this "
                    "data object from the source.");
      this->SetPipelineInformation(0);
      }
    }
  else
    {
    this->SetPipelineInformation(0);
    }
}

//----------------------------------------------------------------------------
void vtkDataObject::Register(vtkObjectBase* o)
{
  this->RegisterInternal(o, 1);
}

//----------------------------------------------------------------------------
void vtkDataObject::UnRegister(vtkObjectBase* o)
{
  this->UnRegisterInternal(o, 1);
}

//----------------------------------------------------------------------------
unsigned long vtkDataObject::GetUpdateTime()
{
  return this->UpdateTime.GetMTime();
}

unsigned long vtkDataObject::GetEstimatedMemorySize()
{
  // This should be implemented in a subclass. If not, default to
  // estimating that no memory is used.
  return 0;
}

//----------------------------------------------------------------------------
vtkExecutive* vtkDataObject::GetExecutive()
{
  if(this->PipelineInformation)
    {
    return vtkExecutive::PRODUCER()->GetExecutive(this->PipelineInformation);
    }
  return 0;
}

//----------------------------------------------------------------------------
int vtkDataObject::GetPortNumber()
{
  if(this->PipelineInformation)
    {
    return vtkExecutive::PRODUCER()->GetPort(this->PipelineInformation);
    }
  return 0;
}

//----------------------------------------------------------------------------
vtkStreamingDemandDrivenPipeline* vtkDataObject::TrySDDP(const char* method)
{
  // Make sure there is an executive.
  if(!this->GetExecutive())
    {
    vtkTrivialProducer* tp = vtkTrivialProducer::New();
    tp->SetOutput(this);
    tp->Delete();
    }

  // Try downcasting the executive to the proper type.
  if(SDDP* sddp = SDDP::SafeDownCast(this->GetExecutive()))
    {
    return sddp;
    }
  else if(method)
    {
    vtkErrorMacro("Method " << method << " cannot be called unless the "
                  "data object is managed by a "
                  "vtkStreamingDemandDrivenPipeline.");
    }
  return 0;
}

//----------------------------------------------------------------------------

unsigned long vtkDataObject::GetActualMemorySize()
{
  return this->FieldData->GetActualMemorySize();
}

//----------------------------------------------------------------------------

void vtkDataObject::CopyInformation( vtkDataObject *data )
{
  if ( this->GetExtentType() == VTK_3D_EXTENT &&
       data->GetExtentType() == VTK_3D_EXTENT )
    {
    this->SetWholeExtent(data->GetWholeExtent());
    }
  else
    {
    this->SetMaximumNumberOfPieces(data->GetMaximumNumberOfPieces());
    }
  this->SetExtentTranslator(data->GetExtentTranslator());
}

//----------------------------------------------------------------------------
void vtkDataObject::ShallowCopy(vtkDataObject *src)
{
  if (!src)
    {
    vtkWarningMacro("Attempted to ShallowCopy from null.");
    return;
    }

  this->InternalDataObjectCopy(src);

  if (!src->FieldData)
    {
    this->SetFieldData(0);
    }
  else
    {
    if (this->FieldData)
      {
      this->FieldData->ShallowCopy(src->FieldData);
      }
    else
      {
      vtkFieldData* fd = vtkFieldData::New();
      fd->ShallowCopy(src->FieldData);
      this->SetFieldData(fd);
      fd->Delete();
      }
    }
}

//----------------------------------------------------------------------------
void vtkDataObject::DeepCopy(vtkDataObject *src)
{
  vtkFieldData *srcFieldData = src->GetFieldData();
  
  this->InternalDataObjectCopy(src);

  if (srcFieldData)
    {
    vtkFieldData *newFieldData = vtkFieldData::New();
    newFieldData->DeepCopy(srcFieldData);
    this->SetFieldData(newFieldData);
    newFieldData->Delete();
    }
  else
    {
    this->SetFieldData(NULL);
    }
}

//----------------------------------------------------------------------------
void vtkDataObject::InternalDataObjectCopy(vtkDataObject *src)
{
  // If the input data object has pipeline information and this object
  // does not, setup a trivial producer so that this object will have
  // pipeline information into which to copy values.
  if(src->GetPipelineInformation() && !this->GetPipelineInformation())
    {
    this->GetProducerPort();
    }

  this->DataReleased = src->DataReleased;
  
  // Do not copy pipeline specific information from data object to
  // data object. This meta-data is specific to the algorithm and the
  // what was requested of it when it executed. What looks like a single
  // piece to an internal algorithm may be a piece to an external
  // algorithm.
  /*
  if(src->Information->Has(DATA_PIECE_NUMBER()))
    {
    this->Information->Set(DATA_PIECE_NUMBER(),
                           src->Information->Get(DATA_PIECE_NUMBER()));
    }
  if(src->Information->Has(DATA_NUMBER_OF_PIECES()))
    {
    this->Information->Set(DATA_NUMBER_OF_PIECES(),
                           src->Information->Get(DATA_NUMBER_OF_PIECES()));
    }
  if(src->Information->Has(DATA_NUMBER_OF_GHOST_LEVELS()))
    {
    this->Information->Set(DATA_NUMBER_OF_GHOST_LEVELS(),
                           src->Information->Get(DATA_NUMBER_OF_GHOST_LEVELS()));
    }
  */
  if(src->Information->Has(DATA_TIME_STEPS()))
    {
    this->Information->CopyEntry(src->Information, DATA_TIME_STEPS(), 1);
    }
  if(src->Information->Has(DATA_RESOLUTION()))
    {
    this->Information->CopyEntry(src->Information, DATA_RESOLUTION(), 1);
    }
  
  vtkInformation* thatPInfo = src->GetPipelineInformation();
  vtkInformation* thisPInfo = this->GetPipelineInformation();
  if(thisPInfo && thatPInfo)
    {
    // copy the pipeline info if it is available
    if(thisPInfo)
      {
      // Do not override info if it exists. Normally WHOLE_EXTENT
      // and MAXIMUM_NUMBER_OF_PIECES should not be copied here since
      // they belong to the pipeline not the data object.
      // However, removing the copy can break things in older filters
      // that rely on ShallowCopy to set these (mostly, sources/filters
      // that use another source/filter internally and shallow copy
      // in RequestInformation). As a compromise, I changed the following
      // code such that these entries are only copied if they do not
      // exist in the output.
      if (!thisPInfo->Has(SDDP::WHOLE_EXTENT()))
        {
        thisPInfo->CopyEntry(thatPInfo, SDDP::WHOLE_EXTENT());
        }
      if (!thisPInfo->Has(SDDP::MAXIMUM_NUMBER_OF_PIECES()))
        {
        thisPInfo->CopyEntry(thatPInfo, SDDP::MAXIMUM_NUMBER_OF_PIECES());
        }
      thisPInfo->CopyEntry(thatPInfo, vtkDemandDrivenPipeline::RELEASE_DATA());
      }
    }
  // This also caused a pipeline problem.
  // An input pipelineMTime was copied to output.  Pipeline did not execute...
  // We do not copy MTime of object, so why should we copy these.
  //this->PipelineMTime = src->PipelineMTime;
  //this->UpdateTime = src->UpdateTime;
  //this->Locality = src->Locality;
}

//----------------------------------------------------------------------------
// This should be a pure virutal method.
void vtkDataObject::Crop()
{
}

//----------------------------------------------------------------------------
void vtkDataObject::ReportReferences(vtkGarbageCollector* collector)
{
  this->Superclass::ReportReferences(collector);
  vtkGarbageCollectorReport(collector, this->Information, "Information");
  vtkGarbageCollectorReport(collector, this->PipelineInformation,
                            "PipelineInformation");
}

//----------------------------------------------------------------------------
void vtkDataObject::Update()
{
  if(SDDP* sddp = this->TrySDDP("Update"))
    {
    sddp->Update(this->GetPortNumber());
    }
}

//----------------------------------------------------------------------------
void vtkDataObject::UpdateInformation()
{
  if(SDDP* sddp = this->TrySDDP("UpdateInformation"))
    {
    sddp->UpdateInformation();
    }
} 

//----------------------------------------------------------------------------
void vtkDataObject::PropagateUpdateExtent()
{
  if(SDDP* sddp = this->TrySDDP("PropagateUpdateExtent"))
    {
    sddp->PropagateUpdateExtent(this->GetPortNumber());
    }
}

//----------------------------------------------------------------------------
void vtkDataObject::TriggerAsynchronousUpdate()
{
}

//----------------------------------------------------------------------------
void vtkDataObject::UpdateData()
{
  if(SDDP* sddp = this->TrySDDP("UpdateData"))
    {
    sddp->UpdateData(this->GetPortNumber());
    }
}

//----------------------------------------------------------------------------
void vtkDataObject::SetUpdateExtentToWholeExtent()
{
  if(SDDP* sddp = this->TrySDDP("SetUpdateExtentToWholeExtent"))
    {
    // this code used to check the return value of the
    // SetUpdateExtentToWholeExtent method which would 
    // indicate if the update extent had changed, and call 
    // Modified(). We actually don't want to do this - just 
    // a change in the update extent does not make this object 
    // modified!
    sddp->SetUpdateExtentToWholeExtent
      (sddp->GetOutputInformation()->GetInformationObject
       (this->GetPortNumber()));
    }
}

//----------------------------------------------------------------------------
void vtkDataObject::SetMaximumNumberOfPieces(int n)
{
  if(SDDP* sddp = this->TrySDDP("SetMaximumNumberOfPieces"))
    {
    if(sddp->SetMaximumNumberOfPieces
       (sddp->GetOutputInformation()->GetInformationObject
        (this->GetPortNumber()), n))
      {
      this->Modified();
      }
    }
}

//----------------------------------------------------------------------------
int vtkDataObject::GetMaximumNumberOfPieces()
{
  if(SDDP* sddp = this->TrySDDP("GetMaximumNumberOfPieces"))
    {
    return sddp->GetMaximumNumberOfPieces
      (sddp->GetOutputInformation()->GetInformationObject
       (this->GetPortNumber()));
    }
  return -1;
}

//----------------------------------------------------------------------------
void vtkDataObject::SetWholeExtent(int x0, int x1, int y0, int y1,
                                   int z0, int z1)
{
  int extent[6] = {x0, x1, y0, y1, z0, z1};
  this->SetWholeExtent(extent);
}

//----------------------------------------------------------------------------
void vtkDataObject::SetWholeExtent(int extent[6])
{
  if(SDDP* sddp = this->TrySDDP("SetWholeExtent"))
    {
    if(sddp->SetWholeExtent
       (sddp->GetOutputInformation()->GetInformationObject
        (this->GetPortNumber()), extent))
      {
      this->Modified();
      }
    }
}

//----------------------------------------------------------------------------
int* vtkDataObject::GetWholeExtent()
{
  if(SDDP* sddp = this->TrySDDP("GetWholeExtent"))
    {
    return sddp->GetWholeExtent
      (sddp->GetOutputInformation()->GetInformationObject
       (this->GetPortNumber()));
    }
  else
    {
    static int extent[6] = {0,-1,0,-1,0,-1};
    return extent;
    }
}

//----------------------------------------------------------------------------
void vtkDataObject::GetWholeExtent(int& x0, int& x1, int& y0, int& y1,
                                   int& z0, int& z1)
{
  int extent[6];
  this->GetWholeExtent(extent);
  x0 = extent[0];
  x1 = extent[1];
  y0 = extent[2];
  y1 = extent[3];
  z0 = extent[4];
  z1 = extent[5];
}

//----------------------------------------------------------------------------
void vtkDataObject::GetWholeExtent(int extent[6])
{
  if(SDDP* sddp = this->TrySDDP("GetWholeExtent"))
    {
    sddp->GetWholeExtent
      (sddp->GetOutputInformation()->GetInformationObject
       (this->GetPortNumber()), extent);
    }
}

//----------------------------------------------------------------------------
void vtkDataObject::SetWholeBoundingBox(double x0, double x1, double y0, 
                                        double y1, double z0, double z1)
{
  double bb[6] = {x0, x1, y0, y1, z0, z1};
  this->SetWholeBoundingBox(bb);
}

//----------------------------------------------------------------------------
void vtkDataObject::SetWholeBoundingBox(double bb[6])
{
  if(SDDP* sddp = this->TrySDDP("SetWholeBoundingBox"))
    {
    if(sddp->SetWholeBoundingBox(this->GetPortNumber(), bb))
      {
      this->Modified();
      }
    }
}

//----------------------------------------------------------------------------
double* vtkDataObject::GetWholeBoundingBox()
{
  if(SDDP* sddp = this->TrySDDP("GetWholeBoundingBox"))
    {
    return sddp->GetWholeBoundingBox(this->GetPortNumber());
    }
  else
    {
    static double bb[6] = {0,-1,0,-1,0,-1};
    return bb;
    }
}

//----------------------------------------------------------------------------
void vtkDataObject::GetWholeBoundingBox(double& x0, double& x1, double& y0, 
                                        double& y1, double& z0, double& z1)
{
  double extent[6];
  this->GetWholeBoundingBox(extent);
  x0 = extent[0];
  x1 = extent[1];
  y0 = extent[2];
  y1 = extent[3];
  z0 = extent[4];
  z1 = extent[5];
}

//----------------------------------------------------------------------------
void vtkDataObject::GetWholeBoundingBox(double extent[6])
{
  if(SDDP* sddp = this->TrySDDP("GetWholeBoundingBox"))
    {
    sddp->GetWholeBoundingBox(this->GetPortNumber(), extent);
    }
}

//----------------------------------------------------------------------------
void vtkDataObject::SetUpdateExtent(int x0, int x1, int y0, int y1,
                                    int z0, int z1)
{
  int extent[6] = {x0, x1, y0, y1, z0, z1};
  this->SetUpdateExtent(extent);
}

//----------------------------------------------------------------------------
void vtkDataObject::SetUpdateExtent(int extent[6])
{
  if(SDDP* sddp = this->TrySDDP("SetUpdateExtent"))
    {
    // this code used to check the return value of the
    // SetUpdateExtent method which would indicate if the 
    // update extent had changed, and call Modified(). We 
    // actually don't want to do this - just a change in 
    // the update extent does not make this object modified!
    sddp->SetUpdateExtent
      (sddp->GetOutputInformation()->GetInformationObject
       (this->GetPortNumber()), extent);
    }
}

//----------------------------------------------------------------------------
int* vtkDataObject::GetUpdateExtent()
{
  if(SDDP* sddp = this->TrySDDP("GetUpdateExtent"))
    {
    return sddp->GetUpdateExtent
      (sddp->GetOutputInformation()->GetInformationObject
       (this->GetPortNumber()));
    }
  else
    {
    static int extent[6] = {0,-1,0,-1,0,-1};
    return extent;
    }
}

//----------------------------------------------------------------------------
void vtkDataObject::GetUpdateExtent(int& x0, int& x1, int& y0, int& y1,
                                    int& z0, int& z1)
{
  int extent[6];
  this->GetUpdateExtent(extent);
  x0 = extent[0];
  x1 = extent[1];
  y0 = extent[2];
  y1 = extent[3];
  z0 = extent[4];
  z1 = extent[5];
}

//----------------------------------------------------------------------------
void vtkDataObject::GetUpdateExtent(int extent[6])
{
  if(SDDP* sddp = this->TrySDDP("GetUpdateExtent"))
    {
    sddp->GetUpdateExtent
      (sddp->GetOutputInformation()->GetInformationObject
       (this->GetPortNumber()), extent);
    }
}

//----------------------------------------------------------------------------
void vtkDataObject::SetUpdatePiece(int piece)
{
  if(SDDP* sddp = this->TrySDDP("SetUpdatePiece"))
    {
    if(sddp->SetUpdatePiece
       (sddp->GetOutputInformation()->GetInformationObject
        (this->GetPortNumber()), piece))
      {
      this->Modified();
      }
    }
}

//----------------------------------------------------------------------------
int vtkDataObject::GetUpdatePiece()
{
  if(SDDP* sddp = this->TrySDDP("GetUpdatePiece"))
    {
    return sddp->GetUpdatePiece
      (sddp->GetOutputInformation()->GetInformationObject
       (this->GetPortNumber()));
    }
  return 0;
}

//----------------------------------------------------------------------------
void vtkDataObject::SetUpdateNumberOfPieces(int n)
{
  if(SDDP* sddp = this->TrySDDP("SetUpdateNumberOfPieces"))
    {
    if(sddp->SetUpdateNumberOfPieces
       (sddp->GetOutputInformation()->GetInformationObject
        (this->GetPortNumber()), n))
      {
      this->Modified();
      }
    }
}

//----------------------------------------------------------------------------
int vtkDataObject::GetUpdateNumberOfPieces()
{
  if(SDDP* sddp = this->TrySDDP("GetUpdateNumberOfPieces"))
    {
    return sddp->GetUpdateNumberOfPieces
      (sddp->GetOutputInformation()->GetInformationObject
        (this->GetPortNumber()));
    }
  return 1;
}

//----------------------------------------------------------------------------
void vtkDataObject::SetUpdateGhostLevel(int level)
{
  if(SDDP* sddp = this->TrySDDP("SetUpdateGhostLevel"))
    {
    if(sddp->SetUpdateGhostLevel
       (sddp->GetOutputInformation()->GetInformationObject
        (this->GetPortNumber()), level))
      {
      this->Modified();
      }
    }
}

//----------------------------------------------------------------------------
int vtkDataObject::GetUpdateGhostLevel()
{
  if(SDDP* sddp = this->TrySDDP("GetUpdateGhostLevel"))
    {
    return sddp->GetUpdateGhostLevel
      (sddp->GetOutputInformation()->GetInformationObject
        (this->GetPortNumber()));
    }
  return 0;
}

//----------------------------------------------------------------------------
void vtkDataObject::SetExtentTranslator(vtkExtentTranslator* translator)
{
  if(SDDP* sddp = this->TrySDDP("SetExtentTranslator"))
    {
    if(sddp->SetExtentTranslator
       (sddp->GetOutputInformation()->GetInformationObject
        (this->GetPortNumber()), translator))
      {
      this->Modified();
      }
    }
}
  
//----------------------------------------------------------------------------
vtkExtentTranslator* vtkDataObject::GetExtentTranslator()
{
  if(SDDP* sddp = this->TrySDDP("GetExtentTranslator"))
    {
    return sddp->GetExtentTranslator
      (sddp->GetOutputInformation()->GetInformationObject
        (this->GetPortNumber()));
    }
  return 0;
}

//----------------------------------------------------------------------------
void vtkDataObject::SetReleaseDataFlag(int value)
{
  if(SDDP* sddp = this->TrySDDP("SetReleaseDataFlag"))
    {
    if(sddp->SetReleaseDataFlag(this->GetPortNumber(), value))
      {
      this->Modified();
      }
    }
}
  
//----------------------------------------------------------------------------
int vtkDataObject::GetReleaseDataFlag()
{
  if(SDDP* sddp = this->TrySDDP("GetReleaseDataFlag"))
    {
    return sddp->GetReleaseDataFlag(this->GetPortNumber());
    }
  return 0;
}

//----------------------------------------------------------------------------
unsigned long vtkDataObject::GetPipelineMTime()
{
  if(SDDP* sddp = this->TrySDDP("GetPipelineMTime"))
    {
    return sddp->GetPipelineMTime();
    }    
  return 0;
}

//----------------------------------------------------------------------------
void vtkDataObject::SetRequestExactExtent(int flag)
{
  if(SDDP* sddp = this->TrySDDP("SetRequestExactExtent"))
    {
    sddp->SetRequestExactExtent(this->GetPortNumber(), flag);
    }
}

//----------------------------------------------------------------------------
int vtkDataObject::GetRequestExactExtent()
{
  if(SDDP* sddp = this->TrySDDP("GetRequestExactExtent"))
    {
    return sddp->GetRequestExactExtent(this->GetPortNumber());
    }
  return 0;
}

//----------------------------------------------------------------------------
vtkDataObject* vtkDataObject::GetData(vtkInformation* info)
{
  return info? info->Get(DATA_OBJECT()) : 0;
}

//----------------------------------------------------------------------------
vtkDataObject* vtkDataObject::GetData(vtkInformationVector* v, int i)
{
  return vtkDataObject::GetData(v->GetInformationObject(i));
}

//----------------------------------------------------------------------------
const char* vtkDataObject::GetAssociationTypeAsString(int associationType)
{
  if (associationType < 0 || associationType >= NUMBER_OF_ASSOCIATIONS)
    {
    vtkGenericWarningMacro("Bad association type.");
    return NULL;
    }
  return vtkDataObject::AssociationNames[associationType];
}

//----------------------------------------------------------------------------
vtkDataSetAttributes* vtkDataObject::GetAttributes(int type)
{
  return vtkDataSetAttributes::SafeDownCast(this->GetAttributesAsFieldData(type));
}

//----------------------------------------------------------------------------
vtkFieldData* vtkDataObject::GetAttributesAsFieldData(int type)
{
  switch (type)
    {
    case FIELD:
      return this->FieldData;
      break;
    }
  return 0;
}

//----------------------------------------------------------------------------
int vtkDataObject::GetAttributeTypeForArray(vtkAbstractArray* arr)
{
  for (int i = 0; i < NUMBER_OF_ATTRIBUTE_TYPES; ++i)
    {
    vtkFieldData* data = this->GetAttributesAsFieldData(i);
    if (data)
      {
      for (int j = 0; j < data->GetNumberOfArrays(); ++j)
        {
        if (data->GetAbstractArray(j) == arr)
          {
          return i;
          }
        }
      }
    }
  return -1;
}

//----------------------------------------------------------------------------
vtkIdType vtkDataObject::GetNumberOfElements(int type)
{
  switch (type)
    {
    case FIELD:
      return this->FieldData->GetNumberOfTuples();
      break;
    }
  return 0;
}
