/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVariantArray.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/

// We do not provide a definition for the copy constructor or
// operator=.  Block the warning.
#ifdef _MSC_VER
# pragma warning (disable: 4661)
#endif

#include "vtkVariantArray.h"

#include "vtkDataArray.h"
#include "vtkIdList.h"
#include "vtkStringArray.h"
#include "vtkVariant.h"

#include "vtkArrayIteratorTemplate.txx"
VTK_ARRAY_ITERATOR_TEMPLATE_INSTANTIATE(vtkVariant);

// 
// Standard functions
//

vtkCxxRevisionMacro(vtkVariantArray, "1.5");
vtkStandardNewMacro(vtkVariantArray);

//----------------------------------------------------------------------------

void vtkVariantArray::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  if(this->Array)
    {
    os << indent << "Array: " << this->Array << "\n";
    }
  else
    {
    os << indent << "Array: (null)\n";
    }
}

//----------------------------------------------------------------------------

vtkVariantArray::vtkVariantArray(vtkIdType numComp) :
  vtkAbstractArray( numComp )
{
  this->Array = 0;
  this->SaveUserArray = 0;
}

//----------------------------------------------------------------------------

vtkVariantArray::~vtkVariantArray()
{
  if ((this->Array) && (!this->SaveUserArray))
    {
    delete [] this->Array;
    }
}

//
// 
// Functions required by vtkAbstractArray
//
//

//----------------------------------------------------------------------------

int vtkVariantArray::Allocate(vtkIdType sz, vtkIdType)
{
  if(sz > this->Size)
    {
    if(this->Array && !this->SaveUserArray)
      {
      delete [] this->Array;
      }

    this->Size = (sz > 0 ? sz : 1);
    this->Array = new vtkVariant[this->Size];
    if(!this->Array)
      {
      return 0;
      }
    this->SaveUserArray = 0;
    }

  this->MaxId = -1;

  return 1;
}

//----------------------------------------------------------------------------

void vtkVariantArray::Initialize()
{
  if(this->Array && !this->SaveUserArray)
    {
    delete [] this->Array;
    }
  this->Array = 0;
  this->Size = 0;
  this->MaxId = -1;
  this->SaveUserArray = 0;
}

//----------------------------------------------------------------------------

int vtkVariantArray::GetDataType()
{
  return VTK_VARIANT;
}

//----------------------------------------------------------------------------

int vtkVariantArray::GetDataTypeSize()
{
  return static_cast<int>(sizeof(vtkVariant));
}

//----------------------------------------------------------------------------

int vtkVariantArray::GetElementComponentSize()
{
  return this->GetDataTypeSize();
}

//----------------------------------------------------------------------------

void vtkVariantArray::SetNumberOfTuples(vtkIdType number)
{
  this->SetNumberOfValues(this->NumberOfComponents * number);
}

//----------------------------------------------------------------------------

void vtkVariantArray::SetTuple(vtkIdType i, vtkIdType j, vtkAbstractArray* source)
{
  if (source->IsA("vtkVariantArray"))
    {
    vtkVariantArray* a = vtkVariantArray::SafeDownCast(source);
    vtkIdType loci = i * this->NumberOfComponents;
    vtkIdType locj = j * a->GetNumberOfComponents();
    for (vtkIdType cur = 0; cur < this->NumberOfComponents; cur++)
      {
      this->SetValue(loci + cur, a->GetValue(locj + cur));
      }
    }
  else if (source->IsA("vtkDataArray"))
    {
    vtkDataArray* a = vtkDataArray::SafeDownCast(source);
    vtkIdType loci = i * this->NumberOfComponents;
    vtkIdType locj = j * a->GetNumberOfComponents();
    for (vtkIdType cur = 0; cur < this->NumberOfComponents; cur++)
      {
      // TODO : This just makes a double variant by default.
      //        We really should make the appropriate type of variant
      //        based on the subclass of vtkDataArray.
      int tuple = (locj + cur) / a->GetNumberOfComponents();
      int component = (locj + cur) % a->GetNumberOfComponents();
      this->SetValue(loci + cur, vtkVariant(a->GetComponent(tuple, component)));
      }
    }
  else if (source->IsA("vtkStringArray"))
    {
    vtkStringArray* a = vtkStringArray::SafeDownCast(source);
    vtkIdType loci = i * this->NumberOfComponents;
    vtkIdType locj = j * a->GetNumberOfComponents();
    for (vtkIdType cur = 0; cur < this->NumberOfComponents; cur++)
      {
      this->SetValue(loci + cur, vtkVariant(a->GetValue(locj + cur)));
      }
    }
  else
    {
    vtkWarningMacro("Unrecognized type is incompatible with vtkVariantArray.");
    }
}

