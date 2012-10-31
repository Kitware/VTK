/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkEnSightWriter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/

/* TODO
 *
 *
 * check that data was written
 *
 */

#include "vtkEnSightWriter.h"

#include "vtkToolkits.h" // for VTK_USE_PARALLEL
#ifdef VTK_USE_PARALLEL
# include "vtkMultiProcessController.h"
#endif

#include "vtkBitArray.h"
#include "vtkByteSwap.h"
#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkCharArray.h"
#include "vtkDataSet.h"
#include "vtkDoubleArray.h"
#include "vtkErrorCode.h"
#include "vtkFieldData.h"
#include "vtkFloatArray.h"
#include "vtkInformation.h"
#include "vtkIntArray.h"
#include "vtkLongArray.h"
#include "vtkLookupTable.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkShortArray.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnsignedIntArray.h"
#include "vtkUnsignedLongArray.h"
#include "vtkUnsignedShortArray.h"

#include "vtkPriorityQueue.h"

#include "vtkUnstructuredGrid.h"

#include <errno.h>
#include <math.h>
#include <ctype.h>

#include <vector>
#include <list>
#include <map>
#include <algorithm>

// If we are building against a slightly older VTK version,
// these cell types are not defined, and won't occur in the input

#ifndef VTK_QUADRATIC_WEDGE
# define VTK_QUADRATIC_WEDGE      26
# define VTK_QUADRATIC_PYRAMID    27
#endif

// this undef is required on the hp. vtkMutexLock ends up including
// /usr/inclue/dce/cma_ux.h which has the gall to #define write as cma_write

#ifdef write
# undef write
#endif

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkEnSightWriter);

//----------------------------------------------------------------------------
// Created object with no filename and timestep 0
vtkEnSightWriter::vtkEnSightWriter()
{

  this->BaseName = NULL;
  this->FileName = NULL;
  this->TimeStep = 0;
  this->Path=NULL;
  this->GhostLevelMultiplier=10000;
  this->GhostLevel = 0;
  this->TransientGeometry=false;
  this->ProcessNumber=0;
  this->NumberOfProcesses=1;
  this->NumberOfBlocks=0;
  this->BlockIDs=0;
  this->TmpInput = NULL;
}

//----------------------------------------------------------------------------
vtkEnSightWriter::~vtkEnSightWriter()
{
  this->SetBaseName(NULL);
  this->SetFileName(NULL);
  this->SetPath(NULL);
}

//----------------------------------------------------------------------------
void vtkEnSightWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "FileName: "
    << (this->FileName ? this->FileName : "(none)") << "\n";
  os << indent << "Path: "
    << (this->Path ? this->Path : "(none)") << "\n";
  os << indent << "BaseName: "
    << (this->BaseName ? this->BaseName : "(none)") << "\n";

  os << indent << "TimeStep: " << this->TimeStep << "\n";
  os << indent << "TransientGeometry: " << this->TransientGeometry << "\n";
  os << indent << "ProcessNumber: " << this->ProcessNumber << endl;
  os << indent << "NumberOfProcesses: " << this->NumberOfProcesses << endl;
  os << indent << "NumberOfBlocks: " << this->NumberOfBlocks << endl;
  os << indent << "BlockIDs: " << this->BlockIDs << endl;
  os << indent << "GhostLevel: " << this->GhostLevel << endl;
}

int vtkEnSightWriter::FillInputPortInformation( int vtkNotUsed(port), vtkInformation* info )
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkUnstructuredGrid");
  return 1;
}

//----------------------------------------------------------------------------
// Specify the input data or filter.
void vtkEnSightWriter::SetInputData(vtkUnstructuredGrid *input)
{
  this->SetInputDataInternal(0, input);
}


//----------------------------------------------------------------------------
// Specify the input data or filter.
vtkUnstructuredGrid *vtkEnSightWriter::GetInput()
{
  if (this->GetNumberOfInputConnections(0) < 1)
    {
    return NULL;
    }
  else if (this->TmpInput)
    {
    return this->TmpInput;
    }
  else
    {
    return (vtkUnstructuredGrid *)(this->Superclass::GetInput());
    }
}

