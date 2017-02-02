/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTRUCHASReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkTRUCHASReader.h"

#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkDataArraySelection.h"
#include "vtkDoubleArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkIntArray.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkUnstructuredGrid.h"

#include "vtk_hdf5.h"

#include <map>
#include <set>
#include <sstream>

//----------------------------------------------------------------------------
class vtkTRUCHASReader::Internal {
public:
  //----------------------------------------------------------------------------
  Internal()
  {
    this->FileIndx = -1;
    this->FileName = "";

    this->TimeFileIndx = this->FileIndx;
    this->Steps = NULL;

    this->GeometryFileIndx = this->FileIndx;
    this->Points = NULL;

    this->BlockFileIndx = this->FileIndx;
    this->bids_out = NULL;
    this->VTKBlockFileIndx = this->FileIndx;
    this->BlockChoiceTime = 0;

    this->TopoFileIndx = this->FileIndx;
    this->cells_out = NULL;
    this->totalNumCells = 0;

    this->ArrayNameFileIndx = this->FileIndx;
    this->PointData = vtkPointData::New();
  };

  //----------------------------------------------------------------------------
  ~Internal()
  {
    this->CloseFile();
    delete[] this->Steps;
    if (this->Points)
    {
      this->Points->Delete();
    }
    this->ReleaseGrids();
    delete[] this->bids_out;
    delete[] this->cells_out;
  };

  //----------------------------------------------------------------------------
  void CloseFile()
  {
    if (this->FileIndx != -1)
    {
      H5Fclose(this->FileIndx);
    }
    this->FileName = "";
    this->FileIndx = -1;
    this->TimeFileIndx = -1;
    this->GeometryFileIndx = -1;
    this->BlockFileIndx = -1;
    this->VTKBlockFileIndx = -1;
    this->BlockChoiceTime = 0;
    this->TopoFileIndx = -1;
    this->ArrayNameFileIndx = -1;
    this->PointData->Delete();
  }

  //----------------------------------------------------------------------------
  hid_t OpenFile(char *_FileName)
  {
    if (this->FileName.compare(_FileName)!=0)
    {
      this->CloseFile();
      if (_FileName != NULL)
      {
        this->FileName = std::string(_FileName);
        this->FileIndx = H5Fopen(_FileName, H5F_ACC_RDONLY, H5P_DEFAULT );
        this->PointData = vtkPointData::New();
      }
    }
    return this->FileIndx;
  }

  //----------------------------------------------------------------------------
  void ReadTimeSteps(unsigned int &numSteps, double **outSteps)
  {
    if (this->TimeFileIndx == this->FileIndx)
    {
      *outSteps = this->Steps;
      numSteps = static_cast<unsigned int>(this->tmap.size());
      return;
    }

    this->TimeFileIndx = this->FileIndx;
    this->tmap.clear();
    delete[] this->Steps;

    #define MAX_NAME 1024
    char memb_name[MAX_NAME];
    hid_t gid = H5Gopen
      (this->FileIndx, "/Simulations/MAIN/Series Data", H5P_DEFAULT);
    hsize_t nobj;
    ssize_t len;
    int otype;
    herr_t status = H5Gget_num_objs(gid, &nobj);
    if (status < 0)
    {
      H5Gclose(gid);
      return;
    }
    for (unsigned int i = 0; i < nobj; i++)
    {
      len = H5Gget_objname_by_idx(gid, (hsize_t)i,
                                  memb_name, (size_t)MAX_NAME );
      if (len<=0)
      {
        continue;
      }
      otype = H5Gget_objtype_by_idx(gid, (size_t)i );
      if (otype == H5G_GROUP)
      {
        hid_t gid2 = H5Gopen(gid, memb_name, H5P_DEFAULT);
        hid_t att = H5Aopen(gid2, "time", H5P_DEFAULT);
        double t;
        status = H5Aread(att, H5T_NATIVE_DOUBLE, &t);
        this->tmap[t] = std::string(memb_name);
        H5Aclose(att);
        H5Gclose(gid2);
      }
    }
    H5Gclose(gid);

    numSteps = static_cast<unsigned int>(this->tmap.size());
    this->Steps = new double[numSteps];
    std::map<double, std::string>::iterator it = this->tmap.begin();
    int i = 0;
    while (it != this->tmap.end())
    {
      this->Steps[i] = it->first;
      ++it;
      i++;
    }
    *outSteps = this->Steps;
  }

