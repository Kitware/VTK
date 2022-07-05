/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkBlueObeliskDataParser.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

  =========================================================================*/

#include "vtkBlueObeliskDataParser.h"

#include "vtkAbstractArray.h"
#include "vtkBlueObeliskData.h"
#include "vtkFloatArray.h"
#include "vtkObjectFactory.h"
#include "vtkStringArray.h"
#include "vtkUnsignedShortArray.h"

#include <vtksys/SystemTools.hxx>

#include <locale>
#include <sstream>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <vector>

#ifdef MSC_VER
#define stat _stat
#endif

//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkBlueObeliskDataParser);

//------------------------------------------------------------------------------
vtkBlueObeliskDataParser::vtkBlueObeliskDataParser()
  : Target(nullptr)
  , IsProcessingAtom(false)
  , IsProcessingValue(false)
  , CurrentValueType(None)
  , CurrentSymbol(new std::string)
  , CurrentName(new std::string)
  , CurrentPeriodicTableBlock(new std::string)
  , CurrentElectronicConfiguration(new std::string)
  , CurrentFamily(new std::string)
{
}

//------------------------------------------------------------------------------
vtkBlueObeliskDataParser::~vtkBlueObeliskDataParser()
{
  this->SetTarget(nullptr);
  delete CurrentSymbol;
  delete CurrentName;
  delete CurrentPeriodicTableBlock;
  delete CurrentElectronicConfiguration;
  delete CurrentFamily;
}

//------------------------------------------------------------------------------
void vtkBlueObeliskDataParser::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
void vtkBlueObeliskDataParser::SetTarget(vtkBlueObeliskData* bodr)
{
  vtkSetObjectBodyMacro(Target, vtkBlueObeliskData, bodr);
}

//------------------------------------------------------------------------------
int vtkBlueObeliskDataParser::Parse()
{
  if (!this->Target)
  {
    vtkWarningMacro(<< "No target set. Aborting.");
    return 0;
  }

  // Setup BlueObeliskData arrays
  this->Target->Reset();
  this->Target->Allocate(119); // 118 elements + dummy (0)

  int ret = this->Superclass::Parse();

  this->Target->Squeeze();

  // Set number of elements to the length of the symbol array minus
  // one (index 0 is a dummy atom type)
  this->Target->NumberOfElements = this->Target->Symbols->GetNumberOfTuples() - 1;

  return ret;
}

//------------------------------------------------------------------------------
int vtkBlueObeliskDataParser::Parse(const char*)
{
  return this->Parse();
}

//------------------------------------------------------------------------------
int vtkBlueObeliskDataParser::Parse(const char*, unsigned int)
{
  return this->Parse();
}

//------------------------------------------------------------------------------
void vtkBlueObeliskDataParser::StartElement(const char* name, const char** attr)
{
  if (this->GetDebug())
  {
    std::string desc;
    desc += "Encountered BODR Element. Name: ";
    desc += name;
    desc += "\n\tAttributes: ";
    int attrIndex = 0;
    while (const char* cur = attr[attrIndex])
    {
      if (attrIndex > 0)
      {
        desc.push_back(' ');
      }
      desc += cur;
      ++attrIndex;
    }
    vtkDebugMacro(<< desc);
  }

  if (strcmp(name, "atom") == 0)
  {
    this->NewAtomStarted(attr);
  }
  else if (strcmp(name, "scalar") == 0 || strcmp(name, "label") == 0 || strcmp(name, "array") == 0)
  {
    this->NewValueStarted(attr);
  }
  else if (this->GetDebug())
  {
    vtkDebugMacro(<< "Unhandled BODR element: " << name);
  }
}

//------------------------------------------------------------------------------
void vtkBlueObeliskDataParser::EndElement(const char* name)
{
  if (strcmp(name, "atom") == 0)
  {
    this->NewAtomFinished();
  }
  else if (strcmp(name, "scalar") == 0 || strcmp(name, "label") == 0 || strcmp(name, "array") == 0)
  {
    this->NewValueFinished();
  }
}

