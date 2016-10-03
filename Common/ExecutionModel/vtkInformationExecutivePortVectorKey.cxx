/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInformationExecutivePortVectorKey.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkInformationExecutivePortVectorKey.h"

#include "vtkExecutive.h"
#include "vtkGarbageCollector.h"
#include "vtkInformation.h"

#include <algorithm>
#include <vector>


// should the pipeline be double or singly linked (referenced) list, single
// make garbage collecting easier but results in a weak reference.
#define VTK_USE_SINGLE_REF 1


//----------------------------------------------------------------------------
vtkInformationExecutivePortVectorKey::vtkInformationExecutivePortVectorKey(const char* name, const char* location):
  vtkInformationKey(name, location)
{
  vtkFilteringInformationKeyManager::Register(this);
}

//----------------------------------------------------------------------------
vtkInformationExecutivePortVectorKey::~vtkInformationExecutivePortVectorKey()
{
}

//----------------------------------------------------------------------------
void vtkInformationExecutivePortVectorKey::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
class vtkInformationExecutivePortVectorValue: public vtkObjectBase
{
public:
  vtkBaseTypeMacro(vtkInformationExecutivePortVectorValue, vtkObjectBase);
  std::vector<vtkExecutive*> Executives;
  std::vector<int> Ports;

  ~vtkInformationExecutivePortVectorValue() VTK_OVERRIDE;
  void UnRegisterAllExecutives();
};

//----------------------------------------------------------------------------
vtkInformationExecutivePortVectorValue
::~vtkInformationExecutivePortVectorValue()
{
  // Remove all our references to executives before erasing the vector.
  this->UnRegisterAllExecutives();
}

//----------------------------------------------------------------------------
void vtkInformationExecutivePortVectorValue::UnRegisterAllExecutives()
{
#ifndef VTK_USE_SINGLE_REF
  for(std::vector<vtkExecutive*>::iterator i = this->Executives.begin();
      i != this->Executives.end(); ++i)
  {
    if(vtkExecutive* e = *i)
    {
      e->UnRegister(0);
    }
  }
#endif
}

//----------------------------------------------------------------------------
void vtkInformationExecutivePortVectorKey::Append(vtkInformation* info,
                                                  vtkExecutive* executive,
                                                  int port)
{
  if(vtkInformationExecutivePortVectorValue* v =
     static_cast<vtkInformationExecutivePortVectorValue *>
     (this->GetAsObjectBase(info)))
  {
    // The entry already exists.  Append to its vector.
#ifndef VTK_USE_SINGLE_REF
    executive->Register(0);
#endif
    v->Executives.push_back(executive);
    v->Ports.push_back(port);
  }
  else
  {
    // The entry does not yet exist.  Just create it.
    this->Set(info, &executive, &port, 1);
  }
}

//----------------------------------------------------------------------------
void vtkInformationExecutivePortVectorKey::Remove(vtkInformation* info,
                                                  vtkExecutive* executive,
                                                  int port)
{
  if(vtkInformationExecutivePortVectorValue* v =
     static_cast<vtkInformationExecutivePortVectorValue *>
     (this->GetAsObjectBase(info)))
  {
    // The entry exists.  Find this executive/port pair and remove it.
    for(unsigned int i=0; i < v->Executives.size(); ++i)
    {
      if(v->Executives[i] == executive && v->Ports[i] == port)
      {
        v->Executives.erase(v->Executives.begin()+i);
        v->Ports.erase(v->Ports.begin()+i);
#ifndef VTK_USE_SINGLE_REF
        executive->UnRegister(0);
#endif
        break;
      }
    }

    // If the last entry was removed, remove the entire value.
    if(v->Executives.empty())
    {
      this->SetAsObjectBase(info, 0);
    }
  }
}

//----------------------------------------------------------------------------
void vtkInformationExecutivePortVectorKey::Set(vtkInformation* info,
                                               vtkExecutive** executives,
                                               int* ports, int length)
{
  if(executives && ports && length > 0)
  {
#ifndef VTK_USE_SINGLE_REF
    // Register our references to all the given executives.
    for(int i=0; i < length; ++i)
    {
      if(executives[i])
      {
        executives[i]->Register(0);
      }
    }
#endif
    // Store the vector of pointers.
    vtkInformationExecutivePortVectorValue* oldv =
      static_cast<vtkInformationExecutivePortVectorValue *>
      (this->GetAsObjectBase(info));
    if(oldv && static_cast<int>(oldv->Executives.size()) == length)
    {
      // Replace the existing value.
      oldv->UnRegisterAllExecutives();
      std::copy(executives, executives+length, oldv->Executives.begin());
      std::copy(ports, ports+length, oldv->Ports.begin());
      // Since this sets a value without call SetAsObjectBase(),
      // the info has to be modified here (instead of
      // vtkInformation::SetAsObjectBase()
      info->Modified();
    }
    else
    {
      // Allocate a new value.
      vtkInformationExecutivePortVectorValue* v =
        new vtkInformationExecutivePortVectorValue;
      v->InitializeObjectBase();
      v->Executives.insert(v->Executives.begin(), executives, executives+length);
      v->Ports.insert(v->Ports.begin(), ports, ports+length);
      this->SetAsObjectBase(info, v);
      v->Delete();
    }
  }
  else
  {
    this->SetAsObjectBase(info, 0);
  }
}

