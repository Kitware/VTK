/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkADIOSWriter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include <cstring>

#include <algorithm>
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>

#include "vtkAbstractArray.h"
#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkDataArray.h"
#include "vtkDataObject.h"
#include "vtkDataSet.h"
#include "vtkDemandDrivenPipeline.h"
#include "vtkFieldData.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkLookupTable.h"
#include "vtkMPIController.h"
#include "vtkMPI.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkUnstructuredGrid.h"

#include "vtkADIOSUtilities.h"

#include "ADIOSDefs.h"
#include "ADIOSWriter.h"

#include "vtkADIOSWriter.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkADIOSWriter);

//----------------------------------------------------------------------------
vtkADIOSWriter::vtkADIOSWriter()
: FileName(NULL),
  TransportMethod(static_cast<int>(ADIOS::TransportMethod_POSIX)),
  TransportMethodArguments(NULL),
  Transform(static_cast<int>(ADIOS::Transform_NONE)),
  CurrentStep(-1), Controller(NULL),
  Writer(NULL),
  NumberOfPieces(-1), RequestPiece(-1), NumberOfGhostLevels(-1),
  WriteAllTimeSteps(false), TimeSteps(), CurrentTimeStepIndex(-1)
{
  std::memset(this->RequestExtent, 0, 6*sizeof(int));
  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(0);
  this->SetController(vtkMultiProcessController::GetGlobalController());
}

//----------------------------------------------------------------------------
vtkADIOSWriter::~vtkADIOSWriter()
{
  delete this->Writer;
  this->SetFileName(NULL);
  this->SetTransportMethodArguments(NULL);
  this->SetController(NULL);
}

//----------------------------------------------------------------------------
void vtkADIOSWriter::PrintSelf(std::ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "FileName: " << (this->FileName ? this->FileName : "(null)")
     << std::endl;
}

//----------------------------------------------------------------------------
const char* vtkADIOSWriter::GetDefaultFileExtension()
{
  static const char ext[] = "vta";
  return ext;
}

//----------------------------------------------------------------------------
void vtkADIOSWriter::SetController(vtkMultiProcessController *controller)
{
  vtkMPIController *mpiController = vtkMPIController::SafeDownCast(controller);
  if(controller && !mpiController)
  {
    vtkErrorMacro("ADIOS Writer can only be used with an MPI controller");
    return;
  }

  vtkSetObjectBodyMacro(Controller, vtkMultiProcessController, controller);

  if(mpiController)
  {
    vtkMPICommunicator *comm = static_cast<vtkMPICommunicator *>(
      this->Controller->GetCommunicator());
    ADIOS::Writer::SetCommunicator(*comm->GetMPIComm()->GetHandle());

    this->NumberOfPieces = this->Controller->GetNumberOfProcesses();
    this->RequestPiece = this->Controller->GetLocalProcessId();
  }
  else
  {
    this->NumberOfPieces = -1;
    this->RequestPiece = -1;
  }
}

//----------------------------------------------------------------------------
template<typename T>
bool vtkADIOSWriter::DefineAndWrite(vtkDataObject *input)
{
  const T *data = T::SafeDownCast(input);
  if(!data)
  {
    return false;
  }

  const int localProc = this->Controller->GetLocalProcessId();

  try
  {
    ++this->CurrentStep;

    // Make sure we're within time bounds
    if(this->CurrentTimeStepIndex >= static_cast<int>(this->TimeSteps.size()))
    {
      vtkErrorMacro(<< "All timesteps have been exhausted");
      return false;
    }

    // Things to do on the first step, before writing any data
    if(this->CurrentStep == 0)
    {
      // Before any data can be written, it's structure must be declared
      this->Define("", data);

      if(localProc == 0)
      {
        // Global time step is only used by Rank 0
        this->Writer->DefineScalar<double>("/TimeStamp");

        // Define all appropriate attributes
        this->Writer->DefineAttribute<int>("::NumberOfPieces",
          this->NumberOfPieces);
      }
    }

    if(localProc == 0)
    {
      if(this->CurrentTimeStepIndex >= 0)
      {
        this->Writer->WriteScalar<double>("/TimeStamp",
          this->TimeSteps[this->CurrentTimeStepIndex]);
      }
      else
      {
        this->Writer->WriteScalar<double>("/TimeStamp", this->CurrentStep);
      }
    }

    this->Write("", data);
    this->Writer->Commit(this->FileName, this->CurrentStep > 0);
  }
  catch(const ADIOS::WriteError &err)
  {
    vtkErrorMacro(<< err.what());
    return false;
  }
  return true;
}

