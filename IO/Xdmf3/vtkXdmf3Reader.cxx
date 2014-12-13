/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXdmf3Reader.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkXdmf3Reader.h"

#include "vtksys/SystemTools.hxx"
#include "vtkDataObjectTypes.h"
#include "vtkDataObjectTreeIterator.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkImageData.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkMultiPieceDataSet.h"
#include "vtkMultiProcessController.h"
#include "vtkMutableDirectedGraph.h"
#include "vtkObjectFactory.h"
#include "vtkRectilinearGrid.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkStructuredGrid.h"
#include "vtkTimerLog.h"
#include "vtkXdmf3ArrayKeeper.h"
#include "vtkXdmf3ArraySelection.h"
#include "vtkXdmf3DataSet.h"
#include "vtkXdmf3HeavyDataHandler.h"
#include "vtkXdmf3LightDataHandler.h"
#include "vtkXdmf3SILBuilder.h"

#include "XdmfCurvilinearGrid.hpp"
#include "XdmfDomain.hpp"
#include "XdmfGridCollection.hpp"
#include "XdmfGridCollectionType.hpp"
#include "XdmfReader.hpp"
#include "XdmfRectilinearGrid.hpp"
#include "XdmfRegularGrid.hpp"
#include "XdmfUnstructuredGrid.hpp"


//TODO: implement fast and approximate CanReadFile
//TODO: read from buffer, allowing for xincludes
//TODO: strided access to structured data
//TODO: when too many grids for SIL, allow selection of top level grids
//TODO: break structured data into pieces
//TODO: make domains entirely optional and selectable

//=============================================================================
class vtkXdmf3Reader::Internals
{
  //Private implementation details for vtkXdmf3Reader
public:
  Internals()
  {
    this->FieldArrays = new vtkXdmf3ArraySelection;
    this->CellArrays = new vtkXdmf3ArraySelection;
    this->PointArrays = new vtkXdmf3ArraySelection;
    this->GridsCache = new vtkXdmf3ArraySelection;
    this->SetsCache = new vtkXdmf3ArraySelection;

    this->SILBuilder = new vtkXdmf3SILBuilder();
    this->SILBuilder->Initialize();

    this->Keeper = new vtkXdmf3ArrayKeeper;
  };

  ~Internals()
  {
    delete this->FieldArrays;
    delete this->CellArrays;
    delete this->PointArrays;
    delete this->GridsCache;
    delete this->SetsCache;
    delete this->SILBuilder;
    this->ReleaseArrays(true);
    this->FileNames.clear();
  };

  //--------------------------------------------------------------------------
  bool PrepareDocument(vtkXdmf3Reader *self, const char *FileName, bool AsTime)
  {
    if (this->Domain)
      {
      return true;
      }

    if (!FileName )
      {
      vtkErrorWithObjectMacro(self, "File name not set");
      return false;
      }
    if (!vtksys::SystemTools::FileExists(FileName))
      {
      vtkErrorWithObjectMacro(self, "Error opening file " << FileName);
      return false;
      }
    if (!this->Domain)
      {
      this->Init(FileName, AsTime);
      }
    return true;
  }

  //--------------------------------------------------------------------------
  vtkGraph *GetSIL()
  {
    return this->SILBuilder->SIL;
  }