//------------------------------------------------------------------------------
void vtkBlueObeliskDataParser::NewAtomStarted(const char**)
{
  this->CurrentAtomicNumber = -1;
  this->CurrentSymbol->clear();
  this->CurrentName->clear();
  this->CurrentPeriodicTableBlock->clear();
  this->CurrentElectronicConfiguration->clear();
  this->CurrentFamily->clear();
  this->CurrentMass = VTK_FLOAT_MAX;
  this->CurrentExactMass = VTK_FLOAT_MAX;
  this->CurrentIonizationEnergy = VTK_FLOAT_MAX;
  this->CurrentElectronAffinity = VTK_FLOAT_MAX;
  this->CurrentPaulingElectronegativity = VTK_FLOAT_MAX;
  this->CurrentCovalentRadius = VTK_FLOAT_MAX;
  this->CurrentVDWRadius = VTK_FLOAT_MAX;
  this->CurrentDefaultColor[0] = 0.0;
  this->CurrentDefaultColor[1] = 0.0;
  this->CurrentDefaultColor[2] = 0.0;
  this->CurrentBoilingPoint = VTK_FLOAT_MAX;
  this->CurrentMeltingPoint = VTK_FLOAT_MAX;
  this->CurrentPeriod = VTK_UNSIGNED_SHORT_MAX;
  this->CurrentGroup = VTK_UNSIGNED_SHORT_MAX;

  this->CurrentValueType = None;

  this->IsProcessingAtom = true;
}

//------------------------------------------------------------------------------
void vtkBlueObeliskDataParser::NewAtomFinished()
{
  if (this->CurrentAtomicNumber < 0)
  {
    vtkWarningMacro(<< "Skipping invalid atom...");
    this->IsProcessingAtom = false;
    return;
  }

  vtkDebugMacro(<< "Adding info for atomic number: " << this->CurrentAtomicNumber);

  vtkIdType index = static_cast<vtkIdType>(this->CurrentAtomicNumber);

  vtkBlueObeliskDataParser::ResizeAndSetValue(this->CurrentSymbol, this->Target->Symbols, index);
  // vtkBlueObeliskDataParser::ToLower will modify the input string, so this
  // must follow this->Symbol
  vtkBlueObeliskDataParser::ResizeAndSetValue(
    vtkBlueObeliskDataParser::ToLower(this->CurrentSymbol), this->Target->LowerSymbols, index);
  vtkBlueObeliskDataParser::ResizeAndSetValue(this->CurrentName, this->Target->Names, index);
  // vtkBlueObeliskDataParser::ToLower will modify the input string, so this
  // must follow this->Name
  vtkBlueObeliskDataParser::ResizeAndSetValue(
    vtkBlueObeliskDataParser::ToLower(this->CurrentName), this->Target->LowerNames, index);
  vtkBlueObeliskDataParser::ResizeAndSetValue(
    this->CurrentPeriodicTableBlock, this->Target->PeriodicTableBlocks, index);
  vtkBlueObeliskDataParser::ResizeAndSetValue(
    this->CurrentElectronicConfiguration, this->Target->ElectronicConfigurations, index);
  vtkBlueObeliskDataParser::ResizeAndSetValue(this->CurrentFamily, this->Target->Families, index);
  vtkBlueObeliskDataParser::ResizeAndSetValue(this->CurrentMass, this->Target->Masses, index);
  vtkBlueObeliskDataParser::ResizeAndSetValue(
    this->CurrentExactMass, this->Target->ExactMasses, index);
  vtkBlueObeliskDataParser::ResizeAndSetValue(
    this->CurrentIonizationEnergy, this->Target->IonizationEnergies, index);
  vtkBlueObeliskDataParser::ResizeAndSetValue(
    this->CurrentElectronAffinity, this->Target->ElectronAffinities, index);
  vtkBlueObeliskDataParser::ResizeAndSetValue(
    this->CurrentPaulingElectronegativity, this->Target->PaulingElectronegativities, index);
  vtkBlueObeliskDataParser::ResizeAndSetValue(
    this->CurrentCovalentRadius, this->Target->CovalentRadii, index);
  vtkBlueObeliskDataParser::ResizeAndSetValue(
    this->CurrentVDWRadius, this->Target->VDWRadii, index);
  // Tuple handled differently
  vtkBlueObeliskDataParser::ResizeArrayIfNeeded(this->Target->DefaultColors, index);
  this->Target->DefaultColors->SetTypedTuple(index, this->CurrentDefaultColor);
  vtkBlueObeliskDataParser::ResizeAndSetValue(
    this->CurrentBoilingPoint, this->Target->BoilingPoints, index);
  vtkBlueObeliskDataParser::ResizeAndSetValue(
    this->CurrentMeltingPoint, this->Target->MeltingPoints, index);
  vtkBlueObeliskDataParser::ResizeAndSetValue(this->CurrentPeriod, this->Target->Periods, index);
  vtkBlueObeliskDataParser::ResizeAndSetValue(this->CurrentGroup, this->Target->Groups, index);
  this->IsProcessingAtom = false;
}

