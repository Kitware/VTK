/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkStringArray.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright 2004 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
  license for use of this work by or on behalf of the
  U.S. Government. Redistribution and use in source and binary forms, with
  or without modification, are permitted provided that this Notice and any
  statement of authorship are reproduced on all copies.

=========================================================================*/



// We do not provide a definition for the copy constructor or
// operator=.  Block the warning.
#ifdef _MSC_VER
# pragma warning (disable: 4661)
#endif

#include <vtkObjectFactory.h>

#include "vtkStringArray.h"
#include "vtkStdString.h"

#include "vtkCharArray.h"
#include "vtkIdList.h"
#include "vtkIdTypeArray.h"

vtkCxxRevisionMacro(vtkStringArray, "1.4.6.2");
vtkStandardNewMacro(vtkStringArray);

//----------------------------------------------------------------------------

vtkStringArray::vtkStringArray(vtkIdType numComp) :
  vtkAbstractArray( numComp )
{
  this->Array = NULL;
  this->SaveUserArray = 0;
}

//----------------------------------------------------------------------------

vtkStringArray::~vtkStringArray()
{
  if ((this->Array) && (!this->SaveUserArray))
    {
    delete [] this->Array;
    }
}

//----------------------------------------------------------------------------
// This method lets the user specify data to be held by the array.  The
// array argument is a pointer to the data.  size is the size of
// the array supplied by the user.  Set save to 1 to keep the class
// from deleting the array when it cleans up or reallocates memory.
// The class uses the actual array provided; it does not copy the data
// from the suppled array.

void vtkStringArray::SetArray(vtkStdString *array, vtkIdType size, int save)
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

  vtkDebugMacro(<<"Setting array to: " << array);

  this->Array = array;
  this->Size = size;
  this->MaxId = size-1;
  this->SaveUserArray = save;
}

//----------------------------------------------------------------------------
// Allocate memory for this array. Delete old storage only if necessary.

int vtkStringArray::Allocate(vtkIdType sz, vtkIdType)
{
  if(sz > this->Size)
    {
    if(this->Array && !this->SaveUserArray)
      {
      delete [] this->Array;
      }

    this->Size = ( sz > 0 ? sz : 1);
    this->Array = new vtkStdString[this->Size];
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
// Release storage and reset array to initial state.

void vtkStringArray::Initialize()
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
// Deep copy of another string array.

void vtkStringArray::DeepCopy(vtkAbstractArray* aa)
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
                  << " into a string array ");
    return;
    }

  vtkStringArray *fa = vtkStringArray::SafeDownCast( aa );
  if ( fa == NULL )
    {
    vtkErrorMacro(<< "Shouldn't Happen: Couldn't downcast array into a vtkStringArray." );
    return;
    }

  // Free our previous memory.
  if(this->Array && !this->SaveUserArray)
    {
    delete [] this->Array;
    }

  // Copy the given array into new memory.
  this->MaxId = fa->GetMaxId();
  this->Size = fa->GetSize();
  this->SaveUserArray = 0;
  this->Array = new vtkStdString[this->Size];

  for (int i = 0; i < this->Size; ++i)
    {
    this->Array[i] = fa->Array[i];
    }
}

//----------------------------------------------------------------------------

void vtkStringArray::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
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
// Protected function does "reallocate"

