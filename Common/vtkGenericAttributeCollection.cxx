/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGenericAttributeCollection.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkGenericAttributeCollection - objects that own attributes of a data set
// .DESCRIPTION They can also select an active attribute component to process
// (contouring, clipping) and others attributes to interpolate.

#include "vtkGenericAttributeCollection.h"

#include "vtkObjectFactory.h"
#include "vtkGenericAttribute.h"


#include <vtkstd/vector>
#include <assert.h>

vtkCxxRevisionMacro(vtkGenericAttributeCollection,"1.4");
vtkStandardNewMacro(vtkGenericAttributeCollection);

class vtkGenericAttributeInternalVector
{
public:
  typedef vtkstd::vector<vtkGenericAttribute* > VectorType;
  VectorType Vector;
};

//----------------------------------------------------------------------------
// Description:
// Default constructor: empty collection
vtkGenericAttributeCollection::vtkGenericAttributeCollection()
{
  this->AttributeInternalVector = new vtkGenericAttributeInternalVector;
  this->ActiveAttribute = 0;
  this->ActiveComponent = 0;
  this->NumberOfAttributesToInterpolate = 0;
  this->NumberOfComponents = 0;
  this->MaxNumberOfComponents = 0; // cache
  this->ActualMemorySize = 0;
}

//----------------------------------------------------------------------------
vtkGenericAttributeCollection::~vtkGenericAttributeCollection()
{
  for(unsigned int i = 0; i < this->AttributeInternalVector->Vector.size(); ++i)
    {
    this->AttributeInternalVector->Vector[i]->Delete();
    }
  delete this->AttributeInternalVector;
}

//----------------------------------------------------------------------------
void vtkGenericAttributeCollection::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  int c = this->GetNumberOfAttributes();

  os << indent << "Number Of Attributes: " << this->GetNumberOfAttributes() << "\n";
  for(int i=0; i<c; ++i)
    {
    os << indent << "Attribute #"<<i<<":\n";
    this->GetAttribute(i)->PrintSelf(os,indent.GetNextIndent());
    }
  os << indent << "Number Of Attributes to interpolate: " << this->GetNumberOfAttributesToInterpolate() << endl;
  
  os << indent << "Attributes to interpolate: " << this->AttributesToInterpolate << endl;
  
  os << indent << "Active Attribute: " << this->ActiveAttribute << endl;
  
  os << indent << "Active Component" << this->ActiveComponent << endl;
}

//----------------------------------------------------------------------------
// Description:
// Number of attributes.
int vtkGenericAttributeCollection::GetNumberOfAttributes()
{
  int result = this->AttributeInternalVector->Vector.size();
  //assert("post: positive_result" && result>=0); size() is an unsigned type according to STL vector definition
  return result;
}

//----------------------------------------------------------------------------
int vtkGenericAttributeCollection::GetNumberOfComponents()
{
  this->ComputeNumbers();

  return this->NumberOfComponents;
}

//----------------------------------------------------------------------------
// Description:
// Maximum number of components encountered among all attributes.
// \post positive_result: result>=0
// \post valid_result: result<=GetNumberOfComponents()
int vtkGenericAttributeCollection::GetMaxNumberOfComponents()
{
  this->ComputeNumbers();
  
  assert("post: positive_result" && this->MaxNumberOfComponents>=0);
  assert("post: valid_result" && this->MaxNumberOfComponents<=GetNumberOfComponents());
  return this->MaxNumberOfComponents;
}

//----------------------------------------------------------------------------
// Description:
// Actual size of the data in kilobytes; only valid after the pipeline has
// updated. It is guaranteed to be greater than or equal to the memory
// required to represent the data.
unsigned long vtkGenericAttributeCollection::GetActualMemorySize()
{
  this->ComputeNumbers();
  return this->ActualMemorySize;
}

//----------------------------------------------------------------------------
// Description:
// Does `this' have no attribute?
int vtkGenericAttributeCollection::IsEmpty()
{
  return this->GetNumberOfAttributes()==0;
}

//----------------------------------------------------------------------------
// Description:
// Attribute `i'.
vtkGenericAttribute *vtkGenericAttributeCollection::GetAttribute(int i)
{
  assert("pre: not_empty" && !IsEmpty());
  assert("pre: valid_i" && (i>=0)&&(i<this->GetNumberOfAttributes()));
  vtkGenericAttribute *result=this->AttributeInternalVector->Vector[i];
  assert("post: result_exists" && result!=0);
  return result;
}