  //--------------------------------------------------------------------------
  //find out what kind of vtkdataobject the xdmf file will produce
  int GetVTKType()
  {
    if (this->VTKType != -1)
      {
      return this->VTKType;
      }
    unsigned int nGridCollections = this->Domain->getNumberGridCollections();
    if (nGridCollections > 1)
      {
      this->VTKType = VTK_MULTIBLOCK_DATA_SET;
      return this->VTKType;
      }

    //check for temporal of atomic, in which case we produce the atomic type
    shared_ptr<XdmfDomain> toCheck = this->Domain;
    bool temporal = false;
    if (nGridCollections == 1)
      {
      shared_ptr<XdmfGridCollection> gc = this->Domain->getGridCollection(0);
      if (gc->getType() == XdmfGridCollectionType::Temporal())
        {
        if (gc->getNumberGridCollections() == 0)
          {
          temporal = true;
          toCheck = gc;
          }
        }
      }

    unsigned int nUnstructuredGrids = toCheck->getNumberUnstructuredGrids();
    unsigned int nRectilinearGrids = toCheck->getNumberRectilinearGrids();
    unsigned int nCurvilinearGrids= toCheck->getNumberCurvilinearGrids();
    unsigned int nRegularGrids = toCheck->getNumberRegularGrids();
    unsigned int nGraphs = toCheck->getNumberGraphs();
    int numtypes = 0;
    numtypes = numtypes + (nUnstructuredGrids>0?1:0);
    numtypes = numtypes + (nRectilinearGrids>0?1:0);
    numtypes = numtypes + (nCurvilinearGrids>0?1:0);
    numtypes = numtypes + (nRegularGrids>0?1:0);
    numtypes = numtypes + (nGraphs>0?1:0);
    bool atomic =
        temporal ||
        (numtypes==1 &&
          (
          nUnstructuredGrids==1||
          nRectilinearGrids==1||
          nCurvilinearGrids==1||
          nRegularGrids==1||
          nGraphs==1));
    if (!atomic
        )
      {
      this->VTKType = VTK_MULTIBLOCK_DATA_SET;
      }
    else
      {
      this->VTKType = VTK_UNIFORM_GRID;
      //keep a reference to get extent from
      this->TopGrid = toCheck->getRegularGrid(0);
      if (nRectilinearGrids>0)
        {
        this->VTKType = VTK_RECTILINEAR_GRID;
        //keep a reference to get extent from
        this->TopGrid = toCheck->getRectilinearGrid(0);
        }
      else if (nCurvilinearGrids>0)
        {
        this->VTKType = VTK_STRUCTURED_GRID;
        //keep a reference to get extent from
        this->TopGrid = toCheck->getCurvilinearGrid(0);
        }
      else if (nUnstructuredGrids>0)
        {
        this->VTKType = VTK_UNSTRUCTURED_GRID;
        this->TopGrid = toCheck->getUnstructuredGrid(0);
        }
      else if (nGraphs>0)
        {
        //VTK_MUTABLE_DIRECTED_GRAPH more specifically
        this->VTKType = VTK_DIRECTED_GRAPH;
        }
      }
      if (this->TopGrid)
        {
        shared_ptr<XdmfGrid> grid =
          shared_dynamic_cast<XdmfGrid>(this->TopGrid);
        if (grid && grid->getNumberSets()>0)
          {
          this->VTKType = VTK_MULTIBLOCK_DATA_SET;
          }
        }
     return this->VTKType;
  }

  //--------------------------------------------------------------------------
  void ReadHeavyData(unsigned int updatePiece, unsigned int updateNumPieces,
                     bool doTime, double time, vtkMultiBlockDataSet* mbds,
                     bool AsTime)
  {
    //traverse the xdmf hierarchy, and convert and return what was requested
    shared_ptr<vtkXdmf3HeavyDataHandler> visitor =
        vtkXdmf3HeavyDataHandler::New(
          this->FieldArrays,
          this->CellArrays,
          this->PointArrays,
          this->GridsCache,
          this->SetsCache,
          updatePiece,
          updateNumPieces,
          doTime,
          time,
          this->Keeper,
          AsTime
          );
      visitor->Populate(this->Domain, mbds);
  }

  //--------------------------------------------------------------------------
  vtkMultiPieceDataSet *Flatten(vtkMultiBlockDataSet *mbds);

  //--------------------------------------------------------------------------
  void ReleaseArrays(bool force=false)
  {
    if (!this->Keeper)
      {
      return;
      }
    this->Keeper->Release(force);
  }

  //--------------------------------------------------------------------------
  void BumpKeeper()
  {
    if (!this->Keeper)
      {
      return;
      }
    this->Keeper->BumpGeneration();
  }