//----------------------------------------------------------------------------
void vtkEnSightWriter::WriteData()
{
  int i;
  unsigned int ui;
  int blockCount=0;
  std::list<int>::iterator iter;

  if (this->TmpInput)
    {
    this->TmpInput->Delete();
    this->TmpInput = NULL;
    }

  //figure out process ID

  this->ProcessNumber = 0;
  this->NumberOfProcesses = 1;

#ifdef VTK_USE_PARALLEL
  vtkMultiProcessController *c = vtkMultiProcessController::GetGlobalController();

  if (c != NULL)
    {
    this->ProcessNumber=c->GetLocalProcessId();
    this->NumberOfProcesses = c->GetNumberOfProcesses();
    }
#endif

  vtkUnstructuredGrid *input=this->GetInput();
  vtkInformation* inInfo = this->GetInputInformation();

  if (this->GhostLevel >
      vtkStreamingDemandDrivenPipeline::GetUpdateGhostLevel(inInfo))
    {
    // re-execute pipeline if necessary to obtain ghost cells

    vtkStreamingDemandDrivenPipeline::SetUpdateGhostLevel(inInfo, this->GhostLevel);
    this->GetInputAlgorithm()->Update();
    }

  //get the BlockID Cell Array
  vtkDataArray *BlockData=input->GetCellData()->GetScalars("BlockId");

  if (BlockData==NULL || strcmp(BlockData->GetName(),"BlockId"))
    {
    BlockData=NULL;
    }

  this->ComputeNames();

  if (!this->BaseName)
    {
    vtkErrorMacro("A FileName or Path/BaseName must be specified.");
    return;
    }

  this->SanitizeFileName(this->BaseName);

  char** blockNames=NULL;
  int * elementIDs=NULL;
  char charBuffer[512];
  char fileBuffer[512];
  sprintf(charBuffer,"%s/%s.%d.%05d.geo",
    this->Path,this->BaseName,this->ProcessNumber,
    this->TimeStep);

  //open the geometry file
  //only if timestep 0 and not transient geometry or transient geometry
  FILE *fd=NULL;
  if (this->ShouldWriteGeometry())
    {
    if (!(fd=OpenFile(charBuffer)))
      return;
    }

  //Get the FILE's for Point Data Fields
  std::vector<FILE*> pointArrayFiles;
  int NumPointArrays=input->GetPointData()->GetNumberOfArrays();
  for (i=0;i<NumPointArrays;i++)
    {
    strcpy(fileBuffer,input->GetPointData()->GetArray(i)->GetName());
    this->SanitizeFileName(fileBuffer);
    sprintf(charBuffer,"%s/%s.%d.%05d_n.%s",this->Path,this->BaseName,this->ProcessNumber,
      this->TimeStep,fileBuffer);
    FILE* ftemp=OpenFile(charBuffer);
    if (!ftemp)
      return;
    pointArrayFiles.push_back(ftemp);

    //write the description line to the file
    WriteStringToFile(fileBuffer,ftemp);
    }

  //Get the FILE's for Cell Data Fields
  std::vector<FILE*> cellArrayFiles;
  int NumCellArrays=input->GetCellData()->GetNumberOfArrays();
  for (i=0;i<NumCellArrays;i++)
    {

    strcpy(fileBuffer,input->GetCellData()->GetArray(i)->GetName());
    this->SanitizeFileName(fileBuffer);
    sprintf(charBuffer,"%s/%s.%d.%05d_c.%s",this->Path,this->BaseName,this->ProcessNumber,
      this->TimeStep,fileBuffer);
    FILE* ftemp=OpenFile(charBuffer);
    if (!ftemp)
      return;
    cellArrayFiles.push_back(ftemp);

    //write the description line to the file
    WriteStringToFile(fileBuffer,ftemp);
    }

  //write the header information
  if (this->ShouldWriteGeometry())
    {
    this->WriteStringToFile("C Binary",fd);
    this->WriteStringToFile("Written by VTK EnSight Writer",fd);
    //if (this->Title)
    // this->WriteStringToFile(this->Title,fd);
    //else
    this->WriteStringToFile("No Title was Specified",fd);
    //we will specify node and element ID's
    this->WriteStringToFile("node id given\n",fd);
    this->WriteStringToFile("element id given\n",fd);
    }

  //get the Ghost Cell Array if it exists
  vtkDataArray *GhostData=input->GetCellData()->GetScalars("vtkGhostLevels");
  //if the strings are not the same then we did not get the ghostData array
  if (GhostData==NULL || strcmp(GhostData->GetName(),"vtkGhostLevels"))
    {
    GhostData=NULL;
    }


  //data structure to get all the cells for a certain part
  //basically sort by part# and cell type
  std::map <int, std::vector <int> > CellsByPart;

  //just a list of part numbers
  std::list<int> partNumbers;

  //get all the part numbers in the unstructured grid and sort the cells
  //by part number
  for (i=0;i<input->GetNumberOfCells();i++)
    {
    int key=1;
    if (BlockData)
      key=(int)(BlockData->GetTuple(i)[0]);
    else
      cout << "No BlockID was found\n";
    if (CellsByPart.count(key)==0)
      {
      CellsByPart[key]=std::vector < int >() ;
      }
    CellsByPart[key].push_back(i);
    partNumbers.push_back(key);

    }

  //remove the duplicates from the partNumbers
  partNumbers.sort();
  partNumbers.unique();

  //write out each part
  for (iter=partNumbers.begin();iter!=partNumbers.end();iter++)
    {
    unsigned int j;
    std::list<int>::iterator iter2;
    int part=*iter;

    //write the part Header
    if (this->ShouldWriteGeometry())
      {
      blockCount+=1;
      this->WriteStringToFile("part",fd);
      this->WriteIntToFile(part,fd);
      //cout << "part is " << part << endl;
      int exodusIndex=-1;
      if (exodusIndex!=-1)
        {
        sprintf(charBuffer,"Exodus-%s-%d",blockNames[exodusIndex],part);
        this->WriteStringToFile(charBuffer,fd);
        }
      else
        {
        this->WriteStringToFile("VTK Part",fd);
        }
      this->WriteStringToFile("coordinates",fd);
      }

    //write the part header for data files
    for (j=0;j<pointArrayFiles.size();j++)
      {
      this->WriteStringToFile("part",pointArrayFiles[j]);
      this->WriteIntToFile(part,pointArrayFiles[j]);
      this->WriteStringToFile("coordinates",pointArrayFiles[j]);
      }
    for (j=0;j<cellArrayFiles.size();j++)
      {
      this->WriteStringToFile("part",cellArrayFiles[j]);
      this->WriteIntToFile(part,cellArrayFiles[j]);
      }

    //list of VTK Node Indices per part
    std::list<int> NodesPerPart;

    //map that goes from NodeID to the order, used for element connectivity
    std::map<int, int> NodeIdToOrder;

    //get a list of all the nodes used for a particular part
    for (j=0;j<CellsByPart[part].size();j++)
      {
      vtkCell* cell=input->GetCell(CellsByPart[part][j]);
      vtkIdList* points=cell->GetPointIds();
      for (int k=0;k<points->GetNumberOfIds();k++)
        {
        NodesPerPart.push_back(points->GetId(k));
        }
      }

    //remove the duplicate Node ID's
    NodesPerPart.sort();
    NodesPerPart.unique();

    //write the number of nodes
    if (this->ShouldWriteGeometry())
      {
      this->WriteIntToFile(static_cast<int>(NodesPerPart.size()),fd);


      //write the Node ID's to the file
      //also set up the NodeID->order map
      int NodeCount=0;
      for (iter2=NodesPerPart.begin();iter2!=NodesPerPart.end();iter2++)
        {
        this->WriteIntToFile(*iter2,fd);
        NodeIdToOrder[*iter2]=NodeCount+1;
        NodeCount++;
        }


      //EnSight format requires all the X's, then all the Y's, then all the Z's
      //write the X Coordinates

      vtkPoints* inputPoints=input->GetPoints();
      for (iter2=NodesPerPart.begin();iter2!=NodesPerPart.end();iter2++)
        {
        this->WriteFloatToFile((float)(inputPoints->GetPoint(*iter2)[0]),fd);
        }
      for (iter2=NodesPerPart.begin();iter2!=NodesPerPart.end();iter2++)
        {
        this->WriteFloatToFile((float)(inputPoints->GetPoint(*iter2)[1]),fd);
        }
      for (iter2=NodesPerPart.begin();iter2!=NodesPerPart.end();iter2++)
        {
        this->WriteFloatToFile((float)(inputPoints->GetPoint(*iter2)[2]),fd);
        }

      }

    //write the Node Data for this part
    for (j=0;j<pointArrayFiles.size();j++)
      {
      vtkDataArray* DataArray=input->GetPointData()->GetArray(j);
      //figure out what type of data it is
      int DataSize=DataArray->GetNumberOfComponents();

      for (int CurrentDimension=0;
        CurrentDimension<DataSize;
        CurrentDimension++)
        {
        for (std::list<int>::iterator k=NodesPerPart.begin();
          k!=NodesPerPart.end();k++)
          {
          this->WriteFloatToFile((float)
            (DataArray->
             GetTuple(*k)[CurrentDimension]),
            pointArrayFiles[j]);
          }
        }
      }



    //now we need to sort the cell list by element type
    //map is indexed by cell type has a vector of cell ID's
    std::map<int, std::vector<int> > CellsByElement;
    for (j=0;j<CellsByPart[part].size();j++)
      {
      int CellType=input->GetCell(CellsByPart[part][j])->GetCellType();
      int ghostLevel=0;
      if (GhostData)
        {
        ghostLevel=(int)(GhostData->GetTuple(CellsByPart[part][j])[0]);
        if (ghostLevel>1) ghostLevel=1;
        }
      //we want to sort out the ghost cells from the normal cells
      //so the element type will be ghostMultiplier*ghostLevel+elementType
      CellType+=ghostLevel*GhostLevelMultiplier;
      if (CellsByElement.count(CellType)==0)
        {
        CellsByElement[CellType]=std::vector < int >() ;
        }
      CellsByElement[CellType].push_back(CellsByPart[part][j]);
      }

    //now we need to go through each element type that EnSight understands
    std::vector<int> elementTypes;

    //list the types that EnSight understands
    //the noticeable absences are the ones without a fixed number of Nodes
    //for the ghost cell types
    elementTypes.push_back(VTK_VERTEX);
    elementTypes.push_back(VTK_LINE);
    elementTypes.push_back(VTK_TRIANGLE);
    elementTypes.push_back(VTK_QUAD);
    elementTypes.push_back(VTK_POLYGON);
    elementTypes.push_back(VTK_TETRA);
    elementTypes.push_back(VTK_HEXAHEDRON);
    elementTypes.push_back(VTK_WEDGE);
    elementTypes.push_back(VTK_PYRAMID);
    elementTypes.push_back(VTK_CONVEX_POINT_SET);
    elementTypes.push_back(VTK_QUADRATIC_EDGE);
    elementTypes.push_back(VTK_QUADRATIC_TRIANGLE);
    elementTypes.push_back(VTK_QUADRATIC_QUAD);
    elementTypes.push_back(VTK_QUADRATIC_TETRA);
    elementTypes.push_back(VTK_QUADRATIC_HEXAHEDRON);
    elementTypes.push_back(VTK_QUADRATIC_WEDGE);
    elementTypes.push_back(VTK_QUADRATIC_PYRAMID);
    elementTypes.push_back(this->GhostLevelMultiplier+VTK_VERTEX);
    elementTypes.push_back(this->GhostLevelMultiplier+VTK_LINE);
    elementTypes.push_back(this->GhostLevelMultiplier+VTK_TRIANGLE);
    elementTypes.push_back(this->GhostLevelMultiplier+VTK_QUAD);
    elementTypes.push_back(this->GhostLevelMultiplier+VTK_POLYGON);
    elementTypes.push_back(this->GhostLevelMultiplier+VTK_TETRA);
    elementTypes.push_back(this->GhostLevelMultiplier+VTK_HEXAHEDRON);
    elementTypes.push_back(this->GhostLevelMultiplier+VTK_WEDGE);
    elementTypes.push_back(this->GhostLevelMultiplier+VTK_PYRAMID);
    elementTypes.push_back(this->GhostLevelMultiplier+VTK_CONVEX_POINT_SET);
    elementTypes.push_back(this->GhostLevelMultiplier+VTK_QUADRATIC_EDGE);
    elementTypes.push_back(this->GhostLevelMultiplier+VTK_QUADRATIC_TRIANGLE);
    elementTypes.push_back(this->GhostLevelMultiplier+VTK_QUADRATIC_QUAD);
    elementTypes.push_back(this->GhostLevelMultiplier+VTK_QUADRATIC_TETRA);
    elementTypes.push_back(this->GhostLevelMultiplier+VTK_QUADRATIC_HEXAHEDRON);
    elementTypes.push_back(this->GhostLevelMultiplier+VTK_QUADRATIC_WEDGE);
    elementTypes.push_back(this->GhostLevelMultiplier+VTK_QUADRATIC_PYRAMID);

    //write out each type of element
    if (this->ShouldWriteGeometry())
      {
      for (j=0;j<elementTypes.size();j++)
        {
        unsigned int k;
        int elementType=elementTypes[j];
        if (CellsByElement.count(elementType)>0)
          {
          //switch on element type to write correct type to file
          this->WriteElementTypeToFile(elementType,fd);

          //number of elements
          this->WriteIntToFile(
            static_cast<int>(CellsByElement[elementType].size()),fd);

          //element ID's
          for (k=0;k<CellsByElement[elementType].size();k++)
            {
            int CellId=CellsByElement[elementType][k];
            this->WriteIntToFile(CellId,fd);
            }

          //element conenctivity information
          for (k=0;k<CellsByElement[elementType].size();k++)
            {
            int CellId=CellsByElement[elementType][k];
            vtkIdList *PointIds=input->GetCell(CellId)->GetPointIds();
            for (int m=0;m<PointIds->GetNumberOfIds();m++)
              {
              int PointId=PointIds->GetId(m);
              this->WriteIntToFile(NodeIdToOrder[PointId],fd);
              }
            }
          }
        }
      }

    //write the Cell Data for this part
    for (j=0;j<cellArrayFiles.size();j++)
      {
      vtkDataArray* DataArray=input->GetCellData()->GetArray(j);
      //figure out what type of data it is
      int DataSize=DataArray->GetNumberOfComponents();

      for (unsigned int k=0;k<elementTypes.size();k++)
        {
        if (CellsByElement[elementTypes[k]].size()>0)
          {
          this->WriteElementTypeToFile(elementTypes[k],
            cellArrayFiles[j]);
          for (unsigned int m=0;m<CellsByElement[elementTypes[k]].size();m++)
            {
            for ( int CurrentDimension = 0; CurrentDimension < DataSize;
                     CurrentDimension ++ )
              {
              this->WriteFloatToFile
                (  (float)
                   (   DataArray
                       ->GetTuple(  CellsByElement[ elementTypes[k] ][m]  )
                         [CurrentDimension]
                   ),
                   cellArrayFiles[j]
                );
              }
            }
          }
        }
      }
    }

  //now write the empty blocks
  //use the block list in the exodus model if it exists, otherwise
  //use the BlockID list if that exists.

  if (this->BlockIDs)
    {
    elementIDs=this->BlockIDs;
    }

  if (elementIDs)
    {
    //cout << "have " << this->NumberOfBlocks << " blocks " << endl;
    for (i=0;i<this->NumberOfBlocks;i++)
      {
      unsigned int j;
      //figure out if the part was already written
      int part=elementIDs[i];
      if (   std::find(partNumbers.begin(), partNumbers.end(), part)
        == partNumbers.end() )
        {
        //no information about the part was written to the output files
        //so write some empty information
        if (this->ShouldWriteGeometry())
          {
          blockCount+=1;
          this->WriteStringToFile("part",fd);
          this->WriteIntToFile(part,fd);

          int exodusIndex =
            this->GetExodusModelIndex(elementIDs,this->NumberOfBlocks,part);

          if (exodusIndex!=-1 && blockNames)
            {
            sprintf(charBuffer,"Exodus-%s-%d",blockNames[exodusIndex],part);
            this->WriteStringToFile(charBuffer,fd);
            }
          else
            {
            this->WriteStringToFile("VTK Part",fd);
            }
          }

        //write the part header for data files
        for (j=0;j<pointArrayFiles.size();j++)
          {
          this->WriteStringToFile("part",pointArrayFiles[j]);
          this->WriteIntToFile(part,pointArrayFiles[j]);
          }
        for (j=0;j<cellArrayFiles.size();j++)
          {
          this->WriteStringToFile("part",cellArrayFiles[j]);
          this->WriteIntToFile(part,cellArrayFiles[j]);
          }
        }
      }
    }
  //cout << "wrote " << blockCount << "parts\n";
  if (this->TmpInput)
    {
    this->TmpInput->Delete();
    this->TmpInput = NULL;
    }

  //close all the files
  if (fd)
    {
    fclose(fd);
    }

  for (ui=0;ui<cellArrayFiles.size();ui++)
    {
    fclose(cellArrayFiles[ui]);
    }
  for (ui=0;ui<pointArrayFiles.size();ui++)
    {
    fclose(pointArrayFiles[ui]);
    }

}