  //----------------------------------------------------------------------------
  vtkPoints *ReadGeometry()
  {
    if (this->GeometryFileIndx == this->FileIndx)
    {
      return this->Points;
    }
    this->GeometryFileIndx = this->FileIndx;
    if (this->Points)
    {
      this->Points->Delete();
    }

    //coordinates
    hid_t coordinates = H5Dopen
      (this->FileIndx, "/Meshes/DEFAULT/Nodal Coordinates", H5P_DEFAULT );
    if( coordinates < 0 )
    {
      return NULL;
    }

    hsize_t dims[2];
    hid_t dataspace = H5Dget_space(coordinates);
    H5Sget_simple_extent_ndims(dataspace);
    herr_t status = H5Sget_simple_extent_dims(dataspace, dims, NULL);
    if( status < 0 )
    {
      return NULL;
    }

    double **points_out = new double *[dims[0]];
    points_out[0] = new double[dims[0]*dims[1]];
    for (unsigned int i = 1; i < dims[0]; i++)
    {
      points_out[i] = points_out[0]+i*dims[1];
    }

    status = H5Dread(coordinates, H5T_NATIVE_DOUBLE, H5S_ALL, H5S_ALL,
                     H5P_DEFAULT, &points_out[0][0]);
    if( status < 0 )
    {
      return NULL;
    }

    this->Points = vtkPoints::New();
    //TODO: pass in pointer directly instead of this loop
    double *ptr = points_out[0];
    unsigned int i;
    for (i = 0; i < dims[0]; i++)
    {
      this->Points->InsertNextPoint(*(ptr+0),*(ptr+1),*(ptr+2));
      ptr+=3;
    }
    H5Dclose(coordinates);
    H5Sclose(dataspace);

    delete[] points_out[0];
    delete[] points_out;

    return this->Points;
  }

  //----------------------------------------------------------------------------
  static std::string to_string(int number)
  {
    //TODO: use c++11's to_string when c++11 is min required
    std::ostringstream oss;
    oss << number;
    return oss.str();
  }

  //----------------------------------------------------------------------------
  bool ReadAvailableBlocks(vtkTRUCHASReader *self)
  {
    if (this->BlockFileIndx == this->FileIndx)
    {
      return true;
    }
    this->BlockFileIndx = this->FileIndx;

    hid_t blockids = H5Dopen
      (this->FileIndx, "/Simulations/MAIN/Non-series Data/BLOCKID", H5P_DEFAULT );
    if( blockids < 0 )
    {
      return false;
    }

    hid_t dataspace;
    hsize_t dims[2];
    herr_t status;
    dataspace = H5Dget_space(blockids);
    H5Sget_simple_extent_ndims(dataspace);
    status = H5Sget_simple_extent_dims(dataspace, dims, NULL);
    if( status < 0 )
    {
      return false;
    }
    dims[1] = 1;
    H5Sclose(dataspace);

    //TODO: this is ugly
    delete[] this->bids_out;
    this->bids_out = new int *[dims[0]];
    bids_out[0] = new int[dims[0]*dims[1]];
    for (unsigned int i = 1; i < dims[0]; i++)
    {
      bids_out[i] = bids_out[0]+i*dims[1];
    }

    status = H5Dread(blockids, H5T_NATIVE_INT, H5S_ALL, H5S_ALL,
                     H5P_DEFAULT, &bids_out[0][0]);
    if( status < 0 )
    {
      return false;
    }
    //bids_out is a list of possible block ids
    //every cell in the data says which block it is part of

    //Reduce to a unique list of blocks
    std::set<int> unique_blocks;
    int *block = bids_out[0];
    for (unsigned int i = 0; i < dims[0]; i++)
    {
      unique_blocks.insert(block[i]);
    }

    //now update some bookkeeping information
    this->blockmap.clear();
    this->mapblock.clear();
    int i = 0;
    for (std::set<int>::iterator it = unique_blocks.begin();
         it!= unique_blocks.end();
         ++it)
    {
      //keep record of the "name" of the block for GUI to choose from
      self->BlockChoices->AddArray(to_string(*it).c_str());
      //keep track of location to block id
      this->blockmap.push_back(*it);
      //keep track of block id to location
      this->mapblock[*it] = i;
      i++;
    }

    H5Dclose(blockids);
    return true;
  }