  vtkXdmf3ArraySelection *FieldArrays;
  vtkXdmf3ArraySelection *CellArrays;
  vtkXdmf3ArraySelection *PointArrays;
  vtkXdmf3ArraySelection *GridsCache;
  vtkXdmf3ArraySelection *SetsCache;
  std::vector<double> TimeSteps;
  shared_ptr<XdmfItem> TopGrid;
  vtkXdmf3ArrayKeeper *Keeper;

  std::vector<std::string> FileNames;

private:

  //--------------------------------------------------------------------------
  void Init(const char *filename, bool AsTime)
  {
    vtkTimerLog::MarkStartEvent("X3R::Init");
    unsigned int idx = static_cast<unsigned int>(this->FileNames.size());

    this->Reader = XdmfReader::New();

    unsigned int updatePiece = 0;
    unsigned int updateNumPieces = 1;
    vtkMultiProcessController* ctrl =
      vtkMultiProcessController::GetGlobalController();
    if (ctrl != NULL)
      {
      updatePiece = ctrl->GetLocalProcessId();
      updateNumPieces = ctrl->GetNumberOfProcesses();
      }
    else
      {
      updatePiece = 0;
      updateNumPieces = 1;
      }

    if (idx == 1)
      {
      this->Domain = shared_dynamic_cast<XdmfDomain>
        (this->Reader->read(filename));
      }
    else
      {
      this->Domain = XdmfDomain::New();
      shared_ptr<XdmfGridCollection> topc = XdmfGridCollection::New();
      if (AsTime)
        {
        topc->setType(XdmfGridCollectionType::Temporal());
        }
      this->Domain->insert(topc);
      for (unsigned int i = 0; i < idx; i++)
        {
        if (AsTime || (i%updateNumPieces == updatePiece))
          {
          //cerr << updatePiece << " reading " << this->FileNames[i] << endl;
          shared_ptr<XdmfDomain> fdomain = shared_dynamic_cast<XdmfDomain>
            (this->Reader->read(this->FileNames[i]));

          unsigned int j;
          for (j = 0; j < fdomain->getNumberGridCollections(); j++)
            {
            topc->insert(fdomain->getGridCollection(j));
            }
          for (j = 0; j < fdomain->getNumberUnstructuredGrids(); j++)
            {
            topc->insert(fdomain->getUnstructuredGrid(j));
            }
          for (j = 0; j < fdomain->getNumberRectilinearGrids(); j++)
            {
            topc->insert(fdomain->getRectilinearGrid(j));
            }
          for (j = 0; j < fdomain->getNumberCurvilinearGrids(); j++)
            {
            topc->insert(fdomain->getCurvilinearGrid(j));
            }
          for (j = 0; j < fdomain->getNumberRegularGrids(); j++)
            {
            topc->insert(fdomain->getRegularGrid(j));
            }
          for (j = 0; j < fdomain->getNumberGraphs(); j++)
            {
            topc->insert(fdomain->getGraph(j));
            }
          }
        }
      }

    this->VTKType = -1;
    vtkTimerLog::MarkStartEvent("X3R::learn");
    this->GatherMetaInformation();
    vtkTimerLog::MarkEndEvent("X3R::learn");

    vtkTimerLog::MarkEndEvent("X3R::Init");
  }

