/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkXMLReader.h"

#include "vtkCallbackCommand.h"
#include "vtkDataArraySelection.h"
#include "vtkDataCompressor.h"
#include "vtkDataSet.h"
#include "vtkDataSetAttributes.h"
#include "vtkInformation.h"
#include "vtkInformationDoubleKey.h"
#include "vtkInformationDoubleVectorKey.h"
#include "vtkInformationIdTypeKey.h"
#include "vtkInformationIntegerKey.h"
#include "vtkInformationIntegerVectorKey.h"
#include "vtkInformationKeyLookup.h"
#include "vtkInformationQuadratureSchemeDefinitionVectorKey.h"
#include "vtkInformationStringKey.h"
#include "vtkInformationStringVectorKey.h"
#include "vtkInformationUnsignedLongKey.h"
#include "vtkInformationVector.h"
#include "vtkInstantiator.h"
#include "vtkObjectFactory.h"
#include "vtkQuadratureSchemeDefinition.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkXMLDataElement.h"
#include "vtkXMLDataParser.h"
#include "vtkXMLFileReadTester.h"
#include "vtkXMLReaderVersion.h"
#include "vtkZLibDataCompressor.h"

#include <algorithm>
#include <cassert>
#include <functional>
#include <locale> // C++ locale
#include <sstream>
#include <vector>
#include <sys/stat.h>

vtkCxxSetObjectMacro(vtkXMLReader,ReaderErrorObserver,vtkCommand);
vtkCxxSetObjectMacro(vtkXMLReader,ParserErrorObserver,vtkCommand);

//-----------------------------------------------------------------------------
static void ReadStringVersion(const char* version, int& major, int& minor)
{
  if (!version)
  {
    major = -1;
    minor = -1;
    return;
  }
  // Extract the major and minor version numbers.
  size_t length = strlen(version);
  const char* begin = version;
  const char* end = version + length;
  const char* s;

  for (s = begin; (s != end) && (*s != '.'); ++s)
  {
    ;
  }

  if (s > begin)
  {
    std::stringstream str;
    str.write(begin, s-begin);
    str >> major;
    if (!str)
    {
      major = 0;
    }
  }
  if (++s < end)
  {
    std::stringstream str;
    str.write(s, end-s);
    str >> minor;
    if (!str)
    {
      minor = 0;
    }
  }
}
//----------------------------------------------------------------------------
vtkXMLReader::vtkXMLReader()
{
  this->FileName = 0;
  this->Stream = 0;
  this->FileStream = 0;
  this->StringStream = 0;
  this->ReadFromInputString = 0;
  this->InputString = "";
  this->XMLParser = 0;
  this->ReaderErrorObserver = 0;
  this->ParserErrorObserver = 0;
  this->FieldDataElement = 0;
  this->PointDataArraySelection = vtkDataArraySelection::New();
  this->CellDataArraySelection = vtkDataArraySelection::New();
  this->InformationError = 0;
  this->DataError = 0;
  this->ReadError = 0;
  this->ProgressRange[0] = 0;
  this->ProgressRange[1] = 1;

  // Setup the selection callback to modify this object when an array
  // selection is changed.
  this->SelectionObserver = vtkCallbackCommand::New();
  this->SelectionObserver->SetCallback(&vtkXMLReader::SelectionModifiedCallback);
  this->SelectionObserver->SetClientData(this);
  this->PointDataArraySelection->AddObserver(
    vtkCommand::ModifiedEvent, this->SelectionObserver);
  this->CellDataArraySelection->AddObserver(
    vtkCommand::ModifiedEvent, this->SelectionObserver);
  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(1);

  // Lower dimensional cell data support.
  this->AxesEmpty[0] = 0;
  this->AxesEmpty[1] = 0;
  this->AxesEmpty[2] = 0;

  // Time support:
  this->TimeStep = 0; // By default the file does not have timestep
  this->TimeStepRange[0] = 0;
  this->TimeStepRange[1] = 0;
  this->NumberOfTimeSteps = 0;
  this->TimeSteps = 0;
  this->CurrentTimeStep = 0;
  this->TimeStepWasReadOnce = 0;

  this->FileMinorVersion = -1;
  this->FileMajorVersion = -1;

  this->CurrentOutput = 0;
}

//----------------------------------------------------------------------------
vtkXMLReader::~vtkXMLReader()
{
  this->SetFileName(0);
  if (this->XMLParser)
  {
    this->DestroyXMLParser();
  }
  this->CloseStream();
  this->CellDataArraySelection->RemoveObserver(this->SelectionObserver);
  this->PointDataArraySelection->RemoveObserver(this->SelectionObserver);
  this->SelectionObserver->Delete();
  this->CellDataArraySelection->Delete();
  this->PointDataArraySelection->Delete();
  if (this->ReaderErrorObserver)
  {
    this->ReaderErrorObserver->Delete();
  }
  if (this->ParserErrorObserver)
  {
    this->ParserErrorObserver->Delete();
  }
  delete[] this->TimeSteps;
}

//----------------------------------------------------------------------------
void vtkXMLReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "FileName: "
     << (this->FileName? this->FileName:"(none)") << "\n";
  os << indent << "CellDataArraySelection: " << this->CellDataArraySelection
     << "\n";
  os << indent << "PointDataArraySelection: " << this->PointDataArraySelection
     << "\n";
  if (this->Stream)
  {
    os << indent << "Stream: " << this->Stream << "\n";
  }
  else
  {
    os << indent << "Stream: (none)\n";
  }
  os << indent << "TimeStep:" << this->TimeStep << "\n";
  os << indent << "NumberOfTimeSteps:" << this->NumberOfTimeSteps << "\n";
  os << indent << "TimeStepRange:(" << this->TimeStepRange[0] << ","
                                    << this->TimeStepRange[1] << ")\n";
}