//----------------------------------------------------------------------------
bool vtkADIOSWriter::WriteInternal(void)
{
  vtkDataObject *input = this->GetInputDataObject(0, 0);
  if(!input)
  {
    return false;
  }

  switch(input->GetDataObjectType())
  {
    case VTK_IMAGE_DATA:
      return this->DefineAndWrite<vtkImageData>(input);
    case VTK_POLY_DATA:
      return this->DefineAndWrite<vtkPolyData>(input);
    case VTK_UNSTRUCTURED_GRID:
      return this->DefineAndWrite<vtkUnstructuredGrid>(input);
    default:
      vtkErrorMacro("Input vtkDataObject type not supported by ADIOS writer");
      return false;
  }
}

//----------------------------------------------------------------------------
int vtkADIOSWriter::FillInputPortInformation(int port, vtkInformation *info)
{
  // Only 1 port
  if(port != 0)
  {
    return 0;
  }

  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkDataObject");
  return 1;
}

//----------------------------------------------------------------------------
int vtkADIOSWriter::ProcessRequest(vtkInformation* request,
  vtkInformationVector** input, vtkInformationVector* output)
{
  // Make sure the ADIOS subsystem is initialized before processing any
  // sort of request.
  if(!this->Writer)
  {
    this->Writer = new ADIOS::Writer(
      static_cast<ADIOS::TransportMethod>(this->TransportMethod),
      this->TransportMethodArguments ? this->TransportMethodArguments : "");
  }

  return this->Superclass::ProcessRequest(request, input, output);
}

//----------------------------------------------------------------------------
int vtkADIOSWriter::RequestInformation(vtkInformation *vtkNotUsed(req),
  vtkInformationVector **input, vtkInformationVector *vtkNotUsed(output))
{
  vtkInformation *inInfo = input[0]->GetInformationObject(0);

  this->TimeSteps.clear();
  this->CurrentTimeStepIndex = -1;
  if(inInfo->Has(vtkStreamingDemandDrivenPipeline::TIME_STEPS()))
  {
    int len = inInfo->Length(vtkStreamingDemandDrivenPipeline::TIME_STEPS());

    double *steps = inInfo->Get(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
    this->TimeSteps.reserve(len);
    this->TimeSteps.insert(this->TimeSteps.begin(), steps, steps+len);
    this->CurrentTimeStepIndex = 0;
  }

  return 1;
}

//----------------------------------------------------------------------------
int vtkADIOSWriter::RequestUpdateExtent(vtkInformation *vtkNotUsed(req),
  vtkInformationVector **input, vtkInformationVector *vtkNotUsed(output))
{
  vtkInformation* inInfo = input[0]->GetInformationObject(0);

  inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(),
    this->NumberOfPieces);
  inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(),
    this->RequestPiece);
  if(this->WriteAllTimeSteps && this->CurrentTimeStepIndex >= 0 &&
     this->CurrentTimeStepIndex < static_cast<int>(this->TimeSteps.size()))
  {
    inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP(),
      this->TimeSteps[this->CurrentTimeStepIndex]);
  }
  return 1;
}

