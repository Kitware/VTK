/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXdmfReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkXdmfReader.h"
#include "vtkXdmfReaderInternal.h"
#include "vtkXdmfHeavyData.h"

#include "vtkCharArray.h"
#include "vtkCompositeDataPipeline.h"
#include "vtkDataObjectTypes.h"
#include "vtkDataSet.h"
#include "vtkExtentTranslator.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkXMLParser.h"

//============================================================================
class vtkXdmfReaderTester : public vtkXMLParser
{
public:
  vtkTypeMacro(vtkXdmfReaderTester, vtkXMLParser);
  static vtkXdmfReaderTester* New();
  int TestReadFile()
  {
      this->Valid = 0;
      if(!this->FileName)
      {
        return 0;
      }

      ifstream inFile(this->FileName);
      if(!inFile)
      {
        return 0;
      }

      this->SetStream(&inFile);
      this->Done = 0;

      this->Parse();

      if(this->Done && this->Valid )
      {
        return 1;
      }
      return 0;
  }
  void StartElement(const char* name, const char**) VTK_OVERRIDE
  {
      this->Done = 1;
      if(strcmp(name, "Xdmf") == 0)
      {
        this->Valid = 1;
      }
  }

protected:
  vtkXdmfReaderTester()
  {
      this->Valid = 0;
      this->Done = 0;
  }

private:
  void ReportStrayAttribute(const char*, const char*, const char*) VTK_OVERRIDE {}
  void ReportMissingAttribute(const char*, const char*) VTK_OVERRIDE {}
  void ReportBadAttribute(const char*, const char*, const char*) VTK_OVERRIDE {}
  void ReportUnknownElement(const char*) VTK_OVERRIDE {}
  void ReportXmlParseError() VTK_OVERRIDE {}

  int ParsingComplete() VTK_OVERRIDE { return this->Done; }
  int Valid;
  int Done;
  vtkXdmfReaderTester(const vtkXdmfReaderTester&) VTK_DELETE_FUNCTION;
  void operator=(const vtkXdmfReaderTester&) VTK_DELETE_FUNCTION;
};
vtkStandardNewMacro(vtkXdmfReaderTester);

vtkStandardNewMacro(vtkXdmfReader);
//----------------------------------------------------------------------------
vtkXdmfReader::vtkXdmfReader()
{
  this->DomainName = 0;
  this->Stride[0] = this->Stride[1] = this->Stride[2] = 1;
  this->XdmfDocument = new vtkXdmfDocument();
  this->LastTimeIndex = 0;
  this->SILUpdateStamp = 0;

  this->PointArraysCache = new vtkXdmfArraySelection;
  this->CellArraysCache = new vtkXdmfArraySelection;
  this->GridsCache = new vtkXdmfArraySelection;
  this->SetsCache = new vtkXdmfArraySelection;
}

//----------------------------------------------------------------------------
vtkXdmfReader::~vtkXdmfReader()
{
  this->SetDomainName(0);
  delete this->XdmfDocument;
  this->XdmfDocument = 0;

  delete this->PointArraysCache;
  delete this->CellArraysCache;
  delete this->GridsCache;
  delete this->SetsCache;

  this->ClearDataSetCache();
}

//----------------------------------------------------------------------------
int vtkXdmfReader::CanReadFile(const char* filename)
{
  vtkXdmfReaderTester* tester = vtkXdmfReaderTester::New();
  tester->SetFileName(filename);
  int res = tester->TestReadFile();
  tester->Delete();
  return res;
}

//----------------------------------------------------------------------------
int vtkXdmfReader::FillOutputPortInformation(int, vtkInformation *info)
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkDataObject");
  return 1;
}

//----------------------------------------------------------------------------
int vtkXdmfReader::ProcessRequest(vtkInformation *request,
    vtkInformationVector **inputVector,
    vtkInformationVector *outputVector)
{
  // create the output
  if (request->Has(vtkDemandDrivenPipeline::REQUEST_DATA_OBJECT()))
  {
    return this->RequestDataObject(outputVector);
  }

  return this->Superclass::ProcessRequest(request, inputVector, outputVector);
}

