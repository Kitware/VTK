/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMergeFilter.cxx
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
#include "vtkMergeFilter.h"
#include "vtkPolyData.h"
#include "vtkStructuredGrid.h"
#include "vtkStructuredPoints.h"
#include "vtkUnstructuredGrid.h"
#include "vtkRectilinearGrid.h"
#include "vtkObjectFactory.h"


class vtkFieldNode
{
public:
  vtkFieldNode(const char* name, vtkDataSet* ptr=0)
    {
      int length = strlen(name);
      if (length > 0)
	{
	this->Name = new char[length+1];
	strcpy(this->Name, name);
	}
      else
	{
	this->Name = 0;
	}
      this->Ptr = ptr;
      this->Next = 0;
    }
  ~vtkFieldNode()
    {
      delete[] this->Name;
    }

  const char* GetName()
    {
      return Name;
    }
  vtkDataSet* Ptr;
  vtkFieldNode* Next;
private:
  vtkFieldNode(const vtkFieldNode&) {}
  void operator=(const vtkFieldNode&) {}
  char* Name;
};

class vtkFieldList
{
public:
  vtkFieldList()
    {
      this->First = 0;
      this->Last = 0;
    }
  ~vtkFieldList()
    {
      vtkFieldNode* node = this->First;
      vtkFieldNode* next;
      while(node)
	{
	next = node->Next;
	delete node;
	node = next;
	}
    }


  void Add(const char* name, vtkDataSet* ptr)
    {
      vtkFieldNode* newNode = new vtkFieldNode(name, ptr);
      if (!this->First)
	{
	this->First = newNode;
	this->Last = newNode;
	}
      else
	{
	this->Last->Next = newNode;
	this->Last = newNode;
	}
    }

  friend class vtkFieldListIterator;

private:
  vtkFieldNode* First;
  vtkFieldNode* Last;
};

class vtkFieldListIterator
{
public:
  vtkFieldListIterator(vtkFieldList* list)
    {
      this->List = list;
      this->Position = 0;
    }
  void Begin()
    {
      this->Position = this->List->First;
    }
  void Next()
    {
      if (this->Position)
	{
 	this->Position = this->Position->Next;
	}
    }
  int End()
    {
      return this->Position ? 0 : 1;
    }
  vtkFieldNode* Get()
    {
      return this->Position;
    }
  
private:
  vtkFieldNode* Position;
  vtkFieldList* List;
};