//----------------------------------------------------------------------------
int vtkADIOSWriter::RequestData(vtkInformation *req,
  vtkInformationVector **vtkNotUsed(input),
  vtkInformationVector *vtkNotUsed(output))
{
  int numSteps = static_cast<int>(this->TimeSteps.size());

  // Make sure the time step is one we know about
  {
    vtkDataObject* inDataObject = this->GetInputDataObject(0, 0);
    vtkInformation *inDataInfo = inDataObject->GetInformation();
    if(inDataInfo->Has(vtkDataObject::DATA_TIME_STEP()))
    {
      double ts = inDataInfo->Get(vtkDataObject::DATA_TIME_STEP());
      std::vector<double>::iterator tsit =
        std::lower_bound(this->TimeSteps.begin(), this->TimeSteps.end(), ts);
      if(tsit != this->TimeSteps.end() && *tsit == ts)
      {
        this->CurrentTimeStepIndex =
          std::distance(this->TimeSteps.begin(), tsit);
      }
      else
      {
        vtkWarningMacro(<< "Unknown timestamp " << ts << " requested.");
        this->CurrentTimeStepIndex = -1;
      }
    }
  }

  // Continue looping if we're not at the end
  if(this->CurrentTimeStepIndex >= 0 && // index of -1 means no steps
    this->WriteAllTimeSteps && this->CurrentTimeStepIndex < numSteps)
  {
    req->Set(vtkStreamingDemandDrivenPipeline::CONTINUE_EXECUTING(), 1);
  }

  if(!this->WriteInternal())
  {
    return 0;
  }

  if(this->CurrentTimeStepIndex >= 0)
  {
    ++this->CurrentTimeStepIndex;

    // End looping if we're at the end
    if(this->WriteAllTimeSteps && this->CurrentTimeStepIndex >= numSteps)
    {
      req->Remove(vtkStreamingDemandDrivenPipeline::CONTINUE_EXECUTING());
    }
  }

  return 1;
}

//----------------------------------------------------------------------------

void vtkADIOSWriter::Define(const std::string& path, const vtkAbstractArray* v)
{
  vtkAbstractArray* valueTmp = const_cast<vtkAbstractArray*>(v);

  // String arrays not currently supported
  if(valueTmp->GetDataType() == VTK_STRING)
  {
    vtkWarningMacro(<< "Skipping string array " << path);
    return;
  }


  this->Writer->DefineScalar<size_t>(path+"#NC");
  this->Writer->DefineScalar<size_t>(path+"#NT");

  std::vector<ADIOS::ArrayDim> dims;
  dims.push_back(ADIOS::ArrayDim(path+"#NC"));
  dims.push_back(ADIOS::ArrayDim(path+"#NT"));

  this->Writer->DefineLocalArray(path,
    ADIOS::Type::VTKToADIOS(valueTmp->GetDataType()), dims,
    static_cast<ADIOS::Transform>(this->Transform));
}

//----------------------------------------------------------------------------
void vtkADIOSWriter::Define(const std::string& path, const vtkDataArray* v)
{
  vtkDataArray* valueTmp = const_cast<vtkDataArray*>(v);

  vtkLookupTable *lut = valueTmp->GetLookupTable();
  if(lut)
  {
/*
    this->Define(path+"/LookupTable", static_cast<vtkAbstractArray*>(lut->GetTable()));
    this->Define(path+"/Values", static_cast<vtkAbstractArray*>(valueTmp));
*/
  }
  else
  {
    this->Define(path, static_cast<vtkAbstractArray*>(valueTmp));
  }
}

//----------------------------------------------------------------------------
void vtkADIOSWriter::Define(const std::string& path, const vtkCellArray* v)
{
  this->Writer->DefineScalar<vtkIdType>(path+"/NumberOfCells");

  vtkCellArray *valueTmp = const_cast<vtkCellArray*>(v);
  this->Define(path+"/IndexArray", valueTmp->GetData());
}

