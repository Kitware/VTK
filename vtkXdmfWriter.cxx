/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXdmfWriter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkXdmfWriter.h"
#include "vtkObjectFactory.h"

#include "vtkDataObject.h"

#include "vtkCompositeDataPipeline.h"
#include "vtkCompositeDataSet.h"
#include "vtkCompositeDataIterator.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkFieldData.h"
#include "vtkDataSet.h"
#include "vtkPointSet.h"
#include "vtkPoints.h"
#include "vtkImageData.h"
#include "vtkRectilinearGrid.h"
#include "vtkStructuredGrid.h"
#include "vtkSmartPointer.h"
#include "vtkCellType.h"
#include "vtkIdList.h"
#include "vtkUnstructuredGrid.h"
#include "vtkPolyData.h"
#include "vtkGenericCell.h"
#include "vtkCellArray.h"
#include "vtkDataSetAttributes.h"
#include "vtkPointData.h"
#include "vtkCellData.h"
#include "vtkTypeTraits.h"

#include "XdmfArray.h"
#include "XdmfAttribute.h"
#include "XdmfDataDesc.h"
#include "XdmfDomain.h"
#include "XdmfDOM.h"
#include "XdmfGeometry.h"
#include "XdmfGrid.h"
#include "XdmfRoot.h"
#include "XdmfTime.h"
#include "XdmfTopology.h"
#include <vector>
#include <map>
#include <algorithm>
#include <stdio.h>
#include <libxml/tree.h> // always after std::blah stuff

#if defined(_WIN32) && !defined(__CYGWIN__)
# define SNPRINTF _snprintf
#else
# define SNPRINTF snprintf
#endif

struct  _xmlNode;
typedef _xmlNode *XdmfXmlNode;
struct vtkXW2NodeHelp {
  XdmfDOM     *DOM;
  XdmfXmlNode  node;
  bool         staticFlag;
  vtkXW2NodeHelp(XdmfDOM *d, XdmfXmlNode n, bool f) : DOM(d), node(n), staticFlag(f) {};
};

class vtkXdmfWriterDomainMemoryHandler
{
  public:
    vtkXdmfWriterDomainMemoryHandler()
      {
        domain = new XdmfDomain();
      }
    ~vtkXdmfWriterDomainMemoryHandler()
      {
        for(std::vector<XdmfGrid*>::iterator iter = domainGrids.begin(); iter != domainGrids.end(); ++iter)
        {
          delete *iter;
        }
        delete domain;
      }
    void InsertGrid(XdmfGrid* grid)
      {
        domain->Insert(grid);
        domainGrids.push_back(grid);
      }
    void InsertIntoRoot(XdmfRoot& root)
      {
        root.Insert(domain);
      }
  private:
    XdmfDomain* domain;
    std::vector<XdmfGrid*> domainGrids;
};

//==============================================================================

struct vtkXdmfWriterInternal
{
  class CellType
    {
  public:
    CellType() : VTKType(0), NumPoints(0) {}
    CellType(const CellType& ct) : VTKType(ct.VTKType), NumPoints(ct.NumPoints) {}
    vtkIdType VTKType;
    vtkIdType NumPoints;
    bool operator<(const CellType& ct) const
      {
      return this->VTKType < ct.VTKType || (this->VTKType == ct.VTKType && this->NumPoints < ct.NumPoints);
      }
    bool operator==(const CellType& ct) const
      {
      return this->VTKType == ct.VTKType && this->NumPoints == ct.NumPoints;
      }
    CellType& operator=(const CellType& ct)
      {
      this->VTKType = ct.VTKType;
      this->NumPoints = ct.NumPoints;
      return *this;
      }

    };
  typedef std::map<CellType, vtkSmartPointer<vtkIdList> > MapOfCellTypes;
  static void DetermineCellTypes(vtkPointSet *t, MapOfCellTypes& vec);
};

//----------------------------------------------------------------------------
void vtkXdmfWriterInternal::DetermineCellTypes(vtkPointSet * t, vtkXdmfWriterInternal::MapOfCellTypes& vec)
{
  if ( !t )
    {
    return;
    }
  vtkIdType cc;
  vtkGenericCell* cell = vtkGenericCell::New();
  for ( cc = 0; cc < t->GetNumberOfCells(); cc ++ )
    {
    vtkXdmfWriterInternal::CellType ct;
    t->GetCell(cc, cell);
    ct.VTKType = cell->GetCellType();
    ct.NumPoints = cell->GetNumberOfPoints();
    vtkXdmfWriterInternal::MapOfCellTypes::iterator it = vec.find(ct);
    if ( it == vec.end() )
      {
      vtkIdList *l = vtkIdList::New();
      it = vec.insert(vtkXdmfWriterInternal::MapOfCellTypes::value_type(ct,
          vtkSmartPointer<vtkIdList>(l))).first;
      l->Delete();
      }
    // it->second->InsertUniqueId(cc);;
    it->second->InsertNextId(cc);;
    }
  cell->Delete();
}

//==============================================================================

vtkStandardNewMacro(vtkXdmfWriter);

//----------------------------------------------------------------------------
vtkXdmfWriter::vtkXdmfWriter()
{
  this->FileName = NULL;
  this->HeavyDataFileName = NULL;
  this->HeavyDataGroupName = NULL;
  this->DOM = NULL;
  this->Piece = 0;  //for parallel
  this->NumberOfPieces = 1;
  this->LightDataLimit = 100;
  this->WriteAllTimeSteps = 0;
  this->NumberOfTimeSteps = 1;
  this->CurrentTimeIndex = 0;
  this->TopTemporalGrid = NULL;
  this->DomainMemoryHandler = NULL;
}

