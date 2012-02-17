/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkBlueObeliskData.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkBlueObeliskData.h"

#include "vtkAbstractArray.h"
#include "vtkBlueObeliskDataParser.h"
#include "vtkFloatArray.h"
#include "vtkObjectFactory.h"
#include "vtkStringArray.h"
#include "vtkMutexLock.h"
#include "vtkUnsignedShortArray.h"

#include <vector>

// Hidden STL reference: std::vector<vtkAbstractArray*>
class MyStdVectorOfVtkAbstractArrays : public std::vector<vtkAbstractArray*>
{
};

vtkStandardNewMacro(vtkBlueObeliskData);

//----------------------------------------------------------------------------
vtkBlueObeliskData::vtkBlueObeliskData()
  : WriteMutex(vtkSimpleMutexLock::New()),
    Initialized(false), NumberOfElements(0),
    Arrays(new MyStdVectorOfVtkAbstractArrays)
{
  // Setup arrays and build Arrays
  this->Arrays->reserve(19);

  this->Symbols->SetNumberOfComponents(1);
  this->Arrays->push_back(this->Symbols.GetPointer());

  this->LowerSymbols->SetNumberOfComponents(1);
  this->Arrays->push_back(this->LowerSymbols.GetPointer());

  this->Names->SetNumberOfComponents(1);
  this->Arrays->push_back(this->Names.GetPointer());

  this->LowerNames->SetNumberOfComponents(1);
  this->Arrays->push_back(this->LowerNames.GetPointer());

  this->PeriodicTableBlocks->SetNumberOfComponents(1);
  this->Arrays->push_back(this->PeriodicTableBlocks.GetPointer());

  this->ElectronicConfigurations->SetNumberOfComponents(1);
  this->Arrays->push_back(this->ElectronicConfigurations.GetPointer());

  this->Families->SetNumberOfComponents(1);
  this->Arrays->push_back(this->Families.GetPointer());

  this->Masses->SetNumberOfComponents(1);
  this->Arrays->push_back(this->Masses.GetPointer());

  this->ExactMasses->SetNumberOfComponents(1);
  this->Arrays->push_back(this->ExactMasses.GetPointer());

  this->IonizationEnergies->SetNumberOfComponents(1);
  this->Arrays->push_back(this->IonizationEnergies.GetPointer());

  this->ElectronAffinities->SetNumberOfComponents(1);
  this->Arrays->push_back(this->ElectronAffinities.GetPointer());

  this->PaulingElectronegativities->SetNumberOfComponents(1);
  this->Arrays->push_back(this->PaulingElectronegativities.GetPointer());

  this->CovalentRadii->SetNumberOfComponents(1);
  this->Arrays->push_back(this->CovalentRadii.GetPointer());

  this->VDWRadii->SetNumberOfComponents(1);
  this->Arrays->push_back(this->VDWRadii.GetPointer());

  this->DefaultColors->SetNumberOfComponents(3);
  this->Arrays->push_back(this->DefaultColors.GetPointer());

  this->BoilingPoints->SetNumberOfComponents(1);
  this->Arrays->push_back(this->BoilingPoints.GetPointer());

  this->MeltingPoints->SetNumberOfComponents(1);
  this->Arrays->push_back(this->MeltingPoints.GetPointer());

  this->Periods->SetNumberOfComponents(1);
  this->Arrays->push_back(this->Periods.GetPointer());

  this->Groups->SetNumberOfComponents(1);
  this->Arrays->push_back(this->Groups.GetPointer());
}

//----------------------------------------------------------------------------
vtkBlueObeliskData::~vtkBlueObeliskData()
{
  delete Arrays;
  this->WriteMutex->Delete();
}