  //----------------------------------------------------------------------------
  bool ReadTopology()
  {
    if (this->TopoFileIndx == this->FileIndx)
    {
      return true;
    }
    this->TopoFileIndx = this->FileIndx;

    //read the cell connectivity
    hid_t elements = H5Dopen
      (this->FileIndx, "/Meshes/DEFAULT/Element Connectivity", H5P_DEFAULT );
    if( elements < 0 )
    {
      return false;
    }

    hid_t dataspace;
    hsize_t dims[2];
    herr_t status;

    dataspace = H5Dget_space(elements);
    H5Sget_simple_extent_ndims(dataspace);
    status  = H5Sget_simple_extent_dims(dataspace, dims, NULL);
    if( status < 0 )
    {
      H5Dclose(elements);
      return false;
    }

    delete[] this->cells_out;
    this->cells_out = new int *[dims[0]];
    cells_out[0] = new int[dims[0]*dims[1]];
    for (unsigned int i = 1; i < dims[0]; i++)
    {
      cells_out[i] = cells_out[0]+i*dims[1];
    }
    this->totalNumCells = dims[0];

    status = H5Dread(elements, H5T_NATIVE_INT, H5S_ALL, H5S_ALL,
                     H5P_DEFAULT, &cells_out[0][0]);
    if( status < 0 )
    {
      H5Dclose(elements);
      return false;
    }

    H5Dclose(elements);


    //read the part ids if present
    int partnum=1;
    bool done = false;
    while (!done)
    {
      std::string nextpartname = "/Simulations/MAIN/Non-series Data/part" +
        to_string(partnum);
      htri_t exists = H5Lexists(this->FileIndx, nextpartname.c_str(),
                                H5P_DEFAULT);
      if( !exists )
      {
        //no (more) moving part info
        done = true;
      }
      else
      {
        hid_t nextpart = H5Dopen
        (this->FileIndx, nextpartname.c_str(), H5P_DEFAULT );

        dataspace = H5Dget_space(nextpart);
        H5Sget_simple_extent_ndims(dataspace);
        status  = H5Sget_simple_extent_dims(dataspace, dims, NULL);
        if( status < 0 )
        {
          H5Dclose(nextpart);
          return false;
        }

        int *blocksinpart = new int[dims[0]];
        status = H5Dread(nextpart, H5T_NATIVE_INT, H5S_ALL, H5S_ALL,
                         H5P_DEFAULT, blocksinpart);
        if( status < 0 )
        {
          H5Dclose(nextpart);
          delete[] blocksinpart;
          return false;
        }

        this->part_to_blocks[partnum-1] = std::vector<int>(dims[0]);
        for (unsigned int i = 0; i < dims[0]; i++)
        {
          this->part_to_blocks[partnum-1][i] = blocksinpart[i];
        }
        delete[] blocksinpart;

        H5Dclose(nextpart);

        partnum++;
      }
    }
    return true;
  }

  //----------------------------------------------------------------------------
  void ReleaseGrids()
  {
    for (unsigned i = 0; i < this->grid.size(); i++)
    {
      if (this->grid[i])
      {
        this->grid[i]->Delete();
      }
    }
    this->grid.clear();
  }

  //----------------------------------------------------------------------------
  bool MakeVTKBlocks(vtkTRUCHASReader *self)
  {
    if (this->VTKBlockFileIndx == this->FileIndx &&
        this->BlockChoiceTime == self->BlockChoices->GetMTime()
        )
    {
      return true;
    }
    this->VTKBlockFileIndx = this->FileIndx;
    this->BlockChoiceTime = self->BlockChoices->GetMTime();

    this->ReleaseGrids();

    //topology
    int ret = this->ReadTopology();
    if (!ret)
    {
      return false;
    }

    unsigned int totalNumBlocks = static_cast<unsigned int>(this->blockmap.size());
    this->grid.resize(totalNumBlocks);
    for(unsigned int b = 0; b < totalNumBlocks; b++)
    {
      if (self->BlockChoices->GetArraySetting(b) != 0)
      {
        this->grid[b] = vtkUnstructuredGrid::New();
        this->grid[b]->Initialize();
        this->grid[b]->SetPoints(this->Points);
        this->grid[b]->Allocate();
      }
      else
      {
        this->grid[b] = NULL;
      }
    }

    int *blockptr = this->bids_out[0];
    int *cptr = this->cells_out[0];
    for (unsigned int c = 0; c < this->totalNumCells; c++)
    {
      int gblockid = *blockptr;
      int blockidx = this->mapblock[gblockid];
      if (self->BlockChoices->GetArraySetting(blockidx) != 0)
      {
        //from truchas's danu_xdmf_mesh.c
        int i;
        vtkIdType list[8] =
          {
            *(cptr+0),*(cptr+1),*(cptr+2),*(cptr+3),
            *(cptr+4),*(cptr+5),*(cptr+6),*(cptr+7)
          };
        if (list[0] == list[1])
        { /* tet element */
          //cerr << "T" << endl;
          for (i=0; i < 4; i++)
          {
            list[i] = list[1+i]-1;
          }
          this->grid[blockidx]->InsertNextCell(VTK_TETRA, 4, &list[0]);
        }
        else if (list[4] == list[5])
        { /* pyramid element */
          //cerr << "P" << endl;
          for (i=0; i < 5; i++)
          {
            list[i] = list[i]-1;
          }
          this->grid[blockidx]->InsertNextCell(VTK_PYRAMID, 5, &list[0]);
        }
        else if (list[5] == list[6])
        { /* wedge element */
          //cerr << "W" << endl;
          i = list[1]; list[1] = list[3]; list[3] = i;  /* swap 1 and 3 */
          i = list[2]; list[2] = list[4]; list[4] = i;  /* swap 2 and 4 */
          /* Convert from Exodus ordering to VTK / XDMF ordering */
          i = list[1]; list[1] = list[2]; list[2] = i;  /* swap 1 and 2 */
          i = list[4]; list[4] = list[5]; list[5] = i;  /* swap 4 and 5 */
          for (i=0; i < 6; i++)
          {
            list[i] = list[i]-1;
          }
          this->grid[blockidx]->InsertNextCell(VTK_WEDGE, 6, &list[0]);
        }
        else
        { /* hex element */
          //cerr << "H" << endl;
          for (i=0; i < 8; i++)
          {
            list[i] = list[i]-1;
          }
          this->grid[blockidx]->InsertNextCell(VTK_HEXAHEDRON, 8, &list[0]);
        }
      }
      blockptr++;
      cptr+=8;
    }

    for(unsigned int b = 0; b < totalNumBlocks; b++)
    {
      if (self->BlockChoices->GetArraySetting(b) != 0)
      {
        this->grid[b]->Squeeze();
      }
    }

    return true;
  }