//----------------------------------------------------------------------------
vtkXdmfWriter::~vtkXdmfWriter()
{
  this->SetFileName(NULL);
  this->SetHeavyDataFileName(NULL);
  this->SetHeavyDataGroupName(NULL);
  if (this->DOM)
    {
    delete this->DOM;
    this->DOM = NULL;
    }
  if (this->DomainMemoryHandler)
    {
    delete this->DomainMemoryHandler;
    }
  if (this->TopTemporalGrid)
    {
    delete this->TopTemporalGrid;
    this->TopTemporalGrid = NULL;
    }
  delete this->DomainMemoryHandler;

  //TODO: Verify memory isn't leaking
}

//-----------------------------------------------------------------------------
vtkExecutive* vtkXdmfWriter::CreateDefaultExecutive()
{
  return vtkCompositeDataPipeline::New();
}

//----------------------------------------------------------------------------
void vtkXdmfWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "FileName: " <<
    (this->FileName ? this->FileName : "(none)") << endl;
  os << indent << "LightDataLimit: " <<
    this->LightDataLimit << endl;
  os << indent << "WriteAllTimeSteps: " <<
    (this->WriteAllTimeSteps?"ON":"OFF") << endl;
}

//------------------------------------------------------------------------------
void vtkXdmfWriter::SetInputData(vtkDataObject *input)
{
  this->SetInputDataInternal(0,input);
}

//------------------------------------------------------------------------------
int vtkXdmfWriter::FillInputPortInformation(int, vtkInformation *info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataObject");
  return 1;
}

//------------------------------------------------------------------------------
int vtkXdmfWriter::Write()
{
  // Make sure we have input.
  if (this->GetNumberOfInputConnections(0) < 1)
    {
    vtkErrorMacro("No input provided!");
    return 0;
    }

  // always write even if the data hasn't changed
  this->Modified();

  //TODO: Specify name of heavy data companion file?
  if (!this->DOM)
    {
    this->DOM = new XdmfDOM();
    }
  this->DOM->SetOutputFileName(this->FileName);

  XdmfRoot root;
  root.SetDOM(this->DOM);
  root.SetVersion(2.2);
  root.Build();

  if (this->DomainMemoryHandler)
    {
    delete this->DomainMemoryHandler;
    }
  this->DomainMemoryHandler = new vtkXdmfWriterDomainMemoryHandler();
  this->DomainMemoryHandler->InsertIntoRoot(root);

  this->Update();

  root.Build();
  this->DOM->Write();

  delete this->DomainMemoryHandler;
  this->DomainMemoryHandler = NULL;

  return 1;
}

//----------------------------------------------------------------------------
int vtkXdmfWriter::RequestInformation(
  vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector,
  vtkInformationVector* vtkNotUsed(outputVector))
{
  // Does the input have timesteps?
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  if ( inInfo->Has(vtkStreamingDemandDrivenPipeline::TIME_STEPS()) )
    {
    this->NumberOfTimeSteps =
      inInfo->Length( vtkStreamingDemandDrivenPipeline::TIME_STEPS() );
    }
  else
    {
    this->NumberOfTimeSteps = 1;
    }

  return 1;
}

//----------------------------------------------------------------------------
int vtkXdmfWriter::RequestUpdateExtent(
  vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector,
  vtkInformationVector* vtkNotUsed(outputVector))
{
  double *inTimes = inputVector[0]->GetInformationObject(0)->Get(
      vtkStreamingDemandDrivenPipeline::TIME_STEPS());
  if (inTimes && this->WriteAllTimeSteps)
    {
    //TODO:? Add a user ivar to specify a particular time,
    //which is different from current time. Can do it by updating
    //to a particular time then writing without writealltimesteps,
    //but that is annoying.
    double timeReq = inTimes[this->CurrentTimeIndex];
    inputVector[0]->GetInformationObject(0)->Set(
        vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP(),
        timeReq);
    }

  return 1;
}