//----------------------------------------------------------------------------
vtkDataSet* vtkXMLReader::GetOutputAsDataSet()
{
  return this->GetOutputAsDataSet(0);
}

//----------------------------------------------------------------------------
vtkDataSet* vtkXMLReader::GetOutputAsDataSet(int index)
{
  return vtkDataSet::SafeDownCast(this->GetOutputDataObject(index));
}

//----------------------------------------------------------------------------
// Major version should be incremented when older readers can no longer
// read files written for this reader. Minor versions are for added
// functionality that can be safely ignored by older readers.
int vtkXMLReader::CanReadFileVersion(int major, int vtkNotUsed(minor))
{
  return (major > vtkXMLReaderMajorVersion) ? 0 : 1;
}

//----------------------------------------------------------------------------
int vtkXMLReader::OpenStream()
{
  if (this->ReadFromInputString)
  {
    return this->OpenVTKString();
  }
  else
  {
    return this->OpenVTKFile();
  }
}

//----------------------------------------------------------------------------
int vtkXMLReader::OpenVTKFile()
{
  if (this->FileStream)
  {
    vtkErrorMacro("File already open.");
    return 1;
  }

  if (!this->Stream && !this->FileName)
  {
    vtkErrorMacro("File name not specified");
    return 0;
  }

  if (this->Stream)
  {
    // Use user-provided stream.
    return 1;
  }

  // Need to open a file.  First make sure it exists.  This prevents
  // an empty file from being created on older compilers.
  struct stat fs;
  if (stat(this->FileName, &fs) != 0)
  {
    vtkErrorMacro("Error opening file " << this->FileName);
    return 0;
  }

#ifdef _WIN32
  this->FileStream = new ifstream(this->FileName, ios::binary | ios::in);
#else
  this->FileStream = new ifstream(this->FileName, ios::in);
#endif

  if (!this->FileStream || !(*this->FileStream))
  {
    vtkErrorMacro("Error opening file " << this->FileName);
    delete this->FileStream;
    this->FileStream = 0;
    return 0;
  }

  // Use the file stream.
  this->Stream = this->FileStream;

  return 1;
}

//----------------------------------------------------------------------------
int vtkXMLReader::OpenVTKString()
{
  if(this->StringStream)
  {
    vtkErrorMacro("string already open.");
    return 1;
  }

  if(!this->Stream && this->InputString.compare("") == 0)
  {
    vtkErrorMacro("Input string not specified");
    return 0;
  }

  if(this->Stream)
  {
    // Use user-provided stream.
    return 1;
  }

  // Open the string stream
  this->StringStream = new std::istringstream(this->InputString);
  if(!this->StringStream || !(*this->StringStream))
  {
    vtkErrorMacro("Error opening string stream");
    delete this->StringStream;
    this->StringStream = 0;
    return 0;
  }

  // Use the string stream.
  this->Stream = this->StringStream;

  return 1;
}

//----------------------------------------------------------------------------
void vtkXMLReader::CloseStream()
{
  if (this->Stream)
  {
    if (this->ReadFromInputString)
    {
      this->CloseVTKString();
    }
    else
    {
      this->CloseVTKFile();
    }
    this->Stream = 0;
  }
}

//----------------------------------------------------------------------------
void vtkXMLReader::CloseVTKFile()
{
  if (!this->Stream)
  {
    vtkErrorMacro("File not open.");
    return;
  }
  if (this->Stream == this->FileStream)
  {
    // We opened the file.  Close it.
    this->FileStream->close();
    delete this->FileStream;
    this->FileStream = 0;
  }
}

//----------------------------------------------------------------------------
void vtkXMLReader::CloseVTKString()
{
  if(!this->Stream)
  {
    vtkErrorMacro("String not open.");
    return;
  }
  if(this->Stream == this->StringStream)
  {
    // We opened the string.  Close it.
    delete this->StringStream;
    this->StringStream = 0;
  }
}

//----------------------------------------------------------------------------
void vtkXMLReader::CreateXMLParser()
{
  if (this->XMLParser)
  {
    vtkErrorMacro("CreateXMLParser() called with existing XMLParser.");
    this->DestroyXMLParser();
  }
  this->XMLParser = vtkXMLDataParser::New();
}

//----------------------------------------------------------------------------
void vtkXMLReader::DestroyXMLParser()
{
  if (!this->XMLParser)
  {
    vtkErrorMacro("DestroyXMLParser() called with no current XMLParser.");
    return;
  }
  this->XMLParser->Delete();
  this->XMLParser = 0;
}

//----------------------------------------------------------------------------
void vtkXMLReader::SetupCompressor(const char* type)
{
  // Instantiate a compressor of the given type.
  if (!type)
  {
    vtkErrorMacro("Compressor has no type.");
    return;
  }
  vtkObject* object = vtkInstantiator::CreateInstance(type);
  vtkDataCompressor* compressor = vtkDataCompressor::SafeDownCast(object);

  // In static builds, the vtkZLibDataCompressor may not have been
  // registered with the vtkInstantiator.  Check for it here.
  if (!compressor && (strcmp(type, "vtkZLibDataCompressor") == 0))
  {
    compressor = vtkZLibDataCompressor::New();
  }

  if (!compressor)
  {
    vtkErrorMacro("Error creating " << type);
    if (object)
    {
      object->Delete();
    }
    return;
  }
  this->XMLParser->SetCompressor(compressor);
  compressor->Delete();
}