//------------------------------------------------------------------------------
void vtkBlueObeliskDataParser::NewValueStarted(const char** attr)
{
  this->IsProcessingValue = true;
  unsigned int attrInd = 0;

  while (const char* cur = attr[attrInd])
  {
    if (strcmp(cur, "value") == 0)
    {
      this->SetCurrentValue(attr[++attrInd]);
    }
    else if (strcmp(cur, "bo:atomicNumber") == 0)
    {
      this->CurrentValueType = AtomicNumber;
    }
    else if (strcmp(cur, "bo:symbol") == 0)
    {
      this->CurrentValueType = Symbol;
    }
    else if (strcmp(cur, "bo:name") == 0)
    {
      this->CurrentValueType = Name;
    }
    else if (strcmp(cur, "bo:periodTableBlock") == 0)
    {
      this->CurrentValueType = PeriodicTableBlock;
    }
    else if (strcmp(cur, "bo:electronicConfiguration") == 0)
    {
      this->CurrentValueType = ElectronicConfiguration;
    }
    else if (strcmp(cur, "bo:family") == 0)
    {
      this->CurrentValueType = Family;
    }
    else if (strcmp(cur, "bo:mass") == 0)
    {
      this->CurrentValueType = Mass;
    }
    else if (strcmp(cur, "bo:exactMass") == 0)
    {
      this->CurrentValueType = ExactMass;
    }
    else if (strcmp(cur, "bo:ionization") == 0)
    {
      this->CurrentValueType = IonizationEnergy;
    }
    else if (strcmp(cur, "bo:electronAffinity") == 0)
    {
      this->CurrentValueType = ElectronAffinity;
    }
    else if (strcmp(cur, "bo:electronegativityPauling") == 0)
    {
      this->CurrentValueType = PaulingElectronegativity;
    }
    else if (strcmp(cur, "bo:radiusCovalent") == 0)
    {
      this->CurrentValueType = CovalentRadius;
    }
    else if (strcmp(cur, "bo:radiusVDW") == 0)
    {
      this->CurrentValueType = VDWRadius;
    }
    else if (strcmp(cur, "bo:elementColor") == 0)
    {
      this->CurrentValueType = DefaultColor;
    }
    else if (strcmp(cur, "bo:boilingpoint") == 0)
    {
      this->CurrentValueType = BoilingPoint;
    }
    else if (strcmp(cur, "bo:meltingpoint") == 0)
    {
      this->CurrentValueType = MeltingPoint;
    }
    else if (strcmp(cur, "bo:period") == 0)
    {
      this->CurrentValueType = Period;
    }
    else if (strcmp(cur, "bo:group") == 0)
    {
      this->CurrentValueType = Group;
    }
    ++attrInd;
  }
}

//------------------------------------------------------------------------------
void vtkBlueObeliskDataParser::NewValueFinished()
{
  this->CurrentValueType = None;
  this->IsProcessingValue = false;
  this->CharacterDataValueBuffer.clear();
}

//------------------------------------------------------------------------------
void vtkBlueObeliskDataParser::CharacterDataHandler(const char* data, int length)
{
  if (this->IsProcessingAtom && this->IsProcessingValue)
  {
    this->SetCurrentValue(data, length);
  }
}

//------------------------------------------------------------------------------
void vtkBlueObeliskDataParser::SetCurrentValue(const char* data, int length)
{
  this->CharacterDataValueBuffer += std::string(data, data + length);

  this->SetCurrentValue(this->CharacterDataValueBuffer.c_str());
}