//----------------------------------------------------------------------------
int vtkXdmfWriter::RequestData(
  vtkInformation* request,
  vtkInformationVector** inputVector,
  vtkInformationVector* vtkNotUsed(outputVector))
{
  if (!this->DomainMemoryHandler)
    {
    //call Write instead of this directly. That does setup first, then calls this.
    return 1;
    }

  if (this->CurrentTimeIndex == 0 &&
      this->WriteAllTimeSteps &&
      this->NumberOfTimeSteps > 1)
    {
    // Tell the pipeline to start looping.
    request->Set(vtkStreamingDemandDrivenPipeline::CONTINUE_EXECUTING(), 1);

    // make a top level temporal grid just under domain
    if (this->TopTemporalGrid)
      {
      delete this->TopTemporalGrid;
      this->TopTemporalGrid = NULL;
      }

    XdmfGrid *tgrid = new XdmfGrid();
    tgrid->SetDeleteOnGridDelete(true);
    tgrid->SetGridType(XDMF_GRID_COLLECTION);
    tgrid->SetCollectionType(XDMF_GRID_COLLECTION_TEMPORAL);
    XdmfTopology *t = tgrid->GetTopology();
    t->SetTopologyType(XDMF_NOTOPOLOGY);
    XdmfGeometry *geo = tgrid->GetGeometry();
    geo->SetGeometryType(XDMF_GEOMETRY_NONE);

    this->DomainMemoryHandler->InsertGrid(tgrid);

    this->TopTemporalGrid = tgrid;
    }

  XdmfGrid *grid = new XdmfGrid();
  grid->SetDeleteOnGridDelete(true);
  if (this->TopTemporalGrid)
    {
    this->TopTemporalGrid->Insert(grid);
    }
  else
    {
    this->DomainMemoryHandler->InsertGrid(grid);
    }

  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkDataObject* input = inInfo->Get(vtkDataObject::DATA_OBJECT());
  vtkInformation *inDataInfo = input->GetInformation();
  if (inDataInfo->Has(vtkDataObject::DATA_TIME_STEP()))
    {
    //I am assuming we are not given a temporal data object and getting just one time.
    double dataT = input->GetInformation()->Get(vtkDataObject::DATA_TIME_STEP());
    //cerr << "Writing " << this->CurrentTimeIndex << " " << *dataT << endl;

    XdmfTime *xT = grid->GetTime();
    xT->SetDeleteOnGridDelete(true);
    xT->SetTimeType(XDMF_TIME_SINGLE);
    xT->SetValue(dataT);
    grid->Insert(xT);
    }

  this->WriteDataSet(input, grid);
  //delete grid; //domain takes care of it?

  this->CurrentTimeIndex++;
  if (this->CurrentTimeIndex >= this->NumberOfTimeSteps &&
      this->WriteAllTimeSteps)
    {
    // Tell the pipeline to stop looping.
    request->Remove(vtkStreamingDemandDrivenPipeline::CONTINUE_EXECUTING());
    this->CurrentTimeIndex = 0;
    //delete this->TopTemporalGrid; //domain takes care of it?
    this->TopTemporalGrid = NULL;
    }

  return 1;
}

//------------------------------------------------------------------------------
void vtkXdmfWriter::WriteDataSet(vtkDataObject *dobj, XdmfGrid *grid)
{
  //TODO:
  // respect parallelism
  if (!dobj)
    {
    //cerr << "Null DS, someone else will take care of it" << endl;
    return;
    }
  if (!grid)
    {
    cerr << "Something is wrong, grid should have already been created for " << dobj << endl;
    return;
    }

  vtkCompositeDataSet *cdobj = vtkCompositeDataSet::SafeDownCast(dobj);
  if (cdobj)//!dobj->IsTypeOf("vtkCompositeDataSet")) //TODO: Why doesn't IsTypeOf work?
    {
    this->WriteCompositeDataSet(cdobj, grid);
    return;
    }

  this->WriteAtomicDataSet(dobj, grid);
}