//----------------------------------------------------------------------------
void vtkEnSightWriter::WriteCaseFile(int TotalTimeSteps)
{

  vtkUnstructuredGrid *input=this->GetInput();
  int i;

  this->ComputeNames();

  if (!this->BaseName)
    {
    vtkErrorMacro("A FileName or Path/BaseName must be specified.");
    return;
    }

  char charBuffer[512];
  sprintf(charBuffer,"%s/%s.%d.case",this->Path,this->BaseName,this->ProcessNumber);

  //open the geometry file
  FILE *fd=NULL;
  if (!(fd=OpenFile(charBuffer)))
    {
    return;
    }

  this->WriteTerminatedStringToFile("FORMAT\n",fd);
  this->WriteTerminatedStringToFile("type: ensight gold\n\n",fd);
  this->WriteTerminatedStringToFile("\nGEOMETRY\n",fd);

  //write the geometry file
  if (!this->TransientGeometry)
    {
    sprintf(charBuffer,"model: %s.%d.00000.geo\n",this->BaseName,this->ProcessNumber);
    this->WriteTerminatedStringToFile(charBuffer,fd);
    }
  else
    {
    sprintf(charBuffer,"model: 1 %s.%d.*****.geo\n",this->BaseName,this->ProcessNumber);
    this->WriteTerminatedStringToFile(charBuffer,fd);
    }

  this->WriteTerminatedStringToFile("\nVARIABLE\n",fd);

  char fileBuffer[512];

  //write the Node variable files
  for (i=0;i<input->GetPointData()->GetNumberOfArrays();i++)
    {

    strcpy(fileBuffer,input->GetPointData()->GetArray(i)->GetName());
    // skip arrays that were not written
    if (strcmp(fileBuffer,"GlobalElementId")==0)
      {
      continue;
      }
    if (strcmp(fileBuffer,"GlobalNodeId")==0)
      {
      continue;
      }
    if (strcmp(fileBuffer,"BlockId")==0)
      {
      continue;
      }
    this->SanitizeFileName(fileBuffer);
    //figure out what kind of data it is
    char SmallBuffer[16];
    switch(input->GetPointData()->GetArray(i)->GetNumberOfComponents())
      {
    case(1):
      strcpy(SmallBuffer,"scalar");
      break;
    case(3):
      strcpy(SmallBuffer,"vector");
      break;
    case(6):
      strcpy(SmallBuffer,"tensor");
      break;
    case(9):
      strcpy(SmallBuffer,"tensor9");
      break;
      }
    if (TotalTimeSteps<=1)
      {
      sprintf(charBuffer,"%s per node: %s_n %s.%d.00000_n.%s\n",
        SmallBuffer,
        fileBuffer,
        this->BaseName,
        this->ProcessNumber,
        fileBuffer);
      }
    else
      {
      sprintf(charBuffer,"%s per node: 1 %s_n %s.%d.*****_n.%s\n",
        SmallBuffer,
        fileBuffer,
        this->BaseName,
        this->ProcessNumber,
        fileBuffer);


      }
    this->WriteTerminatedStringToFile(charBuffer,fd);
    }

  //write the cell variable files
  for (i=0;i<input->GetCellData()->GetNumberOfArrays();i++)
    {
    //figure out what kind of data it is
    char SmallBuffer[16];

    strcpy(fileBuffer,input->GetCellData()->GetArray(i)->GetName());
    // skip arrays that were not written
    if (strcmp(fileBuffer,"GlobalElementId")==0)
      {
      continue;
      }
    if (strcmp(fileBuffer,"GlobalNodeId")==0)
      {
      continue;
      }
    if (strcmp(fileBuffer,"BlockId")==0)
      {
      continue;
      }
    this->SanitizeFileName(fileBuffer);
    switch(input->GetCellData()->GetArray(i)->GetNumberOfComponents())
      {
    case(1):
      strcpy(SmallBuffer,"scalar");
      break;
    case(3):
      strcpy(SmallBuffer,"vector");
      break;
    case(6):
      strcpy(SmallBuffer,"tensor");
      break;
    case(9):
      strcpy(SmallBuffer,"tensor9");
      break;
      }
    if (TotalTimeSteps<=1)
      {
      sprintf(charBuffer,"%s per element: %s_c %s.%d.00000_c.%s\n",
        SmallBuffer,
        fileBuffer,
        this->BaseName,
        this->ProcessNumber,
        fileBuffer);
      }
    else
      {
      sprintf(charBuffer,"%s per element: 1 %s_c %s.%d.*****_c.%s\n",
        SmallBuffer,
        fileBuffer,
        this->BaseName,
        this->ProcessNumber,
        fileBuffer);
      }
    this->WriteTerminatedStringToFile(charBuffer,fd);
    }

  //write time information if we have multiple timesteps
  if (TotalTimeSteps>1)
    {
    this->WriteTerminatedStringToFile("\nTIME\n",fd);
    this->WriteTerminatedStringToFile("time set: 1\n",fd);
    sprintf(charBuffer,"number of steps: %d\n",TotalTimeSteps);
    this->WriteTerminatedStringToFile(charBuffer,fd);
    this->WriteTerminatedStringToFile("filename start number: 00000\n",fd);
    this->WriteTerminatedStringToFile("filename increment: 00001\n",fd);
    this->WriteTerminatedStringToFile("time values: \n",fd);
    for (i=0;i<TotalTimeSteps;i++)
      {
      double timestep=i;

      sprintf(charBuffer,"%f ",timestep);
      this->WriteTerminatedStringToFile(charBuffer,fd);
      if (i%6==0 && i>0)
        {
        this->WriteTerminatedStringToFile("\n",fd);
        }
      }
    }


}

