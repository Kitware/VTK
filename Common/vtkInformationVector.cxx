/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInformationVector.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkInformationVector.h"

#include "vtkInformation.h"
#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"

#include <vtkstd/vector>

vtkCxxRevisionMacro(vtkInformationVector, "1.1");
vtkStandardNewMacro(vtkInformationVector);

class vtkInformationVectorInternals
{
public:
  typedef vtkstd::vector< vtkSmartPointer<vtkInformation> > VectorType;
  VectorType Vector;
};

//----------------------------------------------------------------------------
vtkInformationVector::vtkInformationVector()
{
  this->Internal = new vtkInformationVectorInternals;
}

//----------------------------------------------------------------------------
vtkInformationVector::~vtkInformationVector()
{
  delete this->Internal;
}

//----------------------------------------------------------------------------
void vtkInformationVector::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
int vtkInformationVector::GetNumberOfInformationObjects()
{
  return static_cast<int>(this->Internal->Vector.size());
}

//----------------------------------------------------------------------------
void vtkInformationVector::SetNumberOfInformationObjects(int n)
{
  if(n < 0)
    {
    vtkErrorMacro("SetNumberOfInformationObjects called with n < 0.");
    return;
    }
  if(n != this->GetNumberOfInformationObjects())
    {
    this->Internal->Vector.resize(n);
    for(int i=0; i < n; ++i)
      {
      if(!this->Internal->Vector[i].GetPointer())
        {
        vtkInformation* info = vtkInformation::New();
        this->Internal->Vector[i] = info;
        info->Delete();
        }
      }
    }
}

//----------------------------------------------------------------------------
void vtkInformationVector::SetInformationObject(int index,
                                                vtkInformation* info)
{
  vtkInformation* newInfo = info? 0:vtkInformation::New();
  if(index >= 0 && index < this->GetNumberOfInformationObjects())
    {
    this->Internal->Vector[index] = info?info:newInfo;
    }
  else if(index >= 0)
    {
    this->SetNumberOfInformationObjects(index+1);
    this->Internal->Vector[index] = info?info:newInfo;
    }
  if(newInfo)
    {
    newInfo->Delete();
    }
}

//----------------------------------------------------------------------------
vtkInformation* vtkInformationVector::GetInformationObject(int index)
{
  if(index >= 0 && index < this->GetNumberOfInformationObjects())
    {
    return this->Internal->Vector[index].GetPointer();
    }
  return 0;
}

//----------------------------------------------------------------------------
void vtkInformationVector::ShallowCopy(vtkInformationVector* from)
{
  if(from)
    {
    this->Internal->Vector = from->Internal->Vector;
    }
  else
    {
    this->Internal->Vector.clear();
    }
}

//----------------------------------------------------------------------------
void vtkInformationVector::DeepCopy(vtkInformationVector* from)
{
  if(from)
    {
    this->Internal->Vector.resize(from->Internal->Vector.size());
    for(vtkInformationVectorInternals::VectorType::size_type i = 0;
        i < this->Internal->Vector.size(); ++i)
      {
      if(!this->Internal->Vector[i].GetPointer())
        {
        vtkInformation* info = vtkInformation::New();
        this->Internal->Vector[i] = info;
        info->Delete();
        }
      this->Internal->Vector[i]->Copy(from->Internal->Vector[i].GetPointer());
      }
    }
  else
    {
    this->Internal->Vector.clear();
    }
}
