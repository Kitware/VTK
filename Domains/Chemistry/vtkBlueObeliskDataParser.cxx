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
#include "vtkStdString.h"
#include "vtkStringArray.h"
#include "vtkUnsignedShortArray.h"

// Defines VTK_BODR_DATA_PATH
#include "vtkChemistryConfigure.h"

#include <locale>
#include <sstream>
#include <sys/types.h>
#include <sys/stat.h>

#ifdef MSC_VER
#define stat _stat
#endif

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkBlueObeliskDataParser);

//----------------------------------------------------------------------------
vtkBlueObeliskDataParser::vtkBlueObeliskDataParser()
  : vtkXMLParser(),
    Target(NULL),
    IsProcessingAtom(false),
    IsProcessingValue(false),
    CurrentValueType(None),
    CurrentSymbol(new vtkStdString),
    CurrentName(new vtkStdString),
    CurrentPeriodicTableBlock(new vtkStdString),
    CurrentElectronicConfiguration(new vtkStdString),
    CurrentFamily(new vtkStdString)
{
  // Find elements.xml. Check the share directory first, then the source dir
  // if running from the build directory before installing.
  struct stat buf;
  if (stat(VTK_BODR_DATA_PATH "/elements.xml", &buf) == 0)
    {
    this->SetFileName(VTK_BODR_DATA_PATH "/elements.xml");
    }
  else if (stat(VTK_BODR_DATA_PATH_BUILD "/elements.xml", &buf) == 0)
    {
    this->SetFileName(VTK_BODR_DATA_PATH_BUILD "/elements.xml");
    }
  else
    {
    vtkErrorMacro(<<"Cannot find elements.xml. Checked " VTK_BODR_DATA_PATH
                  " and " VTK_BODR_DATA_PATH_BUILD)
    }
}

//----------------------------------------------------------------------------
vtkBlueObeliskDataParser::~vtkBlueObeliskDataParser()
{
  this->SetTarget(NULL);
  delete CurrentSymbol;
  delete CurrentName;
  delete CurrentPeriodicTableBlock;
  delete CurrentElectronicConfiguration;
  delete CurrentFamily;
}

//----------------------------------------------------------------------------
void vtkBlueObeliskDataParser::SetTarget(vtkBlueObeliskData *bodr)
{
  vtkSetObjectBodyMacro(Target, vtkBlueObeliskData, bodr);
}

//----------------------------------------------------------------------------
int vtkBlueObeliskDataParser::Parse()
{
  if (!this->Target)
    {
    vtkWarningMacro(<<"No target set. Aborting.");
    return 0;
    }

  // Setup BlueObeliskData arrays
  this->Target->Reset();
  this->Target->Allocate(119); // 118 elements + dummy (0)

  int ret = this->Superclass::Parse();

  this->Target->Squeeze();

  // Set number of elements to the length of the symbol array minus
  // one (index 0 is a dummy atom type)
  this->Target->NumberOfElements =
    this->Target->Symbols->GetNumberOfTuples() - 1;

  return ret;
}

//----------------------------------------------------------------------------
int vtkBlueObeliskDataParser::Parse(const char *)
{
  return this->Parse();
}

//----------------------------------------------------------------------------
int vtkBlueObeliskDataParser::Parse(const char *,
                                    unsigned int)
{
  return this->Parse();
}

//----------------------------------------------------------------------------
void vtkBlueObeliskDataParser::StartElement(const char *name,
                                            const char **attr)
{
  if (this->GetDebug())
    {
    std::string desc;
    desc += "Encountered BODR Element. Name: ";
    desc += name;
    desc += "\n\tAttributes: ";
    int attrIndex = 0;
    while (const char * cur = attr[attrIndex])
      {
      if (attrIndex > 0)
        {
        desc.push_back(' ');
        }
      desc += cur;
      ++attrIndex;
      }
    vtkDebugMacro(<<desc);
    }

  if (strcmp(name, "atom") == 0)
    {
    this->NewAtomStarted(attr);
    }
  else if (strcmp(name, "scalar") == 0 ||
           strcmp(name, "label") == 0 ||
           strcmp(name, "array") == 0)
    {
    this->NewValueStarted(attr);
    }
  else if (this->GetDebug())
    {
    vtkDebugMacro(<<"Unhandled BODR element: " << name);
    }

  return;
}

//----------------------------------------------------------------------------
void vtkBlueObeliskDataParser::EndElement(const char *name)
{
  if (strcmp(name, "atom") == 0)
    {
    this->NewAtomFinished();
    }
  else if (strcmp(name, "scalar") == 0 ||
           strcmp(name, "label") == 0 ||
           strcmp(name, "array") == 0)
    {
    this->NewValueFinished();
    }
}