//----------------------------------------------------------------------------
void vtkEnSightWriter::WriteSOSCaseFile(int numProcs)
{
  this->ComputeNames();

  if (!this->BaseName)
    {
    vtkErrorMacro("A FileName or Path/BaseName must be specified.");
    return;
    }

  this->SanitizeFileName(this->BaseName);

  char charBuffer[512];
  sprintf(charBuffer,"%s/%s.case.sos",this->Path,this->BaseName);

  FILE *fd=NULL;
  if (!(fd=OpenFile(charBuffer)))
    return;

  this->WriteTerminatedStringToFile("FORMAT\n",fd);
  this->WriteTerminatedStringToFile("type: master_server gold\n\n",fd);

  this->WriteTerminatedStringToFile("SERVERS\n",fd);
  sprintf(charBuffer,"number of servers: %d\n\n",numProcs);
  this->WriteTerminatedStringToFile(charBuffer,fd);

  //write the servers section with placeholders for the ensight server
  //location and server name
  int i=0;
  for (i=0;i<numProcs;i++)
    {
    sprintf(charBuffer, "#Server %d\n",i);
    this->WriteTerminatedStringToFile(charBuffer,fd);
    this->WriteTerminatedStringToFile("#-------\n",fd);
    sprintf(charBuffer, "machine id: MID%05d\n",i);
    this->WriteTerminatedStringToFile(charBuffer,fd);

    this->WriteTerminatedStringToFile("executable: MEX\n",fd);
    sprintf(charBuffer, "data_path: %s\n",this->Path);
    this->WriteTerminatedStringToFile(charBuffer,fd);
    sprintf(charBuffer,"casefile: %s.%d.case\n\n",this->BaseName,i);
    this->WriteTerminatedStringToFile(charBuffer,fd);
    }


}