//------------------------------------------------------------------------------
void vtkXdmfWriter::WriteCompositeDataSet(vtkCompositeDataSet *dobj, XdmfGrid *grid)
{
  //cerr << "internal node " << dobj << " is a " << dobj->GetClassName() << endl;
  if (dobj->IsA("vtkMultiPieceDataSet"))
    {
    grid->SetGridType(XDMF_GRID_COLLECTION);
    grid->SetCollectionType(XDMF_GRID_COLLECTION_SPATIAL);
    }
  else
    {
    //fine for vtkMultiBlockDataSet
    //vtkHierarchicalBoxDataSet would be better served by a different xdmf tree type
    //vtkTemporalDataSet is internal to the VTK pipeline so I am ingnoring it
    grid->SetGridType(XDMF_GRID_TREE);
    }

  XdmfTopology *t = grid->GetTopology();
  t->SetTopologyType(XDMF_NOTOPOLOGY);
  XdmfGeometry *geo = grid->GetGeometry();
  geo->SetGeometryType(XDMF_GEOMETRY_NONE);

  vtkCompositeDataIterator* iter = dobj->NewIterator();
  iter->VisitOnlyLeavesOff();
  iter->TraverseSubTreeOff();
  iter->GoToFirstItem();
  while (!iter->IsDoneWithTraversal())
    {
    XdmfGrid *childsGrid = new XdmfGrid();
    childsGrid->SetDeleteOnGridDelete(true);
    grid->Insert(childsGrid);
    vtkDataObject* ds = iter->GetCurrentDataObject();
    this->WriteDataSet(ds, childsGrid);
    //delete childsGrid; //parent deletes children in Xdmf
    iter->GoToNextItem();
    }
  iter->Delete();

  return;
}
//------------------------------------------------------------------------------
void vtkXdmfWriter::CreateTopology(vtkDataSet *ds, XdmfGrid *grid, vtkIdType PDims[3], vtkIdType CDims[3], vtkIdType &PRank, vtkIdType &CRank, void *staticdata)
{
  //cerr << "Writing " << dobj << " a " << dobj->GetClassName() << endl;

  grid->SetGridType(XDMF_GRID_UNIFORM);

  const char *heavyName = NULL;
  std::string heavyDataSetName;
  if (this->HeavyDataFileName)
    {
    heavyDataSetName = std::string(this->HeavyDataFileName) + ":";
    if (this->HeavyDataGroupName)
      {
      heavyDataSetName = heavyDataSetName + HeavyDataGroupName + "/Topology";
      }
    heavyName = heavyDataSetName.c_str();
    }

  XdmfTopology *t = grid->GetTopology();

  //
  // If the topology is unchanged from last grid written, we can reuse the XML
  // and avoid writing any heavy data. We must still compute dimensions etc
  // otherwise the attribute arrays don't get initialized properly
  //
  bool reusing_topology = false;
  vtkXW2NodeHelp *staticnode = (vtkXW2NodeHelp*)staticdata;
  if (staticnode) {
    if (staticnode->staticFlag) {
      grid->Set("TopologyConstant", "True");
    }
    if (staticnode->DOM && staticnode->node) {
      XdmfXmlNode       staticTopo = staticnode->DOM->FindElement("Topology", 0, staticnode->node);
      XdmfConstString      xmltext = staticnode->DOM->Serialize(staticTopo->children);
      XdmfConstString   dimensions = staticnode->DOM->Get(staticTopo, "Dimensions");
      XdmfConstString topologyType = staticnode->DOM->Get(staticTopo, "TopologyType");
      //
      t->SetTopologyTypeFromString(topologyType);
      t->SetNumberOfElements(atoi(dimensions));
      t->SetDataXml(xmltext);
      reusing_topology = true;
      // @TODO : t->SetNodesPerElement(ppCell);
    }
  }

  //Topology
  switch (ds->GetDataObjectType()) {
  case VTK_STRUCTURED_POINTS:
  case VTK_IMAGE_DATA:
  case VTK_UNIFORM_GRID:
    {
    t->SetTopologyType(XDMF_3DCORECTMESH);
    t->SetLightDataLimit(this->LightDataLimit);
    vtkImageData *id = vtkImageData::SafeDownCast(ds);
    int wExtent[6];
    id->GetExtent(wExtent);
    XdmfInt64 Dims[3];
    Dims[2] = wExtent[1] - wExtent[0] + 1;
    Dims[1] = wExtent[3] - wExtent[2] + 1;
    Dims[0] = wExtent[5] - wExtent[4] + 1;
    XdmfDataDesc *dd = t->GetShapeDesc();
    dd->SetShape(3, Dims);
    //TODO: verify row/column major ordering

    PDims[0] = Dims[0];
    PDims[1] = Dims[1];
    PDims[2] = Dims[2];
    CDims[0] = Dims[0] - 1;
    CDims[1] = Dims[1] - 1;
    CDims[2] = Dims[2] - 1;
    }
    break;
  case VTK_RECTILINEAR_GRID:
    {
    t->SetTopologyType(XDMF_3DRECTMESH);
    vtkRectilinearGrid *rgrid = vtkRectilinearGrid::SafeDownCast(ds);
    int wExtent[6];
    rgrid->GetExtent(wExtent);
    XdmfInt64 Dims[3];
    Dims[2] = wExtent[1] - wExtent[0] + 1;
    Dims[1] = wExtent[3] - wExtent[2] + 1;
    Dims[0] = wExtent[5] - wExtent[4] + 1;
    XdmfDataDesc *dd = t->GetShapeDesc();
    dd->SetShape(3, Dims);
    //TODO: verify row/column major ordering

    PDims[0] = Dims[0];
    PDims[1] = Dims[1];
    PDims[2] = Dims[2];
    CDims[0] = Dims[0] - 1;
    CDims[1] = Dims[1] - 1;
    CDims[2] = Dims[2] - 1;
    }
    break;
  case VTK_STRUCTURED_GRID:
    {
    t->SetTopologyType(XDMF_3DSMESH);
    vtkStructuredGrid *sgrid = vtkStructuredGrid::SafeDownCast(ds);
    int wExtent[6];
    sgrid->GetExtent(wExtent);
    XdmfInt64 Dims[3];
    Dims[2] = wExtent[1] - wExtent[0] + 1;
    Dims[1] = wExtent[3] - wExtent[2] + 1;
    Dims[0] = wExtent[5] - wExtent[4] + 1;
    XdmfDataDesc *dd = t->GetShapeDesc();
    dd->SetShape(3, Dims);
    //TODO: verify row/column major ordering

    PDims[0] = Dims[0];
    PDims[1] = Dims[1];
    PDims[2] = Dims[2];
    CDims[0] = Dims[0] - 1;
    CDims[1] = Dims[1] - 1;
    CDims[2] = Dims[2] - 1;
    }
    break;
  case VTK_POLY_DATA:
  case VTK_UNSTRUCTURED_GRID:
    {
    PRank = 1;
    PDims[0] = ds->GetNumberOfPoints();
    CRank = 1;
    CDims[0] = ds->GetNumberOfCells();
    if (reusing_topology)
      {
      // don't need to do all this again
      // @TODO : t->SetNodesPerElement(ppCell);
      break;
      }
    vtkXdmfWriterInternal::MapOfCellTypes cellTypes;
    vtkXdmfWriterInternal::DetermineCellTypes(vtkPointSet::SafeDownCast(ds), cellTypes);

    //TODO: When is it beneficial to take advantage of a homogenous topology?
    //If no compelling reason not to used MIXED, then this should go away.
    //This special case code requires an in memory copy just to get rid of
    //each cell's preceeding number of points int.
    //If don't have to do that, could use pointer sharing,
    //and the extra code path is bound to cause problems eventually.
    if ( cellTypes.size() == 1 )
      {
      //cerr << "Homogeneous topology" << endl;
      t->SetNumberOfElements(ds->GetNumberOfCells());
      const vtkXdmfWriterInternal::CellType* ct = &cellTypes.begin()->first;
      vtkIdType ppCell = ct->NumPoints;
      switch(ct->VTKType)
        {
        case VTK_VERTEX :
        case VTK_POLY_VERTEX :
          t->SetTopologyType(XDMF_POLYVERTEX);
          break;
        case VTK_LINE :
        case VTK_POLY_LINE :
          t->SetTopologyType(XDMF_POLYLINE);
          t->SetNodesPerElement(ppCell);
          break;
        case VTK_TRIANGLE :
        case VTK_TRIANGLE_STRIP :
          t->SetTopologyType(XDMF_TRI);
          break;
        case VTK_POLYGON :
          t->SetTopologyType(XDMF_POLYGON);
          t->SetNodesPerElement(ppCell);
          break;
        case VTK_PIXEL :
        case VTK_QUAD :
          t->SetTopologyType(XDMF_QUAD);
          break;
        case VTK_TETRA :
          t->SetTopologyType(XDMF_TET);
          break;
        case VTK_VOXEL :
        case VTK_HEXAHEDRON :
          t->SetTopologyType(XDMF_HEX);
          break;
        case VTK_WEDGE :
          t->SetTopologyType(XDMF_WEDGE);
          break;
        case VTK_PYRAMID :
          t->SetTopologyType(XDMF_PYRAMID);
          break;
        case VTK_EMPTY_CELL :
        default :
          t->SetTopologyType(XDMF_NOTOPOLOGY);
          break;
        }
      XdmfArray *di = t->GetConnectivity();
      di->SetHeavyDataSetName(heavyName);
      if (VTK_SIZEOF_ID_TYPE==sizeof(XDMF_64_INT))
        {
        di->SetNumberType(XDMF_INT64_TYPE);
        }
      else
        {
        di->SetNumberType(XDMF_INT32_TYPE);
        }
      XdmfInt64 hDim[2];
      hDim[0] = ds->GetNumberOfCells();
      hDim[1] = ppCell;
      di->SetShape(2, hDim);
      vtkIdList* il = cellTypes[*ct].GetPointer();
      vtkIdList* cellPoints = vtkIdList::New();
      vtkIdType cvnt=0;
      for(vtkIdType i = 0 ; i < ds->GetNumberOfCells(); i++ )
        {
        ds->GetCellPoints(il->GetId(i), cellPoints);
        if ( ct->VTKType == VTK_VOXEL )
          {
          // Hack for VTK_VOXEL
          di->SetValue(cvnt++, cellPoints->GetId(0));
          di->SetValue(cvnt++, cellPoints->GetId(1));
          di->SetValue(cvnt++, cellPoints->GetId(3));
          di->SetValue(cvnt++, cellPoints->GetId(2));
          di->SetValue(cvnt++, cellPoints->GetId(4));
          di->SetValue(cvnt++, cellPoints->GetId(5));
          di->SetValue(cvnt++, cellPoints->GetId(7));
          di->SetValue(cvnt++, cellPoints->GetId(6));
          }
        else if ( ct->VTKType == VTK_PIXEL )
          {
          // Hack for VTK_PIXEL
          di->SetValue(cvnt++, cellPoints->GetId(0));
          di->SetValue(cvnt++, cellPoints->GetId(1));
          di->SetValue(cvnt++, cellPoints->GetId(3));
          di->SetValue(cvnt++, cellPoints->GetId(2));
          }
        else
          {
          for( vtkIdType j = 0 ; j < ppCell ; j++ )
            {
            di->SetValue(cvnt++, cellPoints->GetId(j));
            }
          }//pd has 4 arrays, so it is rarely homogeoneous
        }
      cellPoints->Delete();
      } //homogenous
    else
      {
      //cerr << "Nonhomogeneous topology" << endl;
      //Non Homogeneous, used mixed topology type to dump them all
      t->SetTopologyType(XDMF_MIXED);
      vtkIdType numCells = ds->GetNumberOfCells();
      t->SetNumberOfElements(numCells);
      XdmfArray *di = t->GetConnectivity();
      di->SetHeavyDataSetName(heavyName);
      if (VTK_SIZEOF_ID_TYPE==sizeof(XDMF_64_INT))
        {
        di->SetNumberType(XDMF_INT64_TYPE);
        }
      else
        {
        di->SetNumberType(XDMF_INT32_TYPE);
        }
      vtkIdTypeArray *da = vtkIdTypeArray::New();
      da->SetNumberOfComponents(1);
      vtkUnstructuredGrid *ugrid = vtkUnstructuredGrid::SafeDownCast(ds);
      const int ESTIMATE=4; /*celltype+numids+id0+id1 or celtype+id0+id1+id2*/
      if (ugrid)
        {
        da->Allocate(ugrid->GetCells()->GetSize()*ESTIMATE);
        }
      else
        {
        vtkPolyData *pd = vtkPolyData::SafeDownCast(ds);
        vtkIdType sizev = pd->GetVerts()->GetSize();
        vtkIdType sizel = pd->GetLines()->GetSize();
        vtkIdType sizep = pd->GetPolys()->GetSize();
        vtkIdType sizes = pd->GetStrips()->GetSize();
        vtkIdType rtotal = sizev+sizel+sizep+sizes;
        da->Allocate(rtotal*ESTIMATE);
        }

      vtkIdType cntr = 0;
      for (vtkIdType cid=0 ; cid < numCells; cid++)
        {
        vtkCell *cell = ds->GetCell(cid);
        vtkIdType cellType = ds->GetCellType(cid);
        vtkIdType numPts = cell->GetNumberOfPoints();
        switch(cellType)
          {
          case VTK_VERTEX :
          case VTK_POLY_VERTEX :
            da->InsertValue(cntr++, XDMF_POLYVERTEX);
            da->InsertValue(cntr++, numPts);
            break;
          case VTK_LINE :
          case VTK_POLY_LINE :
            da->InsertValue(cntr++, XDMF_POLYLINE);
            da->InsertValue(cntr++, cell->GetNumberOfPoints());
            break;
          //case VTK_TRIANGLE_STRIP :
          //TODO: Split tri strips into triangles
          //t->SetTopologyType(XDMF_TRI);
          //break;
          case VTK_TRIANGLE :
            da->InsertValue(cntr++, XDMF_TRI);
            break;
          case VTK_POLYGON :
            da->InsertValue(cntr++, XDMF_POLYGON);
            da->InsertValue(cntr++, cell->GetNumberOfPoints());
            break;
          case VTK_PIXEL :
          case VTK_QUAD :
            da->InsertValue(cntr++, XDMF_POLYGON);
            break;
          case VTK_TETRA :
            da->InsertValue(cntr++, XDMF_TET);
            break;
          case VTK_VOXEL :
            da->InsertValue(cntr++, XDMF_HEX);
            break;
          case VTK_HEXAHEDRON :
            da->InsertValue(cntr++, XDMF_HEX);
            break;
          case VTK_WEDGE :
            da->InsertValue(cntr++, XDMF_WEDGE);
          break;
          case VTK_PYRAMID :
            da->InsertValue(cntr++, XDMF_PYRAMID);
          break;
          default :
            da->InsertValue(cntr++,XDMF_NOTOPOLOGY);
            break;
          }
        if ( cellType == VTK_VOXEL )
          {
          // Hack for VTK_VOXEL
          da->InsertValue(cntr++, cell->GetPointId(0));
          da->InsertValue(cntr++, cell->GetPointId(1));
          da->InsertValue(cntr++, cell->GetPointId(3));
          da->InsertValue(cntr++, cell->GetPointId(2));
          da->InsertValue(cntr++, cell->GetPointId(4));
          da->InsertValue(cntr++, cell->GetPointId(5));
          da->InsertValue(cntr++, cell->GetPointId(7));
          da->InsertValue(cntr++, cell->GetPointId(6));
          }
        else if ( cellType == VTK_PIXEL )
          {
          // Hack for VTK_PIXEL
          da->InsertValue(cntr++, cell->GetPointId(0));
          da->InsertValue(cntr++, cell->GetPointId(1));
          da->InsertValue(cntr++, cell->GetPointId(3));
          da->InsertValue(cntr++, cell->GetPointId(2));
          }
        for (vtkIdType pid=0; pid < numPts; pid++)
          {
          da->InsertValue(cntr++, cell->GetPointId(pid));
          }
        }
      this->ConvertVToXArray(da, di, 1, &cntr, 2, heavyName);
      da->Delete();
      }
    }
    break;
  default:
    t->SetTopologyType(XDMF_NOTOPOLOGY);
    cerr << "Unrecognized dataset type" << endl;
  }
}