  //----------------------------------------------------------------------------
  bool MoveVTKBlocks(vtkTRUCHASReader *self, hid_t now_gid)
  {

    if (this->part_to_blocks.size() == 0)
    {
      return true;
    }

    for (unsigned int i = 0; i < this->part_to_blocks.size(); i++)
    {
      std::string nextpartname = "translate_part" + to_string(i+1);
      hid_t att = H5Aopen(now_gid, nextpartname.c_str(), H5P_DEFAULT);
      double transform[3];
      H5Aread(att, H5T_NATIVE_DOUBLE, &transform);

      bool needed = false;
      for (unsigned int b = 0; b < this->part_to_blocks[i].size(); b++)
      {
        int gblockid = this->part_to_blocks[i][b];
        int blockidx = this->mapblock[gblockid];
        if (self->BlockChoices->GetArraySetting(blockidx) != 0)
        {
          needed = true;
          break;
        }
      }

      if (needed)
      {
        double nextpt[3];
        vtkPoints *pts = vtkPoints::New();
        unsigned int npts = this->Points->GetNumberOfPoints();
        pts->SetNumberOfPoints(npts);
        for (unsigned int p = 0; p < npts; p++)
        {
          this->Points->GetPoint(p, nextpt);
          nextpt[0] = nextpt[0]+transform[0];
          nextpt[1] = nextpt[1]+transform[1];
          nextpt[2] = nextpt[2]+transform[2];
          pts->SetPoint(p, nextpt);
        }

        for (unsigned int b = 0; b < this->part_to_blocks[i].size(); b++)
        {
          int gblockid = this->part_to_blocks[i][b];
          int blockidx = this->mapblock[gblockid];
          if (self->BlockChoices->GetArraySetting(blockidx) == 0)
          {
            continue;
          }

          this->grid[blockidx]->SetPoints(pts);
        }
        pts->Delete();
      }
      H5Aclose(att);
    }

    return true;
  }

  //----------------------------------------------------------------------------
  bool ReadArrayNames(vtkTRUCHASReader *self)
  {
    if (this->ArrayNameFileIndx == this->FileIndx)
    {
      return true;
    }
    this->ArrayNameFileIndx = this->FileIndx;

    std::string time_group_name =
      "/Simulations/MAIN/Series Data/Series 1";

    this->array_names.clear();
    this->array_isFloat.clear();
    self->PointArrayChoices->RemoveAllArrays();
    self->CellArrayChoices->RemoveAllArrays();

    hid_t now_gid = H5Gopen
      (this->FileIndx, time_group_name.c_str(), H5P_DEFAULT);
    hsize_t nobj;
    herr_t status;
    status = H5Gget_num_objs(now_gid, &nobj);
    if( status < 0 )
    {
      return false;
    }

    #define MAX_NAME 1024
    char array_name[MAX_NAME];
    int otype;
    for (unsigned int i = 0; i < nobj; i++)
    {
      otype =  H5Gget_objtype_by_idx(now_gid, (size_t)i );
      if (otype != H5G_DATASET)
      {
        continue;
      }

      H5Gget_objname_by_idx(now_gid, (hsize_t)i,
                            array_name, (size_t)MAX_NAME );
      hid_t did = H5Dopen(now_gid, array_name, H5P_DEFAULT);

      //reject bookkeeping arrays
      if (!H5Aexists_by_name
          (now_gid, array_name, "FIELDTYPE", H5P_DEFAULT))
      {
        H5Dclose(did);
        continue;
      }
      hid_t attr = H5Aopen(did, "FIELDTYPE", H5P_DEFAULT);
      hid_t atype = H5Aget_type(attr);
      hid_t atype_mem = H5Tget_native_type(atype, H5T_DIR_ASCEND);
      char alignment[80];
      H5Aread(attr, atype_mem, alignment);
      H5Aclose(attr);

      int align = -1;
      if (!strcmp(alignment, "CELL"))
      {
        align = 0;
      }
      if (!strcmp(alignment, "NODE"))
      {
        align = 1;
      }
      if (align < 0 || align > 1)
      {
        //probably neutral-neutral ;)
        H5Dclose(did);
        continue;
      }

      hid_t datatype = H5Dget_type(did);
      this->array_isFloat[array_name] = true;
      if (!H5Tequal(datatype, H5T_IEEE_F64LE))
      {
        this->array_isFloat[array_name] = false;
      }
      H5Tclose(datatype);
      this->array_names[array_name] = align;
      if (align == 0)
      {
        self->CellArrayChoices->AddArray(array_name);
      }
      else
      {
        self->PointArrayChoices->AddArray(array_name);
      }

      H5Dclose(did);
    }

    H5Gclose(now_gid);
    return true;
  }

