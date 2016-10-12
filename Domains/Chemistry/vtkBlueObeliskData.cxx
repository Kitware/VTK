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
#include "vtkBlueObeliskDataInternal.h"
#include "vtkBlueObeliskDataParser.h"
#include "vtkFloatArray.h"
#include "vtkObjectFactory.h"
#include "vtkStringArray.h"
#include "vtkTypeTraits.h"
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

// Helpers for reading raw data from the private header into a VTK array.
namespace {

void LoadStringArray(vtkStringArray *array, const char *data[], vtkIdType size)
{
  array->SetNumberOfTuples(size);
  for (vtkIdType i = 0; i < size; ++i)
  {
    array->SetValue(i, data[i]);
  }
}

template <int NumComps, typename ArrayT>
void LoadDataArray(ArrayT *array,
                   const typename ArrayT::ValueType data[][NumComps],
                   vtkIdType numTuples)
{
  array->SetNumberOfTuples(numTuples);

  for (vtkIdType t = 0; t < numTuples; ++t)
  {
    for (int c = 0; c < NumComps; ++c)
    {
      array->SetTypedComponent(t, c, data[t][c]);
    }
  }
}

} // End anon namespace

//----------------------------------------------------------------------------
void vtkBlueObeliskData::Initialize()
{
  if (IsInitialized())
  {
    vtkDebugMacro(<<"vtkBlueObeliskData @" << this
                  << " already initialized.\n");
    return;
  }

  this->NumberOfElements = _vtkBlueObeliskData::numberOfElements;
  vtkIdType arraySize = this->NumberOfElements + 1; // 0 is dummy element

#define READARRAY(name) \
  LoadStringArray(this->name.Get(), _vtkBlueObeliskData::name, arraySize)

  READARRAY(Symbols);
  READARRAY(LowerSymbols);
  READARRAY(Names);
  READARRAY(LowerNames);
  READARRAY(PeriodicTableBlocks);
  READARRAY(ElectronicConfigurations);
  READARRAY(Families);

#undef READARRAY
#define READARRAY(numComps, name) \
  LoadDataArray<numComps>(this->name.Get(), _vtkBlueObeliskData::name, arraySize)

  READARRAY(1, Masses);
  READARRAY(1, ExactMasses);
  READARRAY(1, IonizationEnergies);
  READARRAY(1, ElectronAffinities);
  READARRAY(1, PaulingElectronegativities);
  READARRAY(1, CovalentRadii);
  READARRAY(1, VDWRadii);
  READARRAY(3, DefaultColors);
  READARRAY(1, BoilingPoints);
  READARRAY(1, MeltingPoints);
  READARRAY(1, Periods);
  READARRAY(1, Groups);

#undef READARRAY

  this->Initialized = true;
}

// Helpers for GenerateHeaderFromXML:
namespace {

void WriteStringArray(const std::string &name, vtkStringArray *data,
                      std::ostream &out)
{
  out << "static const char *" << name
      << "[" << data->GetNumberOfTuples() << "] = {\n";

  assert("Single component string array: " &&
         data->GetNumberOfComponents() == 1);
  vtkIdType numTuples = data->GetNumberOfTuples();
  for (vtkIdType i = 0; i < numTuples; ++i)
  {
    out << "  \"" << data->GetValue(i) << "\"";
    if (i < numTuples - 1)
    {
      out << ",";
    }
    out << "\n";
  }

  out << "};\n\n";
}

template <typename T>
struct TypeTraits
{
  static const char* Suffix() { return ""; }
  static void PrepStream(std::ostream &out)
  {
    out.unsetf(std::ostream::floatfield);
  }
};

template <>
struct TypeTraits<float>
{
  // Float literals need the 'f' suffix:
  static const char* Suffix() { return "f"; }
  // Need to make sure that we have a decimal point when using 'f' suffix:
  static void PrepStream(std::ostream &out) { out << std::scientific; }
};

template <typename ArrayT>
void WriteDataArray(const std::string &name, ArrayT *data, std::ostream &out)
{
  typedef typename ArrayT::ValueType ValueType;
  vtkIdType numTuples = data->GetNumberOfTuples();
  int numComps = data->GetNumberOfComponents();
  TypeTraits<ValueType>::PrepStream(out);
  out << "static const " << vtkTypeTraits<ValueType>::Name()
      << " " << name << "[" << numTuples << "][" << numComps << "] = {\n";

  for (vtkIdType t = 0; t < numTuples; ++t)
  {
    out << "  { ";
    for (int c = 0; c < numComps; ++c)
    {
      out << data->GetTypedComponent(t, c)
          << TypeTraits<ValueType>::Suffix();
      if (c < numComps - 1)
      {
        out << ",";
      }
      out << " ";
    }
    out << "}";
    if (t < numTuples - 1)
    {
      out << ",";
    }
    out << "\n";
  }

  out << "};\n\n";
}

} // end anon namespace

//----------------------------------------------------------------------------
bool vtkBlueObeliskData::GenerateHeaderFromXML(std::istream &xml,
                                               std::ostream &out)
{
  vtkNew<vtkBlueObeliskData> data;
  vtkNew<vtkBlueObeliskDataParser> parser;
  parser->SetStream(&xml);
  parser->SetTarget(data.Get());
  if (!parser->Parse())
  {
    return false;
  }

  out << "// Autogenerated by vtkBlueObeliskData::GenerateHeaderFromXML.\n"
         "// Do not edit. Any modifications may be lost.\n"
         "\n"
         "namespace _vtkBlueObeliskData {\n"
         "\n"
         "const static unsigned int numberOfElements = "
      << data->GetNumberOfElements() << ";\n\n";

#define DUMPARRAY(type, name) \
  Write ## type ## Array(#name, data->Get ## name(), out)

  DUMPARRAY(String, Symbols);
  DUMPARRAY(String, LowerSymbols);
  DUMPARRAY(String, Names);
  DUMPARRAY(String, LowerNames);
  DUMPARRAY(String, PeriodicTableBlocks);
  DUMPARRAY(String, ElectronicConfigurations);
  DUMPARRAY(String, Families);

  DUMPARRAY(Data, Masses);
  DUMPARRAY(Data, ExactMasses);
  DUMPARRAY(Data, IonizationEnergies);
  DUMPARRAY(Data, ElectronAffinities);
  DUMPARRAY(Data, PaulingElectronegativities);
  DUMPARRAY(Data, CovalentRadii);
  DUMPARRAY(Data, VDWRadii);
  DUMPARRAY(Data, DefaultColors);
  DUMPARRAY(Data, BoilingPoints);
  DUMPARRAY(Data, MeltingPoints);
  DUMPARRAY(Data, Periods);
  DUMPARRAY(Data, Groups);

#undef DUMPARRAY

  out << "} // end namespace _vtkBlueObeliskData\n";

  return true;
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
