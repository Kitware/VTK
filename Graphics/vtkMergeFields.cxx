/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMergeFields.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include "vtkMergeFields.h"
#include "vtkObjectFactory.h"
#include "vtkDataSetAttributes.h"
#include "vtkFloatArray.h"

char vtkMergeFields::FieldLocationNames[3][12] 
= { "DATA_OBJECT",
    "POINT_DATA",
    "CELL_DATA" };

typedef vtkMergeFields::Component Component;

//--------------------------------------------------------------------------
vtkMergeFields* vtkMergeFields::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkMergeFields");
  if(ret)
    {
    return (vtkMergeFields*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkMergeFields;
}

vtkMergeFields::vtkMergeFields()
{
  this->FieldName = 0;
  this->FieldLocation = -1;
  this->NumberOfComponents = 0;

  this->Head = 0;
  this->Tail = 0;
}

vtkMergeFields::~vtkMergeFields()
{
  delete[] this->FieldName;
  this->FieldName = 0;
  this->DeleteAllComponents();
}

void vtkMergeFields::SetOutputField(const char* name, int fieldLoc)
{
  if (!name)
    {
    return;
    }

  if ( (fieldLoc !=  vtkMergeFields::DATA_OBJECT) &&
       (fieldLoc !=  vtkMergeFields::POINT_DATA) &&
       (fieldLoc !=  vtkMergeFields::CELL_DATA) )
    {
    vtkErrorMacro("The source for the field is wrong.");
    return;
    }

  this->FieldLocation = fieldLoc;

  delete[] this->FieldName;
  this->FieldName = new char[strlen(name)+1];
  strcpy(this->FieldName, name);
}


void vtkMergeFields::SetOutputField(const char* name, const char* fieldLoc)
{
  if ( !name || !fieldLoc)
    {
    return;
    }

  int numFieldLocs = 3;
  int i;

  // Convert fieldLoc to int an call the other SetOutputField()
  int loc=-1;
  for(i=0; i<numFieldLocs; i++)
    {
    if (!strcmp(fieldLoc, FieldLocationNames[i]))
      {
      loc = i;
      break;
      }
    }
  if (loc == -1)
    {
    vtkErrorMacro("Location for the field is invalid.");
    return;
    }

  this->SetOutputField(name, fieldLoc);

}

void vtkMergeFields::Merge(int component, const char* arrayName, 
			   int sourceComp)
{
  if (!arrayName)
    {
    return;
    }

  Component* before;
  Component* comp = this->FindComponent(component);
  if ( comp )
    {
    // If component already exists, replace information
    comp->SetName(arrayName);
    comp->SourceIndex = sourceComp;
    }
  else
    {
    // otherwise create a new one
    comp = new Component;
    comp->SetName(arrayName);
    comp->Index = component;
    comp->SourceIndex = sourceComp;
    this->AddComponent(comp);
    }
}

void vtkMergeFields::Execute()
{
  vtkDataSet *input = static_cast<vtkDataSet*>(this->GetInput());
  vtkDataSet *output = static_cast<vtkDataSet*>(this->GetOutput());

  // This has to be here because it initialized all field datas.
  output->CopyStructure( input );
  
  // Pass all. (data object's field data is passed by the
  // superclass after this method)
  output->GetPointData()->PassData( input->GetPointData() );
  output->GetCellData()->PassData( input->GetCellData() );

  vtkFieldData* fd = 0;
  vtkFieldData* outputFD = 0;
  Component* cur = this->GetFirst();
  Component* before;

  if (!cur) { return; }

  // Get the input and output field data
  if ( this->FieldLocation == vtkMergeFields::DATA_OBJECT)
    {
    fd = input->GetFieldData();
    outputFD = output->GetFieldData();
    if (!fd || !outputFD)
      {
      vtkErrorMacro("No field data in vtkDataObject.");
      return;
      }
    }
  else if ( this->FieldLocation == vtkMergeFields::POINT_DATA )
    {
    fd = input->GetPointData();
    outputFD = output->GetPointData();
    }
  else if ( this->FieldLocation == vtkMergeFields::CELL_DATA )
    {
    fd = input->GetCellData();
    outputFD = output->GetCellData();
    }

  // Check if the data types of the input fields are the same
  // Otherwise warn the user.
  vtkDataArray* inputArray = 0;
  int dataType=-1;
  int sameDataType=1;
  do
    {
    before = cur;
    cur = cur->Next;
    inputArray = fd->GetArray(before->FieldName);
    if (!inputArray)
      {
      continue;
      }
    else
      {
      if (dataType == -1)
	{
	dataType = inputArray->GetDataType();
	}
      else
	{
	if ( inputArray->GetDataType() != dataType )
	  {
	  sameDataType = 0;
	  break;
	  }
	}
      }
    } 
  while (cur);
  if ( dataType == -1 )
    {
    vtkErrorMacro("No input array(s) were found.");
    return;
    }
  vtkDataArray* outputArray;
  if (!sameDataType)
    {
    vtkWarningMacro("The input data types do not match. The output will be "<<
		    "float. This will potentially cause accuracy and speed.");
    outputArray = vtkFloatArray::New();
    }
  else
    {
    outputArray = vtkDataArray::CreateDataArray(dataType);
    }

  if (this->NumberOfComponents <= 0)
    {
    vtkErrorMacro("NumberOfComponents has be set prior to the execution of "
		  "this filter");
    }

  outputArray->SetNumberOfComponents(this->NumberOfComponents);

  // Merge
  cur = this->GetFirst();
  do
    {
    before = cur;
    cur = cur->Next;
    inputArray = fd->GetArray(before->FieldName);
    if (inputArray)
      {
      this->MergeArray(inputArray, outputArray, 
		       before->SourceIndex, before->Index);
      }
    else
      {
      if (before->FieldName)
	{
	vtkWarningMacro("Input array " << before->FieldName 
			<< " does not exist.");
	}
      continue;
      }
    } 
  while (cur);
}

// fast pointer copy
template <class T>
static void CopyTuples(T* input, T* output, vtkIdType numTuples, 
 		       int numInComp, int numOutComp,
		       int inComp, int outComp)
{
  for (int i=0; i<numTuples; i++)
    {
    output[numOutComp*i+outComp] = input[numInComp*i+inComp];
    }
}

int vtkMergeFields::MergeArray(vtkDataArray* in, vtkDataArray* out,
			       int inComp, int outComp)
{
  if ( (inComp < 0) || (inComp > in->GetNumberOfComponents()) ||
       (outComp < 0) || (outComp > out->GetNumberOfComponents()) )
    {
    vtkErrorMacro("Invalid component. Can not merge.");
    return 0;
    }

  int numTuples = in->GetNumberOfTuples();
  int i;

  if ( numTuples != out->GetNumberOfTuples() )
    {
    vtkErrorMacro("Number of tuples do not match. Can not merge.");
    return 0;
    }

  if ( numTuples > 0 )
    {
    // If data types match, use templated, fast method
    if ( in->GetDataType() == out->GetDataType() )
      {
      switch (out->GetDataType())
	{
	vtkTemplateMacro7(CopyTuples, (VTK_TT *)in->GetVoidPointer(0), 
			  (VTK_TT *)out->GetVoidPointer(0), numTuples,
			  in->GetNumberOfComponents(), 
			  out->GetNumberOfComponents(), inComp, outComp );
	// This is not supported by the template macro.
	// Switch to using the float interface.
	case VTK_BIT:
	{
	for(i=0; i<numTuples; i++)
	  {
	  out->SetComponent(i, outComp, in->GetComponent(i, inComp));
	  }
	}
	break;
	default:
	  vtkErrorMacro(<<"Sanity check failed: Unsupported data type.");
	  return 0;
	}
      }
    // otherwise use slow float copy
    else
      {
      for(i=0; i<numTuples; i++)
	{
	out->SetComponent(i, outComp, in->GetComponent(i, inComp));
	}
      }
    }
  
  return 1;

}

// linked list methods
void vtkMergeFields::AddComponent(Component* op)
{
  op->Next = 0;

  if (!this->Head)
    {
    this->Head = op;
    this->Tail = op;
    return;
    }
  this->Tail->Next = op;
  this->Tail = op;
}

Component* vtkMergeFields::FindComponent(int index)
{
  Component* before;
  Component* cur = this->GetFirst();
  if (!cur) { return 0; }

  before = 0;
  if (cur->Index == index) { return cur; }
  while (cur->Next)
    {
    before = cur;
    if (cur->Next->Index == index)
      {
      return cur->Next;
      }
    cur = cur->Next;
    }
  return 0;
}

void vtkMergeFields::DeleteAllComponents()
{
  Component* cur = this->GetFirst();
  if (!cur) {return;}
  Component* before;
  do 
    {
    before = cur;
    cur = cur->Next;
    delete before;
    } 
  while (cur);
  this->Head = 0;
  this->Tail = 0;
}

void vtkMergeFields::PrintSelf(ostream& os, vtkIndent indent)
{
  this->vtkDataSetToDataSetFilter::PrintSelf(os,indent);
  os << indent << "Field name: ";
  if (this->FieldName)
    {
    os << this->FieldName << endl;
    }
  else
    {
    os << "(none)" << endl;
    }
  os << indent << "Field location: " << this->FieldLocation << endl;
  os << indent << "Linked list head: " << this->Head << endl;
  os << indent << "Linked list tail: " << this->Tail << endl;
  os << indent << "Components: " << endl;
  this->PrintAllComponents(os, indent.GetNextIndent());
}