  //--------------------------------------------------------------------------
  void GatherMetaInformation()
  {
    vtkTimerLog::MarkStartEvent("X3R::GatherMetaInfo");

    unsigned int updatePiece = 0;
    unsigned int updateNumPieces = 1;
    vtkMultiProcessController* ctrl =
      vtkMultiProcessController::GetGlobalController();
    if (ctrl != NULL)
      {
      updatePiece = ctrl->GetLocalProcessId();
      updateNumPieces = ctrl->GetNumberOfProcesses();
      }
    else
      {
      updatePiece = 0;
      updateNumPieces = 1;
      }
    shared_ptr<vtkXdmf3LightDataHandler> visitor =
          vtkXdmf3LightDataHandler::New (
              this->SILBuilder,
              this->FieldArrays,
              this->CellArrays,
              this->PointArrays,
              this->GridsCache,
              this->SetsCache,
              updatePiece,
              updateNumPieces);
    visitor->InspectXDMF(this->Domain, -1);
    visitor->ClearGridsIfNeeded(this->Domain);
    if (this->TimeSteps.size())
       {
       this->TimeSteps.erase(this->TimeSteps.begin());
       }
     std::set<double> times = visitor->getTimes();
     std::set<double>::const_iterator it = times.begin();
     while (it != times.end())
       {
       this->TimeSteps.push_back(*it);
       it++;
       }
    vtkTimerLog::MarkEndEvent("X3R::GatherMetaInfo");
  }

  int VTKType;
  shared_ptr<XdmfReader> Reader;
  shared_ptr<XdmfDomain> Domain;
  vtkXdmf3SILBuilder *SILBuilder;
};

//==============================================================================

vtkStandardNewMacro(vtkXdmf3Reader);

//----------------------------------------------------------------------------
vtkXdmf3Reader::vtkXdmf3Reader()
{
  this->FileName = NULL;

  this->Internal = new vtkXdmf3Reader::Internals();
  this->FileSeriesAsTime = true;

  this->FieldArraysCache = this->Internal->FieldArrays;
  this->CellArraysCache = this->Internal->CellArrays;
  this->PointArraysCache = this->Internal->PointArrays;
  this->SetsCache = this->Internal->SetsCache;
  this->GridsCache = this->Internal->GridsCache;
}

//----------------------------------------------------------------------------
vtkXdmf3Reader::~vtkXdmf3Reader()
{

  this->SetFileName(NULL);
  delete this->Internal;
  //XdmfHDF5Controller::closeFiles();
}

//----------------------------------------------------------------------------
void vtkXdmf3Reader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "FileName: " <<
    (this->FileName ? this->FileName : "(none)") << endl;
  os << indent << "FileSeriesAsTime: " <<
    (this->FileSeriesAsTime ? "True" : "False") << endl;
}

//----------------------------------------------------------------------------
void vtkXdmf3Reader::AddFileName(const char* filename)
{
  this->Internal->FileNames.push_back(filename);
  if (this->Internal->FileNames.size()==1)
    {
    this->Superclass::SetFileName(filename);
    }
}

//----------------------------------------------------------------------------
void vtkXdmf3Reader::SetFileName(const char* filename)
{
  this->RemoveAllFileNames();
  if (filename)
    {
    this->Internal->FileNames.push_back(filename);
    }
  this->Superclass::SetFileName(filename);
}

//----------------------------------------------------------------------------
void vtkXdmf3Reader::RemoveAllFileNames()
{
  this->Internal->FileNames.clear();
}

//----------------------------------------------------------------------------
int vtkXdmf3Reader::CanReadFile(const char* filename)
{
  if (!vtksys::SystemTools::FileExists(filename))
    {
    return 0;
    }

 /*
  TODO: this, but really fast
  shared_ptr<XdmfReader> tester = XdmfReader::New();
  try {
    shared_ptr<XdmfItem> item = tester->read(filename);
  } catch (XdmfError & e) {
    return 0;
  }
 */

  return 1;
}

//----------------------------------------------------------------------------
int vtkXdmf3Reader::FillOutputPortInformation(int, vtkInformation *info)
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkDataObject");
  return 1;
}

//----------------------------------------------------------------------------
int vtkXdmf3Reader::ProcessRequest(vtkInformation *request,
    vtkInformationVector **inputVector,
    vtkInformationVector *outputVector)
{
  if (request->Has(vtkDemandDrivenPipeline::REQUEST_DATA_OBJECT()))
    {
    return this->RequestDataObject(outputVector);
    }

  return this->Superclass::ProcessRequest(request, inputVector, outputVector);
}