vtkStdString * vtkStringArray::ResizeAndExtend(vtkIdType sz)
{
  vtkStdString * newArray;
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

  newArray = new vtkStdString[newSize];
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

//----------------------------------------------------------------------------

int vtkStringArray::Resize(vtkIdType sz)
{
  vtkStdString * newArray;
  vtkIdType newSize = sz;

  if(newSize == this->Size)
    {
    return 1;
    }

  if(newSize <= 0)
    {
    this->Initialize();
    return 1;
    }

  newArray = new vtkStdString[newSize];
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

void vtkStringArray::SetNumberOfValues(vtkIdType number)
{
  this->Allocate(number);
  this->MaxId = number - 1;
}

//----------------------------------------------------------------------------

vtkStdString * vtkStringArray::WritePointer(vtkIdType id,
                                     vtkIdType number)
{
  vtkIdType newSize=id+number;
  if ( newSize > this->Size )
    {
    this->ResizeAndExtend(newSize);
    }
  if ( (--newSize) > this->MaxId )
    {
    this->MaxId = newSize;
    }
  return this->Array + id;
}

//----------------------------------------------------------------------------

void vtkStringArray::InsertValue(vtkIdType id, vtkStdString f)
{
  if ( id >= this->Size )
    {
    this->ResizeAndExtend(id+1);
    }
  this->Array[id] = f;
  if ( id > this->MaxId )
    {
    this->MaxId = id;
    }
}

//----------------------------------------------------------------------------

vtkIdType vtkStringArray::InsertNextValue(vtkStdString f)
{
  this->InsertValue (++this->MaxId,f);
  return this->MaxId;
}

// ----------------------------------------------------------------------

int
vtkStringArray::GetDataTypeSize( void )
{ return static_cast<int>(sizeof(vtkStdString)); }


// ----------------------------------------------------------------------

unsigned long
vtkStringArray::GetActualMemorySize( void )
{
  unsigned long totalSize = 0;
  unsigned long  numPrims = this->GetSize();

  for (unsigned long i = 0; i < numPrims; ++i)
    {
    totalSize += sizeof( vtkStdString );
    totalSize += this->Array[i].size() * sizeof( vtkStdString::value_type );
    }

  return (unsigned long) ceil( totalSize / 1000.0 ); // kilobytes
}

// ----------------------------------------------------------------------

vtkStdString &
vtkStringArray::GetValue( vtkIdType id )
{
  return this->Array[id];
}

// ----------------------------------------------------------------------

void
vtkStringArray::GetValues(vtkIdList *indices, vtkAbstractArray *aa)
{
  if (aa == NULL)
    {
    vtkErrorMacro(<<"GetValues: Output array is null!");
    return;
    }

  vtkStringArray *output = vtkStringArray::SafeDownCast(aa);

  if (output == NULL)
    {
    vtkErrorMacro(<< "Can't copy values from a string array into an array "
                  << "of type " << aa->GetDataTypeAsString());
    return;
    }

  for (vtkIdType i = 0; i < indices->GetNumberOfIds(); ++i)
    {
    vtkIdType index = indices->GetId(i);
    output->SetValue(i, this->GetValue(index));
    }
}

// ----------------------------------------------------------------------

void
vtkStringArray::GetValues(vtkIdType startIndex,
                          vtkIdType endIndex, 
                          vtkAbstractArray *aa)
{
  if (aa == NULL)
    {
    vtkErrorMacro(<<"GetValues: Output array is null!");
    return;
    }

  vtkStringArray *output = vtkStringArray::SafeDownCast(aa);

  if (output == NULL)
    {
    vtkErrorMacro(<< "Can't copy values from a string array into an array "
                  << "of type " << aa->GetDataTypeAsString());
    return;
    }

  for (vtkIdType i = 0; i < (endIndex - startIndex) + 1; ++i)
    {
    vtkIdType index = startIndex + i;
    output->SetValue(i, this->GetValue(index));
    }
}


// ----------------------------------------------------------------------

void
vtkStringArray::CopyValue(int toIndex, int fromIndex,
                          vtkAbstractArray *source)
{
  if (source == NULL)
    {
    vtkErrorMacro(<<"CopyValue: Input array is null!");
    return;
    }

  vtkStringArray *realSource = vtkStringArray::SafeDownCast(source);

  if (realSource == NULL)
    {
    vtkErrorMacro(<< "Can't copy values from an array of type " 
                  << source->GetDataTypeAsString()
                  << " into a string array!");
    return;
    }

  this->SetValue(toIndex, realSource->GetValue(fromIndex));
}

// ----------------------------------------------------------------------

void
vtkStringArray::ConvertToContiguous(vtkDataArray **Data, 
                                    vtkIdTypeArray **Offsets)
{
  vtkCharArray *data = vtkCharArray::New();
  vtkIdTypeArray *offsets = vtkIdTypeArray::New();
  int currentPosition = 0;

  for (vtkIdType i = 0; i < this->GetNumberOfValues(); ++i)
    {
    vtkStdString thisString = this->Array[i];
    for (vtkStdString::size_type j = 0; j < this->Array[i].length(); ++j)
      {
      data->InsertNextValue(thisString[j]);
      ++currentPosition;
      }
    offsets->InsertNextValue(currentPosition); 
    }
  
  *Data = data;
  *Offsets = offsets;
}

// ----------------------------------------------------------------------

// This will work with any sort of data array, but if you call it with
// anything other than a char array you might get strange results.
// You have been warned...

void
vtkStringArray::ConvertFromContiguous(vtkDataArray *Data,
                                      vtkIdTypeArray *Offsets)
{
  this->Reset();

  vtkIdType currentStringStart = 0;

  for (vtkIdType i = 0; i < Offsets->GetNumberOfTuples(); ++i)
    {
    // YOU ARE HERE
    vtkStdString newString;
    vtkIdType stringEnd = Offsets->GetValue(i);

    for (vtkIdType here = currentStringStart; 
         here < stringEnd;
         ++here)
      {
      newString += static_cast<char>(Data->GetTuple1(here));
      }
    this->InsertNextValue(newString);
    currentStringStart = stringEnd;
    }
}



// ----------------------------------------------------------------------

// 
//
// Below here are interface methods to allow values to be inserted as
// const char * instead of vtkStdString.  Yes, they're trivial.  The
// wrapper code needs them.
//
//


void
vtkStringArray::SetValue( vtkIdType id, const char *value )
{
  this->SetValue( id, vtkStdString(value) );
}

void
vtkStringArray::InsertValue( vtkIdType id, const char *value )
{
  this->InsertValue( id, vtkStdString( value ) );
}

vtkIdType
vtkStringArray::InsertNextValue( const char *value )
{
  return this->InsertNextValue( vtkStdString( value ) );
}

// ----------------------------------------------------------------------