//----------------------------------------------------------------------------
void vtkXdmfWriter::CreateGeometry(vtkDataSet *ds, XdmfGrid *grid, void *staticdata)
{
  //Geometry
  XdmfGeometry *geo = grid->GetGeometry();
  geo->SetLightDataLimit(this->LightDataLimit);

  const char *heavyName = NULL;
  std::string heavyDataSetName;
  if (this->HeavyDataFileName)
    {
    heavyDataSetName = std::string(this->HeavyDataFileName) + ":";
    if (this->HeavyDataGroupName)
      {
      heavyDataSetName = heavyDataSetName + HeavyDataGroupName + "/Geometry";
      }
    heavyName = heavyDataSetName.c_str();
    }

  vtkXW2NodeHelp *staticnode = (vtkXW2NodeHelp*)staticdata;
  if (staticnode) {
    if (staticnode->staticFlag) {
      grid->Set("GeometryConstant", "True");
    }
    if (staticnode->DOM && staticnode->node) {
      XdmfXmlNode staticGeom = staticnode->DOM->FindElement("Geometry", 0, staticnode->node);
      XdmfConstString text = staticnode->DOM->Serialize(staticGeom->children);
      geo->SetDataXml(text);
      return;
    }
  }

  switch (ds->GetDataObjectType()) {
  case VTK_STRUCTURED_POINTS:
  case VTK_IMAGE_DATA:
  case VTK_UNIFORM_GRID:
    {
    geo->SetGeometryType(XDMF_GEOMETRY_ORIGIN_DXDYDZ);
    vtkImageData *id = vtkImageData::SafeDownCast(ds);
    double orig[3], spacing[3];
    id->GetOrigin(orig);
    double tmp = orig[2];
    orig[2] = orig[0];
    orig[0] = tmp;
    id->GetSpacing(spacing);
    tmp = spacing[2];
    spacing[2] = spacing[0];
    spacing[0] = tmp;
    geo->SetOrigin(orig);
    geo->SetDxDyDz(spacing);
    }
    break;
  case VTK_RECTILINEAR_GRID:
    {
    vtkIdType len;
    geo->SetGeometryType(XDMF_GEOMETRY_VXVYVZ);
    vtkRectilinearGrid *rgrid = vtkRectilinearGrid::SafeDownCast(ds);
    vtkDataArray *da;
    da = rgrid->GetXCoordinates();
    len = da->GetNumberOfTuples();
    XdmfArray *xdax = new XdmfArray;
    this->ConvertVToXArray(da, xdax, 1, &len, 0, heavyName);
    geo->SetVectorX(xdax, 1);
    da = rgrid->GetYCoordinates();
    len = da->GetNumberOfTuples();
    XdmfArray *xday = new XdmfArray;
    this->ConvertVToXArray(da, xday, 1, &len, 0, heavyName);
    geo->SetVectorY(xday, 1);
    da = rgrid->GetZCoordinates();
    len = da->GetNumberOfTuples();
    XdmfArray *xdaz = new XdmfArray;
    this->ConvertVToXArray(da, xdaz, 1, &len, 0, heavyName);
    geo->SetVectorZ(xdaz, 1);
    }
    break;
  case VTK_STRUCTURED_GRID:
  case VTK_POLY_DATA:
  case VTK_UNSTRUCTURED_GRID:
    {
    geo->SetGeometryType(XDMF_GEOMETRY_XYZ);
    vtkPointSet *pset = vtkPointSet::SafeDownCast(ds);
    vtkPoints *pts = pset->GetPoints();
    vtkDataArray *da = pts->GetData();
    XdmfArray *xda = geo->GetPoints();
    vtkIdType shape[2];
    shape[0] = da->GetNumberOfTuples();
    this->ConvertVToXArray(da, xda, 1, shape, 0, heavyName);
    geo->SetPoints(xda);
    }
    break;
  default:
    geo->SetGeometryType(XDMF_GEOMETRY_NONE);
    //TODO: Support non-canonical vtkDataSets (via a callout for extensibility)
    cerr << "Unrecognized dataset type" << endl;
  }
}
//------------------------------------------------------------------------------
void vtkXdmfWriter::WriteAtomicDataSet(vtkDataObject *dobj, XdmfGrid *grid)
{
  cerr << "Writing " << dobj << " a " << dobj->GetClassName() << endl;
  vtkDataSet *ds = vtkDataSet::SafeDownCast(dobj);
  if (!ds)
    {
    //TODO: Fill in non Vis data types
    cerr << "Can not convert " << dobj->GetClassName() << " to XDMF yet." << endl;
    return;
    }

  //Attributes
  vtkIdType FRank = 1;
  vtkIdType FDims[1];
  vtkIdType CRank = 3;
  vtkIdType CDims[3];
  vtkIdType PRank = 3;
  vtkIdType PDims[3];

  this->CreateTopology(ds, grid, PDims, CDims, PRank, CRank, NULL);
  this->CreateGeometry(ds, grid, NULL);

  FDims[0] = ds->GetFieldData()->GetNumberOfTuples();
  this->WriteArrays(ds->GetFieldData(),grid,XDMF_ATTRIBUTE_CENTER_GRID, FRank, FDims, "Field");
  this->WriteArrays(ds->GetCellData(), grid,XDMF_ATTRIBUTE_CENTER_CELL, CRank, CDims, "Cell");
  this->WriteArrays(ds->GetPointData(),grid,XDMF_ATTRIBUTE_CENTER_NODE, PRank, PDims, "Node");
}