//----------------------------------------------------------------------------
bool vtkXdmfReader::PrepareDocument()
{
  // Calling this method repeatedly is okay. It does work only when something
  // has changed.
  if (this->GetReadFromInputString())
  {
    const char* data=0;
    unsigned int data_length=0;
    if (this->InputArray)
    {
      data = this->InputArray->GetPointer(0);
      data_length = static_cast<unsigned int>(
        this->InputArray->GetNumberOfTuples()*
        this->InputArray->GetNumberOfComponents());
    }
    else if (this->InputString)
    {
      data = this->InputString;
      data_length = this->InputStringLength;
    }
    else
    {
      vtkErrorMacro("No input string specified");
      return false;
    }
    if (!this->XdmfDocument->ParseString(data, data_length))
    {
      vtkErrorMacro("Failed to parse xmf.");
      return false;
    }
  }
  else
  {
    // Parse the file...
    if (!this->FileName )
    {
      vtkErrorMacro("File name not set");
      return false;
    }

    // First make sure the file exists.  This prevents an empty file
    // from being created on older compilers.
    if (!vtksys::SystemTools::FileExists(this->FileName))
    {
      vtkErrorMacro("Error opening file " << this->FileName);
      return false;
    }

    if (!this->XdmfDocument->Parse(this->FileName))
    {
      vtkErrorMacro("Failed to parse xmf file: " << this->FileName);
      return false;
    }
  }

  if (this->DomainName)
  {
    if (!this->XdmfDocument->SetActiveDomain(this->DomainName))
    {
      vtkErrorMacro("Invalid domain: " << this->DomainName);
      return false;
    }
  }
  else
  {
    this->XdmfDocument->SetActiveDomain(static_cast<int>(0));
  }

  if (this->XdmfDocument->GetActiveDomain() &&
    this->XdmfDocument->GetActiveDomain()->GetSIL()->GetMTime() >
    this->GetMTime())
  {
    this->SILUpdateStamp++;
  }

  this->LastTimeIndex = 0; // reset time index when the file changes.
  return (this->XdmfDocument->GetActiveDomain() != 0);
}

//----------------------------------------------------------------------------
int vtkXdmfReader::RequestDataObject(vtkInformationVector *outputVector)
{
  if (!this->PrepareDocument())
  {
    return 0;
  }

  int vtk_type = this->XdmfDocument->GetActiveDomain()->GetVTKDataType();
  if (this->XdmfDocument->GetActiveDomain()->GetSetsSelection()->
     GetNumberOfArrays() > 0)
  {
    // if the data has any sets, then we are forced to using multiblock.
    vtk_type = VTK_MULTIBLOCK_DATA_SET;
  }

  vtkDataObject* output = vtkDataObject::GetData(outputVector, 0);
  if (!output || output->GetDataObjectType() != vtk_type)
  {
    output = vtkDataObjectTypes::NewDataObject(vtk_type);
    outputVector->GetInformationObject(0)->Set(
        vtkDataObject::DATA_OBJECT(), output );
    this->GetOutputPortInformation(0)->Set(
      vtkDataObject::DATA_EXTENT_TYPE(), output->GetExtentType());
    output->Delete();
  }
  return 1;
}

