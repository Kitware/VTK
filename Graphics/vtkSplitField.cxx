/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSplitField.cxx
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
#include "vtkSplitField.h"
#include "vtkObjectFactory.h"
#include "vtkDataSetAttributes.h"

char vtkSplitField::FieldLocationNames[3][12] 
= { "DATA_OBJECT",
    "POINT_DATA",
    "CELL_DATA" };

char vtkSplitField::AttributeNames[vtkDataSetAttributes::NUM_ATTRIBUTES][10] 
=  { "SCALARS",
     "VECTORS",
     "NORMALS",
     "TCOORDS",
     "TENSORS" };

typedef vtkSplitField::Component Component;

//--------------------------------------------------------------------------
vtkSplitField* vtkSplitField::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkSplitField");
  if(ret)
    {
    return (vtkSplitField*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkSplitField;
}

vtkSplitField::vtkSplitField()
{
  this->FieldName = 0;
  this->FieldLocation = -1;
  this->AttributeType = -1;
  this->FieldType = -1;

  this->Head = 0;
  this->Tail = 0;
}

vtkSplitField::~vtkSplitField()
{
  delete[] this->FieldName;
  this->FieldName = 0;
  this->DeleteAllComponents();
}

void vtkSplitField::SetInputField(const char* name, int fieldLoc)
{
  if (!name)
    {
    return;
    }

  if ( (fieldLoc !=  vtkSplitField::DATA_OBJECT) &&
       (fieldLoc !=  vtkSplitField::POINT_DATA) &&
       (fieldLoc !=  vtkSplitField::CELL_DATA) )
    {
    vtkErrorMacro("The source for the field is wrong.");
    return;
    }

  this->Modified();
  this->FieldLocation = fieldLoc;
  this->FieldType = vtkSplitField::NAME;

  delete[] this->FieldName;
  this->FieldName = new char[strlen(name)+1];
  strcpy(this->FieldName, name);
}

void vtkSplitField::SetInputField(int attributeType, int fieldLoc)
{
  if ( (fieldLoc !=  vtkSplitField::POINT_DATA) &&
       (fieldLoc !=  vtkSplitField::CELL_DATA) )
    {
    vtkErrorMacro("The source for the field is wrong.");
    return;
    }

  this->Modified();
  this->FieldLocation = fieldLoc;
  this->FieldType = vtkSplitField::ATTRIBUTE;
  this->AttributeType = attributeType;

}

void vtkSplitField::SetInputField(const char* name,
				  const char* fieldLoc)
{
  if ( !name || !fieldLoc)
    {
    return;
    }

  int numAttr = vtkDataSetAttributes::NUM_ATTRIBUTES;
  int numFieldLocs = 3;
  int i;

  // Convert strings to ints and call the appropriate SetInputField()
  int attrType=-1;
  for(i=0; i<numAttr; i++)
    {
    if (!strcmp(name, AttributeNames[i]))
      {
      attrType = i;
      break;
      }
    }

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

  if (attrType == -1)
    {
    this->SetInputField(name, fieldLoc);
    }
  else
    {
    this->SetInputField(attrType, loc);
    }

}

void vtkSplitField::Split(int component, const char* arrayName)
{
  if (!arrayName)
    {
    return;
    }

  this->Modified();
  Component* comp = this->FindComponent(component);
  // If component is already there, just reset the information
  if ( comp )
    {
    comp->SetName(arrayName);
    }
  // otherwise add a new one
  else
    {
    comp = new Component;
    comp->SetName(arrayName);
    comp->Index = component;
    this->AddComponent(comp);
    }
}

void vtkSplitField::Execute()
{
  vtkDataSet *input = static_cast<vtkDataSet*>(this->GetInput());
  vtkDataSet *output = static_cast<vtkDataSet*>(this->GetOutput());

  // This has to be here because it initialized all field datas.
  output->CopyStructure( input );
  
  // Pass all. (data object's field data is passed by the
  // superclass after this method)
  output->GetPointData()->PassData( input->GetPointData() );
  output->GetCellData()->PassData( input->GetCellData() );

  vtkDataArray* outputArray;
  vtkDataArray* inputArray = 0;
  vtkFieldData* fd = 0;
  vtkFieldData* outputFD = 0;
  Component* cur = this->GetFirst();
  Component* before;

  if (!cur) { return; }

  // find the input and output field data
  if ( this->FieldLocation == vtkSplitField::DATA_OBJECT)
    {
    fd = input->GetFieldData();
    outputFD = output->GetFieldData();
    if (!fd || !outputFD)
      {
      vtkErrorMacro("No field data in vtkDataObject.");
      return;
      }
    }
  else if ( this->FieldLocation == vtkSplitField::POINT_DATA )
    {
    fd = input->GetPointData();
    outputFD = output->GetPointData();
    }
  else if ( this->FieldLocation == vtkSplitField::CELL_DATA )
    {
    fd = input->GetCellData();
    outputFD = output->GetCellData();
    }

  if ( this->FieldType == vtkSplitField::NAME )
    {
    inputArray = fd->GetArray(this->FieldName);
    }
  else if ( this->FieldType == vtkSplitField::ATTRIBUTE )
    {
    // If we are working with attributes, we also need to have
    // access to vtkDataSetAttributes methods.
    vtkDataSetAttributes* dsa=vtkDataSetAttributes::SafeDownCast(fd);
    if (!dsa)
      {
      vtkErrorMacro("Sanity check failed, returning.");
      return;
      }
    inputArray = dsa->GetActiveAttribute(this->AttributeType);
    }

  if (!inputArray)
    {
    vtkErrorMacro("Sanity check failed, returning.");
    return;
    }

  // iterate over all components in the linked list and 
  // generate them
  do
    {
    before = cur;
    cur = cur->Next;
    if (before->FieldName)
      {
      outputArray = this->SplitArray(inputArray, before->Index);
      if (outputArray)
	{
	outputArray->SetName(before->FieldName);
	outputFD->AddArray(outputArray);
	outputArray->UnRegister(this);
	}
      }
    } 
  while (cur);
}

// fast pointer copy
template <class T>
static void CopyTuples(T* input, T* output, vtkIdType numTuples, 
		       int numComp, int component)
{
  for (int i=0; i<numTuples; i++)
    {
    output[i] = input[numComp*i+component];
    }
}

vtkDataArray* vtkSplitField::SplitArray(vtkDataArray* da, int component)
{
  if ( (component < 0) || (component > da->GetNumberOfComponents()) )
    {
    vtkErrorMacro("Invalid component. Can not split");
    return 0;
    }

  vtkDataArray* output = da->MakeObject();
  output->SetNumberOfComponents(1);
  int numTuples = da->GetNumberOfTuples();
  output->SetNumberOfTuples(numTuples);
  if ( numTuples > 0 )
    {
    switch (output->GetDataType())
      {
      vtkTemplateMacro5(CopyTuples, (VTK_TT *)da->GetVoidPointer(0), 
			(VTK_TT *)output->GetVoidPointer(0), numTuples,
			da->GetNumberOfComponents(), component );
      // This is not supported by the template macro.
      // Switch to using the float interface.
      case VTK_BIT:
      {
      for(int i=0; i<numTuples; i++)
	{
	output->SetComponent(i, 0, da->GetComponent(i, component));
	}
      }
      break;
      default:
	vtkErrorMacro(<<"Sanity check failed: Unsupported data type.");
	return 0;
      }
    }
  
  return output;

}


// Linked list methods
void vtkSplitField::AddComponent(Component* op)
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

Component* vtkSplitField::FindComponent(int index)
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

void vtkSplitField::DeleteAllComponents()
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

void vtkSplitField::PrintSelf(ostream& os, vtkIndent indent)
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
  os << indent << "Field type: " << this->FieldType << endl;
  os << indent << "Attribute type: " << this->AttributeType << endl;
  os << indent << "Field location: " << this->FieldLocation << endl;
  os << indent << "Linked list head: " << this->Head << endl;
  os << indent << "Linked list tail: " << this->Tail << endl;
  os << indent << "Components: " << endl;
  this->PrintAllComponents(os, indent.GetNextIndent());
}