//----------------------------------------------------------------------------
// Description:
// Return the index of attribute `name', if found. Return -1 otherwise.
int vtkGenericAttributeCollection::FindAttribute(const char *name)
{
  assert("pre: name_exists:" && name!=0);

  int numAtt = this->GetNumberOfAttributes();
  for( int i = 0; i < numAtt; ++i )
    {
    if( strcmp( this->GetAttribute(i)->GetName(), name ) == 0)
      {
      return i;
      }
    }

  return -1;
}

//----------------------------------------------------------------------------
// Description:
// Add attribute `a' at the end.
void vtkGenericAttributeCollection::InsertNextAttribute(vtkGenericAttribute *a)
{
  assert("pre: a_exists" && a!=0);
#ifndef NDEBUG
  int oldnumber=this->GetNumberOfAttributes();
#endif

  this->AttributeInternalVector->Vector.push_back(a);
  this->Modified();
  
  assert("post: more_items" && this->GetNumberOfAttributes()==oldnumber+1);
  assert("post: a_is_set" && this->GetAttribute(this->GetNumberOfAttributes()-1)==a);
}

//----------------------------------------------------------------------------
// Description:
// Replace attribute at index `i' by `a'.
void vtkGenericAttributeCollection::InsertAttribute(int i, vtkGenericAttribute *a)
{
  assert("pre: not_empty" && !this->IsEmpty());
  assert("pre: a_exists" && a!=0);
  assert("pre: valid_i" && (i>=0)&&(i<this->GetNumberOfAttributes()));

#ifndef NDEBUG
int oldnumber = this->GetNumberOfAttributes();
#endif

  this->AttributeInternalVector->Vector[i] = a;
  this->Modified();
  
  assert("post: more_items" && this->GetNumberOfAttributes()==oldnumber);
  assert("post: a_is_set" && this->GetAttribute(i)==a);
}

//----------------------------------------------------------------------------
// Description:
// Remove Attribute at `i'.
void vtkGenericAttributeCollection::RemoveAttribute(int i)
{
  assert("pre: not_empty" && !this->IsEmpty());
  assert("pre: valid_i" && (i>=0)&&(i<this->GetNumberOfAttributes()));
  
#ifndef NDEBUG
  int oldnumber=this->GetNumberOfAttributes();
#endif
  
  this->AttributeInternalVector->Vector.erase(
    this->AttributeInternalVector->Vector.begin()+i);
  this->Modified();
  
  assert("post: fewer_items" && this->GetNumberOfAttributes()==(oldnumber-1));
}

//----------------------------------------------------------------------------
// Description:
// Remove all attributes.
void vtkGenericAttributeCollection::Reset()
{
  for(unsigned int i = 0; i < this->AttributeInternalVector->Vector.size(); ++i)
    {
    this->AttributeInternalVector->Vector[i]->Delete();
    }
  this->AttributeInternalVector->Vector.clear();
  this->Modified();
  
  assert("post: is_empty" && this->IsEmpty());
}

//----------------------------------------------------------------------------
// Description:
// Recursive duplication of `other' in `this'.
void vtkGenericAttributeCollection::DeepCopy(vtkGenericAttributeCollection *other)
{
  assert("pre: other_exists" && other!=0);
  assert("pre: not_self" && other!=this);
  
  this->AttributeInternalVector->Vector.resize(
    other->AttributeInternalVector->Vector.size());
  
  int c = this->AttributeInternalVector->Vector.size();
  for(int i=0; i<c; i++)
    {
    if(this->AttributeInternalVector->Vector[i] == 0)
      {
      this->AttributeInternalVector->Vector[i] = 
        other->AttributeInternalVector->Vector[i]->NewInstance();
      }
    this->AttributeInternalVector->Vector[i]->DeepCopy(
      other->AttributeInternalVector->Vector[i]);
    }
  this->Modified();
  
  assert("post: same_size" && this->GetNumberOfAttributes()==other->GetNumberOfAttributes());
}

//----------------------------------------------------------------------------
// Description:
// Update `this' using fields of `other'.
void vtkGenericAttributeCollection::ShallowCopy(vtkGenericAttributeCollection *other)
{
  assert("pre: other_exists" && other!=0);
  assert("pre: not_self" && other!=this);
  
  this->AttributeInternalVector->Vector = 
    other->AttributeInternalVector->Vector;
  this->Modified();
  
  assert("post: same_size" && this->GetNumberOfAttributes()==other->GetNumberOfAttributes());
}