//----------------------------------------------------------------------------
int vtkXdmfReader::RequestInformation(vtkInformation *, vtkInformationVector **,
  vtkInformationVector *outputVector)
{
  if (!this->PrepareDocument())
  {
    return 0;
  }

  // Pass any cached user-selections to the active domain.
  this->PassCachedSelections();

  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  vtkXdmfDomain* domain = this->XdmfDocument->GetActiveDomain();

  // * Publish the fact that this reader can satisfy any piece request.
  outInfo->Set(CAN_HANDLE_PIECE_REQUEST(), 1);

  this->LastTimeIndex = this->ChooseTimeStep(outInfo);

  // Set the requested time-step on the domain. Thus, now when we go to get
  // information, we can (ideally) get information about that time-step.
  // this->XdmfDocument->GetActiveDomain()->SetTimeStep(this->LastTimeIndex);

  // * If producing structured dataset put information about whole extents etc.
  if (domain->GetNumberOfGrids() == 1 &&
    domain->IsStructured(domain->GetGrid(0)) &&
    domain->GetSetsSelection()->GetNumberOfArrays() == 0)
  {
    xdmf2::XdmfGrid* xmfGrid = domain->GetGrid(0);
    // just in the case the top-level grid is a temporal collection, then pick
    // the sub-grid to fetch the extents etc.
    xmfGrid = domain->GetGrid(xmfGrid,
      domain->GetTimeForIndex(this->LastTimeIndex));
    int whole_extent[6];
    if (domain->GetWholeExtent(xmfGrid, whole_extent))
    {
      // re-scale the whole_extent using the stride.
      whole_extent[1] /= this->Stride[0];
      whole_extent[3] /= this->Stride[1];
      whole_extent[5] /= this->Stride[2];

      outInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),
        whole_extent, 6);
    }
    double origin[3];
    double spacing[3];
    if (domain->GetOriginAndSpacing(xmfGrid, origin, spacing))
    {
      spacing[0] *= this->Stride[0];
      spacing[1] *= this->Stride[1];
      spacing[2] *= this->Stride[2];
      outInfo->Set(vtkDataObject::ORIGIN(), origin, 3);
      outInfo->Set(vtkDataObject::SPACING(), spacing, 3);
    }
  }

  // * Publish the SIL which provides information about the grid hierarchy.
  outInfo->Set(vtkDataObject::SIL(), domain->GetSIL());

  // * Publish time information.
  const std::map<int, XdmfFloat64>& ts = domain->GetTimeStepsRev();
  std::vector<double> time_steps(ts.size());
  std::map<int, XdmfFloat64>::const_iterator it = ts.begin();
  for (int i = 0; it != ts.end(); i++, ++it)
  {
    time_steps[i] = it->second;
  }

  if (time_steps.size() > 0)
  {
    outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_STEPS(),
      &time_steps[0], static_cast<int>(time_steps.size()));
    double timeRange[2];
    timeRange[0] = time_steps.front();
    timeRange[1] = time_steps.back();
    outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_RANGE(), timeRange, 2);
  }

  return 1;
}

