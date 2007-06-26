/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCommunicator.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkCommunicator.h"

#include "vtkBoundingBox.h"
#include "vtkCharArray.h"
#include "vtkDataSetReader.h"
#include "vtkDataSetWriter.h"
#include "vtkDoubleArray.h"
#include "vtkFloatArray.h"
#include "vtkGenericDataObjectReader.h"
#include "vtkGenericDataObjectWriter.h"
#include "vtkIdTypeArray.h"
#include "vtkImageClip.h"
#include "vtkImageData.h"
#include "vtkIntArray.h"
#include "vtkRectilinearGrid.h"
#include "vtkStructuredGrid.h"
#include "vtkStructuredPoints.h"
#include "vtkStructuredPointsReader.h"
#include "vtkStructuredPointsWriter.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnsignedLongArray.h"
#include "vtkDataObjectTypes.h"
#include "vtkMultiGroupDataSet.h"

#include "vtkSmartPointer.h"
#define VTK_CREATE(type, name) \
  vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

vtkCxxRevisionMacro(vtkCommunicator, "1.38");


vtkCommunicator::vtkCommunicator()
{
}

vtkCommunicator::~vtkCommunicator()
{
}

int vtkCommunicator::UseCopy = 0;
void vtkCommunicator::SetUseCopy(int useCopy)
{
  vtkCommunicator::UseCopy = useCopy;
}

void vtkCommunicator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
// Need to add better error checking
int vtkCommunicator::Send(vtkDataObject* data, int remoteHandle, 
                          int tag)
{
  int data_type = data->GetDataObjectType();
  this->Send(&data_type, 1, remoteHandle, tag);
  
  switch(data_type)
    {
    //error on types we can't send
    case VTK_DATA_OBJECT:
    case VTK_DATA_SET:
    case VTK_PIECEWISE_FUNCTION:
    case VTK_POINT_SET:
    case VTK_UNIFORM_GRID:
    case VTK_GENERIC_DATA_SET:
    case VTK_HYPER_OCTREE:
    case VTK_COMPOSITE_DATA_SET:
    default:
      vtkWarningMacro(<< "Cannot send " << data->GetClassName());
      return 0;
      
    //send elemental data objects
    case VTK_GRAPH:
    case VTK_IMAGE_DATA: 
    case VTK_POLY_DATA:
    case VTK_RECTILINEAR_GRID:
    case VTK_STRUCTURED_GRID:
    case VTK_STRUCTURED_POINTS:
    case VTK_TABLE:
    case VTK_TREE:
    case VTK_UNSTRUCTURED_GRID:
      return this->SendElementalDataObject(data, remoteHandle, tag);
      
    //for composite types send type, structure, and then iterate 
    //over the internal dataobjects, sending each one (recursively)
    case VTK_MULTIGROUP_DATA_SET:
    case VTK_HIERARCHICAL_DATA_SET:
    case VTK_HIERARCHICAL_BOX_DATA_SET:
    case VTK_MULTIBLOCK_DATA_SET:
    case VTK_TEMPORAL_DATA_SET:
    {
      int rc = 1;
      vtkMultiGroupDataSet *hdObj = 
        vtkMultiGroupDataSet::SafeDownCast(data);

      int numgroups = hdObj->GetNumberOfGroups();
      int *gptrs = new int[numgroups];
      for (int i = 0; i < numgroups; i++)
        {
        gptrs[i] = hdObj->GetNumberOfDataSets(i);
        }
      this->Send(&numgroups, 1, remoteHandle, tag);
      this->Send(gptrs, numgroups, remoteHandle, tag);
      
      for (int i = 0; i < numgroups; i++)
        {
        int *dtptrs = new int[gptrs[i]];
        for (int j = 0; j < gptrs[i]; j++)
          {
          dtptrs[j] = -1;
          if (hdObj->GetDataSet(i,j))
            {
            dtptrs[j] = hdObj->GetDataSet(i,j)->GetDataObjectType();
            }              
          }
        
        this->Send(dtptrs, gptrs[i], remoteHandle, tag);
        delete[] dtptrs;
        
        for (int j = 0; j < gptrs[i]; j++)
          {
          if (hdObj->GetDataSet(i,j))
            {
            rc &= this->Send(hdObj->GetDataSet(i, j), remoteHandle, tag);
            }
          }
        }      
      delete[] gptrs;
      return rc;
    }
    }
}