//----------------------------------------------------------------------------

void vtkVariantArray::InsertTuple(vtkIdType i, vtkIdType j, vtkAbstractArray* source)
{
  if (source->IsA("vtkVariantArray"))
    {
    vtkVariantArray* a = vtkVariantArray::SafeDownCast(source);
    vtkIdType loci = i * this->NumberOfComponents;
    vtkIdType locj = j * a->GetNumberOfComponents();
    for (vtkIdType cur = 0; cur < this->NumberOfComponents; cur++)
      {
      this->InsertValue(loci + cur, a->GetValue(locj + cur));
      }
    }
  else if (source->IsA("vtkDataArray"))
    {
    vtkDataArray* a = vtkDataArray::SafeDownCast(source);
    vtkIdType loci = i * this->NumberOfComponents;
    vtkIdType locj = j * a->GetNumberOfComponents();
    for (vtkIdType cur = 0; cur < this->NumberOfComponents; cur++)
      {
      int tuple = (locj + cur) / a->GetNumberOfComponents();
      int component = (locj + cur) % a->GetNumberOfComponents();
      this->InsertValue(loci + cur, vtkVariant(a->GetComponent(tuple, component)));
      }
    }
  else if (source->IsA("vtkStringArray"))
    {
    vtkStringArray* a = vtkStringArray::SafeDownCast(source);
    vtkIdType loci = i * this->NumberOfComponents;
    vtkIdType locj = j * a->GetNumberOfComponents();
    for (vtkIdType cur = 0; cur < this->NumberOfComponents; cur++)
      {
      this->InsertValue(loci + cur, vtkVariant(a->GetValue(locj + cur)));
      }
    }
  else
    {
    vtkWarningMacro("Unrecognized type is incompatible with vtkVariantArray.");
    }
}

//----------------------------------------------------------------------------

vtkIdType vtkVariantArray::InsertNextTuple(vtkIdType j, vtkAbstractArray* source)
{
  if (source->IsA("vtkVariantArray"))
    {
    vtkVariantArray* a = vtkVariantArray::SafeDownCast(source);
    vtkIdType locj = j * a->GetNumberOfComponents();
    for (vtkIdType cur = 0; cur < this->NumberOfComponents; cur++)
      {
      this->InsertNextValue(a->GetValue(locj + cur));
      }
    }
  else if (source->IsA("vtkDataArray"))
    {
    vtkDataArray* a = vtkDataArray::SafeDownCast(source);
    vtkIdType locj = j * a->GetNumberOfComponents();
    for (vtkIdType cur = 0; cur < this->NumberOfComponents; cur++)
      {
      int tuple = (locj + cur) / a->GetNumberOfComponents();
      int component = (locj + cur) % a->GetNumberOfComponents();
      this->InsertNextValue(vtkVariant(a->GetComponent(tuple, component)));
      }
    }
  else if (source->IsA("vtkStringArray"))
    {
    vtkStringArray* a = vtkStringArray::SafeDownCast(source);
    vtkIdType locj = j * a->GetNumberOfComponents();
    for (vtkIdType cur = 0; cur < this->NumberOfComponents; cur++)
      {
      this->InsertNextValue(vtkVariant(a->GetValue(locj + cur)));
      }
    }
  else
    {
    vtkWarningMacro("Unrecognized type is incompatible with vtkVariantArray.");
    return -1;
    }

  return (this->GetNumberOfTuples()-1);
}

//----------------------------------------------------------------------------

void* vtkVariantArray::GetVoidPointer(vtkIdType id)
{
  return this->GetPointer(id);
}

//----------------------------------------------------------------------------