//----------------------------------------------------------------------------
void vtkEnSightWriter::WriteStringToFile(const char* cstring, FILE* file)
{
  char cbuffer[80];
  strncpy(cbuffer,cstring,80);
  fwrite(cbuffer, sizeof(char),80,file);
}

//----------------------------------------------------------------------------
void vtkEnSightWriter::WriteTerminatedStringToFile(const char* cstring, FILE* file)
{
  char cbuffer[512];
  strncpy(cbuffer,cstring,512);
  fwrite(cbuffer, sizeof(char),strlen(cbuffer),file);
}

//----------------------------------------------------------------------------
void vtkEnSightWriter::WriteIntToFile(const int i,FILE* file)
{
  //char cbuffer[80];
  //sprintf(cbuffer,"%d",i);
  fwrite(&i, sizeof(int),1,file);
}

//----------------------------------------------------------------------------
void vtkEnSightWriter::WriteFloatToFile(const float f,FILE* file)
{
  //char cbuffer[80];
  //sprintf(cbuffer,"%d",i);
  fwrite(&f, sizeof(float),1,file);
}

//----------------------------------------------------------------------------
void vtkEnSightWriter::WriteElementTypeToFile(int elementType,FILE* fd)
{
  int ghostLevel=elementType/GhostLevelMultiplier;
  elementType=elementType%GhostLevelMultiplier;
  if (ghostLevel==0)
    {
    switch(elementType)
      {
    case(VTK_VERTEX):
      this->WriteStringToFile("point",fd);
      break;
    case(VTK_LINE):
      this->WriteStringToFile("bar2",fd);
      break;
    case(VTK_TRIANGLE):
      this->WriteStringToFile("tria3",fd);
      break;
    case(VTK_QUAD):
      this->WriteStringToFile("quad4",fd);
      break;
    case(VTK_POLYGON):
      this->WriteStringToFile("nsided",fd);
      break;
    case(VTK_TETRA):
      this->WriteStringToFile("tetra4",fd);
      break;
    case(VTK_HEXAHEDRON):
      this->WriteStringToFile("hexa8",fd);
      break;
    case(VTK_WEDGE):
      this->WriteStringToFile("penta6",fd);
      break;
    case(VTK_PYRAMID):
      this->WriteStringToFile("pyramid5",fd);
      break;
    case(VTK_CONVEX_POINT_SET):
      this->WriteStringToFile("nfaced",fd);
      break;
    case(VTK_QUADRATIC_EDGE):
      this->WriteStringToFile("bar3",fd);
      break;
    case(VTK_QUADRATIC_TRIANGLE):
      this->WriteStringToFile("tria6",fd);
      break;
    case(VTK_QUADRATIC_QUAD):
      this->WriteStringToFile("quad8",fd);
      break;
    case(VTK_QUADRATIC_TETRA):
      this->WriteStringToFile("tetra10",fd);
      break;
    case(VTK_QUADRATIC_HEXAHEDRON):
      this->WriteStringToFile("hexa20",fd);
      break;
    case(VTK_QUADRATIC_WEDGE):
      this->WriteStringToFile("penta15",fd);
      break;
    case(VTK_QUADRATIC_PYRAMID):
      this->WriteStringToFile("pyramid13",fd);
      break;
      }
    }
  else
    {
    switch(elementType)
      {
    case(VTK_VERTEX):
      this->WriteStringToFile("g_point",fd);
      break;
    case(VTK_LINE):
      this->WriteStringToFile("g_bar2",fd);
      break;
    case(VTK_TRIANGLE):
      this->WriteStringToFile("g_tria3",fd);
      break;
    case(VTK_QUAD):
      this->WriteStringToFile("g_quad4",fd);
      break;
    case(VTK_POLYGON):
      this->WriteStringToFile("g_nsided",fd);
      break;
    case(VTK_TETRA):
      this->WriteStringToFile("g_tetra4",fd);
      break;
    case(VTK_HEXAHEDRON):
      this->WriteStringToFile("g_hexa8",fd);
      break;
    case(VTK_WEDGE):
      this->WriteStringToFile("g_penta6",fd);
      break;
    case(VTK_PYRAMID):
      this->WriteStringToFile("g_pyramid5",fd);
      break;
    case(VTK_CONVEX_POINT_SET):
      this->WriteStringToFile("g_nfaced",fd);
      break;
    case(VTK_QUADRATIC_EDGE):
      this->WriteStringToFile("g_bar3",fd);
      break;
    case(VTK_QUADRATIC_TRIANGLE):
      this->WriteStringToFile("g_tria6",fd);
      break;
    case(VTK_QUADRATIC_QUAD):
      this->WriteStringToFile("g_quad8",fd);
      break;
    case(VTK_QUADRATIC_TETRA):
      this->WriteStringToFile("g_tetra10",fd);
      break;
    case(VTK_QUADRATIC_HEXAHEDRON):
      this->WriteStringToFile("g_hexa20",fd);
      break;
    case(VTK_QUADRATIC_WEDGE):
      this->WriteStringToFile("g_penta15",fd);
      break;
    case(VTK_QUADRATIC_PYRAMID):
      this->WriteStringToFile("g_pyramid13",fd);
      break;
      }
    }
}