//----------------------------------------------------------------------------
int vtkXMLReader::ReadXMLInformation()
{
  // only Parse if something has changed
  if (this->GetMTime() > this->ReadMTime)
  {
    // Destroy any old information that was parsed.
    if (this->XMLParser)
    {
      this->DestroyXMLParser();
    }

    // Open the input file.  If it fails, the error was already
    // reported by OpenStream.
    if(!this->OpenStream())
    {
      return 0;
    }

    // Create the vtkXMLParser instance used to parse the file.
    this->CreateXMLParser();

    // Configure the parser for this file.
    this->XMLParser->SetStream(this->Stream);

    // Parse the input file.
    if (this->XMLParser->Parse())
    {
      // Let the subclasses read the information they want.
      if (!this->ReadVTKFile(this->XMLParser->GetRootElement()))
      {
        // There was an error reading the file.
        this->ReadError = 1;
      }
      else
      {
        this->ReadError = 0;
      }
    }
    else
    {
      vtkErrorMacro("Error parsing input file.  ReadXMLInformation aborting.");
      // The output should be empty to prevent the rest of the pipeline
      // from executing.
      this->ReadError = 1;
    }

    // Close the input stream to prevent resource leaks.
    this->CloseStream();

    this->ReadMTime.Modified();
  }
  return !this->ReadError;
}

//----------------------------------------------------------------------------
int vtkXMLReader::RequestInformation(vtkInformation *request,
  vtkInformationVector **vtkNotUsed(inputVector),
  vtkInformationVector *outputVector)
{
  if (this->ReadXMLInformation())
  {
    this->InformationError = 0;
    // Let the subclasses read the information they want.
    int outputPort =
      request->Get( vtkDemandDrivenPipeline::FROM_OUTPUT_PORT() );
    outputPort = outputPort >= 0 ? outputPort : 0;
    this->SetupOutputInformation
      (outputVector->GetInformationObject(outputPort) );

    // this->NumberOfTimeSteps has been set during the
    // this->ReadXMLInformation()
    int numTimesteps = this->GetNumberOfTimeSteps();
    this->TimeStepRange[0] = 0;
    this->TimeStepRange[1] = (numTimesteps > 0 ? numTimesteps-1 : 0);
    if (numTimesteps != 0)
    {
      std::vector<double> timeSteps(numTimesteps);
      for (int i = 0; i < numTimesteps; i++)
      {
        timeSteps[i] = i;
      }
      vtkInformation* outInfo = outputVector->GetInformationObject(0);
      outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_STEPS(),
                   &timeSteps[0],
                   numTimesteps);
      double timeRange[2];
      timeRange[0] = timeSteps[0];
      timeRange[1] = timeSteps[numTimesteps-1];
      outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_RANGE(),
                   timeRange, 2);
    }
  }
  else
  {
    this->InformationError = 1;
  }

  return !this->InformationError;
}

//----------------------------------------------------------------------------
int vtkXMLReader::RequestData(vtkInformation *vtkNotUsed(request),
                              vtkInformationVector **vtkNotUsed(inputVector),
                              vtkInformationVector *outputVector)
{
  this->CurrentTimeStep = this->TimeStep;

  // Get the output pipeline information and data object.
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  vtkDataObject* output = outInfo->Get(vtkDataObject::DATA_OBJECT());
  this->CurrentOutput = output;

  // Save the time value in the output data information.
  double* steps =
    outInfo->Get(vtkStreamingDemandDrivenPipeline::TIME_STEPS());

  // Check if a particular time was requested.
  if (steps &&
     outInfo->Has(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP()))
  {
    // Get the requested time step. We only supprt requests of a single time
    // step in this reader right now
    double requestedTimeStep =
      outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP());

    int length =
      outInfo->Length(vtkStreamingDemandDrivenPipeline::TIME_STEPS());

    // find the first time value larger than requested time value
    // this logic could be improved
    int cnt = 0;
    while (cnt < length-1 && steps[cnt] < requestedTimeStep)
    {
      cnt++;
    }
    this->CurrentTimeStep = cnt;

    // Clamp the requested time step to be in bounds.
    if (this->CurrentTimeStep < this->TimeStepRange[0])
    {
      this->CurrentTimeStep = this->TimeStepRange[0];
    }
    else if (this->CurrentTimeStep > this->TimeStepRange[1])
    {
      this->CurrentTimeStep = this->TimeStepRange[1];
    }

    output->GetInformation()->Set(
      vtkDataObject::DATA_TIME_STEP(), steps[this->CurrentTimeStep]);
  }

  // Re-open the input file.  If it fails, the error was already
  // reported by OpenStream.
  if(!this->OpenStream())
  {
    this->SetupEmptyOutput();
    this->CurrentOutput = 0;
    return 0;
  }
  if (!this->XMLParser)
  {
    vtkErrorMacro("ExecuteData called with no current XMLParser.");
  }

  // Give the vtkXMLParser instance its file back so that data section
  // reads will work.
  (*this->Stream).imbue(std::locale::classic());
  this->XMLParser->SetStream(this->Stream);

  // We are just starting to read.  Do not call UpdateProgressDiscrete
  // because we want a 0 progress callback the first time.
  this->UpdateProgress(0.);

  // Initialize progress range to entire 0..1 range.
  float wholeProgressRange[2] = { 0.f, 1.f };
  this->SetProgressRange(wholeProgressRange, 0, 1);

  if (!this->InformationError)
  {
    // We are just starting to execute.  No errors have yet occurred.
    this->XMLParser->SetAbort(0);
    this->DataError = 0;

    // Let the subclasses read the data they want.
    this->ReadXMLData();

    // If we aborted or there was an error, provide empty output.
    if (this->DataError || this->AbortExecute)
    {
      this->SetupEmptyOutput();
    }
  }
  else
  {
    // There was an error reading the file.  Provide empty output.
    this->SetupEmptyOutput();
  }

  // We have finished reading.
  this->UpdateProgressDiscrete(1);

  // Close the input stream to prevent resource leaks.
  this->CloseStream();
  if( this->TimeSteps )
  {
    // The SetupOutput should not reallocate this should be done only in a TimeStep case
    this->TimeStepWasReadOnce = 1;
  }

  this->SqueezeOutputArrays(output);

  this->CurrentOutput = 0;
  return 1;
}


//----------------------------------------------------------------------------
void vtkXMLReader::ReadXMLData()
{
  // Initialize the output's data.
  if (!this->TimeStepWasReadOnce)
  {
    this->SetupOutputData();
  }
}

