/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInformationExecutivePortKey.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkInformationExecutivePortKey.h"

#include "vtkExecutive.h"
#include "vtkSmartPointer.h"

vtkCxxRevisionMacro(vtkInformationExecutivePortKey, "1.1");

//----------------------------------------------------------------------------
vtkInformationExecutivePortKey::vtkInformationExecutivePortKey(const char* name, const char* location):
  vtkInformationKey(name, location)
{
  vtkFilteringInformationKeyManager::Register(this);
}

//----------------------------------------------------------------------------
vtkInformationExecutivePortKey::~vtkInformationExecutivePortKey()
{
}

//----------------------------------------------------------------------------
void vtkInformationExecutivePortKey::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
class vtkInformationExecutivePortValue: public vtkObjectBase
{
public:
  vtkTypeMacro(vtkInformationExecutivePortValue, vtkObjectBase);
  vtkSmartPointer<vtkExecutive> Executive;
  int Port;
};

//----------------------------------------------------------------------------
void vtkInformationExecutivePortKey::Set(vtkInformation* info,
                                         vtkExecutive* executive, int port)
{
  if(executive)
    {
    if(vtkInformationExecutivePortValue* oldv =
       vtkInformationExecutivePortValue::SafeDownCast(
         this->GetAsObjectBase(info)))
      {
      // Replace the existing value.
      oldv->Executive = executive;
      oldv->Port = port;
      }
    else
      {
      // Allocate a new value.
      vtkInformationExecutivePortValue* v =
        new vtkInformationExecutivePortValue;
      this->ConstructClass("vtkInformationExecutivePortValue");
      v->Executive = executive;
      v->Port = port;
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
vtkExecutive*
vtkInformationExecutivePortKey::GetExecutive(vtkInformation* info)
{
  if(vtkInformationExecutivePortValue* v =
     vtkInformationExecutivePortValue::SafeDownCast(
       this->GetAsObjectBase(info)))
    {
    return v->Executive;
    }
  return 0;
}

//----------------------------------------------------------------------------
int vtkInformationExecutivePortKey::GetPort(vtkInformation* info)
{
  vtkInformationExecutivePortValue* v =
    vtkInformationExecutivePortValue::SafeDownCast(
      this->GetAsObjectBase(info));
  return v?v->Port:0;
}

//----------------------------------------------------------------------------
int vtkInformationExecutivePortKey::Has(vtkInformation* info)
{
  vtkInformationExecutivePortValue* v =
    vtkInformationExecutivePortValue::SafeDownCast(
      this->GetAsObjectBase(info));
  return v?1:0;
}

//----------------------------------------------------------------------------
void vtkInformationExecutivePortKey::Copy(vtkInformation* from,
                                          vtkInformation* to)
{
  this->Set(to, this->GetExecutive(from), this->GetPort(from));
}

//----------------------------------------------------------------------------
void vtkInformationExecutivePortKey::Report(vtkInformation* info,
                                            vtkGarbageCollector* collector)
{
  if(vtkInformationExecutivePortValue* v =
     vtkInformationExecutivePortValue::SafeDownCast(
       this->GetAsObjectBase(info)))
    {
    v->Executive.Report(collector, this->GetName());
    }
}