//----------------------------------------------------------------------------
bool vtkEnSightWriter::ShouldWriteGeometry()
{
  return ((this->TransientGeometry || (!this->TransientGeometry && this->TimeStep==0)));
}

//----------------------------------------------------------------------------
void vtkEnSightWriter::SanitizeFileName(char* name)
{

  char buffer[512];
  unsigned int i;
  int BufferPosition=0;
  for (i=0;i<strlen(name);i++)
    {
    if (name[i]!='/')
      {
      buffer[BufferPosition]=name[i];
      BufferPosition++;
      }
    }
  buffer[BufferPosition]=0;
  for (i=0;i<strlen(buffer);i++)
    {
    name[i]=buffer[i];
    }
  name[strlen(buffer)]=0;

}

//----------------------------------------------------------------------------
FILE* vtkEnSightWriter::OpenFile(char* name)
{
  FILE * fd=fopen(name,"wb");

  if (fd == NULL)
    {
    vtkErrorMacro("Error opening " << name
      << ": " << strerror(errno));
    return NULL;
    }
  return fd;
}

//----------------------------------------------------------------------------
int vtkEnSightWriter::GetExodusModelIndex(int *elementArray,int numberElements,int partID)
{
  int i;
  for (i=0;i<numberElements;i++)
    {
    if (elementArray[i]==partID)
      return i;
    }
  return -1;
}