  std::map<double, std::string> tmap;
  int **bids_out;
  std::vector<int> blockmap; //location to blockid
  std::map<int, int> mapblock; //blockid to location
  std::vector< vtkUnstructuredGrid * > grid;
  vtkPoints *Points;

  int **cells_out;
  unsigned int totalNumCells;
  std::map<std::string, int> array_names;
  std::map<std::string, bool> array_isFloat;
  vtkPointData *PointData;

  std::map<int, std::vector<int> > part_to_blocks; //part id to list of blocks

private:
  hid_t FileIndx;
  std::string FileName;

  hid_t TimeFileIndx;
  double *Steps;

  hid_t GeometryFileIndx;

  hid_t BlockFileIndx;

  hid_t TopoFileIndx;

  hid_t VTKBlockFileIndx;
  vtkMTimeType BlockChoiceTime;

  hid_t ArrayNameFileIndx;

};

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkTRUCHASReader);

//----------------------------------------------------------------------------
vtkTRUCHASReader::vtkTRUCHASReader()
  : vtkMultiBlockDataSetAlgorithm()
{
  this->Internals = new vtkTRUCHASReader::Internal;
  this->FileName = NULL;
  this->BlockChoices = vtkDataArraySelection::New();
  this->PointArrayChoices = vtkDataArraySelection::New();
  this->CellArrayChoices = vtkDataArraySelection::New();


  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(1);
}

//----------------------------------------------------------------------------
vtkTRUCHASReader::~vtkTRUCHASReader()
{
  delete this->Internals;

  delete [] this->FileName;
  this->BlockChoices->Delete();
  this->PointArrayChoices->Delete();
  this->CellArrayChoices->Delete();
}

//----------------------------------------------------------------------------
int vtkTRUCHASReader::RequestInformation(
  vtkInformation* reqInfo,
  vtkInformationVector** inVector,
  vtkInformationVector* outVector
  )
{
  if(!this->Superclass::RequestInformation(reqInfo,inVector,outVector))
  {
    return 0;
  }

  hid_t fileIndx = this->Internals->OpenFile(this->FileName);
  if( fileIndx < 0 )
  {
    return 0;
  }

  //tell the caller that I can provide time varying data and
  //tell it what range of times I can deal with
  double *steps;
  double tRange[2];
  unsigned int numSteps;
  this->Internals->ReadTimeSteps(numSteps, &steps);
  tRange[0] = steps[0];
  tRange[1] = steps[numSteps-1];
  vtkInformation *info=outVector->GetInformationObject(0);
  info->Set(vtkStreamingDemandDrivenPipeline::TIME_RANGE(),
            tRange,
            2);
  info->Set(vtkStreamingDemandDrivenPipeline::TIME_STEPS(),
            steps,
            numSteps);


  //what blocks are available to read?
  if (!this->Internals->ReadAvailableBlocks(this))
  {
    return 0;
  }

  //what arrays are available to read?
  if (!this->Internals->ReadArrayNames(this))
  {
    return 0;
  }

  return 1;
}