//----------------------------------------------------------------------------
int vtkXMLReader::ReadVTKFile(vtkXMLDataElement* eVTKFile)
{
  // Check if the file version is one we support.
  const char* version = eVTKFile->GetAttribute("version");
  if (version && !this->CanReadFileVersionString(version))
  {
    vtkWarningMacro("File version: " << version << " is higher than "
                    "this reader supports " << vtkXMLReaderMajorVersion << "."
                    << vtkXMLReaderMinorVersion);
  }

  ::ReadStringVersion(version, this->FileMajorVersion, this->FileMinorVersion);

  // Setup the compressor if there is one.
  const char* compressor = eVTKFile->GetAttribute("compressor");
  if (compressor)
  {
    this->SetupCompressor(compressor);
  }

  // Get the primary element.
  const char* name = this->GetDataSetName();
  vtkXMLDataElement* ePrimary = 0;
  for (int i = 0; i < eVTKFile->GetNumberOfNestedElements(); ++i)
  {
    vtkXMLDataElement* eNested = eVTKFile->GetNestedElement(i);
    if (strcmp(eNested->GetName(), name) == 0)
    {
      ePrimary = eNested;
      break;
    }
  }
  if (!ePrimary)
  {
    vtkErrorMacro("Cannot find " << name << " element in file.");
    return 0;
  }

  // Read the primary element.
  return this->ReadPrimaryElement(ePrimary);
}

//----------------------------------------------------------------------------
int vtkXMLReader::ReadPrimaryElement(vtkXMLDataElement* ePrimary)
{
  // We don't need any information from the primary element here.
  //
  //
  // Let check the "TimeValues" here
  const int tsMax = 4096;
  double timevalues[tsMax];
  int numTimeSteps =
    ePrimary->GetVectorAttribute("TimeValues", tsMax, timevalues);
  assert( numTimeSteps <= tsMax);
  this->SetNumberOfTimeSteps( numTimeSteps );

  // See if there is a FieldData element
  int numNested = ePrimary->GetNumberOfNestedElements();
  for (int i = 0; i < numNested; ++i)
  {
    vtkXMLDataElement* eNested = ePrimary->GetNestedElement(i);
    if (strcmp(eNested->GetName(), "FieldData") == 0)
    {
      this->FieldDataElement = eNested;
      return 1;
    }
  }

  this->FieldDataElement = 0;

  return 1;
}

//----------------------------------------------------------------------------
void vtkXMLReader::SetupOutputData()
{
  // Initialize the output.
  this->CurrentOutput->Initialize();
}