//----------------------------------------------------------------------------
int vtkCommunicator::SendElementalDataObject(
  vtkDataObject* data, int remoteHandle, 
  int tag)
{
  VTK_CREATE(vtkCharArray, buffer);
  if (vtkCommunicator::MarshalDataObject(data, buffer))
    {
    this->Send(buffer, remoteHandle, tag);

    // Send data extents. These make sense only for structured data.
    // However, we still send them. We need to send extents separately
    // because the Legacy writers discard extents.
    int extent[6] = {0,0,0,0,0,0};
    if (data && (data->GetExtentType() == VTK_3D_EXTENT))
      {
      vtkRectilinearGrid* rg = vtkRectilinearGrid::SafeDownCast(data);
      vtkStructuredGrid* sg = vtkStructuredGrid::SafeDownCast(data);
      vtkImageData* id = vtkImageData::SafeDownCast(data);
      if (rg)
        {
        rg->GetExtent(extent);
        }
      else if (sg)
        {
        sg->GetExtent(extent);
        }
      else if (id)
        {
        id->GetExtent(extent);
        }
      }
    this->Send(extent, 6, remoteHandle, tag);

    return 1;
    }
  
  // could not marshal data
  return 0;
}

int vtkCommunicator::Send(vtkDataArray* data, int remoteHandle, int tag)
{

  int type = -1;
  if (data == NULL)
    {
      this->Send( &type, 1, remoteHandle, tag);
      return 1;
    }

  // send array type
  type = data->GetDataType();
  this->Send( &type, 1, remoteHandle, tag);

  // send array tuples
  vtkIdType numTuples = data->GetNumberOfTuples();
  this->Send( &numTuples, 1, remoteHandle, tag);

  // send number of components in array
  int numComponents = data->GetNumberOfComponents();
  this->Send( &numComponents, 1, remoteHandle, tag);

  vtkIdType size = numTuples*numComponents;

  const char* name = data->GetName();
  int len = 0;
  if (name)
    {
    len = static_cast<int>(strlen(name)) + 1;
    }

  // send length of name
  this->Send( &len, 1, remoteHandle, tag);

  if (len > 0)
    {
    // send name
    this->Send( const_cast<char*>(name), len, remoteHandle, tag);
    }

  // do nothing if size is zero.
  if (size == 0)
    {
    return 1;
    }

  // now send the raw array
  this->SendVoidArray(data->GetVoidPointer(0), size, type, remoteHandle, tag);
  return 1;
}


//----------------------------------------------------------------------------
int vtkCommunicator::Receive(vtkDataObject* data, int remoteHandle, 
                             int tag)
{
   //fill in the data object we are given
   return this->ReceiveDataObject(data, remoteHandle, tag, -1);
}

//----------------------------------------------------------------------------
vtkDataObject *vtkCommunicator::ReceiveDataObject(int remoteHandle, int tag)
{
  int data_type = 0;
  this->Receive(&data_type, 1, remoteHandle, tag); 
  //manufacture a data object of the proper type to fill
  vtkDataObject * dObj = vtkDataObjectTypes::NewDataObject(data_type);
  if (dObj != NULL)
    {
    if (this->ReceiveDataObject(dObj, remoteHandle, tag, data_type) == 1)
      {
      return dObj;
      }
    }
  if (dObj)
    {
    dObj->Delete();
    }
  return NULL;
}

//----------------------------------------------------------------------------
int vtkCommunicator::ReceiveDataObject(vtkDataObject* data, int remoteHandle, 
                                       int tag, int dataType)
{
  int data_type = dataType;
  if (data_type == -1)
    {
    this->Receive(&data_type, 1, remoteHandle, tag); 
    if (data->GetDataObjectType() != data_type)
      {
      vtkErrorMacro("Cannot receive object, type sent is different from destination.");
      return 0;
      }
    }
  
  switch(data_type)
    {
    //error on types we can't receive
    case VTK_DATA_OBJECT:
    case VTK_DATA_SET:
    case VTK_PIECEWISE_FUNCTION:
    case VTK_POINT_SET:
    case VTK_UNIFORM_GRID:
    case VTK_GENERIC_DATA_SET:
    case VTK_HYPER_OCTREE:
    case VTK_COMPOSITE_DATA_SET:
    default:
      vtkWarningMacro(
        << "Cannot receive " 
        << vtkDataObjectTypes::GetClassNameFromTypeId(data_type));
      return 0;

    //receive elemental data objects
    case VTK_GRAPH:
    case VTK_IMAGE_DATA: 
    case VTK_POLY_DATA:
    case VTK_RECTILINEAR_GRID:
    case VTK_STRUCTURED_GRID:
    case VTK_STRUCTURED_POINTS:
    case VTK_TABLE:
    case VTK_TREE:
    case VTK_UNSTRUCTURED_GRID:
      return this->ReceiveElementalDataObject(data, remoteHandle, tag);

    //for composite types receive type, structure, and then iterate 
    //over the internal dataobjects, receiving each recursively as needed
    case VTK_MULTIGROUP_DATA_SET:
    case VTK_HIERARCHICAL_DATA_SET:
    case VTK_HIERARCHICAL_BOX_DATA_SET:
    case VTK_MULTIBLOCK_DATA_SET:
    case VTK_TEMPORAL_DATA_SET:
    {
      int numgroups = 0;
      this->Receive(&numgroups, 1, remoteHandle, tag);
      
      int *gptrs = new int[numgroups];
      this->Receive(gptrs, numgroups, remoteHandle, tag);
      
      vtkMultiGroupDataSet *hdObj = 
        vtkMultiGroupDataSet::SafeDownCast(data);
      hdObj->SetNumberOfGroups(numgroups);
      for (int i = 0; i < numgroups; i++)
        {
        hdObj->SetNumberOfDataSets(i, gptrs[i]);
        int *dtptrs = new int[gptrs[i]];
        this->Receive(dtptrs, gptrs[i], remoteHandle, tag);
 
        for (int j = 0; j < gptrs[i]; j++)
          {
          if (dtptrs[j] != -1)
            {
            vtkDataObject *dObj = 
              vtkDataObjectTypes::NewDataObject(dtptrs[j]);
            this->Receive(dObj, remoteHandle, tag);
            hdObj->SetDataSet(i, j, dObj);
            dObj->Delete();
            }
          }
        
        delete[] dtptrs;
        }
      
      delete[] gptrs;
      return 1;
    }
    }
}