//----------------------------------------------------------------------------
void vtkXdmfWriter::WriteArrays(vtkFieldData* fd, XdmfGrid *grid, int association,
                                 vtkIdType rank, vtkIdType *dims, const char *name)
{
  if (fd)
    {
    vtkDataSetAttributes *dsa = vtkDataSetAttributes::SafeDownCast(fd);

    const char *heavyName = NULL;
    std::string heavyDataSetName;
    if (this->HeavyDataFileName)
      {
      heavyDataSetName = std::string(this->HeavyDataFileName) + ":";
      if (this->HeavyDataGroupName)
        {
        heavyDataSetName = heavyDataSetName + std::string(HeavyDataGroupName) + "/" + name;
        }
      heavyName = heavyDataSetName.c_str();
      }

    //
    // Sort alphabetically to avoid potential bad ordering problems
    //
    std::vector<std::string> AttributeNames;
    for (int i=0; i<fd->GetNumberOfArrays(); i++) {
      vtkDataArray *scalars = fd->GetArray(i);
      AttributeNames.push_back(scalars->GetName());
    }
    std::sort(AttributeNames.begin(), AttributeNames.end());

    for (unsigned int i = 0; i < AttributeNames.size(); i++)
      {
      vtkDataArray *da = fd->GetArray(AttributeNames[i].c_str());
      if (!da)
        {
        //TODO: Dump non numeric arrays too
        cerr << "xdmfwriter can not convert non-numeric arrays yet." << endl;
        continue;
        }

      XdmfAttribute *attr = new XdmfAttribute;
      attr->SetLightDataLimit(this->LightDataLimit);
      attr->SetDeleteOnGridDelete(true);
      if (da->GetName())
        {
        attr->SetName(da->GetName());
        }
      else
        {
        attr->SetName("ANONYMOUS");
        }
      attr->SetAttributeCenter(association);

      int attributeType = 0;
      if (dsa)
        {
        attributeType = dsa->IsArrayAnAttribute(i);
        switch (attributeType) {
        case vtkDataSetAttributes::SCALARS:
          attributeType = XDMF_ATTRIBUTE_TYPE_SCALAR; //TODO: Is XDMF ok with 3 component(RGB) active scalars?
          break;
        case vtkDataSetAttributes::VECTORS:
          attributeType = XDMF_ATTRIBUTE_TYPE_VECTOR;
          break;
        case vtkDataSetAttributes::GLOBALIDS:
          attributeType = XDMF_ATTRIBUTE_TYPE_GLOBALID;
          break;
        case vtkDataSetAttributes::TENSORS: //TODO: vtk tensors are 9 component, xdmf tensors are 6?
        case vtkDataSetAttributes::NORMALS: //TODO: mark as vectors?
        case vtkDataSetAttributes::TCOORDS: //TODO: mark as vectors?
        case vtkDataSetAttributes::PEDIGREEIDS: //TODO: ? type is variable
        default:
          attributeType = 0;
          break;
        }
        }

      if (attributeType != 0)
        {
        attr->SetActive(1);
        attr->SetAttributeType(attributeType);
        }
      else
        {
        //vtk doesn't mark it as a special array, use width to tell xdmf what to call it
        if ( da->GetNumberOfComponents() == 1 )
          {
          attr->SetAttributeType(XDMF_ATTRIBUTE_TYPE_SCALAR);
          }
        else if ( da->GetNumberOfComponents() == 3 )
          {
          attr->SetAttributeType(XDMF_ATTRIBUTE_TYPE_VECTOR);
          }
        else if ( da->GetNumberOfComponents() == 6 )
          {
          attr->SetAttributeType(XDMF_ATTRIBUTE_TYPE_TENSOR);
          }
        }

      XdmfArray *xda = attr->GetValues();
      this->ConvertVToXArray(da, xda, rank, dims, 0, heavyName);
      attr->SetValues(xda);
      grid->Insert(attr);
      }
    }
}

