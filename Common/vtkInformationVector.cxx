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

#include "vtkGarbageCollector.h"
#include "vtkInformation.h"
#include "vtkObjectFactory.h"

#include <vtkstd/vector>

vtkStandardNewMacro(vtkInformationVector);

class vtkInformationVectorInternals
{
public:
  vtkstd::vector<vtkInformation*> Vector;

  ~vtkInformationVectorInternals();
};

//----------------------------------------------------------------------------
vtkInformationVectorInternals::~vtkInformationVectorInternals()
{
  // Delete all the information objects.
  for(vtkstd::vector<vtkInformation*>::iterator i = this->Vector.begin();
      i != this->Vector.end(); ++i)
    {
    if(vtkInformation* info = *i)
      {
      info->Delete();
      }
    }
}

//----------------------------------------------------------------------------
vtkInformationVector::vtkInformationVector()
{
  this->Internal = new vtkInformationVectorInternals;
  this->NumberOfInformationObjects = 0;
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
  os << indent << "Number of Information Objects: " << this->NumberOfInformationObjects << "\n";
  os << indent << "Information Objects:\n";
  for(int i=0; i < this->NumberOfInformationObjects; ++i)
    {
    vtkInformation* info = this->GetInformationObject(i);
    vtkIndent nextIndent = indent.GetNextIndent();
    os << nextIndent << info->GetClassName() << "(" << info << "):\n";
    info->PrintSelf(os, nextIndent.GetNextIndent());
    }
}

//----------------------------------------------------------------------------
void vtkInformationVector::SetNumberOfInformationObjects(int newNumber)
{
  // Adjust the number of objects.
  int oldNumber = this->NumberOfInformationObjects;
  if(newNumber > oldNumber)
    {
    // Create new information objects.
    this->Internal->Vector.resize(newNumber, 0);
    for(int i=oldNumber; i < newNumber; ++i)
      {
      this->Internal->Vector[i] = vtkInformation::New();
      }
    }
  else if(newNumber < oldNumber)
    {
    // Delete old information objects.
    for(int i=newNumber; i < oldNumber; ++i)
      {
      if(vtkInformation* info = this->Internal->Vector[i])
        {
        // Set the pointer to NULL first to avoid reporting of the
        // entry if deleting the information object causes a garbage
        // collection reference walk.
        this->Internal->Vector[i] = 0;
        info->Delete();
        }
      }
    this->Internal->Vector.resize(newNumber);
    }
  this->NumberOfInformationObjects = newNumber;
}

//----------------------------------------------------------------------------
void vtkInformationVector::SetInformationObject(int index,
                                                vtkInformation* newInfo)
{
  if(newInfo && index >= 0 && index < this->NumberOfInformationObjects)
    {
    // Replace an existing information object.
    vtkInformation* oldInfo = this->Internal->Vector[index];
    if(oldInfo != newInfo)
      {
      newInfo->Register(this);
      this->Internal->Vector[index] = newInfo;
      oldInfo->UnRegister(this);
      }
    }
  else if(newInfo && index >= this->NumberOfInformationObjects)
    {
    // If a hole will be created fill it with empty objects.
    if(index > this->NumberOfInformationObjects)
      {
      this->SetNumberOfInformationObjects(index);
      }

    // Store the information object in a new entry.
    newInfo->Register(this);
    this->Internal->Vector.push_back(newInfo);
    this->NumberOfInformationObjects++;
    }
  else if(!newInfo && index >= 0 &&
          index < this->NumberOfInformationObjects-1)
    {
    // We do not allow NULL information objects.  Create an empty one
    // to fill in the hole.
    vtkInformation* oldInfo = this->Internal->Vector[index];
    this->Internal->Vector[index] = vtkInformation::New();
    oldInfo->UnRegister(this);
    }
  else if(!newInfo && index >= 0 &&
          index == this->NumberOfInformationObjects-1)
    {
    // Remove the last information object.
    this->SetNumberOfInformationObjects(index);
    }
}

//----------------------------------------------------------------------------
vtkInformation* vtkInformationVector::GetInformationObject(int index)
{
  if(index >= 0 && index < this->NumberOfInformationObjects)
    {
    return this->Internal->Vector[index];
    }
  return 0;
}

//----------------------------------------------------------------------------
void vtkInformationVector::Append(vtkInformation* info)
{
  // Setting an entry beyond the end will automatically append.
  this->SetInformationObject(this->NumberOfInformationObjects, info);
}

//----------------------------------------------------------------------------
void vtkInformationVector::Remove(vtkInformation* info)
{
  // Search for the information object and remove it.
  for(int i=0; i < this->NumberOfInformationObjects; ++i)
    {
    if(this->Internal->Vector[i] == info)
      {
      this->Internal->Vector.erase(this->Internal->Vector.begin()+i);
      info->UnRegister(this);
      this->NumberOfInformationObjects--;
      }
    }
}

//----------------------------------------------------------------------------
void vtkInformationVector::Copy(vtkInformationVector* from, int deep)
{
  // if deep we can reuse existing info objects
  if (deep)
    {
    this->SetNumberOfInformationObjects(from->GetNumberOfInformationObjects());
    for (int i = 0; i < from->GetNumberOfInformationObjects(); ++i)
      {
      this->Internal->Vector[i]->Copy(from->GetInformationObject(i),deep);
      }
     return;
    }
  
  // otherwise it is a shallow copy and we must copy pointers
  this->SetNumberOfInformationObjects(0);
  // copy the data
  for (int i = 0; i < from->GetNumberOfInformationObjects(); ++i)
    {
    vtkInformation *fromI = from->GetInformationObject(i);
    this->SetInformationObject(i,fromI);
    }
}

//----------------------------------------------------------------------------
void vtkInformationVector::Register(vtkObjectBase* o)
{
  this->RegisterInternal(o, 1);
}

//----------------------------------------------------------------------------
void vtkInformationVector::UnRegister(vtkObjectBase* o)
{
  this->UnRegisterInternal(o, 1);
}

//----------------------------------------------------------------------------
void vtkInformationVector::ReportReferences(vtkGarbageCollector* collector)
{
  this->Superclass::ReportReferences(collector);
  for(int i=0; i < this->NumberOfInformationObjects; ++i)
    {
    vtkGarbageCollectorReport(collector, this->Internal->Vector[i], "Entry");
    }
}