void vtkVariantArray::DeepCopy(vtkAbstractArray *aa)
{
  // Do nothing on a NULL input.
  if(!aa)
    {
    return;
    }

  // Avoid self-copy.
  if(this == aa)
    {
    return;
    }

  // If data type does not match, we can't copy. 
  if(aa->GetDataType() != this->GetDataType())
    {
    vtkErrorMacro(<< "Incompatible types: tried to copy an array of type "
                  << aa->GetDataTypeAsString()
                  << " into a variant array ");
    return;
    }

  vtkVariantArray *va = vtkVariantArray::SafeDownCast( aa );
  if ( va == NULL )
    {
    vtkErrorMacro(<< "Shouldn't Happen: Couldn't downcast array into a vtkVariantArray." );
    return;
    }

  // Free our previous memory.
  if(this->Array && !this->SaveUserArray)
    {
    delete [] this->Array;
    }

  // Copy the given array into new memory.
  this->MaxId = va->GetMaxId();
  this->Size = va->GetSize();
  this->SaveUserArray = 0;
  this->Array = new vtkVariant[this->Size];

  for (int i = 0; i < this->Size; ++i)
    {
    this->Array[i] = va->Array[i];
    }
}

//----------------------------------------------------------------------------

void vtkVariantArray::InterpolateTuple(vtkIdType i, vtkIdList *ptIndices,
  vtkAbstractArray* source,  double* weights)
{
  // Note: Something much more fancy could be done here, allowing
  // the source array be any data type.
  if (this->GetDataType() != source->GetDataType())
    {
    vtkErrorMacro("Cannot CopyValue from array of type " 
      << source->GetDataTypeAsString());
    return;
    }
  
  if (ptIndices->GetNumberOfIds() == 0)
    {
    // nothing to do.
    return;
    }

  // We use nearest neighbour for interpolating variants.
  // First determine which is the nearest neighbour using the weights-
  // it's the index with maximum weight.
  vtkIdType nearest = ptIndices->GetId(0);
  double max_weight = weights[0];
  for (int k=1; k < ptIndices->GetNumberOfIds(); k++)
    {
    if (weights[k] > max_weight)
      {
      nearest = k;
      }
    }

  this->InsertTuple(i, nearest, source);
}

//----------------------------------------------------------------------------

void vtkVariantArray::InterpolateTuple(vtkIdType i, 
  vtkIdType id1, vtkAbstractArray* source1, 
  vtkIdType id2, vtkAbstractArray* source2, double t)
{
  // Note: Something much more fancy could be done here, allowing
  // the source array to be any data type.
  if (source1->GetDataType() != VTK_VARIANT || 
    source2->GetDataType() != VTK_VARIANT)
    {
    vtkErrorMacro("All arrays to InterpolateValue() must be of same type.");
    return;
    }

  if (t >= 0.5)
    {
    // Use p2
    this->InsertTuple(i, id2, source2);
    }
  else
    {
    // Use p1.
    this->InsertTuple(i, id1, source1); 
    }
}

//----------------------------------------------------------------------------

void vtkVariantArray::Squeeze()
{
  this->ResizeAndExtend(this->MaxId + 1);
}

//----------------------------------------------------------------------------

int vtkVariantArray::Resize(vtkIdType sz)
{
  vtkVariant* newArray;
  vtkIdType newSize = sz * this->GetNumberOfComponents();

  if(newSize == this->Size)
    {
    return 1;
    }

  if(newSize <= 0)
    {
    this->Initialize();
    return 1;
    }

  newArray = new vtkVariant[newSize];
  if(!newArray)
    {
    vtkErrorMacro(<< "Cannot allocate memory\n");
    return 0;
    }

  if(this->Array)
    {
    int numCopy = (newSize < this->Size ? newSize : this->Size);

    for (int i = 0; i < numCopy; ++i)
      {
      newArray[i] = this->Array[i];
      }
    
    if(!this->SaveUserArray)
      {
      delete[] this->Array;
      }
    }

  if(newSize < this->Size)
    {
    this->MaxId = newSize-1;
    }
  this->Size = newSize;
  this->Array = newArray;
  this->SaveUserArray = 0;
  return 1;
}

//----------------------------------------------------------------------------