//----------------------------------------------------------------------------
// Methods used for deserializing vtkInformation. ----------------------------
namespace {

void ltrim(std::string &s)
{
  s.erase(s.begin(),
          std::find_if(s.begin(),
                       s.end(),
                       std::not1(std::ptr_fun<int, int>(isspace))));
}

void rtrim(std::string &s)
{
  s.erase(std::find_if(s.rbegin(),
                       s.rend(),
                       std::not1(std::ptr_fun<int, int>(isspace))).base(),
          s.end());
}

void trim(std::string &s)
{
  ltrim(s);
  rtrim(s);
}

// Handle type extraction where needed, but trim and pass-through strings.
template <class ValueType>
bool extractValue(const char *valueStr, ValueType &value)
{
  if (!valueStr)
  {
    return false;
  }

  std::istringstream str;
  str.str(valueStr);
  str >> value;
  return !str.fail();
}
bool extractValue(const char *valueStr, std::string &value)
{
  value = std::string(valueStr ? valueStr : "");
  trim(value); // vtkXMLDataElement adds newlines before/after character data.
  return true;
}

template <class ValueType, class KeyType>
bool readScalarInfo(KeyType *key, vtkInformation *info,
                    vtkXMLDataElement *element)
{
  const char *valueStr = element->GetCharacterData();

  // backwards-compat: Old versions of the writer used to store data in
  // a 'value' attribute, but this causes problems with strings (e.g. the
  // XML parser removes newlines from attribute values).
  // Note that this is only for the non-vector information keys, as there were
  // no serialized vector keys in the old writer.
  // If there's no character data, check for a value attribute:
  if ((!valueStr || strlen(valueStr) == 0))
  {
    valueStr = element->GetAttribute("value");
  }

  ValueType value;
  if (!extractValue(valueStr, value))
  {
    return false;
  }
  info->Set(key, value);
  return true;
}

// Generic vector key reader. Stores in a temporary vector and calls Set to
// make sure that keys with RequiredLength work properly.
template <class ValueType, class KeyType>
bool readVectorInfo(KeyType *key, vtkInformation *info,
                    vtkXMLDataElement *element)
{
  const char *lengthData = element->GetAttribute("length");
  int length;
  if (!extractValue(lengthData, length))
  {
    return false;
  }

  if (length == 0)
  {
    info->Set(key, NULL, 0);
  }

  std::vector<ValueType> values;
  for (int i = 0; i < length; ++i)
  {
    std::ostringstream indexStr;
    indexStr << i;
    vtkXMLDataElement *valueElement =
        element->FindNestedElementWithNameAndAttribute("Value", "index",
                                                       indexStr.str().c_str());
    if (!valueElement)
    {
      return false;
    }

    const char *valueData = valueElement->GetCharacterData();
    ValueType value;
    if (!extractValue(valueData, value))
    {
      return false;
    }
    values.push_back(value);
  }
  info->Set(key, &values[0], length);

  return true;
}

// Overload for string vector keys. There is no API for 'set all at once',
// so we'll need to use Append (which can't work with RequiredLength vector
// keys, hence the need for a specialization).
template <typename ValueType>
bool readVectorInfo(vtkInformationStringVectorKey *key, vtkInformation *info,
                    vtkXMLDataElement *element)
{
  const char *lengthData = element->GetAttribute("length");
  int length;
  if (!extractValue(lengthData, length))
  {
    return false;
  }

  for (int i = 0; i < length; ++i)
  {
    std::ostringstream indexStr;
    indexStr << i;
    vtkXMLDataElement *valueElement =
        element->FindNestedElementWithNameAndAttribute("Value", "index",
                                                       indexStr.str().c_str());
    if (!valueElement)
    {
      return false;
    }

    const char *valueData = valueElement->GetCharacterData();
    ValueType value;
    if (!extractValue(valueData, value))
    {
      return false;
    }
    info->Append(key, value);
  }

  return true;
}

} // end anon namespace
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
int vtkXMLReader::CreateInformationKey(vtkXMLDataElement *element,
                                       vtkInformation *info)
{
  const char *name = element->GetAttribute("name");
  const char *location = element->GetAttribute("location");
  if (!name || !location)
  {
    vtkWarningMacro("InformationKey element missing name and/or location "
                    "attributes.");
    return 0;
  }

  vtkInformationKey *key = vtkInformationKeyLookup::Find(name, location);
  if (!key)
  {
    vtkWarningMacro("Could not locate key " << location << "::" << name
                    << ". Is the module in which it is defined linked?");
    return 0;
  }

  vtkInformationDoubleKey *dKey = NULL;
  vtkInformationDoubleVectorKey *dvKey = NULL;
  vtkInformationIdTypeKey *idKey = NULL;
  vtkInformationIntegerKey *iKey = NULL;
  vtkInformationIntegerVectorKey *ivKey = NULL;
  vtkInformationStringKey *sKey = NULL;
  vtkInformationStringVectorKey *svKey = NULL;
  vtkInformationUnsignedLongKey *ulKey = NULL;
  typedef vtkInformationQuadratureSchemeDefinitionVectorKey QuadDictKey;
  QuadDictKey *qdKey = NULL;
  if ((dKey = vtkInformationDoubleKey::SafeDownCast(key)))
  {
    if (!readScalarInfo<double>(dKey, info, element))
    {
      vtkErrorMacro("Error reading InformationKey element for " << location
                      << "::" << name << " of type " << key->GetClassName());
      info->Remove(key);
      return 0;
    }
  }
  else if ((dvKey = vtkInformationDoubleVectorKey::SafeDownCast(key)))
  {
    if (!readVectorInfo<double>(dvKey, info, element))
    {
      vtkErrorMacro("Error reading InformationKey element for " << location
                    << "::" << name << " of type " << key->GetClassName());
      info->Remove(key);
      return 0;
    }
  }
  else if ((idKey = vtkInformationIdTypeKey::SafeDownCast(key)))
  {
    if (!readScalarInfo<vtkIdType>(idKey, info, element))
    {
      vtkErrorMacro("Error reading InformationKey element for " << location
                    << "::" << name << " of type " << key->GetClassName());
      info->Remove(key);
      return 0;
    }
  }
  else if ((iKey = vtkInformationIntegerKey::SafeDownCast(key)))
  {
    if (!readScalarInfo<int>(iKey, info, element))
    {
      vtkErrorMacro("Error reading InformationKey element for " << location
                    << "::" << name << " of type " << key->GetClassName());
      info->Remove(key);
      return 0;
    }
  }
  else if ((ivKey = vtkInformationIntegerVectorKey::SafeDownCast(key)))
  {
    if (!readVectorInfo<int>(ivKey, info, element))
    {
      vtkErrorMacro("Error reading InformationKey element for " << location
                    << "::" << name << " of type " << key->GetClassName());
      info->Remove(key);
      return 0;
    }
  }
  else if ((sKey = vtkInformationStringKey::SafeDownCast(key)))
  {
    if (!readScalarInfo<std::string>(sKey, info, element))
    {
      vtkErrorMacro("Error reading InformationKey element for " << location
                    << "::" << name << " of type " << key->GetClassName());
      info->Remove(key);
      return 0;
    }
  }
  else if ((svKey = vtkInformationStringVectorKey::SafeDownCast(key)))
  {
    if (!readVectorInfo<std::string>(svKey, info, element))
    {
      vtkErrorMacro("Error reading InformationKey element for " << location
                    << "::" << name << " of type " << key->GetClassName());
      info->Remove(key);
      return 0;
    }
  }
  else if ((ulKey = vtkInformationUnsignedLongKey::SafeDownCast(key)))
  {
    if (!readScalarInfo<unsigned long>(ulKey, info, element))
    {
      vtkErrorMacro("Error reading InformationKey element for " << location
                    << "::" << name << " of type " << key->GetClassName());
      info->Remove(key);
      return 0;
    }
  }
  else if ((qdKey = QuadDictKey::SafeDownCast(key)))
  { // Special case:
    if (!qdKey->RestoreState(info, element))
    {
      vtkErrorMacro("Error reading InformationKey element for " << location
                    << "::" << name << " of type " << key->GetClassName());
      info->Remove(key);
      return 0;
    }
  }
  else
  {
    vtkErrorMacro("Could not deserialize information with key "
                  << key->GetLocation() << "::" << key->GetName() << ": "
                  "key type '" << key->GetClassName()
                  << "' is not serializable.");
    return 0;
  }

  return 1;
}

//----------------------------------------------------------------------------
bool vtkXMLReader::ReadInformation(vtkXMLDataElement *infoRoot,
                                   vtkInformation *info)
{
  int numChildren = infoRoot->GetNumberOfNestedElements();
  for (int child = 0; child < numChildren; ++child)
  {
    vtkXMLDataElement *element = infoRoot->GetNestedElement(child);
    if (strncmp("InformationKey", element->GetName(), 14) != 0)
    { // Not an element we care about.
      continue;
    }

    if (!this->CreateInformationKey(element, info))
    {
      return false;
    }
  }

  return true;
}

