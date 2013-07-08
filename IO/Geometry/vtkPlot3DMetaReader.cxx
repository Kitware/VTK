/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile: vtkPlot3DMetaReader.cxx,v $

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPlot3DMetaReader.h"

#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkObjectFactory.h"
#include "vtkMultiBlockPLOT3DReader.h"

#include <vtksys/SystemTools.hxx>

#include <map>
#include <vector>
#include <string>

#include "vtk_jsoncpp.h"

#define CALL_MEMBER_FN(object,ptrToMember)  ((object).*(ptrToMember))

vtkStandardNewMacro(vtkPlot3DMetaReader);

typedef void (vtkPlot3DMetaReader::*Plot3DFunction)(Json::Value* val);

struct Plot3DTimeStep
{
  double Time;
  std::string XYZFile;
  std::string QFile;
  std::string FunctionFile;
};

struct vtkPlot3DMetaReaderInternals
{
  std::map<std::string, Plot3DFunction> FunctionMap;
  std::vector<Plot3DTimeStep> TimeSteps;

  std::string ResolveFileName(std::string metaFileName,
                              std::string fileName)
    {
      if (vtksys::SystemTools::FileIsFullPath(fileName.c_str()))
        {
        return fileName;
        }
      else
        {
        std::string path = vtksys::SystemTools::GetFilenamePath(metaFileName);
        std::vector<std::string> components;
        components.push_back(path + "/");
        components.push_back(fileName);
        return vtksys::SystemTools::JoinPath(components);
        }
    }
};

//-----------------------------------------------------------------------------
vtkPlot3DMetaReader::vtkPlot3DMetaReader()
{
  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(1);

  this->Reader = vtkMultiBlockPLOT3DReader::New();
  this->Reader->AutoDetectFormatOn();

  this->FileName = 0;

  this->Internal = new vtkPlot3DMetaReaderInternals;

  this->Internal->FunctionMap["auto-detect-format"] =
    &vtkPlot3DMetaReader::SetAutoDetectFormat;
  this->Internal->FunctionMap["byte-order"] =
    &vtkPlot3DMetaReader::SetByteOrder;
  this->Internal->FunctionMap["precision"] =
    &vtkPlot3DMetaReader::SetPrecision;
  this->Internal->FunctionMap["multi-grid"] =
    &vtkPlot3DMetaReader::SetMultiGrid;
  this->Internal->FunctionMap["format"] =
    &vtkPlot3DMetaReader::SetFormat;
  this->Internal->FunctionMap["blanking"] =
    &vtkPlot3DMetaReader::SetBlanking;
  this->Internal->FunctionMap["language"] =
    &vtkPlot3DMetaReader::SetLanguage;
  this->Internal->FunctionMap["2D"] =
    &vtkPlot3DMetaReader::Set2D;
  this->Internal->FunctionMap["R"] =
    &vtkPlot3DMetaReader::SetR;
  this->Internal->FunctionMap["gamma"] =
    &vtkPlot3DMetaReader::SetGamma;
  this->Internal->FunctionMap["filenames"] =
    &vtkPlot3DMetaReader::SetFileNames;
  this->Internal->FunctionMap["functions"] =
    &vtkPlot3DMetaReader::AddFunctions;
}

//-----------------------------------------------------------------------------
vtkPlot3DMetaReader::~vtkPlot3DMetaReader()
{
  this->Reader->Delete();

  delete this->Internal;

  delete[] this->FileName;
}

//-----------------------------------------------------------------------------
void vtkPlot3DMetaReader::SetAutoDetectFormat(Json::Value* val)
{
  bool value = val->asBool();
  if (value)
    {
    this->Reader->AutoDetectFormatOn();
    }
  else
    {
    this->Reader->AutoDetectFormatOff();
    }
}

//-----------------------------------------------------------------------------
void vtkPlot3DMetaReader::SetByteOrder(Json::Value* val)
{
  std::string value = val->asString();
  if (value == "little")
    {
    this->Reader->SetByteOrderToLittleEndian();
    }
  else if (value == "big")
    {
    this->Reader->SetByteOrderToBigEndian();
    }
  else
    {
    vtkErrorMacro("Unrecognized byte order: " <<
                  value.c_str()<< ". Valid options are \"little\" and \"big\"."
                  " Setting to little endian");
    this->Reader->SetByteOrderToLittleEndian();
    }
}

