/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPanelMark.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkPanelMark.h"

#include "vtkContext2D.h"
#include "vtkObjectFactory.h"
#include "vtkTransform2D.h"

//-----------------------------------------------------------------------------
vtkCxxRevisionMacro(vtkPanelMark, "1.2");
vtkStandardNewMacro(vtkPanelMark);

//-----------------------------------------------------------------------------
vtkPanelMark::vtkPanelMark()
{
}

//-----------------------------------------------------------------------------
vtkPanelMark::~vtkPanelMark()
{
}

//-----------------------------------------------------------------------------
vtkMark* vtkPanelMark::Add(int type)
{
  vtkSmartPointer<vtkMark> m;
  m.TakeReference(vtkMark::CreateMark(type));
  if (this->Marks.size() > 0)
    {
    m->Extend(this->Marks.back());
    }
  this->Marks.push_back(m);
  m->SetParent(this);
  return m;
}

//-----------------------------------------------------------------------------
void vtkPanelMark::Update()
{
  this->MarkInstances.clear();
  this->Left.Update(this);
  this->Right.Update(this);
  this->Top.Update(this);
  this->Bottom.Update(this);  
  size_t numMarks = this->Marks.size();  
  
  // Create only a single instance if no real data is set on panel. 
  vtkIdType numChildren = 1; 
  vtkDataElement data = this->Data.GetData(this);
  if(data.IsValid())
    {    
    numChildren = data.GetNumberOfChildren();
    }
  for (vtkIdType j = 0; j < numMarks; ++j)
    {
    for (vtkIdType i = 0; i < numChildren; ++i)
      {
      this->Index = i;
      this->Marks[j]->DataChanged();
      this->Marks[j]->Update();
      vtkSmartPointer<vtkMark> m;
      m.TakeReference(vtkMark::CreateMark(this->Marks[j]->GetType()));
      m->Extend(this->Marks[j]);
      m->SetParent(this->Marks[j]->GetParent());
      m->SetParentMarkIndex(j);
      m->SetParentDataIndex(i);
      this->MarkInstances.push_back(m);
      }
    }
}

//-----------------------------------------------------------------------------
bool vtkPanelMark::Paint(vtkContext2D* painter)
{
  //TODO: Be smarter about the update
  this->Update();

  if (!painter->GetTransform())
    {
    vtkSmartPointer<vtkTransform2D> trans = vtkSmartPointer<vtkTransform2D>::New();
    trans->Identity();
    painter->SetTransform(trans);
    }

  double* left = this->Left.GetArray(this);
  double* bottom = this->Bottom.GetArray(this);  
  size_t numMarks = this->Marks.size();
  
  vtkDataElement data = this->Data.GetData(this);
  vtkIdType numChildren = 1;
  if(data.IsValid())
    {
    numChildren = data.GetNumberOfChildren();
    }
  for (size_t j = 0; j < numMarks; ++j)
    {
    for (vtkIdType i = 0; i < numChildren; ++i)
      {
      this->Index = i;
      painter->GetTransform()->Translate(left[i], bottom[i]);            
      painter->SetTransform(painter->GetTransform());      
      this->MarkInstances[j*numChildren + i]->Paint(painter);
      painter->GetTransform()->Translate(-left[i], -bottom[i]);
      painter->SetTransform(painter->GetTransform());
      }
    }
  return true;
}

//-----------------------------------------------------------------------------
void vtkPanelMark::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