//------------------------------------------------------------------------------
void vtkXdmfWriter::ConvertVToXArray(vtkDataArray *vda,
                                      XdmfArray *xda, vtkIdType rank,
                                      vtkIdType *dims, int allocStrategy,
                                      const char *heavyprefix)
{
  XdmfInt32 lRank = rank;
  XdmfInt64 *lDims = new XdmfInt64[rank+1];
  for (vtkIdType i = 0; i < rank; i++)
    {
    lDims[i] = dims[i];
    }
  vtkIdType nc = vda->GetNumberOfComponents();
  //add additional dimension to the xdmf array to match the vtk arrays width,
  //ex coordinate arrays have xyz, so add [3]
  if (nc != 1)
    {
    lDims[rank]=nc;
    lRank+=1;
    }

  switch (vda->GetDataType())
    {
    case VTK_DOUBLE:
      xda->SetNumberType(XDMF_FLOAT64_TYPE);
      break;
    case VTK_FLOAT:
      xda->SetNumberType(XDMF_FLOAT32_TYPE);
      break;
    case VTK_ID_TYPE:
      xda->SetNumberType((VTK_SIZEOF_ID_TYPE==sizeof(XDMF_64_INT)?XDMF_INT64_TYPE:XDMF_INT32_TYPE));
      break;
    case VTK_LONG:
      xda->SetNumberType(XDMF_INT64_TYPE);
      break;
    case VTK_INT:
      xda->SetNumberType(XDMF_INT32_TYPE);
      break;
    case VTK_UNSIGNED_INT:
      xda->SetNumberType(XDMF_UINT32_TYPE);
      break;
    case VTK_SHORT:
      xda->SetNumberType(XDMF_INT16_TYPE);
      break;
    case VTK_UNSIGNED_SHORT:
      xda->SetNumberType(XDMF_INT16_TYPE);
      break;
    case VTK_CHAR:
    case VTK_SIGNED_CHAR:
      xda->SetNumberType(XDMF_INT8_TYPE); //TODO: Do we ever want unicode?
      break;
    case VTK_UNSIGNED_CHAR:
      xda->SetNumberType(XDMF_UINT8_TYPE);
      break;
    case VTK_LONG_LONG:
    case VTK_UNSIGNED_LONG_LONG:
    case VTK___INT64:
    case VTK_UNSIGNED___INT64:
    case VTK_UNSIGNED_LONG:
    case VTK_STRING:
      {
      xda->SetNumberType(XDMF_UNKNOWN_TYPE);
      break;
      }
    }

  if (heavyprefix) {
    std::string dsname = std::string(heavyprefix) + "/" + std::string(vda->GetName());
    xda->SetHeavyDataSetName(dsname.c_str());
  }

  //TODO: if we can make xdmf write out immediately, then wouldn't have to keep around
  //arrays when working with temporal data
  if ((allocStrategy==0 && !this->TopTemporalGrid) || allocStrategy==1)
    {
    //Do not let xdmf allocate its own buffer. xdmf just borrows vtk's and doesn't double mem size.
    xda->SetAllowAllocate(0);
    xda->SetShape(lRank, lDims);
    xda->SetDataPointer(vda->GetVoidPointer(0));
    }
  else //(allocStrategy==0 && this->TopTemporalGrid) || allocStrategy==2)
    {
    //Unfortunately data doesn't stick around with temporal updates, which is exactly when you want it most.
    xda->SetAllowAllocate(1);
    xda->SetShape(lRank, lDims);
    memcpy(xda->GetDataPointer(), vda->GetVoidPointer(0),
           vda->GetNumberOfTuples()*
           vda->GetNumberOfComponents()*
           vda->GetElementComponentSize());
    }

  delete[] lDims;
}