//-----------------------------------------------------------------------------
void vtkPlot3DMetaReader::SetLanguage(Json::Value* val)
{
  std::string value = val->asString();
  if (value == "fortran")
    {
    this->Reader->HasByteCountOn();
    }
  else if (value == "C")
    {
    this->Reader->HasByteCountOff();
    }
  else
    {
    vtkErrorMacro("Unrecognized language: " <<
                  value.c_str()<< ". Valid options are \"fortran\" and \"C\"."
                  " Setting to little fortran");
    this->Reader->HasByteCountOn();
    }
}

//-----------------------------------------------------------------------------
void vtkPlot3DMetaReader::SetPrecision(Json::Value* val)
{
  int value = val->asInt();
  if (value == 32)
    {
    this->Reader->DoublePrecisionOff();
    }
  else if (value == 64)
    {
    this->Reader->DoublePrecisionOn();
    }
  else
    {
    vtkErrorMacro("Unrecognized precision: " <<
                  value << ". Valid options are 32 and 64 (bits)."
                  " Setting to 32 bits");
    this->Reader->DoublePrecisionOff();
    }
}

//-----------------------------------------------------------------------------
void vtkPlot3DMetaReader::SetMultiGrid(Json::Value* val)
{
  bool value = val->asBool();
  if (value)
    {
    this->Reader->MultiGridOn();
    }
  else
    {
    this->Reader->MultiGridOff();
    }
}

//-----------------------------------------------------------------------------
void vtkPlot3DMetaReader::SetFormat(Json::Value* val)
{
  std::string value = val->asString();
  if (value == "binary")
    {
    this->Reader->BinaryFileOn();
    }
  else if (value == "ascii")
    {
    this->Reader->BinaryFileOff();
    }
  else
    {
    vtkErrorMacro("Unrecognized file type: " <<
                  value.c_str()<< ". Valid options are \"binary\" and \"ascii\"."
                  " Setting to binary");
    this->Reader->BinaryFileOn();
    }
}

//-----------------------------------------------------------------------------
void vtkPlot3DMetaReader::SetBlanking(Json::Value* val)
{
  bool value = val->asBool();
  if (value)
    {
    this->Reader->IBlankingOn();
    }
  else
    {
    this->Reader->IBlankingOff();
    }
}

//-----------------------------------------------------------------------------
void vtkPlot3DMetaReader::Set2D(Json::Value* val)
{
  bool value = val->asBool();
  if (value)
    {
    this->Reader->TwoDimensionalGeometryOn();
    }
  else
    {
    this->Reader->TwoDimensionalGeometryOff();
    }
}

//-----------------------------------------------------------------------------
void vtkPlot3DMetaReader::SetR(Json::Value* val)
{
  double R = val->asDouble();
  this->Reader->SetR(R);
}

//-----------------------------------------------------------------------------
void vtkPlot3DMetaReader::SetGamma(Json::Value* val)
{
  double gamma = val->asDouble();
  this->Reader->SetGamma(gamma);
}

//-----------------------------------------------------------------------------
void vtkPlot3DMetaReader::AddFunctions(Json::Value* val)
{
  const Json::Value& functions = *val;
  for ( size_t index = 0; index < functions.size(); ++index )
    {
    this->Reader->AddFunction(functions[(int)index].asInt());
    }
}

//-----------------------------------------------------------------------------
void vtkPlot3DMetaReader::SetFileNames(Json::Value* val)
{
  const Json::Value& filenames = *val;
  for ( size_t index = 0; index < filenames.size(); ++index )
    {
    const Json::Value& astep = filenames[(int)index];
    bool doAdd = true;
    Plot3DTimeStep aTime;
    if (!astep.isMember("time"))
      {
      vtkErrorMacro("Missing time value in timestep " << index);
      doAdd = false;
      }
    else
      {
      aTime.Time = astep["time"].asDouble();
      }

    if (!astep.isMember("xyz"))
      {
      vtkErrorMacro("Missing xyz filename in timestep " << index);
      doAdd = false;
      }
    else
      {
      std::string xyzfile = astep["xyz"].asString();
      aTime.XYZFile = this->Internal->ResolveFileName(this->FileName,
                                                      xyzfile);
      }

    if (astep.isMember("q"))
      {
      std::string qfile = astep["q"].asString();
      aTime.QFile = this->Internal->ResolveFileName(this->FileName,
                                                    qfile);
      }

    if (astep.isMember("function"))
      {
      std::string functionfile = astep["function"].asString();
      aTime.FunctionFile = this->Internal->ResolveFileName(this->FileName,
                                                           functionfile);
      }

    if (doAdd)
      {
      this->Internal->TimeSteps.push_back(aTime);
      }
    }
}