//----------------------------------------------------------------------------
vtkAbstractArray* vtkXMLReader::CreateArray(vtkXMLDataElement* da)
{
  int dataType = 0;
  if (!da->GetWordTypeAttribute("type", dataType))
  {
    return 0;
  }

  vtkAbstractArray* array = vtkAbstractArray::CreateArray(dataType);

  array->SetName(da->GetAttribute("Name"));


  //if NumberOfComponents fails, we have 1 component
  int components = 1;

  if (da->GetScalarAttribute("NumberOfComponents", components))
  {
    array->SetNumberOfComponents(components);
  }

  //determine what component names have been saved in the file.
  const char* compName = 0;
  std::ostringstream buff;
  for (int i = 0; i < components && i < 10; ++i)
  {
    //get the component names
    buff << "ComponentName" << i;
    compName = da->GetAttribute(buff.str().c_str());
    if (compName)
    {
      //detected a component name, add it
      array->SetComponentName(i, compName);
      compName = 0;
    }
    buff.str("");
    buff.clear();
  }


  // Scan/load for vtkInformationKey data.
  int nElements = da->GetNumberOfNestedElements();
  for (int i = 0; i<nElements; ++i)
  {
    vtkXMLDataElement *eInfoKeyData = da->GetNestedElement(i);
    if (strcmp(eInfoKeyData->GetName(), "InformationKey") == 0)
    {
      vtkInformation *info=array->GetInformation();
      this->CreateInformationKey(eInfoKeyData, info);
    }
  }

  return array;
}

//----------------------------------------------------------------------------
int vtkXMLReader::CanReadFile(const char* name)
{
  // First make sure the file exists.  This prevents an empty file
  // from being created on older compilers.
  struct stat fs;
  if (stat(name, &fs) != 0)
  {
    return 0;
  }

  // Test if the file with the given name is a VTKFile with the given
  // type.
  vtkXMLFileReadTester* tester = vtkXMLFileReadTester::New();
  tester->SetFileName(name);

  int result = 0;
  if (tester->TestReadFile() && tester->GetFileDataType())
  {
    if (this->CanReadFileWithDataType(tester->GetFileDataType()))
    {
      result = 1;
    }
  }

  tester->Delete();
  return result;
}

//----------------------------------------------------------------------------
int vtkXMLReader::CanReadFileWithDataType(const char* dsname)
{
  return (dsname && strcmp(dsname, this->GetDataSetName()) == 0) ? 1 : 0;
}

//----------------------------------------------------------------------------
int vtkXMLReader::CanReadFileVersionString(const char* version)
{
  int major = 0;
  int minor = 0;
  ::ReadStringVersion(version, major, minor);
  return this->CanReadFileVersion(major, minor);
}

//----------------------------------------------------------------------------
int vtkXMLReader::IntersectExtents(int* extent1, int* extent2, int* result)
{
  if ((extent1[0] > extent2[1]) || (extent1[2] > extent2[3]) ||
    (extent1[4] > extent2[5]) || (extent1[1] < extent2[0]) ||
    (extent1[3] < extent2[2]) || (extent1[5] < extent2[4]))
  {
    // No intersection of extents.
    return 0;
  }

  // Get the intersection of the extents.
  result[0] = this->Max(extent1[0], extent2[0]);
  result[1] = this->Min(extent1[1], extent2[1]);
  result[2] = this->Max(extent1[2], extent2[2]);
  result[3] = this->Min(extent1[3], extent2[3]);
  result[4] = this->Max(extent1[4], extent2[4]);
  result[5] = this->Min(extent1[5], extent2[5]);

  return 1;
}

//----------------------------------------------------------------------------
int vtkXMLReader::Min(int a, int b)
{
  return (a < b) ? a : b;
}

//----------------------------------------------------------------------------
int vtkXMLReader::Max(int a, int b)
{
  return (a > b) ? a : b;
}

//----------------------------------------------------------------------------
void vtkXMLReader::ComputePointDimensions(int* extent, int* dimensions)
{
  dimensions[0] = extent[1] - extent[0] + 1;
  dimensions[1] = extent[3] - extent[2] + 1;
  dimensions[2] = extent[5] - extent[4] + 1;
}

//----------------------------------------------------------------------------
void vtkXMLReader::ComputePointIncrements(int* extent, vtkIdType* increments)
{
  increments[0] = 1;
  increments[1] = increments[0] * (extent[1] - extent[0] + 1);
  increments[2] = increments[1] * (extent[3] - extent[2] + 1);
}

//----------------------------------------------------------------------------
void vtkXMLReader::ComputeCellDimensions(int* extent, int* dimensions)
{
  // For structured cells, axes that are empty of cells are treated as
  // having one cell when computing cell counts.  This allows cell
  // dimensions lower than 3.
  for (int a = 0; a < 3; ++a)
  {
    if (this->AxesEmpty[a] && extent[2*a+1] == extent[2*a])
    {
      dimensions[a] = 1;
    }
    else
    {
      dimensions[a] = extent[2*a+1] - extent[2*a];
    }
  }
}

//----------------------------------------------------------------------------
void vtkXMLReader::ComputeCellIncrements(int* extent, vtkIdType* increments)
{
  // For structured cells, axes that are empty of cells do not
  // contribute to the memory layout of cell data.
  vtkIdType nextIncrement = 1;
  for (int a = 0; a < 3; ++a)
  {
    if (this->AxesEmpty[a] && extent[2*a+1] == extent[2*a])
    {
      increments[a] = 0;
    }
    else
    {
      increments[a] = nextIncrement;
      nextIncrement *= extent[2*a+1] - extent[2*a];
    }
  }
}

//----------------------------------------------------------------------------
vtkIdType vtkXMLReader::GetStartTuple(
  int* extent, vtkIdType* increments, int i, int j, int k)
{
  vtkIdType offset = (i - extent[0]) * increments[0];
  offset += (j - extent[2]) * increments[1];
  offset += (k - extent[4]) * increments[2];
  return offset;
}