//------------------------------------------------------------------------------
vtkMergeFilter* vtkMergeFilter::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkMergeFilter");
  if(ret)
    {
    return (vtkMergeFilter*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkMergeFilter;
}




// Create object with no input or output.
vtkMergeFilter::vtkMergeFilter()
{
  this->FieldList = new vtkFieldList;
}

vtkMergeFilter::~vtkMergeFilter()
{
  delete this->FieldList;
}

void vtkMergeFilter::SetScalars(vtkDataSet *input)
{
  this->vtkProcessObject::SetNthInput(1, input);
}
vtkDataSet *vtkMergeFilter::GetScalars()
{
  if (this->NumberOfInputs < 2)
    {
    return NULL;
    }
  return (vtkDataSet *)(this->Inputs[1]);
}

void vtkMergeFilter::SetVectors(vtkDataSet *input)
{
  this->vtkProcessObject::SetNthInput(2, input);
}
vtkDataSet *vtkMergeFilter::GetVectors()
{
  if (this->NumberOfInputs < 3)
    {
    return NULL;
    }
  return (vtkDataSet *)(this->Inputs[2]);
}

void vtkMergeFilter::SetNormals(vtkDataSet *input)
{
  this->vtkProcessObject::SetNthInput(3, input);
}
vtkDataSet *vtkMergeFilter::GetNormals()
{
  if (this->NumberOfInputs < 4)
    {
    return NULL;
    }
  return (vtkDataSet *)(this->Inputs[3]);
}

void vtkMergeFilter::SetTCoords(vtkDataSet *input)
{
  this->vtkProcessObject::SetNthInput(4, input);
}
vtkDataSet *vtkMergeFilter::GetTCoords()
{
  if (this->NumberOfInputs < 5)
    {
    return NULL;
    }
  return (vtkDataSet *)(this->Inputs[4]);
}

void vtkMergeFilter::SetTensors(vtkDataSet *input)
{
  this->vtkProcessObject::SetNthInput(5, input);
}
vtkDataSet *vtkMergeFilter::GetTensors()
{
  if (this->NumberOfInputs < 6)
    {
    return NULL;
    }
  return (vtkDataSet *)(this->Inputs[5]);
}

void vtkMergeFilter::AddField(const char* name, vtkDataSet* input)
{
  this->FieldList->Add(name, input);
}

void vtkMergeFilter::Execute()
{
  int numPts, numScalars=0, numVectors=0, numNormals=0, numTCoords=0;
  int numTensors=0;
  int numCells, numCellScalars=0, numCellVectors=0, numCellNormals=0;
  int numCellTCoords=0, numCellTensors=0;
  vtkPointData *pd;
  vtkDataArray *scalars = NULL;
  vtkDataArray *vectors = NULL;
  vtkDataArray *normals = NULL;
  vtkDataArray *tcoords = NULL;
  vtkDataArray *tensors = NULL;
  vtkCellData *cd;
  vtkDataArray *cellScalars = NULL;
  vtkDataArray *cellVectors = NULL;
  vtkDataArray *cellNormals = NULL;
  vtkDataArray *cellTCoords = NULL;
  vtkDataArray *cellTensors = NULL;
  vtkDataSet *output = this->GetOutput();
  vtkPointData *outputPD = output->GetPointData();
  vtkCellData *outputCD = output->GetCellData();
  
  vtkDebugMacro(<<"Merging data!");

  // geometry needs to be copied
  output->CopyStructure(this->GetInput());
  if ( (numPts = this->GetInput()->GetNumberOfPoints()) < 1 )
    {
    vtkWarningMacro(<<"Nothing to merge!");
    }
  numCells = this->GetInput()->GetNumberOfCells();
  
  if ( this->GetScalars() ) 
    {
    pd = this->GetScalars()->GetPointData();
    scalars = pd->GetActiveScalars();
    if ( scalars != NULL )
      {
      numScalars = scalars->GetNumberOfTuples();
      }
    cd = this->GetScalars()->GetCellData();
    cellScalars = cd->GetActiveScalars();
    if ( cellScalars != NULL )
      {
      numCellScalars = cellScalars->GetNumberOfTuples();
      }
    }

  if ( this->GetVectors() ) 
    {
    pd = this->GetVectors()->GetPointData();
    vectors = pd->GetActiveVectors();
    if ( vectors != NULL )
      {
      numVectors= vectors->GetNumberOfTuples();
      }
    cd = this->GetVectors()->GetCellData();
    cellVectors = cd->GetActiveVectors();
    if ( cellVectors != NULL )
      {
      numCellVectors = cellVectors->GetNumberOfTuples();
      }
    }

  if ( this->GetNormals() ) 
    {
    pd = this->GetNormals()->GetPointData();
    normals = pd->GetActiveNormals();
    if ( normals != NULL )
      {
      numNormals= normals->GetNumberOfTuples();
      }
    cd = this->GetNormals()->GetCellData();
    cellNormals = cd->GetActiveNormals();
    if ( cellNormals != NULL )
      {
      numCellNormals = cellNormals->GetNumberOfTuples();
      }
    }

  if ( this->GetTCoords() ) 
    {
    pd = this->GetTCoords()->GetPointData();
    tcoords = pd->GetActiveTCoords();
    if ( tcoords != NULL )
      {
      numTCoords= tcoords->GetNumberOfTuples();
      }
    cd = this->GetTCoords()->GetCellData();
    cellTCoords = cd->GetActiveTCoords();
    if ( cellTCoords != NULL )
      {
      numCellTCoords = cellTCoords->GetNumberOfTuples();
      }
    }

  if ( this->GetTensors() ) 
    {
    pd = this->GetTensors()->GetPointData();
    tensors = pd->GetActiveTensors();
    if ( tensors != NULL )
      {
      numTensors = tensors->GetNumberOfTuples();
      }
    cd = this->GetTensors()->GetCellData();
    cellTensors = cd->GetActiveTensors();
    if ( cellTensors != NULL )
      {
      numCellTensors = cellTensors->GetNumberOfTuples();
      }
    }

  // merge data only if it is consistent
  if ( numPts == numScalars )
    {
    outputPD->SetScalars(scalars);
    }
  if ( numCells == numCellScalars )
    {
    outputCD->SetScalars(cellScalars);
    }

  if ( numPts == numVectors )
    {
    outputPD->SetVectors(vectors);
    }
  if ( numCells == numCellVectors )
    {
    outputCD->SetVectors(cellVectors);
    }
    
  if ( numPts == numNormals )
    {
    outputPD->SetNormals(normals);
    }
  if ( numCells == numCellNormals )
    {
    outputCD->SetNormals(cellNormals);
    }

  if ( numPts == numTCoords )
    {
    outputPD->SetTCoords(tcoords);
    }
  if ( numCells == numCellTCoords )
    {
    outputCD->SetTCoords(cellTCoords);
    }

  if ( numPts == numTensors )
    {
    outputPD->SetTensors(tensors);
    }
  if ( numCells == numCellTensors )
    {
    outputCD->SetTensors(cellTensors);
    }

  vtkFieldListIterator it(this->FieldList);
  vtkDataArray* da;
  const char* name;
  int num;
  for(it.Begin(); !it.End() ; it.Next())
    {
    pd = it.Get()->Ptr->GetPointData();
    cd = it.Get()->Ptr->GetCellData();
    name = it.Get()->GetName();
    if ( (da=pd->GetArray(name)) )
      {
      num = da->GetNumberOfTuples();
      if (num == numPts)
	{
	outputPD->AddArray(da);
	}
      }
    if ( (da=cd->GetArray(name)) )
      {
      num = da->GetNumberOfTuples();
      if (num == numPts)
	{
	outputCD->AddArray(da);
	}
      }
    }
}

//----------------------------------------------------------------------------
//  Trick:  Abstract data types that may or may not be the same type
// (structured/unstructured), but the points/cells match up.
// Output/Geometry may be structured while ScalarInput may be 
// unstructured (but really have same triagulation/topology as geometry).
// Just request all the input. Always generate all of the output (todo).
void vtkMergeFilter::ComputeInputUpdateExtents(vtkDataObject *vtkNotUsed(data))
{
  vtkDataSet *input;
  int idx;
  
  for (idx = 0; idx < this->NumberOfInputs; ++idx)
    {
    input = (vtkDataSet *)(this->Inputs[idx]);
    if (input)
      {
      input->SetUpdateExtent(0, 1, 0);
      input->RequestExactExtentOn();
      }
    }
}

void vtkMergeFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkDataSetToDataSetFilter::PrintSelf(os,indent);

}