//------------------------------------------------------------------------------
void vtkBlueObeliskDataParser::SetCurrentValue(const char* data)
{
  if (!data)
  {
    vtkWarningMacro(<< "Cannot parse `nullptr` for datatype " << this->CurrentValueType << ".");
    return;
  }

  vtkDebugMacro(<< "Parsing string '" << data << "' for datatype " << this->CurrentValueType
                << ".");
  switch (this->CurrentValueType)
  {
    case AtomicNumber:
      this->CurrentAtomicNumber = vtkBlueObeliskDataParser::parseInt(data);
      return;
    case Symbol:
      this->CurrentSymbol->assign(data);
      return;
    case Name:
      this->CurrentName->assign(data);
      return;
    case PeriodicTableBlock:
      this->CurrentPeriodicTableBlock->assign(data);
      return;
    case ElectronicConfiguration:
      this->CurrentElectronicConfiguration->assign(data);
      return;
    case Family:
      this->CurrentFamily->assign(data);
      return;
    case Mass:
      this->CurrentMass = vtkBlueObeliskDataParser::parseFloat(data);
      return;
    case ExactMass:
      this->CurrentExactMass = vtkBlueObeliskDataParser::parseFloat(data);
      return;
    case IonizationEnergy:
      this->CurrentIonizationEnergy = vtkBlueObeliskDataParser::parseFloat(data);
      return;
    case ElectronAffinity:
      this->CurrentElectronAffinity = vtkBlueObeliskDataParser::parseFloat(data);
      return;
    case PaulingElectronegativity:
      this->CurrentPaulingElectronegativity = vtkBlueObeliskDataParser::parseFloat(data);
      return;
    case CovalentRadius:
      this->CurrentCovalentRadius = vtkBlueObeliskDataParser::parseFloat(data);
      return;
    case VDWRadius:
      this->CurrentVDWRadius = vtkBlueObeliskDataParser::parseFloat(data);
      return;
    case DefaultColor:
      vtkBlueObeliskDataParser::parseFloat3(data, this->CurrentDefaultColor);
      return;
    case BoilingPoint:
      this->CurrentBoilingPoint = vtkBlueObeliskDataParser::parseFloat(data);
      return;
    case MeltingPoint:
      this->CurrentMeltingPoint = vtkBlueObeliskDataParser::parseFloat(data);
      return;
    case Period:
      this->CurrentPeriod = vtkBlueObeliskDataParser::parseUnsignedShort(data);
      return;
    case Group:
      this->CurrentGroup = vtkBlueObeliskDataParser::parseUnsignedShort(data);
      return;
    case None:
    default:
      vtkDebugMacro(<< "Called with no CurrentValueType. data: " << data);
  }
}

//------------------------------------------------------------------------------
void vtkBlueObeliskDataParser::ResizeArrayIfNeeded(vtkAbstractArray* arr, vtkIdType ind)
{
  if (ind >= arr->GetNumberOfTuples())
  {
    arr->SetNumberOfTuples(ind + 1);
  }
}

//------------------------------------------------------------------------------
void vtkBlueObeliskDataParser::ResizeAndSetValue(
  std::string* val, vtkStringArray* arr, vtkIdType ind)
{
  vtkBlueObeliskDataParser::ResizeArrayIfNeeded(arr, ind);
  arr->SetValue(ind, val->c_str());
}

//------------------------------------------------------------------------------
void vtkBlueObeliskDataParser::ResizeAndSetValue(float val, vtkFloatArray* arr, vtkIdType ind)
{
  vtkBlueObeliskDataParser::ResizeArrayIfNeeded(arr, ind);
  arr->SetValue(ind, val);
}

//------------------------------------------------------------------------------
void vtkBlueObeliskDataParser::ResizeAndSetValue(
  unsigned short val, vtkUnsignedShortArray* arr, vtkIdType ind)
{
  vtkBlueObeliskDataParser::ResizeArrayIfNeeded(arr, ind);
  arr->SetValue(ind, val);
}

//------------------------------------------------------------------------------
inline int vtkBlueObeliskDataParser::parseInt(const char* d)
{
  return atoi(d);
}

//------------------------------------------------------------------------------
inline float vtkBlueObeliskDataParser::parseFloat(const char* d)
{
  float value;
  std::stringstream stream(d);
  stream >> value;

  if (stream.fail())
  {
    return 0.f;
  }

  return value;
}

//------------------------------------------------------------------------------
inline void vtkBlueObeliskDataParser::parseFloat3(const char* str, float arr[3])
{
  unsigned short ind = 0;

  std::vector<std::string> tokens;
  vtksys::SystemTools::Split(str, tokens, ' ');

  for (auto&& tok : tokens)
  {
    arr[ind++] = std::stof(tok);
  }

  if (ind != 3)
  {
    arr[0] = arr[1] = arr[2] == VTK_FLOAT_MAX;
  }
}

//------------------------------------------------------------------------------
inline unsigned short vtkBlueObeliskDataParser::parseUnsignedShort(const char* d)
{
  return static_cast<unsigned short>(atoi(d));
}

//------------------------------------------------------------------------------
inline std::string* vtkBlueObeliskDataParser::ToLower(std::string* str)
{
  for (std::string::iterator it = str->begin(), it_end = str->end(); it != it_end; ++it)
  {
    *it = static_cast<char>(tolower(*it));
  }
  return str;
}