//----------------------------------------------------------------------------
int vtkXdmf3Reader::RequestDataObject(vtkInformationVector *outputVector)
{
  vtkTimerLog::MarkStartEvent("X3R::RDO");
  //let libXdmf parse XML
  if (!this->Internal->PrepareDocument(this, this->FileName,
                                       this->FileSeriesAsTime))
    {
    vtkTimerLog::MarkEndEvent("X3R::RDO");
    return 0;
    }

  //Determine what vtkDataObject we should produce
  int vtk_type = this->Internal->GetVTKType();

  //Make an empty vtkDataObject
  vtkDataObject* output = vtkDataObject::GetData(outputVector, 0);
  if (!output || output->GetDataObjectType() != vtk_type)
    {
    if (vtk_type == VTK_DIRECTED_GRAPH)
      {
      output = vtkMutableDirectedGraph::New();
      }
    else
      {
      output = vtkDataObjectTypes::NewDataObject(vtk_type);
      }
    outputVector->GetInformationObject(0)->Set(
        vtkDataObject::DATA_OBJECT(), output );
    this->GetOutputPortInformation(0)->Set(
      vtkDataObject::DATA_EXTENT_TYPE(), output->GetExtentType());
    output->Delete();
    }

  vtkTimerLog::MarkEndEvent("X3R::RDO");
  return 1;
}

//----------------------------------------------------------------------------
int vtkXdmf3Reader::RequestInformation(vtkInformation *,
  vtkInformationVector **,
  vtkInformationVector *outputVector)
{
  vtkTimerLog::MarkStartEvent("X3R::RI");
  if (!this->Internal->PrepareDocument(this, this->FileName,
                                       this->FileSeriesAsTime))
    {
    vtkTimerLog::MarkEndEvent("X3R::RI");
    return 0;
    }

  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  // Publish the fact that this reader can satisfy any piece request.
  outInfo->Set(CAN_HANDLE_PIECE_REQUEST(), 1);

  // Publish the SIL which provides information about the grid hierarchy.
  outInfo->Set(vtkDataObject::SIL(), this->Internal->GetSIL());

  // Publish the times that we have data for
  if (this->Internal->TimeSteps.size() > 0)
    {
    outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_STEPS(),
      &this->Internal->TimeSteps[0],
      static_cast<int>(this->Internal->TimeSteps.size()));
    double timeRange[2];
    timeRange[0] = this->Internal->TimeSteps.front();
    timeRange[1] = this->Internal->TimeSteps.back();
    outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_RANGE(), timeRange, 2);
    }

  // Structured atomic must announce the whole extent it can provide
  int vtk_type = this->Internal->GetVTKType();
  if (vtk_type == VTK_STRUCTURED_GRID ||
      vtk_type == VTK_RECTILINEAR_GRID ||
      vtk_type == VTK_IMAGE_DATA ||
      vtk_type == VTK_UNIFORM_GRID)
    {
    int whole_extent[6];
    double origin[3];
    double spacing[3];
    whole_extent[0] = 0;
    whole_extent[1] = -1;
    whole_extent[2] = 0;
    whole_extent[3] = -1;
    whole_extent[4] = 0;
    whole_extent[5] = -1;
    origin[0] = 0.0;
    origin[1] = 0.0;
    origin[2] = 0.0;
    spacing[0] = 1.0;
    spacing[1] = 1.0;
    spacing[2] = 1.0;

    shared_ptr<XdmfRegularGrid> regGrid =
      shared_dynamic_cast<XdmfRegularGrid>(this->Internal->TopGrid);
    if (regGrid)
      {
      vtkImageData *dataSet = vtkImageData::New();
      vtkXdmf3DataSet::CopyShape(regGrid.get(), dataSet, this->Internal->Keeper);
      dataSet->GetExtent(whole_extent);
      dataSet->GetOrigin(origin);
      dataSet->GetSpacing(spacing);
      dataSet->Delete();
      }
    shared_ptr<XdmfRectilinearGrid> recGrid =
      shared_dynamic_cast<XdmfRectilinearGrid>(this->Internal->TopGrid);
    if (recGrid)
      {
      vtkRectilinearGrid *dataSet = vtkRectilinearGrid::New();
      vtkXdmf3DataSet::CopyShape(recGrid.get(), dataSet, this->Internal->Keeper);
      dataSet->GetExtent(whole_extent);
      dataSet->Delete();
      }
    shared_ptr<XdmfCurvilinearGrid> crvGrid =
      shared_dynamic_cast<XdmfCurvilinearGrid>(this->Internal->TopGrid);
    if (crvGrid)
      {
      vtkStructuredGrid *dataSet = vtkStructuredGrid::New();
      vtkXdmf3DataSet::CopyShape(crvGrid.get(), dataSet, this->Internal->Keeper);
      dataSet->GetExtent(whole_extent);
      dataSet->Delete();
      }

    outInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),
        whole_extent, 6);
    outInfo->Set(vtkDataObject::ORIGIN(), origin, 3);
    outInfo->Set(vtkDataObject::SPACING(), spacing, 3);
    }

  vtkTimerLog::MarkEndEvent("X3R::RI");
  return 1;
}