//----------------------------------------------------------------------------
void vtkADIOSWriter::Define(const std::string& path, const vtkFieldData* v)
{
  vtkFieldData* valueTmp = const_cast<vtkFieldData*>(v);
  for(int i = 0; i < valueTmp->GetNumberOfArrays(); ++i)
  {
    vtkDataArray *da = valueTmp->GetArray(i);
    vtkAbstractArray *aa = da ? da : valueTmp->GetAbstractArray(i);

    std::string name = aa->GetName();
    if(name.empty()) // skip unnamed arrays
    {
      vtkWarningMacro(<< "Skipping unnamed array in " << path);
      continue;
    }
    this->Define(path+"/"+name, da ? da : aa);
  }
}

//----------------------------------------------------------------------------
void vtkADIOSWriter::Define(const std::string& path, const vtkDataSet* v)
{
  vtkDataSet* valueTmp = const_cast<vtkDataSet*>(v);

  this->Define(path+"/FieldData", valueTmp->GetFieldData());
  this->Define(path+"/CellData", valueTmp->GetCellData());
  this->Define(path+"/PointData", valueTmp->GetPointData());
}

//----------------------------------------------------------------------------
void vtkADIOSWriter::Define(const std::string& path, const vtkImageData* v)
{
  this->Define(path+"/DataSet", static_cast<const vtkDataSet*>(v));

  this->Writer->DefineScalar<vtkTypeUInt8>(path+"/DataObjectType");
  this->Writer->DefineScalar<double>(path+"/OriginX");
  this->Writer->DefineScalar<double>(path+"/OriginY");
  this->Writer->DefineScalar<double>(path+"/OriginZ");
  this->Writer->DefineScalar<double>(path+"/SpacingX");
  this->Writer->DefineScalar<double>(path+"/SpacingY");
  this->Writer->DefineScalar<double>(path+"/SpacingZ");
  this->Writer->DefineScalar<int>(path+"/ExtentXMin");
  this->Writer->DefineScalar<int>(path+"/ExtentXMax");
  this->Writer->DefineScalar<int>(path+"/ExtentYMin");
  this->Writer->DefineScalar<int>(path+"/ExtentYMax");
  this->Writer->DefineScalar<int>(path+"/ExtentZMin");
  this->Writer->DefineScalar<int>(path+"/ExtentZMax");
}

//----------------------------------------------------------------------------
void vtkADIOSWriter::Define(const std::string& path, const vtkPolyData* v)
{
  this->Define(path+"/DataSet", static_cast<const vtkDataSet*>(v));

  vtkPolyData *valueTmp = const_cast<vtkPolyData*>(v);
  this->Writer->DefineScalar<vtkTypeUInt8>(path+"/DataObjectType");

  vtkPoints *p;
  if((p = valueTmp->GetPoints()))
  {
    this->Define(path+"/Points", p->GetData());
  }

  this->Define(path+"/Verticies", valueTmp->GetVerts());
  this->Define(path+"/Lines", valueTmp->GetLines());
  this->Define(path+"/Polygons", valueTmp->GetPolys());
  this->Define(path+"/Strips", valueTmp->GetStrips());
}

//----------------------------------------------------------------------------
void vtkADIOSWriter::Define(const std::string& path,
  const vtkUnstructuredGrid* v)
{
  this->Define(path+"/DataSet", static_cast<const vtkDataSet*>(v));

  vtkUnstructuredGrid *valueTmp = const_cast<vtkUnstructuredGrid*>(v);
  this->Writer->DefineScalar<vtkTypeUInt8>(path+"/DataObjectType");

  vtkPoints *p;
  if((p = valueTmp->GetPoints()))
  {
    this->Define(path+"/Points", p->GetData());
  }

  vtkUnsignedCharArray *cta = valueTmp->GetCellTypesArray();
  vtkIdTypeArray *cla = valueTmp->GetCellLocationsArray();
  vtkCellArray *ca = valueTmp->GetCells();
  if(cta && cla && ca)
  {
    this->Define(path+"/CellTypes", cta);
    this->Define(path+"/CellLocations", cla);
    this->Define(path+"/Cells", ca);
  }
}