//----------------------------------------------------------------------------
int vtkCommunicator::ReceiveElementalDataObject(
  vtkDataObject* data, int remoteHandle, 
  int tag)
{
  VTK_CREATE(vtkCharArray, buffer);
  if (!this->Receive(buffer, remoteHandle, tag))
    {
    return 0;
    }

  if (vtkCommunicator::UnMarshalDataObject(buffer, data))
    {
    int extent[6];
    // Receive the extents.
    if (!this->Receive(extent, 6, remoteHandle, tag))
      {
      return 0;
      }

    // Set the extents if the dataobject supports it.
    if (data && (data->GetExtentType() == VTK_3D_EXTENT))
      {
      vtkRectilinearGrid* rg = vtkRectilinearGrid::SafeDownCast(data);
      vtkStructuredGrid* sg = vtkStructuredGrid::SafeDownCast(data);
      vtkImageData* id = vtkImageData::SafeDownCast(data);
      if (rg)
        {
        rg->SetExtent(extent);
        }
      else if (sg)
        {
        sg->SetExtent(extent);
        }
      else if (id)
        {
        id->SetExtent(extent);
        }
      }
    }

  return 1;
}

int vtkCommunicator::Receive(vtkDataArray* data, int remoteHandle, int tag)
{
  // First receive the data type.
  int type;
  if (!this->Receive( &type, 1, remoteHandle, tag))
    {
    vtkErrorMacro("Could not receive data!");
    return 0;
    }

  if (type == -1) 
    { // This indicates a NULL object was sent. Do nothing.
    return 1;   
    }

  if (type != data->GetDataType())
    {
    vtkErrorMacro("Send/receive data types do not match!");
    return 0;
    }

  // Next receive the number of tuples.
  vtkIdType numTuples;
  if (!this->Receive( &numTuples, 1, remoteHandle, tag))
    {
    vtkErrorMacro("Could not receive data!");
    return 0;
    }

  // Next receive the number of components.
  int numComponents;
  this->Receive( &numComponents, 1, remoteHandle, tag);

  vtkIdType size = numTuples*numComponents;
  if (data->GetSize() != size)
    {
    // Clear out data so that a data resize does not require memory copies.
    data->Initialize();
    }
  data->SetNumberOfComponents(numComponents);
  data->SetNumberOfTuples(numTuples);

  // Next receive the length of the name.
  int nameLength;
  this->Receive( &nameLength, 1, remoteHandle, tag);

  if ( nameLength > 0 )
    {
    char *str = new char[nameLength]; 
    
    // Receive the name
    this->Receive(str, nameLength, remoteHandle, tag);
    data->SetName(str);
    }
  else
    {
    data->SetName(NULL);
    }

  if (size < 0)
    {
    vtkErrorMacro("Bad data length");
    return 0;
    }
  
  // Do nothing if size is zero.
  if (size == 0)
    {
    return 1;   
    }

  // now receive the raw array.
  this->ReceiveVoidArray(data->GetVoidPointer(0), size, type, remoteHandle,tag);

  return 1;
}

