/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMergeFilter.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkMergeFilter.h"
#include "vtkPolyData.h"
#include "vtkStructuredGrid.h"
#include "vtkStructuredPoints.h"
#include "vtkUnstructuredGrid.h"
#include "vtkRectilinearGrid.h"
#include "vtkObjectFactory.h"

vtkCxxRevisionMacro(vtkMergeFilter, "1.65");
vtkStandardNewMacro(vtkMergeFilter);

class vtkFieldNode
{
public:
  vtkFieldNode(const char* name, vtkDataSet* ptr=0)
    {
      int length = static_cast<int>(strlen(name));
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
  vtkIdType numPts, numScalars=0, numVectors=0, numNormals=0, numTCoords=0;
  vtkIdType numTensors=0;
  vtkIdType numCells, numCellScalars=0, numCellVectors=0, numCellNormals=0;
  vtkIdType numCellTCoords=0, numCellTensors=0;
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
    scalars = pd->GetScalars();
    if ( scalars != NULL )
      {
      numScalars = scalars->GetNumberOfTuples();
      }
    cd = this->GetScalars()->GetCellData();
    cellScalars = cd->GetScalars();
    if ( cellScalars != NULL )
      {
      numCellScalars = cellScalars->GetNumberOfTuples();
      }
    }

  if ( this->GetVectors() ) 
    {
    pd = this->GetVectors()->GetPointData();
    vectors = pd->GetVectors();
    if ( vectors != NULL )
      {
      numVectors= vectors->GetNumberOfTuples();
      }
    cd = this->GetVectors()->GetCellData();
    cellVectors = cd->GetVectors();
    if ( cellVectors != NULL )
      {
      numCellVectors = cellVectors->GetNumberOfTuples();
      }
    }

  if ( this->GetNormals() ) 
    {
    pd = this->GetNormals()->GetPointData();
    normals = pd->GetNormals();
    if ( normals != NULL )
      {
      numNormals= normals->GetNumberOfTuples();
      }
    cd = this->GetNormals()->GetCellData();
    cellNormals = cd->GetNormals();
    if ( cellNormals != NULL )
      {
      numCellNormals = cellNormals->GetNumberOfTuples();
      }
    }

  if ( this->GetTCoords() ) 
    {
    pd = this->GetTCoords()->GetPointData();
    tcoords = pd->GetTCoords();
    if ( tcoords != NULL )
      {
      numTCoords= tcoords->GetNumberOfTuples();
      }
    cd = this->GetTCoords()->GetCellData();
    cellTCoords = cd->GetTCoords();
    if ( cellTCoords != NULL )
      {
      numCellTCoords = cellTCoords->GetNumberOfTuples();
      }
    }

  if ( this->GetTensors() ) 
    {
    pd = this->GetTensors()->GetPointData();
    tensors = pd->GetTensors();
    if ( tensors != NULL )
      {
      numTensors = tensors->GetNumberOfTuples();
      }
    cd = this->GetTensors()->GetCellData();
    cellTensors = cd->GetTensors();
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
  vtkIdType num;
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
  this->Superclass::PrintSelf(os,indent);

}