//----------------------------------------------------------------------------
void vtkBlueObeliskData::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "NumberOfElements: " << this->NumberOfElements << "\n";

  this->PrintSelfIfExists("this->Symbols",
                          this->Symbols.GetPointer(), os, indent);
  this->PrintSelfIfExists("this->LowerSymbols",
                          this->LowerSymbols.GetPointer(), os, indent);
  this->PrintSelfIfExists("this->Names",
                          this->Names.GetPointer(), os, indent);
  this->PrintSelfIfExists("this->LowerNames",
                          this->LowerNames.GetPointer(), os, indent);
  this->PrintSelfIfExists("this->PeriodicTableBlocks",
                          this->PeriodicTableBlocks.GetPointer(), os, indent);
  this->PrintSelfIfExists("this->ElectronicConfigurations",
                          this->ElectronicConfigurations.GetPointer(),
                          os, indent);
  this->PrintSelfIfExists("this->Families",
                          this->Families.GetPointer(), os, indent);

  this->PrintSelfIfExists("this->Masses",
                          this->Masses.GetPointer(), os, indent);
  this->PrintSelfIfExists("this->ExactMasses",
                          this->ExactMasses.GetPointer(), os, indent);
  this->PrintSelfIfExists("this->IonizationEnergies",
                          this->IonizationEnergies.GetPointer(), os, indent);
  this->PrintSelfIfExists("this->ElectronAffinities",
                          this->ElectronAffinities.GetPointer(), os, indent);
  this->PrintSelfIfExists("this->PaulingElectronegativities",
                          this->PaulingElectronegativities.GetPointer(),
                          os, indent);
  this->PrintSelfIfExists("this->CovalentRadii",
                          this->CovalentRadii.GetPointer(), os, indent);
  this->PrintSelfIfExists("this->VDWRadii",
                          this->VDWRadii.GetPointer(), os, indent);
  this->PrintSelfIfExists("this->DefaultColors",
                          this->DefaultColors.GetPointer(), os, indent);
  this->PrintSelfIfExists("this->BoilingPoints",
                          this->BoilingPoints.GetPointer(), os, indent);
  this->PrintSelfIfExists("this->MeltingPoints",
                          this->MeltingPoints.GetPointer(), os, indent);
  this->PrintSelfIfExists("this->Periods",
                          this->Periods.GetPointer(), os, indent);
  this->PrintSelfIfExists("this->Groups",
                          this->Groups.GetPointer(), os, indent);
}

//----------------------------------------------------------------------------
void vtkBlueObeliskData::PrintSelfIfExists(const char * name, vtkObject *obj,
                                ostream& os, vtkIndent indent)
{
  if (obj)
    {
    os << indent << name << ": @" << obj << "\n";
    obj->PrintSelf(os, indent.GetNextIndent());
    }
  else
    {
    os << indent << name << " is null.\n";
    }
}

//----------------------------------------------------------------------------
void vtkBlueObeliskData::Initialize()
{
  if (IsInitialized())
    {
    vtkDebugMacro(<<"vtkBlueObeliskData @" << this
                  << " already initialized.\n");
    return;
    }

  vtkNew<vtkBlueObeliskDataParser> parser;
  parser->SetTarget(this);
  parser->Parse();

  this->Initialized = true;
}

//----------------------------------------------------------------------------
int vtkBlueObeliskData::Allocate(vtkIdType sz, vtkIdType ext)
{
  for (MyStdVectorOfVtkAbstractArrays::iterator it = this->Arrays->begin(),
         it_end = this->Arrays->end(); it != it_end; ++it)
    {
    if ((*it)->Allocate(sz * (*it)->GetNumberOfComponents(), ext) == 0)
      {
      return 0;
      }
    }
  return 1;
}

//----------------------------------------------------------------------------
void vtkBlueObeliskData::Squeeze()
{
  for (MyStdVectorOfVtkAbstractArrays::iterator it = this->Arrays->begin(),
         it_end = this->Arrays->end(); it != it_end; ++it)
    {
    (*it)->Squeeze();
    }
}

//----------------------------------------------------------------------------
void vtkBlueObeliskData::Reset()
{
  for (MyStdVectorOfVtkAbstractArrays::iterator it = this->Arrays->begin(),
         it_end = this->Arrays->end(); it != it_end; ++it)
    {
    (*it)->Reset();
    }
}