//----------------------------------------------------------------------------
int vtkXdmf3Reader::RequestData(vtkInformation *,
  vtkInformationVector **,
  vtkInformationVector *outputVector)
{
  vtkTimerLog::MarkStartEvent("X3R::RD");

  if (!this->Internal->PrepareDocument(this, this->FileName,
                                       this->FileSeriesAsTime))
    {
    vtkTimerLog::MarkEndEvent("X3R::RD");
    return 0;
    }

  vtkTimerLog::MarkStartEvent("X3R::Release");
  this->Internal->ReleaseArrays();
  this->Internal->BumpKeeper();
  vtkTimerLog::MarkEndEvent("X3R::Release");

  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  // Collect information about what spatial extent is requested.
  unsigned int updatePiece = 0;
  unsigned int updateNumPieces = 1;
  if (outInfo->Has(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER()) &&
      outInfo->Has(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES()))
    {
    updatePiece = static_cast<unsigned int>(
        outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER()));
    updateNumPieces =  static_cast<unsigned int>(
        outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES()));
    }
  /*
  int ghost_levels = 0;
  if (outInfo->Has(
      vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS()))
    {
    ghost_levels = outInfo->Get(
        vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS());
    }
  */
  /*
  int update_extent[6] = {0, -1, 0, -1, 0, -1};
  if (outInfo->Has(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT()))
    {
    outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(),
        update_extent);
    }
  */

  // Collect information about what temporal extent is requested.
  double time = 0.0;
  bool doTime = false;
  if (outInfo->Has(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP()) &&
      this->Internal->TimeSteps.size())
    {
    doTime = true;
    time =
      outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP());
    //find the nearest match (floor), so we have something exact to search for
    std::vector<double>::iterator it = upper_bound(
      this->Internal->TimeSteps.begin(), this->Internal->TimeSteps.end(), time);
    if (it != this->Internal->TimeSteps.begin())
      {
      it--;
      }
    time = *it;
    }

  vtkDataObject* output = vtkDataObject::GetData(outInfo);
  if (!output)
    {
    return 0;
    }
  if (doTime)
    {
    output->GetInformation()->Set(vtkDataObject::DATA_TIME_STEP(), time);
    }

  vtkMultiBlockDataSet *mbds = vtkMultiBlockDataSet::New();
  this->Internal->ReadHeavyData(
      updatePiece, updateNumPieces,
      doTime, time,
      mbds,
      this->FileSeriesAsTime);

  if (mbds->GetNumberOfBlocks()==1)
    {
    vtkMultiBlockDataSet *ibds = vtkMultiBlockDataSet::SafeDownCast(mbds->GetBlock(0));
    vtkMultiBlockDataSet *obds = vtkMultiBlockDataSet::SafeDownCast(output);
    if (!this->FileSeriesAsTime && ibds && obds)
      {
      vtkMultiPieceDataSet *mpds = this->Internal->Flatten(ibds);
      obds->SetBlock(0, mpds);
      mpds->Delete();
      }
    else
      {
      output->ShallowCopy(mbds->GetBlock(0));
      }
    }
  else
    {
    vtkMultiBlockDataSet *obds = vtkMultiBlockDataSet::SafeDownCast(output);
    if (!this->FileSeriesAsTime && obds)
      {
      vtkMultiPieceDataSet *mpds = this->Internal->Flatten(mbds);
      obds->SetBlock(0, mpds);
      mpds->Delete();
      }
    else
      {
      output->ShallowCopy(mbds);
      }
    }
  mbds->Delete();

  vtkTimerLog::MarkEndEvent("X3R::RD");

  return 1;
}