void vtkVariantArray::SetVoidArray(void *arr, vtkIdType size, int save)
{
  this->SetArray(static_cast<vtkVariant*>(arr), size, save);
}

//----------------------------------------------------------------------------

unsigned long vtkVariantArray::GetActualMemorySize()
{
  // NOTE: Currently does not take into account the "pointed to" data.
  unsigned long totalSize = 0;
  unsigned long numPrims = this->GetSize();

  totalSize = numPrims*sizeof(vtkVariant);

  return (unsigned long) ceil(totalSize / 1024.0); // kilobytes
}

//----------------------------------------------------------------------------

int vtkVariantArray::IsNumeric()
{
  return 0;
}

//----------------------------------------------------------------------------

vtkArrayIterator* vtkVariantArray::NewIterator()
{
  vtkArrayIteratorTemplate<vtkVariant>* iter = 
    vtkArrayIteratorTemplate<vtkVariant>::New();
  iter->Initialize(this);
  return iter;
}

//
//
// Additional functions
//
//

//----------------------------------------------------------------------------

vtkVariant& vtkVariantArray::GetValue(vtkIdType id) const
{
  return this->Array[id];
}

//----------------------------------------------------------------------------

void vtkVariantArray::SetValue(vtkIdType id, vtkVariant value)
{
  this->Array[id] = value;
}

//----------------------------------------------------------------------------

void vtkVariantArray::InsertValue(vtkIdType id, vtkVariant value)
{
  if ( id >= this->Size )
    {
    this->ResizeAndExtend(id+1);
    }
  this->Array[id] = value;
  if ( id > this->MaxId )
    {
    this->MaxId = id;
    }
}

//----------------------------------------------------------------------------

vtkIdType vtkVariantArray::InsertNextValue(vtkVariant value)
{
  this->InsertValue(++this->MaxId, value);
  return this->MaxId;
}

//----------------------------------------------------------------------------

void vtkVariantArray::SetNumberOfValues(vtkIdType number)
{
  this->Allocate(number);
  this->MaxId = number - 1;
}

//----------------------------------------------------------------------------

vtkVariant* vtkVariantArray::GetPointer(vtkIdType id)
{
  return this->Array + id;
}

//----------------------------------------------------------------------------

void vtkVariantArray::SetArray(vtkVariant* arr, vtkIdType size, int save)
{
  if ((this->Array) && (!this->SaveUserArray))
    {
    vtkDebugMacro (<< "Deleting the array...");
    delete [] this->Array;
    }
  else
    {
    vtkDebugMacro (<<"Warning, array not deleted, but will point to new array.");
    }

  vtkDebugMacro(<<"Setting array to: " << arr);

  this->Array = arr;
  this->Size = size;
  this->MaxId = size-1;
  this->SaveUserArray = save;
}

//----------------------------------------------------------------------------

vtkVariant* vtkVariantArray::ResizeAndExtend(vtkIdType sz)
{
  vtkVariant* newArray;
  vtkIdType newSize;

  if(sz > this->Size)
    {
    // Requested size is bigger than current size.  Allocate enough
    // memory to fit the requested size and be more than double the
    // currently allocated memory.
    newSize = this->Size + sz;
    }
  else if (sz == this->Size)
    {
    // Requested size is equal to current size.  Do nothing.
    return this->Array;
    }
  else
    {
    // Requested size is smaller than current size.  Squeeze the
    // memory.
    newSize = sz;
    }

  if(newSize <= 0)
    {
    this->Initialize();
    return 0;
    }

  newArray = new vtkVariant[newSize];
  if(!newArray)
    {
    vtkErrorMacro("Cannot allocate memory\n");
    return 0;
    }

  if(this->Array)
    {
    // can't use memcpy here
    int numCopy = (newSize < this->Size ? newSize : this->Size);
    for (int i = 0; i < numCopy; ++i)
      {
      newArray[i] = this->Array[i];
      }
    if(!this->SaveUserArray)
      {
      delete [] this->Array;
      }
    }

  if(newSize < this->Size)
    {
    this->MaxId = newSize-1;
    }
  this->Size = newSize;
  this->Array = newArray;
  this->SaveUserArray = 0;

  return this->Array;
}