//-----------------------------------------------------------------------------
int vtkCommunicator::MarshalDataObject(vtkDataObject *object,
                                       vtkCharArray *buffer)
{
  buffer->SetNumberOfComponents(1);

  if (object == NULL)
    {
    buffer->SetNumberOfTuples(0);
    return 1;
    }

  VTK_CREATE(vtkGenericDataObjectWriter, writer);

  vtkSmartPointer<vtkDataObject> copy;
  copy.TakeReference(object->NewInstance());
  copy->ShallowCopy(object);

  writer->SetFileTypeToBinary();
  // There is a problem with binary files with no data.
  if (vtkDataSet::SafeDownCast(copy) != NULL)
    {
    vtkDataSet *ds = vtkDataSet::SafeDownCast(copy);
    if (ds->GetNumberOfCells() + ds->GetNumberOfPoints() == 0)
      {
      writer->SetFileTypeToASCII();
      }
    }
  writer->WriteToOutputStringOn();
  writer->SetInput(copy);

  writer->Write();
  unsigned int size = writer->GetOutputStringLength();
  buffer->SetArray(writer->RegisterAndGetOutputString(), size, 0);
  buffer->SetNumberOfTuples(size);
  return 1;
}

//-----------------------------------------------------------------------------
int vtkCommunicator::UnMarshalDataObject(vtkCharArray *buffer,
                                         vtkDataObject *object)
{
  if (buffer->GetNumberOfTuples() <= 0)
    {
    object = NULL;
    return 1;
    }

  VTK_CREATE(vtkGenericDataObjectReader, reader);
  reader->ReadFromInputStringOn();

  reader->SetInputArray(buffer);

  reader->Update();
  object->ShallowCopy(reader->GetOutput());

  return 1;
}

// The processors are views as a heap tree. The root is the processor of
// id 0.
//-----------------------------------------------------------------------------
int vtkCommunicator::GetParentProcessor(int proc)
{
  int result;
  if(proc%2==1)
    {
    result=proc>>1; // /2
    }
  else
    {
    result=(proc-1)>>1; // /2
    }
  return result;
}

int vtkCommunicator::GetLeftChildProcessor(int proc)
{
  return (proc<<1)+1; // *2+1
}

int vtkCommunicator::ComputeGlobalBounds(int processNumber, int numProcessors,
                                         vtkBoundingBox *bounds,
                                         int *rhb, int *lhb,
                                         int hasBoundsTag,
                                         int localBoundsTag,
                                         int globalBoundsTag)
{
  int parent = 0;
  int leftHasBounds = 0, rightHasBounds = 0;
  int left = this->GetLeftChildProcessor(processNumber);
  int right=left+1;
  if(processNumber>0) // not root (nothing to do if root)
    {
    parent=this->GetParentProcessor(processNumber);
    }
  
  double otherBounds[6];
  if(left<numProcessors)
    {
    this->Receive(&leftHasBounds, 1, left, hasBoundsTag);
    if (lhb)
      {
      *lhb = leftHasBounds;
      }

    if(leftHasBounds)
      {
      this->Receive(otherBounds, 6, left, localBoundsTag);
      bounds->AddBounds(otherBounds);
      }
    }
  if(right<numProcessors)
    {
    // Grab the bounds from right child
    this->Receive(&rightHasBounds, 1, right, hasBoundsTag);
    
    if (rhb)
      {
      *rhb = rightHasBounds;
      }

    if(rightHasBounds)
      {
      this->Receive(otherBounds, 6, right, localBoundsTag);
      bounds->AddBounds(otherBounds);
      }
    }
  
  // If there are bounds to send do so
  int boundsHaveBeenSet = bounds->IsValid();
  double b[6];
  // Send local to parent, Receive global from the parent.
  if(processNumber > 0) // not root (nothing to do if root)
    {
    this->Send(&boundsHaveBeenSet, 1, parent, hasBoundsTag);
    if(boundsHaveBeenSet)
      {
      // Copy the bounds to an array so we can send them
      
      bounds->GetBounds(b);
      this->Send(b, 6, parent, localBoundsTag);
      
      this->Receive(b, 6, parent, globalBoundsTag);
      bounds->AddBounds(b);
      }
    }
  
  if(!boundsHaveBeenSet) // empty, no bounds, nothing to do
    {
    return 1;
    }
  
  // Send it to children.
  bounds->GetBounds(b);
  if(left<numProcessors)
    {
    if(leftHasBounds)
      {
      this->Send(b, 6, left, globalBoundsTag);
      }
    if(right<numProcessors)
      {
      if(rightHasBounds)
        {
        this->Send(b, 6, right, globalBoundsTag);
        }
      }
    }
  return 1;
}