//----------------------------------------------------------------------------
vtkMultiPieceDataSet * vtkXdmf3Reader::Internals::Flatten
  (vtkMultiBlockDataSet* ibds)
{
  vtkDataObjectTreeIterator *it = ibds->NewTreeIterator();
  unsigned int i = 0;

  //found out how many pieces we have locally
  it->InitTraversal();
  it->VisitOnlyLeavesOn();
  while(!it->IsDoneWithTraversal())
    {
    it->GoToNextItem();
    i++;
    }

  //communicate to find out where mine should go
  int mylen = i;
  int *allLens;
  unsigned int procnum;
  unsigned int numProcs ;
  vtkMultiProcessController* ctrl =
    vtkMultiProcessController::GetGlobalController();
  if (ctrl != NULL)
    {
    procnum = ctrl->GetLocalProcessId();
    numProcs = ctrl->GetNumberOfProcesses();
    allLens = new int[numProcs];
    ctrl->AllGather(&mylen, allLens, 1);
    }
  else
    {
    procnum = 0;
    numProcs = 1;
    allLens = new int[1];
    allLens[0] = mylen;
    }
  unsigned int myStart = 0;
  unsigned int total = 0;
  for (i = 0; i < numProcs; i++)
    {
    if (i < procnum)
      {
      myStart += allLens[i];
      }
    total += allLens[i];
    }
  delete[] allLens;

  //cerr << "PROC " << procnum << " starts at " << myStart << endl;
  //zero out everyone else's
  vtkMultiPieceDataSet *mpds = vtkMultiPieceDataSet::New();
  for (i = 0; i < total; i++)
    {
    mpds->SetPiece(i++, NULL);
    }

  //fill in my pieces
  it->GoToFirstItem();
  while(!it->IsDoneWithTraversal())
    {
    mpds->SetPiece(myStart++, it->GetCurrentDataObject());
    it->GoToNextItem();
    }

  it->Delete();

  return mpds; //caller must Delete
}

//----------------------------------------------------------------------------
int vtkXdmf3Reader::GetNumberOfFieldArrays()
{
  return this->GetFieldArraySelection()->GetNumberOfArrays();
}

//----------------------------------------------------------------------------
void vtkXdmf3Reader::SetFieldArrayStatus(const char* arrayname, int status)
{
  this->GetFieldArraySelection()->SetArrayStatus(arrayname, status != 0);
  this->Modified();
}

//----------------------------------------------------------------------------
int vtkXdmf3Reader::GetFieldArrayStatus(const char* arrayname)
{
  return this->GetFieldArraySelection()->GetArraySetting(arrayname);
}

//----------------------------------------------------------------------------
const char* vtkXdmf3Reader::GetFieldArrayName(int index)
{
  return this->GetFieldArraySelection()->GetArrayName(index);
}

//----------------------------------------------------------------------------
vtkXdmf3ArraySelection* vtkXdmf3Reader::GetFieldArraySelection()
{
  return this->FieldArraysCache;
}

//----------------------------------------------------------------------------
int vtkXdmf3Reader::GetNumberOfCellArrays()
{
  return this->GetCellArraySelection()->GetNumberOfArrays();
}