//----------------------------------------------------------------------------
int vtkTRUCHASReader::RequestData(
  vtkInformation *,
  vtkInformationVector **,
  vtkInformationVector *outputVector)
{
  vtkInformation *outInfo = outputVector->GetInformationObject(0);
  vtkMultiBlockDataSet *output = vtkMultiBlockDataSet::SafeDownCast(
    outInfo->Get(vtkMultiBlockDataSet::DATA_OBJECT()));

  hid_t fileIndx = this->Internals->OpenFile(this->FileName);
  if( fileIndx < 0 )
  {
    return 0;
  }

  vtkDebugMacro(<<"Reading truchas unstructured grid...");

  hsize_t dims[2];
  hid_t dataspace;
  herr_t status;
  bool ret;

  //coordinates
  vtkPoints *pts = this->Internals->ReadGeometry();
  if (!pts)
  {
    return 0;
  }

  //blockids
  ret = this->Internals->ReadAvailableBlocks(this);
  if (!ret)
  {
    return 0;
  }

  //block containers up to topology
  ret = this->Internals->MakeVTKBlocks(this);
  if (!ret)
  {
    return 0;
  }

  std::vector< vtkUnstructuredGrid * > &grid = this->Internals->grid;
  unsigned int totalNumBlocks = static_cast<unsigned int>(grid.size());
  output->SetNumberOfBlocks(totalNumBlocks);
  unsigned int totalNumCells = this->Internals->totalNumCells;
  unsigned int totalNumPoints = this->Internals->Points->GetNumberOfPoints();

  //what time to produce data for?
  double reqTime = 0.0;
  double reqTS(0);
  if (outInfo->Has(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP()))
  {
    reqTS = outInfo->Get
      (vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP());
    reqTime = reqTS;
  }
  std::map<double, std::string>::iterator tit
    = this->Internals->tmap.begin();

  for (std::map<double, std::string>::iterator ttit =
         this->Internals->tmap.begin();
       ttit != this->Internals->tmap.end();)
  {
    if (ttit->first < reqTime)
    {
      tit = ttit;
    }
    ++ttit;
  }
  //open the corresponding section in the hdf5 file to get arrays from
  std::string time_group_name ="/Simulations/MAIN/Series Data/" + tit->second;
  hid_t now_gid = H5Gopen
    (fileIndx, time_group_name.c_str(), H5P_DEFAULT);
  hsize_t nobj;
  status = H5Gget_num_objs(now_gid, &nobj);
  if( status < 0 )
  {
    return 0;
  }

  //save time by determining what blocks are enabled/disabled once
  std::map<int, bool> gBlockToEnabled;
  for (unsigned b = 0; b < totalNumBlocks; b++)
  {
    int gblockid = this->Internals->blockmap[b];
    if (this->BlockChoices->GetArraySetting(b) != 0)
    {
      gBlockToEnabled[gblockid] = true;
    }
    else
    {
      gBlockToEnabled[gblockid] = false;
    }
  }

  //move any moving blocks accordingly
  ret = this->Internals->MoveVTKBlocks(this, now_gid);
  if (!ret)
  {
    H5Gclose(now_gid);
    return 0;
  }

  #define MAX_NAME 1024
  char array_name[MAX_NAME];
  std::map<std::string, int>::iterator nit;
  for (nit = this->Internals->array_names.begin();
       nit != this->Internals->array_names.end();
       ++nit)
  {
    std::string name = nit->first;
    int align = nit->second;
    if (align==0)
    {
      if (this->CellArrayChoices->GetArraySetting(name.c_str()) == 0)
      {
        //prevent stale (deselected) arrays from sticking around
        for (unsigned b = 0; b < totalNumBlocks; b++)
        {
          if (grid[b])
          {
            grid[b]->GetCellData()->RemoveArray(name.c_str());
          }
        }
        continue;
      }
    }
    else
    {
      if (this->PointArrayChoices->GetArraySetting(name.c_str()) == 0)
      {
        //prevent stale (deselected) arrays from sticking around
        for (unsigned b = 0; b < totalNumBlocks; b++)
        {
          if (grid[b])
          {
            grid[b]->GetPointData()->RemoveArray(name.c_str());
          }
        }
        continue;
      }
    }

    strncpy(array_name, name.c_str(), MAX_NAME);
    hid_t did = H5Dopen(now_gid, array_name, H5P_DEFAULT);
    dataspace = H5Dget_space(did);
    //we either get 2 or 1 d
    //this ensures that when we go down to 1, we don't have leftover junk
    dims[1] = 1;
    status = H5Sget_simple_extent_dims(dataspace, dims, NULL);
    if (status < 0)
    {
      H5Dclose(did);
      continue;
    }
    if ((align == 0 && dims[0] != totalNumCells) ||
        (align == 1 && dims[0] != totalNumPoints))
    {
      H5Dclose(did);
      continue;
    }


    bool isFloat = this->Internals->array_isFloat[array_name];
    double **vals_out = NULL;;
    int **ivals_out = NULL;;
    if (isFloat)
    {
      vals_out = new double *[dims[0]];
      vals_out[0] = new double[dims[0]*dims[1]];
      for (unsigned int i = 1; i < dims[0]; i++)
      {
        vals_out[i] = vals_out[0]+i*dims[1];
      }
      status = H5Dread(did, H5T_NATIVE_DOUBLE, H5S_ALL, H5S_ALL,
                       H5P_DEFAULT, &vals_out[0][0]);
    }
    else
    {
      ivals_out = new int *[dims[0]];
      ivals_out[0] = new int[dims[0]*dims[1]];
      for (unsigned int i = 1; i < dims[0]; i++)
      {
        ivals_out[i] = ivals_out[0]+i*dims[1];
      }
      status = H5Dread(did, H5T_NATIVE_INT, H5S_ALL, H5S_ALL,
                       H5P_DEFAULT, &ivals_out[0][0]);
    }

    int *blockptr = this->Internals->bids_out[0];
    bool forcells = (align == 0);

    //save time by keeping track of what array goes to what block once
    std::map<int, vtkDataArray *> gblockToArray;
    for (unsigned b = 0; b < totalNumBlocks; b++)
    {
      gblockToArray[b] = NULL;
    }

    //likewise we keep a master set of point data arrays
    //speed things up by keeping track of array name to location
    std::map<std::string, vtkDataArray *> nameToPointArray;
    for (int a = 0; a < this->Internals->PointData->GetNumberOfArrays(); a++)
    {
      //populate the map when filled in a different timestep
      vtkDataArray *da = vtkDataArray::SafeDownCast
        (this->Internals->PointData->GetArray(a));
      nameToPointArray[std::string(da->GetName())] = da;
    }

    //add this array to each enabled block
    std::map<int, int> tcnt; //a counter so we can insert instead of append
    for (unsigned int b = 0; b < totalNumBlocks; b++)
    {
      if (this->BlockChoices->GetArraySetting(b) == 0)
      {
        continue;
      }
      int gblockid = this->Internals->blockmap[b];
      tcnt[gblockid] = 0;
      vtkDataArray *vArray = NULL;
      vtkDataSetAttributes *arrayGroup = grid[b]->GetCellData();
      if (!forcells)
      {
        arrayGroup = grid[b]->GetPointData();
      }
      vArray = vtkDoubleArray::SafeDownCast
        (arrayGroup->GetArray(array_name));
      if (!vArray)
      {
        if (forcells)
        {
          if (isFloat)
          {
            vArray = vtkDoubleArray::New();
          }
          else
          {
            vArray = vtkIntArray::New();
          }
          vArray->SetName(array_name);
          vArray->SetNumberOfComponents(dims[1]);
          vArray->SetNumberOfTuples(grid[b]->GetNumberOfCells());
          arrayGroup->AddArray(vArray);
          vArray->Delete();
        }
        else
        {
          vtkDataArray *mArray = nameToPointArray[std::string(array_name)];
          if (!mArray)
          {
            if (isFloat)
            {
              mArray = vtkDoubleArray::SafeDownCast
                (this->Internals->PointData->GetArray(array_name));
            }
            else
            {
              mArray = vtkIntArray::SafeDownCast
                (this->Internals->PointData->GetArray(array_name));
            }
            if (!mArray)
            {
              if (isFloat)
              {
                mArray = vtkDoubleArray::New();
              }
              else
              {
                mArray = vtkIntArray::New();
              }
              mArray->SetName(array_name);
              mArray->SetNumberOfComponents(dims[1]);
              mArray->SetNumberOfTuples(totalNumPoints);
              this->Internals->PointData->AddArray(mArray);
              mArray->Delete();
            }
            nameToPointArray[std::string(array_name)] = mArray;
          }
          vArray = mArray;
          arrayGroup->AddArray(vArray);
        }
      }
      vArray->Modified(); //be sure consumers know, we modify in place and that won't trigger
      gblockToArray[gblockid] = vArray;
    }

    //now move the array contents into place, element by element
    double *ptr = NULL;
    int *iptr = NULL;
    if (isFloat)
    {
      ptr = vals_out[0];
    }
    else
    {
      iptr = ivals_out[0];
    }
    for (unsigned int elem = 0; elem < dims[0]; elem++)
    {
      if (forcells)
      {
        int gblockid = blockptr[elem];
        if (!gBlockToEnabled[gblockid])
        {
          ptr += dims[1];
          continue;
        }
        vtkDataArray *vArray = gblockToArray[gblockid];
        for (unsigned int comp = 0; comp < dims[1]; comp++)
        {
          if (isFloat)
          {
            vArray->SetComponent(tcnt[gblockid], comp, *ptr);
          }
          else
          {
            vArray->SetComponent(tcnt[gblockid], comp, *iptr);
          }
          ptr++;
        }
        tcnt[gblockid]++;
      }
      else
      {
        vtkDataArray *vArray = nameToPointArray[std::string(array_name)];
        for (unsigned int comp = 0; comp < dims[1]; comp++)
        {
          if (isFloat)
          {
            vArray->SetComponent(elem, comp, *ptr);
          }
          else
          {
            vArray->SetComponent(elem, comp, *iptr);
          }
          ptr++;
        }
      }
    }

    if (isFloat)
    {
      delete[] vals_out[0];
      delete[] vals_out;
    }
    else
    {
      delete[] ivals_out[0];
      delete[] ivals_out;
    }

    H5Dclose(did);
  }

  H5Gclose(now_gid);

  for(unsigned int b = 0; b < totalNumBlocks; b++)
  {
    int gblockid = this->Internals->blockmap[b];
    std::string bname = vtkTRUCHASReader::Internal::to_string(gblockid);
    output->SetBlock(b, grid[b]);
    output->GetMetaData(b)->Set(vtkCompositeDataSet::NAME(), bname);
  }

  return 1;
}