//----------------------------------------------------------------------------
void vtkEnSightWriter::DefaultNames()
{
  char *path = new char [4];
  char *base = new char [20];
  strcpy(path, "./");
  strcpy(base, "EnSightWriter.out");

  this->SetPath(path);
  this->SetBaseName(base);
}

//----------------------------------------------------------------------------
void vtkEnSightWriter::ComputeNames()
{
  if (this->Path && this->BaseName)
    {
    return;
    }

  if (!this->FileName)
    {
    this->DefaultNames();
    return;
    }

  // FileName = Path/BaseName.digits.digits

  char *path = NULL;
  char *base = NULL;

  char *f = this->FileName;

  while (!isgraph(*f)) f++;  // find first printable character

  if (!*f)
    {
    // FileName is garbage
    DefaultNames();
    return;
    }

  char *buf = new char [strlen(f) + 1];
  strcpy(buf, f);

  char *slash = strrchr(buf, '/');  // final slash

  if (slash)
    {
    *slash = 0;
    path = new char [strlen(buf) + 1];
    strcpy(path, buf);
    f = slash + 1;
    }
  else
    {
    path = new char [4];
    strcpy(path, "./");

    f = buf;
    }

  char *firstChar = f;
  while (*f && (*f != '.')) f++;
  *f = 0;

  base = new char [strlen(firstChar) + 1];
  strcpy(base, firstChar);

  this->SetPath(path);
  this->SetBaseName(base);

  delete [] buf;
}