//----------------------------------------------------------------------------
void vtkBlueObeliskDataParser::NewAtomStarted(const char **)
{
  this->CurrentAtomicNumber = -1;
  this->CurrentSymbol->clear();
  this->CurrentName->clear();
  this->CurrentPeriodicTableBlock->clear();
  this->CurrentElectronicConfiguration->clear();
  this->CurrentFamily->clear();
  this->CurrentMass = VTK_FLOAT_MAX;
  this->CurrentExactMass =VTK_FLOAT_MAX;
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

//----------------------------------------------------------------------------
void vtkBlueObeliskDataParser::NewAtomFinished()
{
  if (this->CurrentAtomicNumber < 0)
    {
    vtkWarningMacro(<<"Skipping invalid atom...");
    this->IsProcessingAtom = false;
    return;
    }

  vtkDebugMacro(<<"Adding info for atomic number: " <<
                this->CurrentAtomicNumber);

  vtkIdType index = static_cast<vtkIdType>(this->CurrentAtomicNumber);

  this->ResizeAndSetValue(this->CurrentSymbol,
                          this->Target->Symbols.GetPointer(),
                          index);
  // this->ToLower will modify the input string, so this must follow
  // this->Symbol
  this->ResizeAndSetValue(this->ToLower(this->CurrentSymbol),
                          this->Target->LowerSymbols.GetPointer(),
                          index);
  this->ResizeAndSetValue(this->CurrentName,
                          this->Target->Names.GetPointer(),
                          index);
  // this->ToLower will modify the input string, so this must follow
  // this->Name
  this->ResizeAndSetValue(this->ToLower(this->CurrentName),
                          this->Target->LowerNames.GetPointer(),
                          index);
  this->ResizeAndSetValue(this->CurrentPeriodicTableBlock,
                          this->Target->PeriodicTableBlocks.GetPointer(),
                          index);
  this->ResizeAndSetValue(this->CurrentElectronicConfiguration,
                          this->Target->ElectronicConfigurations.GetPointer(),
                          index);
  this->ResizeAndSetValue(this->CurrentFamily,
                          this->Target->Families.GetPointer(),
                          index);
  this->ResizeAndSetValue(this->CurrentMass,
                          this->Target->Masses.GetPointer(),
                          index);
  this->ResizeAndSetValue(this->CurrentExactMass,
                          this->Target->ExactMasses.GetPointer(),
                          index);
  this->ResizeAndSetValue(this->CurrentIonizationEnergy,
                          this->Target->IonizationEnergies.GetPointer(),
                          index);
  this->ResizeAndSetValue(this->CurrentElectronAffinity,
                          this->Target->ElectronAffinities.GetPointer(),
                          index);
  this->ResizeAndSetValue
    (this->CurrentPaulingElectronegativity,
     this->Target->PaulingElectronegativities.GetPointer(), index);
  this->ResizeAndSetValue(this->CurrentCovalentRadius,
                          this->Target->CovalentRadii.GetPointer(),
                          index);
  this->ResizeAndSetValue(this->CurrentVDWRadius,
                          this->Target->VDWRadii.GetPointer(),
                          index);
  // Tuple handled differently
  this->ResizeArrayIfNeeded(this->Target->DefaultColors.GetPointer(),
                            index);
  this->Target->DefaultColors->SetTupleValue(index,
                                             this->CurrentDefaultColor);
  this->ResizeAndSetValue(this->CurrentBoilingPoint,
                          this->Target->BoilingPoints.GetPointer(),
                          index);
  this->ResizeAndSetValue(this->CurrentMeltingPoint,
                          this->Target->MeltingPoints.GetPointer(),
                          index);
  this->ResizeAndSetValue(this->CurrentPeriod,
                          this->Target->Periods.GetPointer(),
                          index);
  this->ResizeAndSetValue(this->CurrentGroup,
                          this->Target->Groups.GetPointer(),
                          index);
  this->IsProcessingAtom = false;
}

//----------------------------------------------------------------------------
void vtkBlueObeliskDataParser::NewValueStarted(const char **attr)
{
  this->IsProcessingValue = true;
  unsigned int attrInd = 0;

  while (const char * cur = attr[attrInd])
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

//----------------------------------------------------------------------------
void vtkBlueObeliskDataParser::NewValueFinished()
{
  this->IsProcessingValue = false;
  this->CharacterDataValueBuffer.clear();
}

//----------------------------------------------------------------------------
void vtkBlueObeliskDataParser::CharacterDataHandler(const char *data,
                                                    int length)
{
  if (this->IsProcessingAtom && this->IsProcessingValue)
    {
    this->SetCurrentValue(data, length);
    }
}

//----------------------------------------------------------------------------
void vtkBlueObeliskDataParser::SetCurrentValue(const char *data, int length)
{
  this->CharacterDataValueBuffer += std::string(data, data+length);

  this->SetCurrentValue(this->CharacterDataValueBuffer.c_str());
}

//----------------------------------------------------------------------------
void vtkBlueObeliskDataParser::SetCurrentValue(const char *data)
{
  vtkDebugMacro(<<"Parsing string '" << data << "' for datatype "
                << this->CurrentValueType << ".");
  switch (this->CurrentValueType)
    {
    case AtomicNumber:
      this->CurrentAtomicNumber = this->parseInt(data);
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
      this->CurrentMass = this->parseFloat(data);
      return;
    case ExactMass:
      this->CurrentExactMass = this->parseFloat(data);
      return;
    case IonizationEnergy:
      this->CurrentIonizationEnergy = this->parseFloat(data);
      return;
    case ElectronAffinity:
      this->CurrentElectronAffinity = this->parseFloat(data);
      return;
    case PaulingElectronegativity:
      this->CurrentPaulingElectronegativity = this->parseFloat(data);
      return;
    case CovalentRadius:
      this->CurrentCovalentRadius = this->parseFloat(data);
      return;
    case VDWRadius:
      this->CurrentVDWRadius = this->parseFloat(data);
      return;
    case DefaultColor:
      this->parseFloat3(data, this->CurrentDefaultColor);
      return;
    case BoilingPoint:
      this->CurrentBoilingPoint = this->parseFloat(data);
      return;
    case MeltingPoint:
      this->CurrentMeltingPoint = this->parseFloat(data);
      return;
    case Period:
      this->CurrentPeriod = this->parseUnsignedShort(data);
      return;
    case Group:
      this->CurrentGroup = this->parseUnsignedShort(data);
      return;
    case None:
    default:
      vtkDebugMacro(<<"Called with no CurrentValueType. data: "<<data);
    }
  return;
}

//----------------------------------------------------------------------------
void  vtkBlueObeliskDataParser::ResizeArrayIfNeeded(vtkAbstractArray *arr,
                                                    vtkIdType ind)
{
  if (ind >= arr->GetNumberOfTuples())
    {
    arr->SetNumberOfTuples(ind + 1);
    }
}

//----------------------------------------------------------------------------
void vtkBlueObeliskDataParser::ResizeAndSetValue(vtkStdString *val,
                                                 vtkStringArray *arr,
                                                 vtkIdType ind)
{
  vtkBlueObeliskDataParser::ResizeArrayIfNeeded(arr, ind);
  arr->SetValue(ind, val->c_str());
}

//----------------------------------------------------------------------------
void vtkBlueObeliskDataParser::ResizeAndSetValue(float val,
                                                 vtkFloatArray *arr,
                                                 vtkIdType ind)
{
  vtkBlueObeliskDataParser::ResizeArrayIfNeeded(arr, ind);
  arr->SetValue(ind, val);
}

//----------------------------------------------------------------------------
void vtkBlueObeliskDataParser::ResizeAndSetValue(unsigned short val,
                                                 vtkUnsignedShortArray *arr,
                                                 vtkIdType ind)
{
  vtkBlueObeliskDataParser::ResizeArrayIfNeeded(arr, ind);
  arr->SetValue(ind, val);
}

//----------------------------------------------------------------------------
inline int vtkBlueObeliskDataParser::parseInt(const char *d)
{
  return atoi(d);
}

//----------------------------------------------------------------------------
inline float vtkBlueObeliskDataParser::parseFloat(const char *d)
{
  float value;
  std::stringstream stream(d);
  stream >> value;

  if(stream.fail())
    {
    return 0.f;
    }

  return value;
}

//----------------------------------------------------------------------------
inline void vtkBlueObeliskDataParser::parseFloat3(const char *str,
                                                  float arr[3])
{
  unsigned short ind = 0;
  // Make copy of d for strtok.:
  char *strcopy = new char[strlen(str) + 1];
  strcpy(strcopy, str);

  char *curTok = strtok(strcopy, " ");

  while (curTok != NULL)
    {
    if (ind == 3)
      break;

    arr[ind++] = static_cast<float>(atof(curTok));
    curTok = strtok(NULL, " ");
    }

  if (ind != 3)
    {
    arr[0] = arr[1] = arr[2] == VTK_FLOAT_MAX;
    }

  delete [] strcopy;
}

//----------------------------------------------------------------------------
inline unsigned short
vtkBlueObeliskDataParser::parseUnsignedShort(const char *d)
{
  return static_cast<unsigned short>(atoi(d));
}

//----------------------------------------------------------------------------
inline vtkStdString * vtkBlueObeliskDataParser::ToLower(vtkStdString *str)
{
  for (vtkStdString::iterator it = str->begin(), it_end = str->end();
         it != it_end; ++it)
    {
    *it = static_cast<char>(tolower(*it));
    }
  return str;
}