//----------------------------------------------------------------------------
vtkExecutive**
vtkInformationExecutivePortVectorKey::GetExecutives(vtkInformation* info)
{
  vtkInformationExecutivePortVectorValue* v =
    static_cast<vtkInformationExecutivePortVectorValue *>
    (this->GetAsObjectBase(info));
  return (v && !v->Executives.empty())?(&v->Executives[0]):0;
}

//----------------------------------------------------------------------------
int* vtkInformationExecutivePortVectorKey::GetPorts(vtkInformation* info)
{
  vtkInformationExecutivePortVectorValue* v =
    static_cast<vtkInformationExecutivePortVectorValue *>
    (this->GetAsObjectBase(info));
  return (v && !v->Ports.empty())?(&v->Ports[0]):0;
}

//----------------------------------------------------------------------------
void vtkInformationExecutivePortVectorKey::Get(vtkInformation* info,
                                               vtkExecutive** executives,
                                               int* ports)
{
  if(vtkInformationExecutivePortVectorValue* v =
     static_cast<vtkInformationExecutivePortVectorValue *>
     (this->GetAsObjectBase(info)))
  {
    std::copy(v->Executives.begin(), v->Executives.end(), executives);
    std::copy(v->Ports.begin(), v->Ports.end(), ports);
  }
}

//----------------------------------------------------------------------------
int vtkInformationExecutivePortVectorKey::Length(vtkInformation* info)
{
  vtkInformationExecutivePortVectorValue* v =
    static_cast<vtkInformationExecutivePortVectorValue *>
    (this->GetAsObjectBase(info));
  return v?static_cast<int>(v->Executives.size()):0;
}

//----------------------------------------------------------------------------
void vtkInformationExecutivePortVectorKey::ShallowCopy(vtkInformation* from,
                                                vtkInformation* to)
{
  this->Set(to, this->GetExecutives(from), this->GetPorts(from),
            this->Length(from));
}

//----------------------------------------------------------------------------
void vtkInformationExecutivePortVectorKey::Remove(vtkInformation* info)
{
  this->Superclass::Remove(info);
}

//----------------------------------------------------------------------------
void vtkInformationExecutivePortVectorKey::Print(ostream& os,
                                                 vtkInformation* info)
{
  // Print the value.
  if(this->Has(info))
  {
    vtkExecutive** executives = this->GetExecutives(info);
    int* ports = this->GetPorts(info);
    int length = this->Length(info);
    const char* sep = "";
    for(int i=0; i < length; ++i)
    {
      if(executives[i])
      {
        os << sep << executives[i]->GetClassName()
           << "(" << executives[i] << ") port " << ports[i];
      }
      else
      {
        os << sep << "(NULL) port " << ports[i];
      }
      sep = ", ";
    }
  }
}

//----------------------------------------------------------------------------
void
#ifdef VTK_USE_SINGLE_REF
vtkInformationExecutivePortVectorKey::Report(vtkInformation*,
                                             vtkGarbageCollector*)
{
#else
vtkInformationExecutivePortVectorKey::Report(vtkInformation* info,
                                             vtkGarbageCollector* collector)
{
  if(vtkInformationExecutivePortVectorValue* v =
     static_cast<vtkInformationExecutivePortVectorValue *>
     (this->GetAsObjectBase(info)))
  {
    for(std::vector<vtkExecutive*>::iterator i = v->Executives.begin();
        i != v->Executives.end(); ++i)
    {
      vtkGarbageCollectorReport(collector, *i, this->GetName());
    }
  }
#endif
}

//----------------------------------------------------------------------------
vtkExecutive**
vtkInformationExecutivePortVectorKey
::GetExecutivesWatchAddress(vtkInformation* info)
{
  vtkInformationExecutivePortVectorValue* v =
    static_cast<vtkInformationExecutivePortVectorValue*>
    (this->GetAsObjectBase(info));
  return (v && !v->Executives.empty())?(&v->Executives[0]):0;
}

//----------------------------------------------------------------------------
int*
vtkInformationExecutivePortVectorKey
::GetPortsWatchAddress(vtkInformation* info)
{
  vtkInformationExecutivePortVectorValue* v =
    static_cast<vtkInformationExecutivePortVectorValue*>
    (this->GetAsObjectBase(info));
  return (v && !v->Ports.empty())?(&v->Ports[0]):0;
}