//----------------------------------------------------------------------------
int vtkXdmfReader::RequestData(vtkInformation *, vtkInformationVector **,
  vtkInformationVector *outputVector)
{
  if (!this->PrepareDocument())
  {
    return 0;
  }

  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  // * Collect information about what part of the data is requested.
  unsigned int updatePiece = 0;
  unsigned int updateNumPieces = 1;
  int ghost_levels = 0;
  if (outInfo->Has(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER()) &&
    outInfo->Has(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES()))
  {
    updatePiece = static_cast<unsigned int>(
      outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER()));
    updateNumPieces =  static_cast<unsigned int>(
      outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES()));
  }
  if (outInfo->Has(
      vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS()))
  {
    ghost_levels = outInfo->Get(
      vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS());
  }

  // will be set for structured datasets only.
  int update_extent[6] = {0, -1, 0, -1, 0, -1};
  if (outInfo->Has(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT()))
  {
    outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(),
      update_extent);
    if (outInfo->Has(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()))
    {
      int wholeExtent[6];
      outInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), wholeExtent);
      vtkNew<vtkExtentTranslator> et;
      et->SetWholeExtent(wholeExtent);
      et->SetPiece(updatePiece);
      et->SetNumberOfPieces(updateNumPieces);
      et->SetGhostLevel(ghost_levels);
      et->PieceToExtent();
      et->GetExtent(update_extent);
    }
  }

  this->LastTimeIndex = this->ChooseTimeStep(outInfo);
  if (this->LastTimeIndex == 0)
  {
    this->ClearDataSetCache();
  }

  vtkXdmfHeavyData dataReader(this->XdmfDocument->GetActiveDomain(), this);
  dataReader.Piece = updatePiece;
  dataReader.NumberOfPieces = updateNumPieces;
  dataReader.GhostLevels = ghost_levels;
  dataReader.Extents[0] = update_extent[0]*this->Stride[0];
  dataReader.Extents[1] = update_extent[1]*this->Stride[0];
  dataReader.Extents[2] = update_extent[2]*this->Stride[1];
  dataReader.Extents[3] = update_extent[3]*this->Stride[1];
  dataReader.Extents[4] = update_extent[4]*this->Stride[2];
  dataReader.Extents[5] = update_extent[5]*this->Stride[2];
  dataReader.Stride[0] = this->Stride[0];
  dataReader.Stride[1] = this->Stride[1];
  dataReader.Stride[2] = this->Stride[2];
  dataReader.Time = this->XdmfDocument->GetActiveDomain()->GetTimeForIndex(
    this->LastTimeIndex);

  vtkDataObject* data = dataReader.ReadData();
  if (!data)
  {
    vtkErrorMacro("Failed to read data.");
    return 0;
  }

  vtkDataObject* output = vtkDataObject::GetData(outInfo);

  if (!output->IsA(data->GetClassName()))
  {
    // BUG #0013766: Just in case the data type expected doesn't match the
    // produced data type, we should print a warning.
    vtkWarningMacro("Data type generated (" << data->GetClassName() << ") "
      "does not match data type expected (" << output->GetClassName() << "). "
      "Reader may not produce valid data.");
  }
  output->ShallowCopy(data);
  data->Delete();

  if (this->LastTimeIndex <
    this->XdmfDocument->GetActiveDomain()->GetTimeSteps().size())
  {
    double time =
      this->XdmfDocument->GetActiveDomain()->GetTimeForIndex(this->LastTimeIndex);
    output->GetInformation()->Set(vtkDataObject::DATA_TIME_STEP(), time);
  }
  return 1;
}

//----------------------------------------------------------------------------
int vtkXdmfReader::ChooseTimeStep(vtkInformation* outInfo)
{
  if (outInfo->Has(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP()))
  {
    // we do not support multiple timestep requests.
    double time =
      outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP());

    return this->XdmfDocument->GetActiveDomain()->GetIndexForTime(time);
  }

  // if no timestep was requested, just return what we read last.
  return this->LastTimeIndex;
}

//----------------------------------------------------------------------------
vtkXdmfArraySelection* vtkXdmfReader::GetPointArraySelection()
{
  return this->XdmfDocument->GetActiveDomain()?
    this->XdmfDocument->GetActiveDomain()->GetPointArraySelection() :
    this->PointArraysCache;
}

//----------------------------------------------------------------------------
vtkXdmfArraySelection* vtkXdmfReader::GetCellArraySelection()
{
  return this->XdmfDocument->GetActiveDomain()?
    this->XdmfDocument->GetActiveDomain()->GetCellArraySelection() :
    this->CellArraysCache;
}

//----------------------------------------------------------------------------
vtkXdmfArraySelection* vtkXdmfReader::GetGridSelection()
{
  return this->XdmfDocument->GetActiveDomain()?
    this->XdmfDocument->GetActiveDomain()->GetGridSelection() :
    this->GridsCache;
}

//----------------------------------------------------------------------------
vtkXdmfArraySelection* vtkXdmfReader::GetSetsSelection()
{
  return this->XdmfDocument->GetActiveDomain()?
    this->XdmfDocument->GetActiveDomain()->GetSetsSelection() :
    this->SetsCache;
}

//----------------------------------------------------------------------------
int vtkXdmfReader::GetNumberOfGrids()
{
  return this->GetGridSelection()->GetNumberOfArrays();
}

//----------------------------------------------------------------------------
void vtkXdmfReader::SetGridStatus(const char* gridname, int status)
{
  this->GetGridSelection()->SetArrayStatus(gridname, status !=0);
  this->Modified();
}

//----------------------------------------------------------------------------
int vtkXdmfReader::GetGridStatus(const char* arrayname)
{
  return this->GetGridSelection()->GetArraySetting(arrayname);
}