//----------------------------------------------------------------------------
// Description:
// Collection is composite object and need to check each part for MTime.
unsigned long int vtkGenericAttributeCollection::GetMTime()
{
  unsigned long result;
  unsigned long mtime;
  
  result = vtkObject::GetMTime();
  
  for(int i = 0; i < this->GetNumberOfAttributes(); ++i)
    {
    mtime = this->GetAttribute(i)->GetMTime();
    result = ( mtime > result ? mtime : result );
    }

  return result;
}

//----------------------------------------------------------------------------
// Description:
// Compute number of components, max number of components and actual memory
// size.
void vtkGenericAttributeCollection::ComputeNumbers()
{
  if ( this->GetMTime() > this->ComputeTime )
    {
    int nb = 0;
    int count = 0;
    int maxNb = 0;
    unsigned long memory=0;
    
    int c = this->GetNumberOfAttributes();
    
    for(int i = 0; i < c; ++i)
      {
      count = this->GetAttribute(i)->GetNumberOfComponents();
      memory=memory+this->GetAttribute(i)->GetActualMemorySize();
      if(count > maxNb)
        {
        maxNb = count;
        }
      nb += count;
      }
    
    this->NumberOfComponents = nb;
    this->MaxNumberOfComponents = maxNb;
    this->ActualMemorySize = memory;
    
    assert("check: positive_number" && this->NumberOfComponents>=0);
    assert("check: positiveMaxNumber" && this->MaxNumberOfComponents>=0);
    assert("check: valid_number" && this->MaxNumberOfComponents<=this->NumberOfComponents);
    this->ComputeTime.Modified();
    }
}

//----------------------------------------------------------------------------
// *** ALL THE FOLLOWING METHODS SHOULD BE REMOVED WHEN vtkInformation
// will be ready.
// *** BEGIN

// Description:
// Set the scalar attribute to be processed.
void vtkGenericAttributeCollection::SetActiveAttribute(int attribute,
                                                       int component)
{
  assert("pre: not_empty" && !IsEmpty());
  assert("pre: valid_attribute" && (attribute>=0)&&(attribute<this->GetNumberOfAttributes()));
  assert("pre: valid_component" && (component>=0)&&(component<this->GetAttribute(attribute)->GetNumberOfComponents()));

  this->ActiveAttribute = attribute;
  this->ActiveComponent = component;
  
  assert("post: is_set" && (this->GetActiveAttribute()==attribute) && (this->GetActiveComponent()==component));
}

//----------------------------------------------------------------------------
// Description
// Does the array `attributes' of size `size' have `attribute'?
int vtkGenericAttributeCollection::HasAttribute(int size,
                                                int *attributes,
                                                int attribute)
{
  assert("pre: positive_size" && size>=0);
  assert("pre: valid_attributes" && ((!size>0)||(attributes!=0))); // size>0 => attributes!=0 (A=>B: !A||B )

  int result = 0; // false
  int i;

  if(size != 0)
    {
    i = 0;
    while( !result && i++ < size )
      {
      result = attributes[i] == attribute;
      }
    }
  return result;
}

//----------------------------------------------------------------------------
// Description:
// Set the attributes to interpolate.
void vtkGenericAttributeCollection::SetAttributesToInterpolate(int size,
                                                               int *attributes)
{
  assert("pre: not_empty" && !this->IsEmpty());
  assert("pre: positive_size" && size>=0);
  assert("pre: magic_number" && size<=10);
  assert("pre: valid_attributes" && ((!size>0)||(attributes!=0)));  // size>0 => attributes!=0 (A=>B: !A||B )
  assert("pre: valid_attributes_contents" && (!(attributes!=0) || !(!this->HasAttribute(size,attributes,this->GetActiveAttribute())))); // attributes!=0 => !this->HasAttribute(size,attributes,this->GetActiveAttribute()) (A=>B: !A||B )
  
  this->NumberOfAttributesToInterpolate = size;
  for(int i=0; i<size; ++i)
    {
    this->AttributesToInterpolate[i] = attributes[i];
    }

  assert("post: is_set" && (this->GetNumberOfAttributesToInterpolate()==size));
}

//----------------------------------------------------------------------------
// Description:
// Set the attributes to interpolate.
void vtkGenericAttributeCollection::SetAttributesToInterpolateToAll()
{
  assert("pre: not_empty" && !this->IsEmpty());

  this->NumberOfAttributesToInterpolate = this->GetMaxNumberOfComponents();
  for(int i=0; i<this->NumberOfAttributesToInterpolate; ++i)
    {
    this->AttributesToInterpolate[i] = i;
    }
}

//----------------------------------------------------------------------------
// *** ALL THE PREVIOUS METHODS SHOULD BE REMOVED WHEN vtkInformation
// will be ready.
// *** END