//----------------------------------------------------------------------------
int vtkPlot3DMetaReader::RequestInformation(
  vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector),
  vtkInformationVector* outputVector)
{
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  this->Internal->TimeSteps.clear();

  this->Reader->RemoveAllFunctions();

  if (!this->FileName)
    {
    vtkErrorMacro("No file name was specified. Cannot execute.");
    return 0;
    }

  ifstream file(this->FileName);

  Json::Value root;
  Json::Reader reader;
  bool parsingSuccessful = reader.parse(file, root);
  if (!parsingSuccessful)
    {
    // report to the user the failure and their locations in the document.
    vtkErrorMacro("Failed to parse configuration\n"
                  << reader.getFormatedErrorMessages().c_str());
    return 0;
    }

  Json::Value::Members members = root.getMemberNames();
  Json::Value::Members::iterator memberIterator;
  for (memberIterator = members.begin();
       memberIterator != members.end();
       memberIterator++)
    {
    std::map<std::string, Plot3DFunction>::iterator iter =
      this->Internal->FunctionMap.find(*memberIterator);
    if (iter != this->Internal->FunctionMap.end())
      {
      Json::Value val = root[*memberIterator];
      Plot3DFunction func = iter->second;
      CALL_MEMBER_FN(*this, func)(&val);
      }
    else
      {
      vtkErrorMacro("Syntax error in file. Option \""
                    << memberIterator->c_str()
                    << "\" is not valid.");
      }
    }

  std::vector<Plot3DTimeStep>::iterator iter;
  std::vector<double> timeValues;
  for (iter  = this->Internal->TimeSteps.begin();
       iter != this->Internal->TimeSteps.end();
       iter++)
    {
    timeValues.push_back(iter->Time);
    }

  size_t numSteps = timeValues.size();
  if (numSteps > 0)
    {
    outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_STEPS(),
                 &timeValues[0],
                 (int)numSteps);

    double timeRange[2];
    timeRange[0] = timeValues[0];
    timeRange[1] = timeValues[numSteps - 1];
    outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_RANGE(),
                 timeRange,
                 2);
    }

  return 1;
}

//----------------------------------------------------------------------------
int vtkPlot3DMetaReader::RequestData(
  vtkInformation*,
  vtkInformationVector**,
  vtkInformationVector* outputVector)
{
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  vtkMultiBlockDataSet* output = vtkMultiBlockDataSet::GetData(
    outputVector->GetInformationObject(0));

  double timeValue = 0;
  if (outInfo->Has(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP()))
    {
    // Get the requested time step. We only supprt requests of a single time
    // step in this reader right now
    timeValue =
      outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP());
    }

  int tsLength =
    outInfo->Length(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
  double *steps =
    outInfo->Get(vtkStreamingDemandDrivenPipeline::TIME_STEPS());

  if (tsLength < 1)
    {
    vtkErrorMacro("No timesteps were found. Please specify at least one "
                  "filenames entry in the input file.");
    return 0;
    }


  // find the first time value larger than requested time value
  // this logic could be improved
  int cnt = 0;
  while (cnt < tsLength-1 && steps[cnt] < timeValue)
    {
    cnt++;
    }

  int updateTime = cnt;

  if (updateTime >= tsLength)
    {
    updateTime = tsLength - 1;
    }

  if (tsLength > updateTime)
    {
    this->Reader->SetXYZFileName(
      this->Internal->TimeSteps[updateTime].XYZFile.c_str());
    const char* qname = this->Internal->TimeSteps[updateTime].QFile.c_str();
    if (strlen(qname) > 0)
      {
      this->Reader->SetQFileName(qname);
      }
    else
      {
      this->Reader->SetQFileName(0);
      }
    const char* fname =
      this->Internal->TimeSteps[updateTime].FunctionFile.c_str();
    if (strlen(fname) > 0)
      {
      this->Reader->SetFunctionFileName(fname);
      }
    else
      {
      this->Reader->SetFunctionFileName(0);
      }
    this->Reader->Update();
    output->ShallowCopy(this->Reader->GetOutput());
    }
  else
    {
    vtkErrorMacro("Time step " << updateTime << " was not found.");
    return 0;
    }

  return 1;
}

//-----------------------------------------------------------------------------
void vtkPlot3DMetaReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