//----------------------------------------------------------------------------
bool vtkADIOSWriter::UpdateMTimeTable(const std::string& path,
  const vtkObject* value)
{
  vtkMTimeType &mtimeCurrent = this->LastUpdated[path];
  vtkMTimeType mtimeNew = const_cast<vtkObject*>(value)->GetMTime();
  vtkMTimeType mtimePrev = mtimeCurrent;

  mtimeCurrent = mtimeNew;
  return mtimeNew != mtimePrev;
}

//----------------------------------------------------------------------------
void vtkADIOSWriter::Write(const std::string& path, const vtkAbstractArray* v)
{
  if(!this->UpdateMTimeTable(path, v))
  {
    return;
  }

  vtkAbstractArray* valueTmp = const_cast<vtkAbstractArray*>(v);

  // String arrays not currently supported
  if(valueTmp->GetDataType() == VTK_STRING)
  {
    return;
  }

  size_t nc = valueTmp->GetNumberOfComponents();
  size_t nt = valueTmp->GetNumberOfTuples();

  this->Writer->WriteScalar<size_t>(path+"#NC", nc);
  this->Writer->WriteScalar<size_t>(path+"#NT", nt);
  this->Writer->WriteArray(path, valueTmp->GetVoidPointer(0));
}

//----------------------------------------------------------------------------
void vtkADIOSWriter::Write(const std::string& path, const vtkDataArray* v)
{
  vtkDataArray* valueTmp = const_cast<vtkDataArray*>(v);
  vtkLookupTable *lut = valueTmp->GetLookupTable();

  if(lut)
  {
    // Only check the mtime here if a LUT is present.  Otherwise it will be
    // handled appropriately by the abstract array writer
    if(!this->UpdateMTimeTable(path, v))
    {
      return;
    }

    this->Write(path+"/LookupTable", static_cast<vtkAbstractArray*>(lut->GetTable()));
    this->Write(path+"/Values", static_cast<vtkAbstractArray*>(valueTmp));
  }
  else
  {
    this->Write(path, static_cast<vtkAbstractArray*>(valueTmp));
  }
}

//----------------------------------------------------------------------------
void vtkADIOSWriter::Write(const std::string& path, const vtkCellArray* v)
{
  if(!this->UpdateMTimeTable(path, v))
  {
    return;
  }

  vtkCellArray* valueTmp = const_cast<vtkCellArray*>(v);

  this->Writer->WriteScalar<vtkIdType>(path+"/NumberOfCells",
    valueTmp->GetNumberOfCells());
  this->Write(path+"/IndexArray", valueTmp->GetData());
}

//----------------------------------------------------------------------------
void vtkADIOSWriter::Write(const std::string& path, const vtkFieldData* v)
{
  if(!this->UpdateMTimeTable(path, v))
  {
    return;
  }

  vtkFieldData* valueTmp = const_cast<vtkFieldData*>(v);
  for(int i = 0; i < valueTmp->GetNumberOfArrays(); ++i)
  {
    vtkDataArray *da = valueTmp->GetArray(i);
    vtkAbstractArray *aa = da ? da : valueTmp->GetAbstractArray(i);

    std::string name = aa->GetName();
    if(name.empty()) // skip unnamed arrays
    {
      continue;
    }
    this->Write(path+"/"+name, da ? da : aa);
  }
}

//----------------------------------------------------------------------------
void vtkADIOSWriter::Write(const std::string& path, const vtkDataSet* v)
{
  if(!this->UpdateMTimeTable(path, v))
  {
    return;
  }

  vtkDataSet* valueTmp = const_cast<vtkDataSet*>(v);

  this->Write(path+"/FieldData", valueTmp->GetFieldData());
  this->Write(path+"/CellData", valueTmp->GetCellData());
  this->Write(path+"/PointData", valueTmp->GetPointData());
}