//----------------------------------------------------------------------------
int vtkTRUCHASReader::CanReadFile(const char *filename)
{
  size_t len = strlen(filename);
  if (len < 3 || strcmp(filename+len-3, ".h5"))
  {
    return 0;
  }

  // Silence error messages to stdout generated by HDF5
  H5Eset_auto(H5E_DEFAULT, NULL, NULL);

  hid_t fileIndx = H5Fopen(filename, H5F_ACC_RDONLY, H5P_DEFAULT );
  if( fileIndx < 0 )
  {
    return 0;
  }

  const char *needful_things[5]= {
    //datasets
    "/Meshes/DEFAULT/Nodal Coordinates",
    "/Simulations/MAIN/Non-series Data/BLOCKID",
    "/Meshes/DEFAULT/Element Connectivity",
    //groups
    "/Simulations/MAIN/Series Data",
    "/Simulations/MAIN/Series Data/Series 1"
  };

  for (int i = 0; i < 3; i++)
  {
    htri_t exists = H5Lexists(fileIndx, needful_things[i], H5P_DEFAULT);
    if( !exists )
    {
      H5Fclose(fileIndx);
      return 0;
    }
    hid_t dset = H5Dopen
      (fileIndx, needful_things[i], H5P_DEFAULT );
    if( dset < 0 )
    {
      H5Fclose(fileIndx);
      return 0;
    }
    H5Dclose(dset);
  }

  for (int i = 3; i < 5; i++)
  {
    htri_t exists = H5Lexists(fileIndx, needful_things[i], H5P_DEFAULT);
    if( !exists )
    {
      H5Fclose(fileIndx);
      return 0;
    }
    hid_t gid = H5Gopen
      (fileIndx, needful_things[i], H5P_DEFAULT);
    if( gid < 0 )
    {
      H5Fclose(fileIndx);
      return 0;
    }
    H5Gclose(gid);
  }

  //it most likely has everything everything we need

  H5Fclose(fileIndx);
  return 1;
}