//----------------------------------------------------------------------------
void vtkXMLReader::ReadAttributeIndices(
  vtkXMLDataElement* eDSA, vtkDataSetAttributes* dsa)
{
  // Setup attribute indices.
  for (int i = 0;i < vtkDataSetAttributes::NUM_ATTRIBUTES;++i)
  {
    const char* attrName = vtkDataSetAttributes::GetAttributeTypeAsString(i);
    if (eDSA && eDSA->GetAttribute(attrName))
    {
      dsa->SetActiveAttribute(eDSA->GetAttribute(attrName), i);
    }
  }
}

//----------------------------------------------------------------------------
char** vtkXMLReader::CreateStringArray(int numStrings)
{
  char** strings = new char*[numStrings];
  for (int i = 0; i < numStrings; ++i)
  {
    strings[i] = 0;
  }
  return strings;
}

//----------------------------------------------------------------------------
void vtkXMLReader::DestroyStringArray(int numStrings, char** strings)
{
  for (int i = 0; i < numStrings; ++i)
  {
    delete[] strings[i];
  }
  delete[] strings;
}

//----------------------------------------------------------------------------
void vtkXMLReader::SetDataArraySelections(
  vtkXMLDataElement* eDSA, vtkDataArraySelection* sel)
{
  if (!eDSA)
  {
    sel->SetArrays(0, 0);
    return;
  }

  int numArrays = eDSA->GetNumberOfNestedElements();
  if (!numArrays)
  {
    sel->SetArrays(0, 0);
    return;
  }

  for (int i = 0; i < numArrays; ++i)
  {
    vtkXMLDataElement* eNested = eDSA->GetNestedElement(i);
    const char* name = eNested->GetAttribute("Name");
    if (name)
    {
      sel->AddArray( name );
    }
    else
    {
      std::ostringstream s;
      s << "Array " << i;
      sel->AddArray(s.str().c_str());
    }
  }
}

//----------------------------------------------------------------------------
int vtkXMLReader::SetFieldDataInfo(vtkXMLDataElement *eDSA,
  int association, int numTuples, vtkInformationVector *(&infoVector))
{
  if (!eDSA)
  {
    return 1;
  }

  char *(attributeName[vtkDataSetAttributes::NUM_ATTRIBUTES]);

  for (int i = 0; i < vtkDataSetAttributes::NUM_ATTRIBUTES; i++)
  {
    const char* attrName = vtkDataSetAttributes::GetAttributeTypeAsString(i);
    const char* name = eDSA->GetAttribute(attrName);
    if (name)
    {
      attributeName[i] = new char[strlen(name)+1];
      strcpy(attributeName[i], name);
    }
    else
    {
      attributeName[i] = 0;
    }
  }

  if (!infoVector)
  {
    infoVector = vtkInformationVector::New();
  }

  vtkInformation *info = 0;

  // Cycle through each data array
  for (int i = 0; i < eDSA->GetNumberOfNestedElements(); i++)
  {
    vtkXMLDataElement* eNested = eDSA->GetNestedElement(i);
    int components, dataType, activeFlag = 0;

    info = vtkInformation::New();
    info->Set(vtkDataObject::FIELD_ASSOCIATION(), association);
    info->Set(vtkDataObject::FIELD_NUMBER_OF_TUPLES(), numTuples);

    const char* name = eNested->GetAttribute("Name");
    if (!name)
    {
      this->InformationError = 1;
      break;
    }
    info->Set(vtkDataObject::FIELD_NAME(), name);

    // Search for matching attribute name
    for (int j = 0; j < vtkDataSetAttributes::NUM_ATTRIBUTES; j++)
    {
      if (attributeName[j] && !strcmp(name, attributeName[j]))
      {
        // set appropriate bit to indicate an active attribute type
        activeFlag |= 1 << j;
        break;
      }
    }

    if (!eNested->GetWordTypeAttribute("type", dataType))
    {
      this->InformationError = 1;
      break;
    }
    info->Set(vtkDataObject::FIELD_ARRAY_TYPE(), dataType);

    if (eNested->GetScalarAttribute("NumberOfComponents", components))
    {
      info->Set(vtkDataObject::FIELD_NUMBER_OF_COMPONENTS(), components);
    }
    else
    {
      info->Set(vtkDataObject::FIELD_NUMBER_OF_COMPONENTS(), 1);
    }

    double range[2];
    if (eNested->GetScalarAttribute("RangeMin", range[0]) &&
        eNested->GetScalarAttribute("RangeMax", range[1]))
    {
      info->Set(vtkDataObject::FIELD_RANGE(), range, 2);
    }

    info->Set(vtkDataObject::FIELD_ACTIVE_ATTRIBUTE(), activeFlag);
    infoVector->Append(info);
    info->Delete();
  }

  for (int i = 0; i < vtkDataSetAttributes::NUM_ATTRIBUTES; i++)
  {
    delete[] attributeName[i];
  }

  if (this->InformationError)
  {
    info->Delete();
    infoVector->Delete();
    infoVector = 0;
    return 0;
  }

  return 1;
}


//----------------------------------------------------------------------------
int vtkXMLReader::PointDataArrayIsEnabled(vtkXMLDataElement* ePDA)
{
  const char* name = ePDA->GetAttribute("Name");
  return (name && this->PointDataArraySelection->ArrayIsEnabled(name));
}

//----------------------------------------------------------------------------
int vtkXMLReader::CellDataArrayIsEnabled(vtkXMLDataElement* eCDA)
{
  const char* name = eCDA->GetAttribute("Name");
  return (name && this->CellDataArraySelection->ArrayIsEnabled(name));
}

//----------------------------------------------------------------------------
void vtkXMLReader::SelectionModifiedCallback(
  vtkObject*, unsigned long, void* clientdata, void*)
{
  static_cast<vtkXMLReader*>(clientdata)->Modified();
}