//----------------------------------------------------------------------------
void vtkXdmf3Reader::SetCellArrayStatus(const char* arrayname, int status)
{
  this->GetCellArraySelection()->SetArrayStatus(arrayname, status != 0);
  this->Modified();
}

//----------------------------------------------------------------------------
int vtkXdmf3Reader::GetCellArrayStatus(const char* arrayname)
{
  return this->GetCellArraySelection()->GetArraySetting(arrayname);
}

//----------------------------------------------------------------------------
const char* vtkXdmf3Reader::GetCellArrayName(int index)
{
  return this->GetCellArraySelection()->GetArrayName(index);
}

//----------------------------------------------------------------------------
vtkXdmf3ArraySelection* vtkXdmf3Reader::GetCellArraySelection()
{
  return this->CellArraysCache;
}

//----------------------------------------------------------------------------
int vtkXdmf3Reader::GetNumberOfPointArrays()
{
  return this->GetPointArraySelection()->GetNumberOfArrays();
}

//----------------------------------------------------------------------------
void vtkXdmf3Reader::SetPointArrayStatus(const char* arrayname, int status)
{
  this->GetPointArraySelection()->SetArrayStatus(arrayname, status != 0);
  this->Modified();
}

//----------------------------------------------------------------------------
int vtkXdmf3Reader::GetPointArrayStatus(const char* arrayname)
{
  return this->GetPointArraySelection()->GetArraySetting(arrayname);
}

//----------------------------------------------------------------------------
const char* vtkXdmf3Reader::GetPointArrayName(int index)
{
  return this->GetPointArraySelection()->GetArrayName(index);
}

//----------------------------------------------------------------------------
vtkXdmf3ArraySelection* vtkXdmf3Reader::GetPointArraySelection()
{
  return this->PointArraysCache;
}

//----------------------------------------------------------------------------
int vtkXdmf3Reader::GetNumberOfGrids()
{
  return this->GetGridsSelection()->GetNumberOfArrays();
}

//----------------------------------------------------------------------------
void vtkXdmf3Reader::SetGridStatus(const char* gridname, int status)
{
  this->GetGridsSelection()->SetArrayStatus(gridname, status !=0);
  this->Modified();
}

//----------------------------------------------------------------------------
int vtkXdmf3Reader::GetGridStatus(const char* arrayname)
{
  return this->GetGridsSelection()->GetArraySetting(arrayname);
}

//----------------------------------------------------------------------------
const char* vtkXdmf3Reader::GetGridName(int index)
{
  return this->GetGridsSelection()->GetArrayName(index);
}

//----------------------------------------------------------------------------
vtkXdmf3ArraySelection* vtkXdmf3Reader::GetGridsSelection()
{
  return this->GridsCache;
}

//----------------------------------------------------------------------------
int vtkXdmf3Reader::GetNumberOfSets()
{
  return this->GetSetsSelection()->GetNumberOfArrays();
}

//----------------------------------------------------------------------------
void vtkXdmf3Reader::SetSetStatus(const char* arrayname, int status)
{
  this->GetSetsSelection()->SetArrayStatus(arrayname, status != 0);
  this->Modified();
}

//----------------------------------------------------------------------------
int vtkXdmf3Reader::GetSetStatus(const char* arrayname)
{
  return this->GetSetsSelection()->GetArraySetting(arrayname);
}

//----------------------------------------------------------------------------
const char* vtkXdmf3Reader::GetSetName(int index)
{
  return this->GetSetsSelection()->GetArrayName(index);
}

//----------------------------------------------------------------------------
vtkXdmf3ArraySelection* vtkXdmf3Reader::GetSetsSelection()
{
  return this->SetsCache;
}

//----------------------------------------------------------------------------
vtkGraph* vtkXdmf3Reader::GetSIL()
{
  vtkGraph * ret = this->Internal->GetSIL();
  return ret;
}

//----------------------------------------------------------------------------
int vtkXdmf3Reader::GetSILUpdateStamp()
{
  return this->Internal->GetSIL()->GetMTime();
}