//----------------------------------------------------------------------------
void vtkTRUCHASReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "File Name: "
     << (this->FileName ? this->FileName : "(none)") << "\n";
}

//----------------------------------------------------------------------------
int vtkTRUCHASReader::GetNumberOfBlockArrays()
{
  return this->BlockChoices->GetNumberOfArrays();
}

//----------------------------------------------------------------------------
void vtkTRUCHASReader::SetBlockArrayStatus(const char* gridname, int status)
{
  if (status!=0)
  {
    this->BlockChoices->EnableArray(gridname);
  }
  else
  {
    this->BlockChoices->DisableArray(gridname);
  }
  this->Modified();
}

//----------------------------------------------------------------------------
int vtkTRUCHASReader::GetBlockArrayStatus(const char* arrayname)
{
  return this->BlockChoices->ArrayIsEnabled(arrayname);
}

//----------------------------------------------------------------------------
const char* vtkTRUCHASReader::GetBlockArrayName(int index)
{
  return this->BlockChoices->GetArrayName(index);
}

//----------------------------------------------------------------------------
int vtkTRUCHASReader::GetNumberOfPointArrays()
{
  return this->PointArrayChoices->GetNumberOfArrays();
}

//----------------------------------------------------------------------------
void vtkTRUCHASReader::SetPointArrayStatus(const char* gridname, int status)
{
  if (status!=0)
  {
    this->PointArrayChoices->EnableArray(gridname);
  }
  else
  {
    this->PointArrayChoices->DisableArray(gridname);
  }
  this->Modified();
}

//----------------------------------------------------------------------------
int vtkTRUCHASReader::GetPointArrayStatus(const char* arrayname)
{
  return this->PointArrayChoices->ArrayIsEnabled(arrayname);
}

//----------------------------------------------------------------------------
const char* vtkTRUCHASReader::GetPointArrayName(int index)
{
  return this->PointArrayChoices->GetArrayName(index);
}

//----------------------------------------------------------------------------
int vtkTRUCHASReader::GetNumberOfCellArrays()
{
  return this->CellArrayChoices->GetNumberOfArrays();
}

//----------------------------------------------------------------------------
void vtkTRUCHASReader::SetCellArrayStatus(const char* gridname, int status)
{
  if (status!=0)
  {
    this->CellArrayChoices->EnableArray(gridname);
  }
  else
  {
    this->CellArrayChoices->DisableArray(gridname);
  }
  this->Modified();
}

//----------------------------------------------------------------------------
int vtkTRUCHASReader::GetCellArrayStatus(const char* arrayname)
{
  return this->CellArrayChoices->ArrayIsEnabled(arrayname);
}

//----------------------------------------------------------------------------
const char* vtkTRUCHASReader::GetCellArrayName(int index)
{
  return this->CellArrayChoices->GetArrayName(index);
}