//----------------------------------------------------------------------------
int vtkXMLReader::GetNumberOfPointArrays()
{
  return this->PointDataArraySelection->GetNumberOfArrays();
}

//----------------------------------------------------------------------------
const char* vtkXMLReader::GetPointArrayName(int index)
{
  return this->PointDataArraySelection->GetArrayName(index);
}

//----------------------------------------------------------------------------
int vtkXMLReader::GetPointArrayStatus(const char* name)
{
  return this->PointDataArraySelection->ArrayIsEnabled(name);
}

//----------------------------------------------------------------------------
void vtkXMLReader::SetPointArrayStatus(const char* name, int status)
{
  if (status)
  {
    this->PointDataArraySelection->EnableArray(name);
  }
  else
  {
    this->PointDataArraySelection->DisableArray(name);
  }
}

//----------------------------------------------------------------------------
int vtkXMLReader::GetNumberOfCellArrays()
{
  return this->CellDataArraySelection->GetNumberOfArrays();
}

//----------------------------------------------------------------------------
const char* vtkXMLReader::GetCellArrayName(int index)
{
  return this->CellDataArraySelection->GetArrayName(index);
}

//----------------------------------------------------------------------------
int vtkXMLReader::GetCellArrayStatus(const char* name)
{
  return this->CellDataArraySelection->ArrayIsEnabled(name);
}

//----------------------------------------------------------------------------
void vtkXMLReader::SetCellArrayStatus(const char* name, int status)
{
  if (status)
  {
    this->CellDataArraySelection->EnableArray(name);
  }
  else
  {
    this->CellDataArraySelection->DisableArray(name);
  }
}

//----------------------------------------------------------------------------
void vtkXMLReader::GetProgressRange(float* range)
{
  range[0] = this->ProgressRange[0];
  range[1] = this->ProgressRange[1];
}

//----------------------------------------------------------------------------
void vtkXMLReader::SetProgressRange(const float range[2], int curStep, int numSteps)
{
  float stepSize = (range[1] - range[0]) / numSteps;
  this->ProgressRange[0] = range[0] + stepSize*curStep;
  this->ProgressRange[1] = range[0] + stepSize*(curStep + 1);
  this->UpdateProgressDiscrete(this->ProgressRange[0]);
}

//----------------------------------------------------------------------------
void vtkXMLReader::SetProgressRange(const float range[2], int curStep,
                                    const float* fractions)
{
  float width = range[1] - range[0];
  this->ProgressRange[0] = range[0] + fractions[curStep]*width;
  this->ProgressRange[1] = range[0] + fractions[curStep+1]*width;
  this->UpdateProgressDiscrete(this->ProgressRange[0]);
}

//----------------------------------------------------------------------------
void vtkXMLReader::UpdateProgressDiscrete(float progress)
{
  if (!this->AbortExecute)
  {
    // Round progress to nearest 100th.
    float rounded = static_cast<float>(int((progress * 100) + 0.5f)) / 100.f;
    if (this->GetProgress() != rounded)
    {
      this->UpdateProgress(rounded);
    }
  }
}

//----------------------------------------------------------------------------
int vtkXMLReader::ProcessRequest(vtkInformation* request,
 vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  this->CurrentOutputInformation = outputVector->GetInformationObject(0);
  // FIXME This piece of code should be rewritten to handle at the same
  // time Pieces and TimeSteps. The REQUEST_DATA_NOT_GENERATED should
  // ideally be changed during execution, so that allocation still
  // happen when needed but can be skipped in demand (when doing
  // timesteps)
  if (this->NumberOfTimeSteps &&
    request->Has(vtkDemandDrivenPipeline::REQUEST_DATA_NOT_GENERATED()))
  {
    vtkInformation* outInfo = outputVector->GetInformationObject(0);
    outInfo->Set(vtkDemandDrivenPipeline::DATA_NOT_GENERATED(), 1);
    this->CurrentOutputInformation = 0;
    return 1;
  }
  // END FIXME

  // generate the data
  if (request->Has(vtkDemandDrivenPipeline::REQUEST_DATA()))
  {
    int retVal = this->RequestData(request, inputVector, outputVector);
    this->CurrentOutputInformation = 0;
    return retVal;
  }

  // create the output
  if (request->Has(vtkDemandDrivenPipeline::REQUEST_DATA_OBJECT()))
  {
    int retVal = this->RequestDataObject(request, inputVector, outputVector);
    this->CurrentOutputInformation = 0;
    return retVal;
  }

  // execute information
  if (request->Has(vtkDemandDrivenPipeline::REQUEST_INFORMATION()))
  {
    int retVal = this->RequestInformation(request, inputVector, outputVector);
    this->CurrentOutputInformation = 0;
    return retVal;
  }

  int retVal =
    this->Superclass::ProcessRequest(request, inputVector, outputVector);
  this->CurrentOutputInformation = 0;
  return retVal;
}

//----------------------------------------------------------------------------
void vtkXMLReader::SetNumberOfTimeSteps(int num)
{
  if (num && (this->NumberOfTimeSteps != num))
  {
    this->NumberOfTimeSteps = num;
    delete[] this->TimeSteps;
    // Reallocate a buffer large enough
    this->TimeSteps = new int[num];
    this->Modified();
  }
}

//----------------------------------------------------------------------------
int vtkXMLReader::IsTimeStepInArray(int timestep, int* timesteps, int length)
{
  for (int i = 0; i < length; i++)
  {
    if (timesteps[i] == timestep)
    {
      return 1;
    }
  }
  return 0;
}

//----------------------------------------------------------------------------
vtkDataObject* vtkXMLReader::GetCurrentOutput()
{
  return this->CurrentOutput;
}

//----------------------------------------------------------------------------
vtkInformation* vtkXMLReader::GetCurrentOutputInformation()
{
  return this->CurrentOutputInformation;
}