//----------------------------------------------------------------------------
const char* vtkXdmfReader::GetGridName(int index)
{
  return this->GetGridSelection()->GetArrayName(index);
}

//----------------------------------------------------------------------------
int vtkXdmfReader::GetNumberOfPointArrays()
{
  return this->GetPointArraySelection()->GetNumberOfArrays();
}

//----------------------------------------------------------------------------
void vtkXdmfReader::SetPointArrayStatus(const char* arrayname, int status)
{
  this->GetPointArraySelection()->SetArrayStatus(arrayname, status != 0);
  this->Modified();
}

//----------------------------------------------------------------------------
int vtkXdmfReader::GetPointArrayStatus(const char* arrayname)
{
  return this->GetPointArraySelection()->GetArraySetting(arrayname);
}

//----------------------------------------------------------------------------
const char* vtkXdmfReader::GetPointArrayName(int index)
{
  return this->GetPointArraySelection()->GetArrayName(index);
}

//----------------------------------------------------------------------------
int vtkXdmfReader::GetNumberOfCellArrays()
{
  return this->GetCellArraySelection()->GetNumberOfArrays();
}

//----------------------------------------------------------------------------
void vtkXdmfReader::SetCellArrayStatus(const char* arrayname, int status)
{
  this->GetCellArraySelection()->SetArrayStatus(arrayname, status != 0);
  this->Modified();
}

//----------------------------------------------------------------------------
int vtkXdmfReader::GetCellArrayStatus(const char* arrayname)
{
  return this->GetCellArraySelection()->GetArraySetting(arrayname);
}

//----------------------------------------------------------------------------
const char* vtkXdmfReader::GetCellArrayName(int index)
{
  return this->GetCellArraySelection()->GetArrayName(index);
}

//----------------------------------------------------------------------------
int vtkXdmfReader::GetNumberOfSets()
{
  return this->GetSetsSelection()->GetNumberOfArrays();
}

//----------------------------------------------------------------------------
void vtkXdmfReader::SetSetStatus(const char* arrayname, int status)
{
  this->GetSetsSelection()->SetArrayStatus(arrayname, status != 0);
  this->Modified();
}

//----------------------------------------------------------------------------
int vtkXdmfReader::GetSetStatus(const char* arrayname)
{
  return this->GetSetsSelection()->GetArraySetting(arrayname);
}

//----------------------------------------------------------------------------
const char* vtkXdmfReader::GetSetName(int index)
{
  return this->GetSetsSelection()->GetArrayName(index);
}

//----------------------------------------------------------------------------
void vtkXdmfReader::PassCachedSelections()
{
  if (!this->XdmfDocument->GetActiveDomain())
  {
    return;
  }

  this->GetPointArraySelection()->Merge(*this->PointArraysCache);
  this->GetCellArraySelection()->Merge(*this->CellArraysCache);
  this->GetGridSelection()->Merge(*this->GridsCache);
  this->GetSetsSelection()->Merge(*this->SetsCache);

  // Clear the cache.
  this->PointArraysCache->clear();
  this->CellArraysCache->clear();
  this->GridsCache->clear();
  this->SetsCache->clear();
}

//----------------------------------------------------------------------------
void vtkXdmfReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
//----------------------------------------------------------------------------
vtkGraph* vtkXdmfReader::GetSIL()
{
  if(vtkXdmfDomain* domain = this->XdmfDocument->GetActiveDomain())
  {
    return domain->GetSIL();
  }
  return 0;
}

//----------------------------------------------------------------------------
void vtkXdmfReader::ClearDataSetCache()
{
  XdmfReaderCachedData::iterator it = this->DataSetCache.begin();
  while (it != this->DataSetCache.end())
  {
    if (it->second.dataset != NULL)
    {
      it->second.dataset->Delete();
    }
    ++it;
  }
  this->DataSetCache.clear();
}

//----------------------------------------------------------------------------
vtkXdmfReader::XdmfReaderCachedData& vtkXdmfReader::GetDataSetCache()
{
  return this->DataSetCache;
}