//----------------------------------------------------------------------------
void vtkADIOSWriter::Write(const std::string& path, const vtkImageData* v)
{
  if(!this->UpdateMTimeTable(path, v))
  {
    return;
  }

  this->Write(path+"/DataSet", static_cast<const vtkDataSet*>(v));

  vtkImageData* valueTmp = const_cast<vtkImageData*>(v);
  this->Writer->WriteScalar<vtkTypeUInt8>(path+"/DataObjectType", VTK_IMAGE_DATA);

  double *origin = valueTmp->GetOrigin();
  this->Writer->WriteScalar<double>(path+"/OriginX", origin[0]);
  this->Writer->WriteScalar<double>(path+"/OriginY", origin[1]);
  this->Writer->WriteScalar<double>(path+"/OriginZ", origin[2]);

  double *spacing = valueTmp->GetSpacing();
  this->Writer->WriteScalar<double>(path+"/SpacingX", spacing[0]);
  this->Writer->WriteScalar<double>(path+"/SpacingY", spacing[1]);
  this->Writer->WriteScalar<double>(path+"/SpacingZ", spacing[2]);

  int *extent = valueTmp->GetExtent();
  this->Writer->WriteScalar<int>(path+"/ExtentXMin", extent[0]);
  this->Writer->WriteScalar<int>(path+"/ExtentXMax", extent[1]);
  this->Writer->WriteScalar<int>(path+"/ExtentYMin", extent[2]);
  this->Writer->WriteScalar<int>(path+"/ExtentYMax", extent[3]);
  this->Writer->WriteScalar<int>(path+"/ExtentZMin", extent[4]);
  this->Writer->WriteScalar<int>(path+"/ExtentZMax", extent[5]);
}

//----------------------------------------------------------------------------
void vtkADIOSWriter::Write(const std::string& path, const vtkPolyData* v)
{
  if(!this->UpdateMTimeTable(path, v))
  {
    return;
  }

  this->Write(path+"/DataSet", static_cast<const vtkDataSet*>(v));

  vtkPolyData* valueTmp = const_cast<vtkPolyData*>(v);
  this->Writer->WriteScalar<vtkTypeUInt8>(path+"/DataObjectType",
    VTK_POLY_DATA);

  vtkPoints *p;
  if((p = valueTmp->GetPoints()))
  {
    this->Write(path+"/Points", p->GetData());
  }

  this->Write(path+"/Verticies", valueTmp->GetVerts());
  this->Write(path+"/Lines", valueTmp->GetLines());
  this->Write(path+"/Polygons", valueTmp->GetPolys());
  this->Write(path+"/Strips", valueTmp->GetStrips());
}

//----------------------------------------------------------------------------
void vtkADIOSWriter::Write(const std::string& path,
  const vtkUnstructuredGrid* v)
{
  if(!this->UpdateMTimeTable(path, v))
  {
    return;
  }

  this->Write(path+"/DataSet", static_cast<const vtkDataSet*>(v));

  vtkUnstructuredGrid *valueTmp = const_cast<vtkUnstructuredGrid*>(v);
  this->Writer->WriteScalar<vtkTypeUInt8>(path+"/DataObjectType",
    VTK_UNSTRUCTURED_GRID);

  vtkPoints *p;
  if((p = valueTmp->GetPoints()))
  {
    this->Write(path+"/Points", p->GetData());
  }

  vtkUnsignedCharArray *cta = valueTmp->GetCellTypesArray();
  vtkIdTypeArray *cla = valueTmp->GetCellLocationsArray();
  vtkCellArray *ca = valueTmp->GetCells();
  if(cta && cla && ca)
  {
    this->Write(path+"/CellTypes", cta);
    this->Write(path+"/CellLocations", cla);
    this->Write(path+"/Cells", ca);
  }
}
