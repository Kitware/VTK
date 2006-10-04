/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenFOAMReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// Thanks to Terry Jordan of SAIC at the National Energy 
// Technology Laboratory who developed this class.
// Please address all comments to Terry Jordan (terry.jordan@sa.netl.doe.gov)
//

#include "vtkOpenFOAMReader.h"

#include "vtkIntArray.h"
#include "vtkFloatArray.h"
#include "vtkDoubleArray.h"
#include "vtkPoints.h"
#include "vtkCellData.h"
#include "vtkHexahedron.h"
#include "vtkWedge.h"
#include "vtkPyramid.h"
#include "vtkVertex.h"
#include "vtkTetra.h"
#include "vtkConvexPointSet.h"
#include "vtkTriangle.h"
#include "vtkQuad.h"
#include "vtkPolygon.h"
#include "vtkUnstructuredGrid.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkUnstructuredGridAlgorithm.h"
#include "vtkObjectFactory.h"
#include "vtkDirectory.h"
#include "vtkstd/vector"
#include "vtkstd/string"
#include "vtkstd/map"
#include "vtksys/ios/fstream"
#include "vtksys/ios/sstream"
using vtkstd::istringstream;

#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkErrorCode.h"
#include "vtkDataArraySelection.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkCellArray.h"
#include "vtkDataArraySelection.h"


vtkCxxRevisionMacro(vtkOpenFOAMReader, "1.5");
vtkStandardNewMacro(vtkOpenFOAMReader);

struct stdString
{
  vtkstd::string value;
};

struct stringVector
{
  vtkstd::vector< vtkstd::string > value;
};

struct intVector
{
  vtkstd::vector< int > value;
};

struct intVectorVector
{
  vtkstd::vector< vtkstd::vector< int > > value;
};

struct faceVectorVector
{
  vtkstd::vector< vtkstd::vector< face > > value;
};

vtkOpenFOAMReader::vtkOpenFOAMReader()
{
  vtkDebugMacro(<<"Constructor");
  this->SetNumberOfInputPorts(0);

  //INTIALIZE FILE NAME
  this->FileName = NULL;

  //VTK CLASSES
  this->Points = vtkPoints::New();
  this->CellDataArraySelection = vtkDataArraySelection::New();

  //DATA COUNTS
  this->NumFaces = 0;
  this->NumPoints = 0;
  this->NumCells = 0;

  this->TimeStepData = new stringVector;
  this->Path = new stdString;
  this->PathPrefix = new stdString;
  this->PolyMeshPointsDir = new stringVector;
  this->PolyMeshFacesDir = new stringVector;
  this->BoundaryNames = new stringVector;
  this->PointZoneNames = new stringVector;
  this->FaceZoneNames = new stringVector;
  this->CellZoneNames = new stringVector;

  this->FacePoints = new intVectorVector;
  this->FacesOwnerCell = new intVectorVector;
  this->FacesNeighborCell = new intVectorVector;
  this->FacesOfCell = new faceVectorVector;
  this->SizeOfBoundary = new intVector;

  //DATA TIMES
  this->NumberOfTimeSteps = 0;
  this->Steps = NULL;
  this->TimeStep = 0;
  this->TimeStepRange[0] = 0;
  this->TimeStepRange[1] = 0;
  this->RequestInformationFlag = true;
}

vtkOpenFOAMReader::~vtkOpenFOAMReader()
{
  vtkDebugMacro(<<"DeConstructor");
  this->Points->Delete();
  this->CellDataArraySelection->Delete();

  delete this->TimeStepData;
  delete this->Path;
  delete this->PathPrefix;
  delete this->PolyMeshPointsDir;
  delete this->PolyMeshFacesDir;
  delete this->BoundaryNames;
  delete this->PointZoneNames;
  delete this->FaceZoneNames;
  delete this->CellZoneNames;
  delete this->FacePoints;
  delete this->FacesOwnerCell;
  delete this->FacesNeighborCell;
  delete this->FacesOfCell;
  delete this->SizeOfBoundary;
}

int vtkOpenFOAMReader::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **vtkNotUsed(inputVector),
  vtkInformationVector *outputVector)
{
  vtkDebugMacro(<<"Request Data");
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  vtkMultiBlockDataSet *output = vtkMultiBlockDataSet::SafeDownCast(
    outInfo->Get(vtkMultiBlockDataSet::DATA_OBJECT()));
  if (!this->FileName)
    {
    vtkErrorMacro("FileName has to be specified!");
    return 0;
    }

  this->CreateDataSet(output);
  return 1;
}

void vtkOpenFOAMReader::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkDebugMacro(<<"Print Self");
  this->Superclass::PrintSelf(os,indent);
  os << indent << "File Name: " 
     << (this->FileName ? this->FileName : "(none)") << "\n";
  os << indent << "Number Of Nodes: " << this->NumPoints << endl;
  os << indent << "Number Of Cells: " << this->NumCells << endl;
  os << indent << "Number of Time Steps: " << this->NumberOfTimeSteps 
     << endl;
  os << indent << "Time Step Range: " 
     << this->TimeStepRange[0] << " - " << this->TimeStepRange[1]
     << endl;
  os << indent << "Time Step: " << this->TimeStep << endl;
}

int vtkOpenFOAMReader::RequestInformation(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **vtkNotUsed(inputVector),
  vtkInformationVector *outputVector)
{
  if (!this->FileName)
    {
    vtkErrorMacro("FileName has to be specified!");
    return 0;
    }

  vtkDebugMacro(<<"Request Info");
  if(RequestInformationFlag)
    {
    vtkDebugMacro(<<this->FileName);
    this->Path->value.append(this->FileName);
    this->ReadControlDict();
    this->TimeStepRange[0] = 0;
    this->TimeStepRange[1] = this->NumberOfTimeSteps-1;
    this->PopulatePolyMeshDirArrays();
    outputVector->GetInformationObject(0)->Set(
      vtkStreamingDemandDrivenPipeline::TIME_STEPS(), this->Steps,
      this->NumberOfTimeSteps);
    this->RequestInformationFlag = false;
    }

  //Add scalars and vectors to metadata
  //create path to current time step
  vtkstd::stringstream tempPath;
  tempPath << this->PathPrefix->value;
  tempPath << this->Steps[this->TimeStep];
  //open the directory and get num of files
  int numSolvers;
  vtkDirectory * directory = vtkDirectory::New();
  int opened = directory->Open(tempPath.str().c_str());
  if(opened)
    {
    numSolvers = directory->GetNumberOfFiles();
    }
  else
    {
    numSolvers = -1; //no dir
    }
  //loop over all files and locate
  //volScalars and volVectors
  for(int j = 0; j < numSolvers; j++)
    {
    const char * tempSolver = directory->GetFile(j);
    if(tempSolver != (char *)"polyMesh")
      {
      if(tempSolver != (char *)"." && tempSolver != (char *)"..")
        {
        vtkstd::string type(this->GetDataType(tempPath.str().c_str(),
                                              tempSolver));
        if(strcmp(type.c_str(), "Scalar") == 0)
        {
          this->TimeStepData->value.push_back(vtkstd::string(tempSolver));
          this->CellDataArraySelection->AddArray(tempSolver);
          }
        else if(strcmp(type.c_str(), "Vector") == 0)
          {
          this->TimeStepData->value.push_back(vtkstd::string(tempSolver));
          this->CellDataArraySelection->AddArray(tempSolver);
          }
        }
      }
    }
  directory->Delete();
  return 1;
}

//
// CELL METHODS
//
int vtkOpenFOAMReader::GetNumberOfCellArrays()
{
  return this->CellDataArraySelection->GetNumberOfArrays();
}

const char* vtkOpenFOAMReader::GetCellArrayName(int index)
{
  return this->CellDataArraySelection->GetArrayName(index);
}

int vtkOpenFOAMReader::GetCellArrayStatus(const char* name)
{
  return this->CellDataArraySelection->ArrayIsEnabled(name);
}

void vtkOpenFOAMReader::SetCellArrayStatus(const char* name, int status)
{
  if(status)
    {
    this->CellDataArraySelection->EnableArray(name);
    }
  else
    {
    this->CellDataArraySelection->DisableArray(name);
    }
}

void vtkOpenFOAMReader::DisableAllCellArrays()
{
  this->CellDataArraySelection->DisableAllArrays();
}

void vtkOpenFOAMReader::EnableAllCellArrays()
{
  this->CellDataArraySelection->EnableAllArrays();
}

// ****************************************************************************
//  Method: vtkOpenFOAMReader::CombineOwnerNeigbor
//
//  Purpose:
//  add Owner faces to the faces of a cell and then add the neighor faces
//
// ****************************************************************************
void vtkOpenFOAMReader::CombineOwnerNeigbor()
{
  vtkDebugMacro(<<"Combine owner & neighbor faces");
  //reintialize faces of the cells
  face tempFace;
  this->FacesOfCell->value.clear();
  this->FacesOfCell->value.resize(this->NumCells);

  //add owner faces to cell
  for(int i = 0; i < (int)this->FacesOwnerCell->value.size(); i++)
    {
    for(int j = 0; j < (int)this->FacesOwnerCell->value[i].size(); j++)
      {
      tempFace.faceIndex = this->FacesOwnerCell->value[i][j];
      tempFace.neighborFace = false;
      this->FacesOfCell->value[i].push_back(tempFace);
      }
    }

  //add neighbor faces to cell
  for(int i = 0; i < (int)this->FacesNeighborCell->value.size(); i++)
    {
    for(int j = 0; j < (int)this->FacesNeighborCell->value[i].size(); j++)
      {
      tempFace.faceIndex = this->FacesNeighborCell->value[i][j];
      tempFace.neighborFace = true;
      this->FacesOfCell->value[i].push_back(tempFace);
      }
    }

  //clean up memory
  this->FacesOwnerCell->value.clear();
  this->FacesNeighborCell->value.clear();
}

// ****************************************************************************
//  Method: vtkOpenFOAMReader::MakeInternalMesh
//
//  Purpose:
//  derive cell types and create the internal mesh
//
// ****************************************************************************
vtkUnstructuredGrid * vtkOpenFOAMReader::MakeInternalMesh()
{
  vtkDebugMacro(<<"Make internal mesh");
  //initialize variables
  bool foundDup = false;
  vtkstd::vector< int > cellPoints;
  vtkstd::vector< int > tempFaces[2];
  vtkstd::vector< int > firstFace;
  int pivotPoint = 0;
  int i, j, k, l, pCount;
  int faceCount = 0;

  //Create Mesh
  vtkUnstructuredGrid *  internalMesh = vtkUnstructuredGrid::New();

  //loop through each cell, derive type and insert it into the mesh
  //hexahedron, prism, pyramid, tetrahedron, wedge&tetWedge
  for(i = 0; i < (int)this->FacesOfCell->value.size(); i++)  //each cell
    {

    //calculate the total points for the cell
    //used to derive cell type
    int totalPointCount = 0;
    for(j = 0; j < (int)this->FacesOfCell->value[i].size(); j++)  //each face
      {
      totalPointCount += 
        (int)this->FacePoints->
                 value[this->FacesOfCell->value[i][j].faceIndex].size();
      }

    // using cell type - order points, create cell, & add to mesh

    //OFhex | vtkHexahedron
    if (totalPointCount == 24) 
      {
      faceCount = 0;

      //get first face
      for(j = 0; j <
        (int)this->FacePoints->
                    value[this->FacesOfCell->value[i][0].faceIndex].size(); j++)
        {
        firstFace.push_back(this->FacePoints->value[
          this->FacesOfCell->value[i][0].faceIndex][j]);
        }

      //patch: if it is a neighbor face flip the points
      if(this->FacesOfCell->value[i][0].neighborFace)
        {
        int tempPop;
        for(k = 0; k < (int)firstFace.size() - 1; k++)
          {
          tempPop = firstFace[firstFace.size()-1];
          firstFace.pop_back();
          firstFace.insert(firstFace.begin()+1+k, tempPop);
          }
        }

      //add first face to cell points
      for(j =0; j < (int)firstFace.size(); j++)
        {
        cellPoints.push_back(firstFace[j]);
        }

      //find the opposite face and order the points correctly
      for(int pointCount = 0; pointCount < (int)firstFace.size(); pointCount++)
        {

        //find the other 2 faces containing each point
        for(j = 1; j < (int)this->FacesOfCell->value[i].size(); j++) //each face
          {
          for(k = 0; k < (int)this->FacePoints->value[
            this->FacesOfCell->value[i][j].faceIndex].size(); k++) //each point
            {
            if(firstFace[pointCount] == this->FacePoints->value[
              this->FacesOfCell->value[i][j].faceIndex][k])
              {
              //ANOTHER FACE WITH THE POINT
              for(l = 0; l < (int)this->FacePoints->value[
                this->FacesOfCell->value[i][j].faceIndex].size(); l++)
                {
                tempFaces[faceCount].push_back(this->FacePoints->value[
                  this->FacesOfCell->value[i][j].faceIndex][l]);
                }
              faceCount++;
              }
            }
          }

        //locate the pivot point contained in faces 0 & 1
        for(j = 0; j < (int)tempFaces[0].size(); j++)
          {
          for(k = 0; k < (int)tempFaces[1].size(); k++)
            {
            if(tempFaces[0][j] == tempFaces[1][k] && tempFaces[0][j] != 
              firstFace[pointCount])
              {
              pivotPoint = tempFaces[0][j];
              break;
              }
            }
          }
        cellPoints.push_back(pivotPoint);
        tempFaces[0].clear();
        tempFaces[1].clear();
        faceCount=0;
        }

      //create the hex cell and insert it into the mesh
      vtkHexahedron * hexahedron= vtkHexahedron::New();
      for(pCount = 0; pCount < (int)cellPoints.size(); pCount++)
        {
        hexahedron->GetPointIds()->SetId(pCount, cellPoints[pCount]);
        }
      internalMesh->InsertNextCell(hexahedron->GetCellType(),
        hexahedron->GetPointIds());
      hexahedron->Delete();
      cellPoints.clear();
      firstFace.clear();
      }

    //OFprism | vtkWedge
    else if (totalPointCount == 18) 
      {
      faceCount = 0;
      int index = 0;

      //find first triangular face
      for(j = 0; j < (int)this->FacesOfCell->value[i].size(); j++)  //each face
        {
        if((int)this->FacePoints->
            value[this->FacesOfCell->value[i][j].faceIndex].size() == 3)
          {
          for(k = 0; k < (int)this->FacePoints->value[
              this->FacesOfCell->value[i][j].faceIndex].size(); k++)
            {
            firstFace.push_back(this->FacePoints->value[
              this->FacesOfCell->value[i][j].faceIndex][k]);
            index = j;
            }
          break;
          }
        }

      //patch: if it is a neighbor face flip the points
      if(this->FacesOfCell->value[i][0].neighborFace)
        {
        int tempPop;
        for(k = 0; k < (int)firstFace.size() - 1; k++)
          {
          tempPop = firstFace[firstFace.size()-1];
          firstFace.pop_back();
          firstFace.insert(firstFace.begin()+1+k, tempPop);
          }
        }

      //add first face to cell points
      for(j =0; j < (int)firstFace.size(); j++)
        {
        cellPoints.push_back(firstFace[j]);
        }

      //find the opposite face and order the points correctly
      for(int pointCount = 0; pointCount < (int)firstFace.size(); pointCount++)
        {
        //find the 2 other faces containing each point
        for(j = 0; j < (int)this->FacesOfCell->value[i].size(); j++) //each face
          {
          for(k = 0; k < (int)this->FacePoints->value[
              this->FacesOfCell->value[i][j].faceIndex].size(); k++)
            {
            if(firstFace[pointCount] == this->FacePoints->value[
               this->FacesOfCell->value[i][j].faceIndex][k] && j != index)
              {
              //ANOTHER FACE WITH POINT
              for(l = 0; l < (int)this->FacePoints->value[
                  this->FacesOfCell->value[i][j].faceIndex].size(); l++)
                {
                tempFaces[faceCount].push_back(this->FacePoints->value[
                  this->FacesOfCell->value[i][j].faceIndex][l]);
                }
              faceCount++;
              }
            }
          }

        //locate the pivot point of faces 0 & 1
        for(j = 0; j < (int)tempFaces[0].size(); j++)
          {
          for(k = 0; k < (int)tempFaces[1].size(); k++)
            {
            if(tempFaces[0][j] == tempFaces[1][k] && tempFaces[0][j] !=
              firstFace[pointCount])
              {
              pivotPoint = tempFaces[0][j];
              break;
              }
            }
          }
        cellPoints.push_back(pivotPoint);
        tempFaces[0].clear();
        tempFaces[1].clear();
        faceCount=0;
        }

      //create the wedge cell and insert it into the mesh
      vtkWedge * wedge= vtkWedge::New();
      for(pCount = 0; pCount < (int)cellPoints.size(); pCount++)
        {
        wedge->GetPointIds()->SetId(pCount, cellPoints[pCount]);
        }
      internalMesh->InsertNextCell(wedge->GetCellType(),
        wedge->GetPointIds());
      cellPoints.clear();
      wedge->Delete();
      firstFace.clear();
      }

    //OFpyramid | vtkPyramid
    else if (totalPointCount == 16) 
      {
      foundDup = false;

      //find the quadratic face
      for(j = 0; j < (int)this->FacesOfCell->value[i].size(); j++)  //each face
        {
        if((int)this->FacePoints->
            value[this->FacesOfCell->value[i][j].faceIndex].size() == 4)
          {
          for(k = 0; k < (int)this->FacePoints->value[
              this->FacesOfCell->value[i][j].faceIndex].size(); k++)
            {
            cellPoints.push_back(this->FacePoints->value[
              this->FacesOfCell->value[i][j].faceIndex][k]);
            }
          break;
          }
        }

      //compare first face points to other faces
      for(j = 0; j < (int)cellPoints.size(); j++) //each point
        {
        for(k = 0; k < (int)this->FacePoints->value[
          this->FacesOfCell->value[i][1].faceIndex].size(); k++)
          {
          if(cellPoints[j] == this->FacePoints->value[
            this->FacesOfCell->value[i][1].faceIndex][k])
            {
            foundDup = true;
            }
          }
        if(!foundDup)
          {
          cellPoints.push_back(this->FacePoints->value[
            this->FacesOfCell->value[i][j].faceIndex][k]);
          break;
          }
        }

      //create the pyramid cell and insert it into the mesh
      vtkPyramid * pyramid = vtkPyramid::New();
      for(pCount = 0; pCount < (int)cellPoints.size(); pCount++)
        {
        pyramid->GetPointIds()->SetId(pCount, cellPoints[pCount]);
        }
      internalMesh->InsertNextCell(pyramid->GetCellType(), 
        pyramid->GetPointIds());
      cellPoints.clear();
      pyramid->Delete();
      }

    //OFtet | vtkTetrahedron
    else if (totalPointCount == 12) 
      {
      foundDup = false;

      //add first face to cell points
      for(j = 0; j < (int)this->FacePoints->value[
        this->FacesOfCell->value[i][0].faceIndex].size(); j++)
        {
        cellPoints.push_back(this->FacePoints->value[
          this->FacesOfCell->value[i][0].faceIndex][j]);
        }

      //compare first face to the points of second face
      for(j = 0; j < (int)cellPoints.size(); j++) //each point
        {
        for(k = 0; k < (int)this->FacePoints->value[
          this->FacesOfCell->value[i][1].faceIndex].size(); k++)
          {
          if(cellPoints[j] == this->FacePoints->value[
            this->FacesOfCell->value[i][1].faceIndex][k])
            {
            foundDup = true;
            }
          }
        if(!foundDup)
          {
          cellPoints.push_back(this->FacePoints->value[
            this->FacesOfCell->value[i][j].faceIndex][k]);
          break;
          }
        }

      //create the wedge cell and insert it into the mesh
      vtkTetra * tetra = vtkTetra::New();
      for(pCount = 0; pCount < (int)cellPoints.size(); pCount++)
        {
        tetra->GetPointIds()->SetId(pCount, cellPoints[pCount]);
        }
      internalMesh->InsertNextCell(tetra->GetCellType(), 
        tetra->GetPointIds());
      cellPoints.clear();
      tetra->Delete();
      }

    //OFpolyhedron || vtkConvexPointSet
    else
      {
      cout<<"Warning: Polyhedral Data is very Slow!"<<endl;
      foundDup = false;

      //get first face
      for(j = 0; j < (int)this->FacePoints->value[
        this->FacesOfCell->value[i][0].faceIndex].size(); j++)
        {
        firstFace.push_back(this->FacePoints->value[
          this->FacesOfCell->value[i][0].faceIndex][j]);
        }

      //add first face to cell points
      for(j =0; j < (int)firstFace.size(); j++)
        {
        cellPoints.push_back(firstFace[j]);
        }

      //loop through faces and create a list of all points
      //j = 1 skip firstFace
      for(j = 1; j < (int)this->FacesOfCell->value[i].size(); j++)
        {
        //remove duplicate points from faces
        for(k = 0; k < (int)this->FacePoints->value[
          this->FacesOfCell->value[i][j].faceIndex].size(); k++)
          {
          for(l = 0; l < (int)cellPoints.size(); l++);
            {
            if(cellPoints[l] == this->FacePoints->value[
              this->FacesOfCell->value[i][j].faceIndex][k])
              {
              foundDup = true;
              }
            }
          if(!foundDup)
            {
            cellPoints.push_back(this->FacePoints->value[
              this->FacesOfCell->value[i][j].faceIndex][k]);
            foundDup = false;
            }
          }
        }

      //create the poly cell and insert it into the mesh
      vtkConvexPointSet * poly = vtkConvexPointSet::New();
      poly->GetPointIds()->SetNumberOfIds(cellPoints.size());
      for(pCount = 0; pCount < (int)cellPoints.size(); pCount++)
        {
        poly->GetPointIds()->SetId(pCount, cellPoints[pCount]);
        }
      internalMesh->InsertNextCell(poly->GetCellType(), 
        poly->GetPointIds());
      cellPoints.clear();
      firstFace.clear();
      poly->Delete();
      }
    }

  //set the internal mesh points
  internalMesh->SetPoints(Points);
  vtkDebugMacro(<<"Internal mesh made");
  return internalMesh;
}

// ****************************************************************************
//  Method: vtkOpenFOAMReader::ControlDictDataParser
//
//  Purpose:
//  parse out double values for controlDict entries
//  utility function
//
// ****************************************************************************
double vtkOpenFOAMReader::ControlDictDataParser(const char * lineIn)
{
  double value;
  vtkstd::string line(lineIn);
  line.erase(line.begin()+line.find(";"));
  vtkstd::string token;
  vtkstd::stringstream tokenizer(line);

  //parse to the final entry - double
  while(tokenizer>>token);
  vtkstd::stringstream conversion(token);
  conversion>>value;
  return value;
}

// ****************************************************************************
//  Method: vtkOpenFOAMReader::ReadControlDict
//
//  Pupose:
//  reads the controlDict File
//  gather the necessary information to create a path to the data
//
// ****************************************************************************
void vtkOpenFOAMReader::ReadControlDict ()
{
  vtkDebugMacro(<<"Read controlDict");
  //create variables
  ifstream input;
  vtkstd::string temp;
  double startTime;
  double endTime;
  double deltaT;
  double writeInterval;
  double timeStepIncrement;
  vtkstd::string writeControl;
  vtkstd::string timeFormat;

  input.open(this->Path->value.c_str(),ios::in);

  //create the path to the data directory
  this->PathPrefix->value = this->Path->value;
  this->PathPrefix->value.erase(this->PathPrefix->value.begin()+
    this->PathPrefix->value.find("system"),this->PathPrefix->value.end());
  vtkDebugMacro(<<"Path: "<<this->PathPrefix->value.c_str());

  //find Start Time
  getline(input, temp);
  //while(!(temp.compare(0,8,"startTime",0,8) == 0))

  while (strcmp(temp.substr(0,9).c_str(), "startTime"))
    {
    getline(input, temp);
    }
  startTime = this->ControlDictDataParser(temp.c_str());
  vtkDebugMacro(<<"Start time: "<<startTime);

  //find End Time
  //while(!(temp.compare(0,6,"endTime",0,6) == 0))
  while (strcmp(temp.substr(0,7).c_str(), "endTime"))
    {
    getline(input, temp);
    }
  endTime = this->ControlDictDataParser(temp.c_str());
  vtkDebugMacro(<<"End time: "<<endTime);

  //find Delta T
  //while(!(temp.compare(0,5,"deltaT",0,5) == 0))
  while (strcmp(temp.substr(0,6).c_str(), "deltaT"))
    {
    getline(input, temp);
    }
  deltaT = this->ControlDictDataParser(temp.c_str());
  vtkDebugMacro(<<"deltaT: "<<deltaT);

  //find write control
  //while(!(temp.compare(0,11,"writeControl",0,11) == 0))
  while (strcmp(temp.substr(0,12).c_str(), "writeControl"))
    {
    getline(input, temp);
    }
  temp.erase(temp.begin()+temp.find(";"));
  vtkstd::string token;
  vtkstd::stringstream tokenizer(temp);
  while(tokenizer>>token);
  writeControl = token;
  vtkDebugMacro(<<"Write control: "<<writeControl.c_str());

  //find write interval
  //while(!(temp.compare(0,12,"writeInterval",0,12) == 0))
  while (strcmp(temp.substr(0,13).c_str(), "writeInterval"))
    {
    getline(input, temp);
    }
  writeInterval = this->ControlDictDataParser(temp.c_str());
  vtkDebugMacro(<<"Write interval: "<<writeInterval);
  //  cout <<"Write Interval: "<< writeInterval<<endl;

  //calculate the time step increment based on type of run
  //if(writeControl.compare(0,7,"timeStep",0,7) == 0)
  if(!strcmp(temp.substr(0,8).c_str(), "timeStep"))
    {
    vtkDebugMacro(<<"Time step type data");
    timeStepIncrement = writeInterval * deltaT;
    }
  else
    {
    vtkDebugMacro(<<"Run time type data");
    //cout<<"Run Time Type Data"<<endl;
    timeStepIncrement = writeInterval;
    }

  //find time format
  while(temp.find("timeFormat") == vtkstd::string::npos)
    {
    getline(input, temp);
    }
  timeFormat = temp;

  //calculate how many timesteps there should be
  float tempResult = ((endTime-startTime)/timeStepIncrement);
  int tempNumTimeSteps = (int)(tempResult+0.5)+1;  //+0.1 to round up
  //make sure time step dir exists
  vtkstd::vector< double > tempSteps;
  vtkDirectory * test = vtkDirectory::New();
  vtkstd::stringstream parser;
  double tempStep;
  for(int i = 0; i < tempNumTimeSteps; i++)
    {
    tempStep = i*timeStepIncrement + startTime;
    parser.str("");
    parser.clear();
    if(timeFormat.find("general") != vtkstd::string::npos)
      {
      parser << tempStep;
      }
    else
      {
      parser << ios::scientific <<tempStep;
      }
    if(test->Open((this->PathPrefix->value+parser.str()).c_str()))
      {
      tempSteps.push_back(tempStep);
      }
    }

  //Add the time steps that actually exist to steps
  //allows the run to be stopped short of controlDict spec
  //allows for removal of timesteps
  this->NumberOfTimeSteps = tempSteps.size();
  this->Steps = new double[this->NumberOfTimeSteps];
  for(int i = 0; i < this->NumberOfTimeSteps; i++)
    {
    this->Steps[i] =tempSteps[i];
    }

  input.close();
  vtkDebugMacro(<<"controlDict read");
}

// ****************************************************************************
//  Method: vtkOpenFOAMReader::GetPoints
//
//  Purpose:
//  read the points file into a vtkPoints
//
// ****************************************************************************
void vtkOpenFOAMReader::GetPoints (int timeState)
{
  //path to points file
  vtkstd::string pointPath = this->PathPrefix->value +
                             this->PolyMeshPointsDir->value[timeState] +
                             "/polyMesh/points";
  vtkDebugMacro(<<"Read points file: "<<pointPath.c_str());

  vtkstd::string temp;
  ifstream input;
  bool binaryWriteFormat;
  input.open(pointPath.c_str(),ios::in);
  //make sure file exists
  if(input.fail())
    {
    return;
    }

  //determine if file is binary or ascii
  while(temp.find("format") == vtkstd::string::npos)
    {
    getline(input, temp);
    }
  input.close();

  //reopen file in correct format
  if(temp.find("binary") != vtkstd::string::npos)
    {
    input.open(pointPath.c_str(),ios::binary);
    binaryWriteFormat = true;
    }
  else
    {
    input.open(pointPath.c_str(),ios::in);
    binaryWriteFormat = false;
    }

  double x,y,z;
  vtkstd::stringstream tokenizer;

  //instantiate the points class
  this->Points->Reset();

  //find end of header
  //while(temp.compare(0,4, "// *", 0, 4) != 0)
  while (strcmp(temp.substr(0,4).c_str(), "// *"))
    {
    getline(input, temp);
    }

  //find number of points
  getline(input, temp);
  while(temp.empty())
    {
    getline(input, temp);
    }

  //read number of points
  tokenizer.clear();
  tokenizer.str(temp);
  tokenizer>>NumPoints;

  //binary data
  if(binaryWriteFormat)
    {
    input.get(); //parenthesis
    for(int i = 0; i < NumPoints; i++)
      {
      input.read((char *)&x,sizeof(double));
      input.read((char *)&y,sizeof(double));
      input.read((char *)&z,sizeof(double));
      this->Points->InsertPoint(i,x,y,z);
      }
    }

  //ascii data
  else
    {
    getline(input, temp); //THROW OUT "("
    for(int i = 0; i < this->NumPoints; i++)
      {
      getline(input, temp);
      temp.erase(temp.begin()+temp.find("("));
      temp.erase(temp.begin()+temp.find(")"));
      tokenizer.clear();
      tokenizer.str(temp);
      tokenizer>>x;
      tokenizer>>y;
      tokenizer>>z;
      this->Points->InsertPoint(i,x,y,z);
      }
    }

  input.close();
  vtkDebugMacro(<<"Point file read");
}

// ****************************************************************************
//  Method: vtkOpenFOAMReader::ReadFacesFile
//
//  Purpose:
//  read the faces into a vtkstd::vector
//
// ****************************************************************************
void vtkOpenFOAMReader::ReadFacesFile (const char * facePathIn)
{
  vtkstd::string facePath(facePathIn);
  vtkDebugMacro(<<"Read faces file: "<<facePath.c_str());
  vtkstd::string temp;
  ifstream input;
  bool binaryWriteFormat;
  input.open(facePath.c_str(),ios::in);
  //make sure file exists
  if(input.fail())
    {
    return;
    }

  //determine if file is binary or ascii
  while(temp.find("format") == vtkstd::string::npos)
    {
    getline(input, temp);
    }
  input.close();

  //reopen file in correct format
  if(temp.find("binary") != vtkstd::string::npos)
    {
    input.open(facePath.c_str(),ios::binary);
    binaryWriteFormat = true;
    }
  else
    {
    input.open(facePath.c_str(),ios::in);
    binaryWriteFormat = false;
    }

  vtkstd::istringstream tokenizer;
  size_t pos;
  int numFacePoints;
  this->FacePoints->value.clear();

  //find end of header
  //while(temp.compare(0,4, "// *", 0, 4) != 0)
  while (strcmp(temp.substr(0,4).c_str(), "// *"))
    {
    getline(input, temp);
    }

  //find number of faces
  getline(input, temp);
  while(temp.empty())
    {
    getline(input, temp);
    }

  //read number of faces
  tokenizer.clear();
  tokenizer.str(temp);
  tokenizer>>this->NumFaces;

  this->FacePoints->value.resize(this->NumFaces);

  getline(input, temp);//THROW OUT "("

  //binary data
  if(binaryWriteFormat)
    {
    char paren;
    int tempPoint;
    for(int i = 0; i < NumFaces; i++)
      {
      getline(input, temp);//THROW OUT blankline
      getline(input, temp); //grab point count
      tokenizer.clear();
      tokenizer.str(temp);
      tokenizer >> numFacePoints;
      this->FacePoints->value[i].resize(numFacePoints);
      paren = input.get();  //grab (
      for(int j = 0; j < numFacePoints; j++)
        {
        input.read((char *) &tempPoint, sizeof(int));
        this->FacePoints->value[i][j] = tempPoint;
        }
      getline(input, temp); //throw out ) and rest of line
      }
    }

  //ascii data
  else
    {
    //create vtkstd::vector of points in each face
    for(int i = 0; i < this->NumFaces; i++)
      {
      getline(input, temp);
      pos = temp.find("(");
      tokenizer.clear();
      tokenizer.str(temp.substr(0, pos));
      temp.erase(0, pos+1);
      tokenizer>>numFacePoints;
      this->FacePoints->value[i].resize(numFacePoints);
      for(int j = 0; j < numFacePoints; j++)
        {
        pos = temp.find(" ");
        tokenizer.clear();
        tokenizer.str(temp.substr(0, pos));
        temp.erase(0, pos+1);
        tokenizer>>this->FacePoints->value[i][j];
        }
      }
    }

  input.close();
  vtkDebugMacro(<<"Faces read");
}

// ****************************************************************************
//  Method: vtkOpenFOAMReader::ReadOwnerFile
//
//  Purpose:
//  read the owner file into a vtkstd::vector
//
// ****************************************************************************
void vtkOpenFOAMReader::ReadOwnerFile(const char * ownerPathIn)
{
  vtkstd::string ownerPath(ownerPathIn);
  vtkDebugMacro(<<"Read owner file: "<<ownerPath.c_str());
  vtkstd::string temp;
  ifstream input;
  bool binaryWriteFormat;
  input.open(ownerPath.c_str(),ios::in);
  //make sure file exists
  if(input.fail())
    {
    return;
    }

  //determine if file is binary or ascii
  while(temp.find("format") == vtkstd::string::npos)
    {
    getline(input, temp);
    }
  input.close();

  //reopen file in correct format
  if(temp.find("binary") != vtkstd::string::npos)
    {
    input.open(ownerPath.c_str(),ios::binary);
    binaryWriteFormat = true;
    }
  else
    {
    input.open(ownerPath.c_str(),ios::in);
    binaryWriteFormat = false;
    }

  vtkstd::string numFacesStr;
  vtkstd::stringstream tokenizer;
  int faceValue;

  this->FaceOwner = vtkIntArray::New();

  tokenizer<<this->NumFaces;
  tokenizer>>numFacesStr;

  //find end of header & number of faces
  //while(temp.compare(0,numFacesStr.size(),numFacesStr,
  //0,numFacesStr.size())!=0)
  while(strcmp(temp.substr(0,numFacesStr.size()).c_str(), numFacesStr.c_str()))
    {
    getline(input, temp);
    }

  this->FaceOwner->SetNumberOfValues(this->NumFaces);

  //binary data
  if(binaryWriteFormat)
    {
    input.get(); //parenthesis
    for(int i = 0; i < NumFaces; i++)
      {
      input.read((char *) &faceValue, sizeof(int));
      this->FaceOwner->SetValue(i, faceValue);
      }
    }

  //ascii data
  else
    {
    getline(input, temp);//throw away (
    //read face owners into int array
    for(int i = 0; i < this->NumFaces; i++)
      {
      getline(input, temp);
      tokenizer.clear();
      tokenizer.str(temp);
      tokenizer>>faceValue;
      this->FaceOwner->SetValue(i, faceValue);
      }
    }

  //find the number of cells
  double  * range;
  range = this->FaceOwner->GetRange();
  this->NumCells = (int)range[1]+1;

  //add the face number to the correct cell
  //according to owner
  this->FacesOwnerCell->value.resize(this->NumCells);
  int tempCellId;
  for(int j = 0; j < this->NumFaces; j++)
    {
    tempCellId = this->FaceOwner->GetValue(j);
    if(tempCellId != -1)
      {
      this->FacesOwnerCell->value[tempCellId].push_back(j);
      }
    }

  //FaceOwner->Delete();
  input.close();
  vtkDebugMacro(<<"Owner file read");
}

// ****************************************************************************
//  Method: vtkOpenFOAMReader::ReadNeighborFile
//
//  Purpose:
//  read the neighbor file into a vtkstd::vector
//
// ****************************************************************************
void vtkOpenFOAMReader::ReadNeighborFile(const char * neighborPathIn)
{
  vtkstd::string neighborPath(neighborPathIn);
  vtkDebugMacro(<<"Read neighbor file: "<<neighborPath.c_str());
  vtkstd::string temp;
  ifstream input;
  bool binaryWriteFormat;
  input.open(neighborPath.c_str(),ios::in);
  //make sure file exists
  if(input.fail())
    {
    return;
    }

  //determine if file is binary or ascii
  while(temp.find("format") == vtkstd::string::npos)
    {
    getline(input, temp);
    }
  input.close();

  //reopen file in correct format
  if(temp.find("binary") != vtkstd::string::npos)
    {
    input.open(neighborPath.c_str(),ios::binary);
    binaryWriteFormat = true;
    }
  else
    {
    input.open(neighborPath.c_str(),ios::in);
    binaryWriteFormat = false;
    }

  vtkstd::string numFacesStr;
  vtkstd::stringstream tokenizer;
  int faceValue;

  vtkIntArray * faceNeighbor = vtkIntArray::New();

  tokenizer<<this->NumFaces;
  tokenizer>>numFacesStr;

  //find end of header & number of faces
  //while(temp.compare(0,numFacesStr.size(),numFacesStr,0,
  //numFacesStr.size())!=0)
  while (strcmp(temp.substr(0,numFacesStr.size()).c_str(), numFacesStr.c_str()))
    {
    getline(input, temp);
    }

  //read face owners into int array
  faceNeighbor->SetNumberOfValues(this->NumFaces);

  //binary data
  if(binaryWriteFormat)
    {
    input.get(); //parenthesis
    for(int i = 0; i < this->NumFaces; i++)
      {
      input.read((char *) &faceValue, sizeof(int));
      faceNeighbor->SetValue(i, faceValue);
      }
    }

  //ascii data
  else
    {
    getline(input, temp);//throw away (
    //read face owners into int array
    for(int i = 0; i < this->NumFaces; i++)
      {
      getline(input, temp);
      tokenizer.clear();
      tokenizer.str(temp);
      tokenizer>>faceValue;
      faceNeighbor->SetValue(i, faceValue);
      }
    }

  //No need to recalulate the Number of Cells
  this->FacesNeighborCell->value.resize(this->NumCells);

  //add face number to correct cell
  int tempCellId;
  for(int j = 0; j < this->NumFaces; j++)
    {
    tempCellId = faceNeighbor->GetValue(j);
    if(tempCellId != -1)
      {
      this->FacesNeighborCell->value[tempCellId].push_back(j);
      }
    }

  faceNeighbor->Delete();
  input.close();
  vtkDebugMacro(<<"Neighbor file read");
}

// ****************************************************************************
//  Method: vtkOpenFOAMReader::PopulatePolyMeshDirArrays
//
//  Purpose:
//  create a Lookup Table containing the location of the points
//  and faces files for each time steps mesh
//
// ****************************************************************************
void vtkOpenFOAMReader::PopulatePolyMeshDirArrays()
{
  vtkDebugMacro(<<"Create list of points/faces file directories");
  vtkstd::ostringstream path;
  vtkstd::stringstream timeStep;
  bool facesFound;
  bool pointsFound;
  bool polyMeshFound;
  ifstream input;

  //intialize size to number of timesteps
  this->PolyMeshPointsDir->value.resize(this->NumberOfTimeSteps);
  this->PolyMeshFacesDir->value.resize(this->NumberOfTimeSteps);

  //loop through each timestep
  for(int i = 0; i < this->NumberOfTimeSteps; i++)
    {
    polyMeshFound = false;
    facesFound = false;
    pointsFound = false;

    //create the path to the timestep
    path.clear();
    path.str("");
    timeStep.clear();
    timeStep.str("");
    timeStep << Steps[i];
    path << this->PathPrefix->value <<timeStep.str() << "/";

    //get the number of files
    vtkDirectory * directory = vtkDirectory::New();
    directory->Open(path.str().c_str());
    int numFiles = directory->GetNumberOfFiles();

    //Look for polyMesh Dir
    for(int j = 0; j < numFiles; j++)
      {
      vtkstd::string tempFile(directory->GetFile(j));
      if(tempFile.find("polyMesh") != vtkstd::string::npos)
        {
        polyMeshFound = true;

        path << "polyMesh/";

        //get number of files in the polyMesh dir
        vtkDirectory * polyMeshDirectory = vtkDirectory::New();
        polyMeshDirectory->Open(path.str().c_str());
        int numPolyMeshFiles = polyMeshDirectory->GetNumberOfFiles();

        //Look for points/faces files
        for(int k = 0; k < numPolyMeshFiles; k++)
          {
          vtkstd::string tempFile2 = vtkstd::string(polyMeshDirectory->
                                                    GetFile(k));
          if(tempFile2.find("points") != vtkstd::string::npos)
            {
            this->PolyMeshPointsDir->value[i] = timeStep.str();
            pointsFound = true;
            }
          else if(tempFile2.find("faces") != vtkstd::string::npos)
            {
            this->PolyMeshFacesDir->value[i] = timeStep.str();
            facesFound = true;
            }
          }

        //if there is no points or faces found in this timestep
        //set it equal to previous time step if no previous
        //set it equal to "constant" dir
        if(!pointsFound)
          {
          if(i != 0)
            {
            this->PolyMeshPointsDir->value[i] =
              this->PolyMeshPointsDir->value[i-1];
            }
          else
            {
            this->PolyMeshPointsDir->value[i] = vtkstd::string("constant");
            }
          }
        if(!facesFound)
          {
          if(i != 0)
            {
            this->PolyMeshFacesDir->value[i] =
              this->PolyMeshFacesDir->value[i-1];
            }
          else
            {
            this->PolyMeshFacesDir->value[i] = vtkstd::string("constant");
            }
          }

        polyMeshDirectory->Delete();
        break;  //found - stop looking
        }
      }

    //if there is no polyMesh dir
    //set  it equal to prev timestep
    //if no prev set to "constant" dir
    if(!polyMeshFound)
      {
      if(i != 0)
        {
        //set points/faces location to previous timesteps value
        this->PolyMeshPointsDir->value[i] = this->PolyMeshPointsDir->value[i-1];
        this->PolyMeshFacesDir->value[i] = this->PolyMeshFacesDir->value[i-1];
        }
      else
        {
        //set points/faces to constant
        this->PolyMeshPointsDir->value[i] = vtkstd::string("constant");
        this->PolyMeshFacesDir->value[i] = vtkstd::string("constant");
        }
      }
    directory->Delete();
    }
  vtkDebugMacro(<<"Points/faces list created");
}

// ****************************************************************************
//  Method: vtkOpenFOAMReader::GetDataType
//
//  Purpose:
//  determines whether a variable is a volume scalar, vector or neither
//  for meta data
//
// ****************************************************************************
const char * vtkOpenFOAMReader::GetDataType(const char * pathIn,
                                      const char * fileNameIn)
{
  vtkstd::string path(pathIn);
  vtkstd::string fileName(fileNameIn);
  vtkstd::string filePath = path+"/"+fileName;
  vtkDebugMacro(<<"Get data type of: "<<filePath.c_str());
  ifstream input;
  input.open(filePath.c_str(),ios::in);
  //make sure file exists
  if(input.fail())
    {
    return "Null";
    }

  vtkstd::string temp;
  vtkstd::string foamClass;
  vtkstd::stringstream tokenizer;
  int opened;

  //see if fileName is a file or directory
  vtkDirectory * directory = vtkDirectory::New();
  opened = directory->Open(filePath.c_str());
  if(opened)
    {
    return "Directory";
    }

  //find class entry
  vtkstd::getline(input, temp);
  while(temp.find("class") == vtkstd::string::npos && !input.eof())
    {
    vtkstd::getline(input, temp);
    }

  //return type
  if(!input.eof())
    {
    temp.erase(temp.begin()+temp.find(";"));
    //PARSE OUT CLASS TYPE
    tokenizer.str(temp);
    while(tokenizer>>foamClass)
      {
      };
    //return scalar, vector, or invalid
    if(foamClass =="volScalarField")
      {
      return "Scalar";
      }
    else if (foamClass =="volVectorField")
      {
      return "Vector";
      }
    else
      {
      return "Invalid";
      }
    }

  //if the file format is wrong return invalid
  else
    {
    return "invalid";
    }
}

// ****************************************************************************
//  Method: vtkOpenFOAMReader::GetInternalVariableAtTimestep
//
//  Purpose:
//  returns the values for a request variable for the internal mesh
//
// ****************************************************************************
vtkDoubleArray * vtkOpenFOAMReader::GetInternalVariableAtTimestep
                 (const char * varNameIn, int timeState)
{
  vtkstd::string varName(varNameIn);
  vtkstd::stringstream varPath;
  varPath << this->PathPrefix->value << this->Steps[timeState] << "/" <<
    varName;
  vtkDebugMacro(<<"Get internal variable: "<<varPath.str().c_str());
  vtkDoubleArray *data = vtkDoubleArray::New();

  vtkstd::string temp;
  ifstream input;
  bool binaryWriteFormat;
  input.open(varPath.str().c_str(),ios::in);
  //make sure file exists
  if(input.fail())
    {
    return data;
    }

  //determine if file is binary or ascii
  while(temp.find("format") == vtkstd::string::npos)
    {
    getline(input, temp);
    }
  input.close();

  //reopen file in correct format
  if(temp.find("binary") != vtkstd::string::npos)
    {
    input.open(varPath.str().c_str(),ios::binary);
    binaryWriteFormat = true;
    }
  else
    {
    input.open(varPath.str().c_str(),ios::in);
    binaryWriteFormat = false;
    }


  vtkstd::string foamClass;
  vtkstd::stringstream tokenizer;
  double value;

  //find class
  tokenizer.str("");
  tokenizer.clear();
  vtkstd::getline(input, temp);
  while(temp.find("class") == vtkstd::string::npos)
    {
    vtkstd::getline(input, temp);
    }
  temp.erase(temp.begin()+temp.find(";"));
  tokenizer.str(temp);
  while(tokenizer >> foamClass)
    {
    };
  temp = "";
  tokenizer.str("");
  tokenizer.clear();
  //create scalar arrays
  if(foamClass =="volScalarField")
    {
    while(temp.find("internalField") == vtkstd::string::npos)
      {
      vtkstd::getline(input, temp);
      }
    //nonuniform
    if(!(temp.find("nonuniform") == vtkstd::string::npos))
      {
      //create an array
      vtkstd::getline(input,temp);
      tokenizer.str(temp);
      int scalarCount;
      tokenizer >> scalarCount;
      data->SetNumberOfValues(NumCells);

      //binary data
      if(binaryWriteFormat)
        {
        //add values to array
        input.get(); //parenthesis
        for(int i = 0; i < scalarCount; i++)
          {
          input.read((char *) &value, sizeof(double));
          data->SetValue(i, value);
          }
        }

      //ascii data
      else
        {
        //add values to array
        vtkstd::getline(input, temp); //discard (

        for(int i = 0; i < scalarCount; i++)
          {
          tokenizer.str("");
          tokenizer.clear();
          vtkstd::getline(input,temp);
          tokenizer.str(temp);
          tokenizer >> value;
          data->SetValue(i, value);
          }
        }
      }

    //uniform
    else if(!(temp.find("uniform") == vtkstd::string::npos))
      {
      //parse out the uniform value
      vtkstd::string token;
      temp.erase(temp.begin()+temp.find(";"));
      tokenizer.str(temp);
      while(tokenizer>>token);
      tokenizer.str("");
      tokenizer.clear();
      tokenizer.str(token);
      tokenizer>>value;
      data->SetNumberOfValues(NumCells);

      //create array of uniform values
      for(int i = 0; i < NumCells; i++)
        {
        data->SetValue(i, value);
        }
      }

    //no data
    else
      {
      return data;
      }
    }

  //create vector arrays
  else if(foamClass == "volVectorField")
    {
    vtkstd::getline(input, temp);
    while(temp.find("internalField") == vtkstd::string::npos)
      {
      vtkstd::getline(input, temp);
      }
    if(!(temp.find("nonuniform") == vtkstd::string::npos))
      {
      //create an array
      vtkstd::getline(input,temp);
      tokenizer.str(temp);
      int vectorCount;
      tokenizer >> vectorCount;
      data->SetNumberOfComponents(3);

      //binary data
      if(binaryWriteFormat)
        {
        //add values to the array
        input.get(); //parenthesis
        for(int i = 0; i < vectorCount; i++)
          {
          input.read((char *) &value, sizeof(double));
          data->InsertComponent(i, 0, value);
          input.read((char *) &value, sizeof(double));
          data->InsertComponent(i, 1, value);
          input.read((char *) &value, sizeof(double));
          data->InsertComponent(i, 2, value);
          }
        }

      //ascii data
      else
        {
        //add values to the array
        vtkstd::getline(input, temp); //discard (

        for(int i = 0; i < vectorCount; i++)
          {
          tokenizer.str("");
          tokenizer.clear();
          vtkstd::getline(input,temp);

          //REMOVE BRACKETS
          temp.erase(temp.begin()+temp.find("("));
          temp.erase(temp.begin()+temp.find(")"));

          //GRAB X,Y,&Z VALUES
          tokenizer.str(temp);
          tokenizer >> value;
          data->InsertComponent(i, 0, value);
          tokenizer >> value;
          data->InsertComponent(i, 1, value);
          tokenizer >> value;
          data->InsertComponent(i, 2, value);
          }
        }
      }
    else if(!(temp.find("uniform") == vtkstd::string::npos))
      {
      //create an array of uniform values
      double value1, value2, value3;

      //parse out the uniform values
      temp.erase(temp.begin(), temp.begin()+temp.find("(")+1);
      temp.erase(temp.begin()+temp.find(")"), temp.end());
      tokenizer.str(temp);
      tokenizer >> value1;
      tokenizer >> value2;
      tokenizer >> value3;

      data->SetNumberOfComponents(3);
      for(int i = 0; i < NumCells; i++)
        {
        data->InsertComponent(i, 0, value1);
        data->InsertComponent(i, 1, value2);
        data->InsertComponent(i, 2, value3);
        }
      }

    //no data
    else
      {
      return data;
      }
    }
  vtkDebugMacro(<<"Internal variable data read");
  return data;
}

// ****************************************************************************
//  Method: vtkOpenFOAMReader::GetBoundaryVariableAtTimestep
//
//  Purpose:
//  returns the values for a request variable for a bondary region
//
// ****************************************************************************
vtkDoubleArray * vtkOpenFOAMReader::GetBoundaryVariableAtTimestep
                (int boundaryIndex, const char * varNameIn, int timeState,
                 vtkUnstructuredGrid * internalMesh)
{
  vtkstd::string varName(varNameIn);
  vtkstd::stringstream varPath;
  varPath << this->PathPrefix->value << this->Steps[timeState] << "/" <<
    varName;
  vtkDebugMacro(<<"Get boundary variable: "<<varPath.str().c_str());
  vtkDoubleArray *data = vtkDoubleArray::New();

  vtkstd::string temp;
  ifstream input;
  bool binaryWriteFormat;
  input.open(varPath.str().c_str(),ios::in);
  //make sure file exists
  if(input.fail())
    {
    return data;
    }

  //determine if file is binary or ascii
  while(temp.find("format") == vtkstd::string::npos)
    {
    getline(input, temp);
    }
  input.close();

  //reopen file in correct format
  if(temp.find("binary") != vtkstd::string::npos)
    {
    input.open(varPath.str().c_str(),ios::binary);
    binaryWriteFormat = true;
    }
  else
    {
    input.open(varPath.str().c_str(),ios::in);
    binaryWriteFormat = false;
    }

  vtkstd::string foamClass;
  vtkstd::stringstream tokenizer;
  double value;

  //find class
  tokenizer.str("");
  tokenizer.clear();
  vtkstd::getline(input, temp);
  while(temp.find("class") == vtkstd::string::npos)
    {
    vtkstd::getline(input, temp);
    }
  temp.erase(temp.begin()+temp.find(";"));
  tokenizer.str(temp);
  while(tokenizer >> foamClass);
  temp = "";
  tokenizer.str("");
  tokenizer.clear();
  //create scalar arrays
  if(foamClass =="volScalarField")
    {
    //find desired mesh
    while(temp.find(this->BoundaryNames->value[boundaryIndex]) == 
            vtkstd::string::npos && !input.eof())
      {
      vtkstd::getline(input, temp);
      }
    if(input.eof())
      {
      return data;
      }
    //find value entry
    while(temp.find("}") == vtkstd::string::npos &&
          temp.find("value ") == vtkstd::string::npos)
      {
      vtkstd::getline(input, temp);  //find value
      }

    //nonuniform
    if(!(temp.find("nonuniform") == vtkstd::string::npos))
      {

      //binary data
      if(binaryWriteFormat)
        {
        //create an array
        vtkstd::getline(input,temp);
        tokenizer.str(temp);
        int scalarCount;
        tokenizer >> scalarCount;
        data->SetNumberOfValues(scalarCount);

        //assign values to the array
        input.get(); //parenthesis
        for(int i = 0; i < scalarCount; i++)
          {
          input.read((char *) &value, sizeof(double));
          data->SetValue(i, value);
          }
        }

      //ascii data
      else
        {
        temp.erase(temp.begin(), temp.begin()+temp.find(">")+1);
        //ascii data with 10 or less values are on the same line
        //>10
        if(temp == vtkstd::string(" "))
          {
          //create an array of data
          vtkstd::getline(input,temp);
          tokenizer.str(temp);
          int scalarCount;
          tokenizer >> scalarCount;
          data->SetNumberOfValues(scalarCount);
          vtkstd::getline(input, temp); //discard (

          for(int i = 0; i < scalarCount; i++)
            {
            tokenizer.str("");
            tokenizer.clear();
            vtkstd::getline(input,temp);
            tokenizer.str(temp);
            tokenizer >> value;
            data->SetValue(i, value);
            }
          }
        //=<10
        else
          {
          //create an array with data
          tokenizer.str(temp);
          int scalarCount;
          tokenizer >> scalarCount;
          data->SetNumberOfValues(scalarCount);
          temp.erase(temp.begin(), temp.begin()+temp.find("(")+1);
          temp.erase(temp.begin()+temp.find(")"), temp.end());

          tokenizer.str("");
          tokenizer.clear();
          tokenizer.str(temp);
          for(int i = 0; i < scalarCount; i++)
            {
            tokenizer >> value;
            data->SetValue(i, value);
            }
          }
        }
      }

    //uniform
    else if(!(temp.find("uniform") == vtkstd::string::npos))
      {
      //create an array of uniform values
      double value1 = 0;
      temp.erase(temp.begin(), temp.begin()+temp.find("uniform")+7);
      temp.erase(temp.begin()+temp.find(";"), temp.end());
      tokenizer.str("");
      tokenizer.clear();
      tokenizer.str(temp);
      tokenizer >> value1;
      data->SetNumberOfValues(this->SizeOfBoundary->value[boundaryIndex]);
      for(int i = 0; i < this->SizeOfBoundary->value[boundaryIndex]; i++)
        {
        data->SetValue(i, value1);
        }
      }

    //no data
    else
      {
      int cellId;
      vtkDataArray * internalData = internalMesh->GetCellData()->
                                    GetArray(varName.c_str());
      data->SetNumberOfValues(this->SizeOfBoundary->value[boundaryIndex]);
      for(int i = 0; i < this->SizeOfBoundary->value[boundaryIndex]; i++)
        {
        cellId = this->FaceOwner->GetValue(this->StartFace + i);
        data->SetValue(i, internalData->GetComponent(cellId, 0));
        }
      return data;
      }
    }
  //CREATE VECTOR ARRAYS
  else if(foamClass == "volVectorField")
    {
    while(temp.find(this->BoundaryNames->value[boundaryIndex]) == 
          vtkstd::string::npos && !input.eof())
      {
      vtkstd::getline(input, temp);
      }
    if(input.eof())
      {
      return data;
      }
    while(temp.find("}") == vtkstd::string::npos &&
          temp.find("value ") == vtkstd::string::npos)
      {
      vtkstd::getline(input, temp);  //find value
      }
    //nonuniform
    if(!(temp.find("nonuniform") == vtkstd::string::npos))
      {
      //create an array
      vtkstd::getline(input,temp);
      tokenizer.str(temp);
      int vectorCount;
      tokenizer >> vectorCount;
      data->SetNumberOfComponents(3);

      //binary data
      if(binaryWriteFormat)
        {
        //insert values into the array
        input.get(); //parenthesis
        for(int i = 0; i < vectorCount; i++)
          {
          input.read((char *) &value, sizeof(double));
          data->InsertComponent(i, 0, value);
          input.read((char *) &value, sizeof(double));
          data->InsertComponent(i, 1, value);
          input.read((char *) &value, sizeof(double));
          data->InsertComponent(i, 2, value);
          }
        }

        //ascii data
        else
          {
          //insert values into the array
          vtkstd::getline(input, temp); //discard (
          for(int i = 0; i < vectorCount; i++)
            {
            tokenizer.str("");
            tokenizer.clear();
            vtkstd::getline(input,temp);

            //REMOVE BRACKETS
            temp.erase(temp.begin()+temp.find("("));
            temp.erase(temp.begin()+temp.find(")"));

            //GRAB X,Y,&Z VALUES
            tokenizer.str(temp);
            tokenizer >> value;
            data->InsertComponent(i, 0, value);
            tokenizer >> value;
            data->InsertComponent(i, 1, value);
            tokenizer >> value;
            data->InsertComponent(i, 2, value);
            }
          }
        }

    //uniform
    else if(!(temp.find("uniform") == vtkstd::string::npos))
      {
      //create an array of uniform values
      double value1 = 0, value2 = 0, value3 = 0;
      temp.erase(temp.begin(), temp.begin()+temp.find("(")+1);
      temp.erase(temp.begin()+temp.find(")"), temp.end());
      tokenizer.str("");
      tokenizer.clear();
      tokenizer.str(temp);
      tokenizer >> value1;
      tokenizer >> value2;
      tokenizer >> value3;

      data->SetNumberOfComponents(3);
      for(int i = 0; i < this->SizeOfBoundary->value[boundaryIndex]; i++)
        {
        data->InsertComponent(i, 0, value1);
        data->InsertComponent(i, 1, value2);
        data->InsertComponent(i, 2, value3);
        }
      }

    //no data
    else
      {
      int cellId;
      vtkDataArray * internalData = internalMesh->GetCellData()->
                                    GetArray(varName.c_str());
      data->SetNumberOfComponents(3);
      for(int i = 0; i < this->SizeOfBoundary->value[boundaryIndex]; i++)
        {
        cellId = this->FaceOwner->GetValue(this->StartFace + i);
        data->InsertComponent(i, 0, internalData->GetComponent(cellId, 0));
        data->InsertComponent(i, 1, internalData->GetComponent(cellId, 1));
        data->InsertComponent(i, 2, internalData->GetComponent(cellId, 2));
        }
      return data;
      }
    }
  vtkDebugMacro(<<"Boundary data read");
  return data;
}

// ****************************************************************************
//  Method: vtkOpenFOAMReader::GatherBlocks
//
//  Purpose:
//  returns a vector of block names for a specified domain
//
// ****************************************************************************
stringVector *vtkOpenFOAMReader::GatherBlocks(const char * typeIn,
                                              int timeState)
{
  vtkstd::string type(typeIn);
  vtkstd::string blockPath = this->PathPrefix->value +
                             this->PolyMeshFacesDir->value[timeState] +
                             "/polyMesh/"+type;
  vtkstd::vector< vtkstd::string > blocks;
  stringVector *returnValue = new stringVector;
  vtkDebugMacro(<<"Get blocks: "<<blockPath.c_str());

  ifstream input;
  input.open(blockPath.c_str(), ios::in);
  //if there is no file return a null vector
  if(input.fail())
    {
    returnValue->value = blocks;
    return returnValue;
    }

  vtkstd::string temp;
  vtkstd::string token;
  vtkstd::stringstream tokenizer;
  vtkstd::string tempName;

  //find end of header
  //while(temp.compare(0,4,"// *",0,4)!=0)
  while (strcmp(temp.substr(0,4).c_str(), "// *"))
    {
    vtkstd::getline(input, temp);
    }
  vtkstd::getline(input, temp); //throw out blank line
  vtkstd::getline(input, temp);

  //Number of blocks
  tokenizer.str(temp);
  tokenizer >> this->NumBlocks;
  blocks.resize(this->NumBlocks);

  //loop through each block
  for(int i = 0; i < this->NumBlocks; i++)
    {
    vtkstd::getline(input, temp); //throw out blank line

    //NAME
    vtkstd::getline(input, temp); //name
    tokenizer.clear();
    tokenizer.str(temp);
    tokenizer>>tempName;
    blocks[i] = tempName;
    //while(temp.compare(0,1,"}",0,1) != 0)
    while (strcmp(temp.substr(0,1).c_str(), "}"))
      {
      vtkstd::getline(input, temp);
      }
    }
  returnValue->value = blocks;
  return returnValue;
}

// ****************************************************************************
//  Method: vtkOpenFOAMReader::GetBoundaryMesh
//
//  Purpose:
//  returns a requested boundary mesh
//
// ****************************************************************************
vtkUnstructuredGrid * vtkOpenFOAMReader::GetBoundaryMesh(int timeState,
                                                         int boundaryIndex)
{
  vtkUnstructuredGrid * boundaryMesh = vtkUnstructuredGrid::New();
  vtkstd::string boundaryPath = this->PathPrefix->value +
                                this->PolyMeshFacesDir->value[timeState] +
                                "/polyMesh/boundary";
  vtkDebugMacro(<<"Create boundary mesh: "<<boundaryPath.c_str());

  int nFaces;

  ifstream input;
  input.open(boundaryPath.c_str(), ios::in);
  //return a Null object
  if(input.fail())
    {
    return boundaryMesh;
    }

  vtkstd::string temp;
  vtkstd::string token;
  vtkstd::stringstream tokenizer;

  //find desired mesh entry
  while(temp.find(this->BoundaryNames->value[boundaryIndex]) == 
        vtkstd::string::npos)
    {
    vtkstd::getline(input, temp);
    }

  //get nFaces
  while(temp.find("nFaces") == vtkstd::string::npos)
    {
    vtkstd::getline(input, temp);
    }
  temp.erase(temp.begin()+temp.find(";")); //remove ;
  tokenizer.clear();
  tokenizer.str(temp);
  while(tokenizer >> token);
  tokenizer.clear();
  tokenizer.str(token);
  tokenizer >> nFaces;
  //get startface
  vtkstd::getline(input, temp);
  //look for "startFaces"
  while(temp.find("startFace") == vtkstd::string::npos)
    {
    vtkstd::getline(input, temp);
    }
  temp.erase(temp.begin()+temp.find(";")); //remove ;
  tokenizer.clear();
  tokenizer.str(temp);
  while(tokenizer >> token);
  tokenizer.clear();
  tokenizer.str(token);
  tokenizer>>this->StartFace;

  //Create the mesh
  int j, k;
  vtkTriangle * triangle;
  vtkQuad * quad;
  vtkPolygon * polygon;
  int endFace = this->StartFace + nFaces;
  //loop through each face
  for(j = this->StartFace; j < endFace; j++)
    {

    //triangle
    if(this->FacePoints->value[j].size() == 3)
      {
      triangle = vtkTriangle::New();
      for(k = 0; k < 3; k++)
        {
        triangle->GetPointIds()->SetId(k, this->FacePoints->value[j][k]);
        }
      boundaryMesh->InsertNextCell(triangle->GetCellType(),
      triangle->GetPointIds());
      triangle->Delete();
      }

    //quad
    else if(this->FacePoints->value[j].size() == 4)
      {
      quad = vtkQuad::New();
      for(k = 0; k < 4; k++) 
        {
        quad->GetPointIds()->SetId(k, this->FacePoints->value[j][k]);
        }
      boundaryMesh->InsertNextCell(quad->GetCellType(),
      quad->GetPointIds());
      quad->Delete();
      }

    //polygon
    else
      {
      polygon = vtkPolygon::New();
      for(k = 0; k < (int)this->FacePoints->value[j].size(); k++) 
        {
        polygon->GetPointIds()->InsertId(k, this->FacePoints->value[j][k]);
        }
      boundaryMesh->InsertNextCell(polygon->GetCellType(),
      polygon->GetPointIds());
      polygon->Delete();
      }
    }

  //set points for boundary
  boundaryMesh->SetPoints(this->Points);
  //add size of mesh
  this->SizeOfBoundary->value.push_back(boundaryMesh->GetNumberOfCells());
  vtkDebugMacro(<<"Boundary mesh created");
  return boundaryMesh;
}

// ****************************************************************************
//  Method: vtkOpenFOAMReader::GetPointZoneMesh
//
//  Purpose:
//  returns a requested point zone mesh
//
// ****************************************************************************
vtkUnstructuredGrid * vtkOpenFOAMReader::GetPointZoneMesh(int timeState,
                                                          int pointZoneIndex)
{
  vtkUnstructuredGrid * pointZoneMesh = vtkUnstructuredGrid::New();
  vtkstd::string pointZonesPath = this->PathPrefix->value[timeState]+
                               "/polyMesh/pointZones";
  vtkDebugMacro(<<"Create point zone mesh: "<<pointZonesPath.c_str());

  vtkstd::string temp;
  ifstream input;
  bool binaryWriteFormat;
  input.open(pointZonesPath.c_str(),ios::in);
  //make sure file exists  
  if(input.fail()) 
    {
    return pointZoneMesh;
    }

  //determine if file is binary or ascii
  while(temp.find("format") == vtkstd::string::npos)
    {
    getline(input, temp);
    }
  input.close();

  //reopen file in correct format
  if(temp.find("binary") != vtkstd::string::npos)
    {
    input.open(pointZonesPath.c_str(),ios::binary);
    binaryWriteFormat = true;
    }
  else
    {
    input.open(pointZonesPath.c_str(),ios::in);
    binaryWriteFormat = false;
    }

  vtkstd::string token;
  vtkstd::stringstream tokenizer;
  vtkVertex * pointCell;
  int tempElement;
  vtkstd::vector< vtkstd::vector < int > > tempElementZones;
  int numElement;

  //find desired mesh entry
  while(temp.find(this->PointZoneNames->value[pointZoneIndex]) == 
        vtkstd::string::npos)
    {
    vtkstd::getline(input, temp);
    }
  vtkstd::getline(input, temp); //throw out {
  vtkstd::getline(input, temp);  //type
  vtkstd::getline(input, temp);  //label
  vtkstd::getline(input, temp);  //number of elements or {

  //number of elements
  if(temp.find("}") == vtkstd::string::npos)
    {
    tokenizer.clear();
    tokenizer.str(temp);
    tokenizer>>numElement;
    if(numElement == 0)
      {
      return NULL;
      }

    //binary data
    if(binaryWriteFormat)
      {
      input.get(); //parenthesis
      for(int j = 0; j < numElement; j++)
        {
        input.read((char *) &tempElement, sizeof(int));
        pointCell = vtkVertex::New();
        pointCell->GetPointIds()->SetId(0,tempElement);
        pointZoneMesh->InsertNextCell(pointCell->GetCellType(),
                                      pointCell->GetPointIds());
        pointCell->Delete();
        }
      }

    //ascii data
    else
      {
      vtkstd::getline(input, temp);//THROW OUT (

      //GET EACH ELEMENT & ADD TO VECTOR
      for(int j = 0; j < numElement; j++)
        {
        vtkstd::getline(input, temp);
        tokenizer.clear();
        tokenizer.str(temp);
        tokenizer>>tempElement;
        pointCell = vtkVertex::New();
        pointCell->GetPointIds()->SetId(0,tempElement);
        pointZoneMesh->InsertNextCell(pointCell->GetCellType(),
                                      pointCell->GetPointIds());
        pointCell->Delete();
        }
      }
    }

  //there is no entry
  else
    {
    return NULL;
    }
  //set point zone points
  pointZoneMesh->SetPoints(Points);
  vtkDebugMacro(<<"Point zone mesh created");
  return pointZoneMesh;
}

// ****************************************************************************
//  Method: vtkOpenFOAMReader::GetFaceZoneMesh
//
//  Purpose:
//  returns a requested face zone mesh
//
// ****************************************************************************
vtkUnstructuredGrid * vtkOpenFOAMReader::GetFaceZoneMesh(int timeState,
                                                         int faceZoneIndex)
{
  vtkUnstructuredGrid * faceZoneMesh = vtkUnstructuredGrid::New();
  vtkstd::string faceZonesPath = this->PathPrefix->value +
                                 this->PolyMeshFacesDir->value[timeState] +
                                 "/polyMesh/faceZones";
  vtkDebugMacro(<<"Create face zone mesh: "<<faceZonesPath.c_str());

  vtkstd::string temp;
  ifstream input;
  bool binaryWriteFormat;
  input.open(faceZonesPath.c_str(),ios::in);
  //make sure file exists  
  if(input.fail()) 
    {
    return faceZoneMesh;
    }

  //determine if file is binary or ascii
  while(temp.find("format") == vtkstd::string::npos)
    {
    getline(input, temp);
    }
  input.close();

  //reopen file in correct format
  if(temp.find("binary") != vtkstd::string::npos)
    {
    input.open(faceZonesPath.c_str(),ios::binary);
    binaryWriteFormat = true;
    }
  else
    {
    input.open(faceZonesPath.c_str(),ios::in);
    binaryWriteFormat = false;
    }

  vtkstd::string token;
  vtkstd::stringstream tokenizer;
  vtkstd::vector< int > faceZone;
  int tempElement;
  vtkstd::vector< vtkstd::vector < int > > tempElementZones;
  int numElement;

  //find desired mesh entry
  while(temp.find(this->FaceZoneNames->value[faceZoneIndex]) == 
        vtkstd::string::npos)
    {
    vtkstd::getline(input, temp);
    }

  vtkstd::getline(input, temp); //throw out {
  vtkstd::getline(input, temp);  //type
  vtkstd::getline(input, temp);  //label
  vtkstd::getline(input, temp);  //number of values or flipmap

  if(temp.find("flipMap") == vtkstd::string::npos)
    {
    //number of elements
    tokenizer.clear();
    tokenizer.str(temp);
    tokenizer>>numElement;
    if(numElement == 0)
      {
      return NULL;
      }

    //binary
    if(binaryWriteFormat)
      {
      input.get(); //parenthesis
      for(int j = 0; j < numElement; j++)
        {
        input.read((char *) &tempElement, sizeof(int));
        faceZone.push_back(tempElement);
        }
      }

    //ascii
    else
      {
      //THROW OUT (
      vtkstd::getline(input, temp);

      //get each element & add to vector
      for(int j = 0; j < numElement; j++)
        {
        vtkstd::getline(input, temp);
        tokenizer.clear();
        tokenizer.str(temp);
        tokenizer>>tempElement;
        faceZone.push_back(tempElement);
        }
      }
    }

  //Create the mesh
  int k;
  vtkTriangle * triangle;
  vtkQuad * quad;
  vtkPolygon * polygon;

  //LOOP THROUGH EACH FACE
  for(int j = 0; j < (int)faceZone.size(); j++)
    {

    //Triangular Face
    if(this->FacePoints->value[faceZone[j]].size() == 3)
      {
      triangle = vtkTriangle::New();
      for(k = 0; k < 3; k++)
        {
        triangle->GetPointIds()->SetId(k, this->FacePoints->value[
        faceZone[j]][k]);
        }
      faceZoneMesh->InsertNextCell(triangle->GetCellType(),
        triangle->GetPointIds());
      triangle->Delete();
      }

    //Quadraic Face
    else if(this->FacePoints->value[faceZone[j]].size() == 4)
      {
      quad = vtkQuad::New();
      for(k = 0; k < 4; k++)
        {
        quad->GetPointIds()->SetId(k,
          this->FacePoints->value[faceZone[j]][k]);
        }
      faceZoneMesh->InsertNextCell(quad->GetCellType(),
        quad->GetPointIds());
      quad->Delete();
      }

    //Polygonal Face
    else
      {
      polygon = vtkPolygon::New();
      for(k = 0; k < (int)this->FacePoints->value[faceZone[j]].size(); k++)
        {
        polygon->GetPointIds()->InsertId(k, this->FacePoints->value[
          faceZone[j]][k]);
        }
      faceZoneMesh->InsertNextCell(polygon->GetCellType(),
        polygon->GetPointIds());
      polygon->Delete();
      }
    }

  //set the face zone points
  faceZoneMesh->SetPoints(this->Points);
  vtkDebugMacro(<<"Face zone mesh created");
  return faceZoneMesh;
}

// ****************************************************************************
//  Method: vtkOpenFOAMReader::GetCellZoneMesh
//
//  Purpose:
//  returns a requested cell zone mesh
//
// ****************************************************************************
vtkUnstructuredGrid * vtkOpenFOAMReader::GetCellZoneMesh(int timeState,
                                                         int cellZoneIndex)
{
  vtkUnstructuredGrid * cellZoneMesh = vtkUnstructuredGrid::New();
  vtkstd::string cellZonesPath = this->PathPrefix->value +
                                 this->PolyMeshFacesDir->value[timeState] +
                                 "/polyMesh/cellZones";
  vtkDebugMacro(<<"Create cell zone mesh: "<<cellZonesPath.c_str());

  vtkstd::string temp;
  ifstream input;
  bool binaryWriteFormat;
  input.open(cellZonesPath.c_str(),ios::in);
  //make sure file exists  
  if(input.fail()) 
    {
    return cellZoneMesh;
    }

  //determine if file is binary or ascii
  while(temp.find("format") == vtkstd::string::npos)
    {
    getline(input, temp);
    }
  input.close();

  //reopen file in correct format
  if(temp.find("binary") != vtkstd::string::npos)
    {
    input.open(cellZonesPath.c_str(),ios::binary);
    binaryWriteFormat = true;
    }
  else
    {
    input.open(cellZonesPath.c_str(),ios::in);
    binaryWriteFormat = false;
    }

  vtkstd::string token;
  vtkstd::stringstream tokenizer;
  vtkstd::vector< int > cellZone;
  int tempElement;
  vtkstd::vector< vtkstd::vector < int > > tempElementZones;
  int numElement;

  //find desired mesh entry
  while(temp.find(this->CellZoneNames->value[cellZoneIndex]) == 
        vtkstd::string::npos)
    {
    vtkstd::getline(input, temp);
    }
  vtkstd::getline(input, temp);  //throw out {
  vtkstd::getline(input, temp);  //type
  vtkstd::getline(input, temp);  //label
  vtkstd::getline(input, temp);

  //number of elements
  tokenizer.clear();
  tokenizer.str(temp);
  tokenizer>>numElement;

  //binary
  if(binaryWriteFormat)
    {
    input.get(); //parenthesis
    for(int j = 0; j < numElement; j++)
      {
      input.read((char *) &tempElement, sizeof(int));
      cellZone.push_back(tempElement);
      }
    }

  //ascii
  else
    {
    vtkstd::getline(input, temp); //throw out (

    //get each element & add to vector
    for(int j = 0; j < numElement; j++)
      {
      vtkstd::getline(input, temp);
      tokenizer.clear();
      tokenizer.str(temp);
      tokenizer>>tempElement;
      cellZone.push_back(tempElement);
      }
    }

  //Create the mesh
  bool foundDup = false;
  vtkstd::vector< int > cellPoints;
  vtkstd::vector< int > tempFaces[2];
  vtkstd::vector< int > firstFace;
  int pivotPoint = 0;
  int i, j, k, l, pCount;
  int faceCount = 0;

  //Create Mesh
  for(i = 0; i < (int)cellZone.size(); i++)  //each cell
    {
    //calculate total points for all faces of a cell
    //used to determine cell type
    int totalPointCount = 0;
    for(j = 0; j < (int)this->FacesOfCell->value[
      cellZone[i]].size(); j++)  //each face
      {
      totalPointCount += (int)this->FacePoints->value[this->FacesOfCell->value[
        cellZone[i]][j].faceIndex].size();
      }

      // using cell type - order points, create cell, add to mesh

    //OFhex | vtkHexahedron
    if (totalPointCount == 24) 
      {
      faceCount = 0;

      //get first face
      for(j = 0; j < (int)this->FacePoints->value[this->FacesOfCell->value[
        cellZone[i]][0].faceIndex].size(); j++)
        {
        firstFace.push_back(this->FacePoints->value[this->FacesOfCell->value[
          cellZone[i]][0].faceIndex][j]);
        }

      //-if it is a neighbor face flip it
      if(FacesOfCell->value[i][0].neighborFace)
        {
        int tempPop;
        for(k = 0; k < (int)firstFace.size() - 1; k++)
          {
          tempPop = firstFace[firstFace.size()-1];
          firstFace.pop_back();
          firstFace.insert(firstFace.begin()+1+k, tempPop);
          }
        }

      //add first face to cell points
      for(j =0; j < (int)firstFace.size(); j++)
        {
        cellPoints.push_back(firstFace[j]);
        }

      for(int pointCount = 0;pointCount < (int)firstFace.size();pointCount++)
        {
        //find the 2 other faces containing each point - start with face 1
        for(j = 1; j < (int)this->FacesOfCell->value[
          cellZone[i]].size(); j++)  //each face
          {
          for(k = 0; k < (int)this->FacePoints->value[this->FacesOfCell->
              value[cellZone[i]][j].faceIndex].size(); k++)
            {
            if(firstFace[pointCount] == this->FacePoints->
               value[this->FacesOfCell->value[cellZone[i]][j].faceIndex][k])
              {
              //another face with the point
              for(l = 0; l < (int)this->FacePoints->value[this->FacesOfCell->
                                  value[cellZone[i]][j].faceIndex].size(); l++)
                {
                tempFaces[faceCount].push_back(this->FacePoints->value[
                  this->FacesOfCell->value[cellZone[i]][j].faceIndex]
                  [l]);
                }
              faceCount++;
              }
            }
          }

        //locate the pivot point contained in faces 0 & 1
        for(j = 0; j < (int)tempFaces[0].size(); j++)
          {
          for(k = 0; k < (int)tempFaces[1].size(); k++)
            {
            if(tempFaces[0][j] == tempFaces[1][k] && tempFaces[0][j] !=
              firstFace[pointCount])
              {
              pivotPoint = tempFaces[0][j];
              break;
              }
            }
          }
        cellPoints.push_back(pivotPoint);
        tempFaces[0].clear();
        tempFaces[1].clear();
        faceCount=0;
        }

        //create the hex cell and insert it into the mesh
        vtkHexahedron * hexahedron = vtkHexahedron::New();
        for(pCount = 0; pCount < (int)cellPoints.size(); pCount++)
          {
          hexahedron->GetPointIds()->SetId(pCount, cellPoints[pCount]);
          }
        cellZoneMesh->InsertNextCell(hexahedron->GetCellType(),
                                     hexahedron->GetPointIds());
        hexahedron->Delete();
        cellPoints.clear();
        firstFace.clear();
        }

    //OFprism | vtkWedge
    else if (totalPointCount == 18) 
      {
      faceCount = 0;
      int index = 0;

      //find first triangular face
      for(j = 0; j < (int)this->FacesOfCell->value[
        cellZone[i]].size(); j++)  //each face
        {
        if((int)this->FacePoints->value[this->FacesOfCell->value[
          cellZone[i]][j].faceIndex].size() == 3)
          {
          for(k = 0; k < (int)this->FacePoints->value[this->FacesOfCell->value[
            cellZone[i]][j].faceIndex].size(); k++)
            {
            firstFace.push_back(this->FacePoints->value[this->FacesOfCell->
                                  value[cellZone[i]][j].faceIndex][k]);
            index = j;
            }
          break;
          }
        }

      //-if it is a neighbor face flip it
      if(this->FacesOfCell->value[i][0].neighborFace)
        {
        int tempPop;
        for(k = 0; k < (int)firstFace.size() - 1; k++)
          {
          tempPop = firstFace[firstFace.size()-1];
          firstFace.pop_back();
          firstFace.insert(firstFace.begin()+1+k, tempPop);
          }
        }

      //add first face to cell points
      for(j =0; j < (int)firstFace.size(); j++)
        {
        cellPoints.push_back(firstFace[j]);
        }

      for(int pointCount = 0;pointCount < (int)firstFace.size();pointCount++)
        {
        //find the 2 other faces containing each point
        for(j = 0; j < (int)this->FacesOfCell->value[
          cellZone[i]].size(); j++)  //each face
          {
          for(k = 0; k < (int)this->FacePoints->value[this->FacesOfCell->value[
            cellZone[i]][j].faceIndex].size(); k++) //each point
            {
            if(firstFace[pointCount] == this->FacePoints->
               value[this->FacesOfCell->value[cellZone[i]][j].faceIndex][k] &&
               j != index)
              {
              //another face with point
              for(l = 0; l < (int)this->FacePoints->value[this->
                  FacesOfCell->value[cellZone[i]][j].faceIndex].size(); l++)
                {
                tempFaces[faceCount].push_back(this->FacePoints->value[
                  this->FacesOfCell->value[cellZone[i]][j].faceIndex]
                  [l]);
                }
              faceCount++;
              }
            }
          }

        //locate the pivot point contained in faces 0 & 1
        for(j = 0; j < (int)tempFaces[0].size(); j++)
          {
          for(k = 0; k < (int)tempFaces[1].size(); k++)
            {
            if(tempFaces[0][j] == tempFaces[1][k] && tempFaces[0][j] != 
              firstFace[pointCount])
              {
              pivotPoint = tempFaces[0][j];
              break;
              }
            }
          }
        cellPoints.push_back(pivotPoint);
        tempFaces[0].clear();
        tempFaces[1].clear();
        faceCount=0;
        }

      //create the wedge cell and insert it into the mesh
      vtkWedge * wedge = vtkWedge::New();
      for(pCount = 0; pCount < (int)cellPoints.size(); pCount++)
        {
        wedge->GetPointIds()->SetId(pCount, cellPoints[pCount]);
        }
      cellZoneMesh->InsertNextCell(wedge->GetCellType(),
                                   wedge->GetPointIds());
      cellPoints.clear();
      wedge->Delete(); 
      firstFace.clear();
      }

    //OFpyramid | vtkPyramid
    else if (totalPointCount == 16) 
      {
      foundDup = false;

      //find quad
      for(j = 0; j < (int)this->FacesOfCell->value[
        cellZone[i]].size(); j++)  //each face
        {
        if((int)this->FacePoints->value[this->FacesOfCell->value[
          cellZone[i]][j].faceIndex].size() == 4)
          {
          for(k = 0; k < (int)this->FacePoints->value[this->FacesOfCell->value[
            cellZone[i]][j].faceIndex].size(); k++)
            {
            cellPoints.push_back(this->FacePoints->value[this->FacesOfCell->
              value[cellZone[i]][j].faceIndex][k]);
            }
          break;
          }
        }

      //compare first face points to second faces
      for(j = 0; j < (int)cellPoints.size(); j++) //each point
        {
        for(k = 0; k < (int)this->FacePoints->value[this->FacesOfCell->value[
          cellZone[i]][1].faceIndex].size(); k++)
          {
          if(cellPoints[j] == this->FacePoints->value[this->FacesOfCell->value[
            cellZone[i]][1].faceIndex][k])
            {
            foundDup = true;
            }
          }
        if(!foundDup)
          {
          cellPoints.push_back(this->FacePoints->value[this->FacesOfCell->value[
            cellZone[i]][j].faceIndex][k]);
          break;
          }
        }

      //create the pyramid cell and insert it into the mesh
      vtkPyramid * pyramid = vtkPyramid::New();
      for(pCount = 0; pCount < (int)cellPoints.size(); pCount++)
        {
        pyramid->GetPointIds()->SetId(pCount, cellPoints[pCount]);
        }
      cellZoneMesh->InsertNextCell(pyramid->GetCellType(),
                                   pyramid->GetPointIds());
      cellPoints.clear();
      pyramid->Delete(); 
      }

    //OFtet | vtkTetrahedron
    else if (totalPointCount == 12) 
      {
      foundDup = false;

      //grab first face
      for(j = 0; j < (int)this->FacePoints->value[this->FacesOfCell->value[
        cellZone[i]][0].faceIndex].size(); j++)
        {
        cellPoints.push_back(this->FacePoints->value[this->FacesOfCell->value[
          cellZone[i]][0].faceIndex][j]);
        }

      //compare first face points to second faces
      for(j = 0; j < (int)cellPoints.size(); j++) //each point
        {
        for(k = 0; k < (int)this->FacePoints->value[this->FacesOfCell->value[
          cellZone[i]][1].faceIndex].size(); k++)
          {
          if(cellPoints[j] == this->FacePoints->value[this->FacesOfCell->value[
            cellZone[i]][1].faceIndex][k])
            {
            foundDup = true;
            }
          }
        if(!foundDup)
          {
          cellPoints.push_back(this->FacePoints->value[this->FacesOfCell->value[
            cellZone[i]][j].faceIndex][k]);
          break;
          }
        }

      //create the wedge cell and insert it into the mesh
      vtkTetra * tetra = vtkTetra::New();
      for(pCount = 0; pCount < (int)cellPoints.size(); pCount++)
        {
        tetra->GetPointIds()->SetId(pCount, cellPoints[pCount]);
        }
      cellZoneMesh->InsertNextCell(tetra->GetCellType(),
                                   tetra->GetPointIds());
      cellPoints.clear();
      tetra->Delete();
      }

    //OFpolyhedron || vtkConvexPointSet
    else
      {
      cout<<"Warning: Polyhedral Data is very Slow!"<<endl;
      foundDup = false;

      //grab face 0
      for(j = 0; j < (int)this->FacePoints->value[this->FacesOfCell->value[
        cellZone[i]][0].faceIndex].size(); j++)
        {
        firstFace.push_back(this->FacePoints->value[
          this->FacesOfCell->value[i][0].faceIndex][j]);
        }

      //ADD FIRST FACE TO CELL POINTS
      for(j =0; j < (int)firstFace.size(); j++)
        {
        cellPoints.push_back(firstFace[j]);
        }
      //j = 1 skip firstFace
      for(j = 1; j < (int) this->FacesOfCell->value[
           cellZone[i]].size(); j++)
        {
        //remove duplicate points from faces
        for(k = 0; k < (int)this->FacePoints->value[
            this->FacesOfCell->value[i][j].faceIndex].size(); k++)
          {
          for(l = 0; l < (int)cellPoints.size(); l++);
            {
            if(cellPoints[l] == this->FacePoints->value[this->FacesOfCell->
                                value[cellZone[i]][j].faceIndex][k])
              {
              foundDup = true;
              }
            }
          if(!foundDup)
            {
            cellPoints.push_back(this->FacePoints->value[this->FacesOfCell->
                                 value[cellZone[i]][j].faceIndex][k]);
            foundDup = false;
            }
          }
        }

      //create the poly cell and insert it into the mesh
      vtkConvexPointSet * poly = vtkConvexPointSet::New();
      poly->GetPointIds()->SetNumberOfIds(cellPoints.size());
      for(pCount = 0; pCount < (int)cellPoints.size(); pCount++)
        {
        poly->GetPointIds()->SetId(pCount, cellPoints[pCount]);
        }
      cellZoneMesh->InsertNextCell(poly->GetCellType(),
                                   poly->GetPointIds());
      cellPoints.clear();
      firstFace.clear();
      poly->Delete();
      }
    }

  //set cell zone points
  cellZoneMesh->SetPoints(Points);
  vtkDebugMacro(<<"Cell zone mesh created");
  return cellZoneMesh;
}

void vtkOpenFOAMReader::CreateDataSet(vtkMultiBlockDataSet *output)
{
  int timeState = this->TimeStep;
  //create paths to polyMesh files
  vtkstd::string boundaryPath = this->PathPrefix->value +
                                this->PolyMeshFacesDir->value[timeState] +
                                "/polyMesh/boundary";
  vtkstd::string facePath = this->PathPrefix->value +
                            this->PolyMeshFacesDir->value[timeState] +
                            "/polyMesh/faces";
  vtkstd::string ownerPath = this->PathPrefix->value +
                             this->PolyMeshFacesDir->value[timeState] +
                             "/polyMesh/owner";
  vtkstd::string neighborPath = this->PathPrefix->value +
                                this->PolyMeshFacesDir->value[timeState] +
                                "/polyMesh/neighbour";
  //create the faces vector
  this->ReadFacesFile(facePath.c_str());

  //create the faces owner vector
  this->ReadOwnerFile(ownerPath.c_str());

  //create the faces neighbor vector
  this->ReadNeighborFile(neighborPath.c_str());

  //create a vector containing a faces of each cell
  this->CombineOwnerNeigbor();

  this->GetPoints(timeState); //get the points

  //get the names of the regions
  this->BoundaryNames->value =
    this->GatherBlocks("boundary", timeState)->value;
  this->PointZoneNames->value =
    this->GatherBlocks("pointZones", timeState)->value;
  this->FaceZoneNames->value =
    this->GatherBlocks("faceZones", timeState)->value;
  this->CellZoneNames->value =
    this->GatherBlocks("cellZones", timeState)->value;

  int numBoundaries = this->BoundaryNames->value.size();
  int numPointZones = this->PointZoneNames->value.size();
  int numFaceZones = this->FaceZoneNames->value.size();
  int numCellZones = this->CellZoneNames->value.size();

  //Internal Mesh
  vtkUnstructuredGrid * internalMesh = this->MakeInternalMesh();
  for(int i = 0; i < (int)this->TimeStepData->value.size(); i++)
    {
    vtkDoubleArray * data =
      this->GetInternalVariableAtTimestep(this->TimeStepData->value[i].c_str(),
                                          timeState);
    if(data->GetSize() > 0)
      {
      data->SetName(this->TimeStepData->value[i].c_str());
      internalMesh->GetCellData()->AddArray(data);
      }
    data->Delete();
    }
  //output->SetNumberOfDataSets(0,output->GetNumberOfDataSets(0));
  output->SetDataSet(0, output->GetNumberOfDataSets(0), internalMesh);

  //Boundary Meshes
  for(int i = 0; i < (int)numBoundaries; i++)
    {
    vtkUnstructuredGrid * boundaryMesh = GetBoundaryMesh(timeState, i);
    for(int j = 0; j < (int)this->TimeStepData->value.size(); j++)
      {
      vtkDoubleArray * data =
        this->GetBoundaryVariableAtTimestep(i, TimeStepData->value[j].c_str(),
                                            timeState, internalMesh);
      if(data->GetSize() > 0)
        {
        data->SetName(this->TimeStepData->value[j].c_str());
        boundaryMesh->GetCellData()->AddArray(data);
        }
      data->Delete();
      }
    output->SetDataSet(0, output->GetNumberOfDataSets(0), boundaryMesh);
    boundaryMesh->Delete();
    }

  internalMesh->Delete();
  this->FaceOwner->Delete();
  //Zone Meshes
  for(int i = 0; i < (int)numPointZones; i++)
    {
    output->SetDataSet(0, output->GetNumberOfDataSets(0),
                       this->GetPointZoneMesh(timeState, i));
    }
  for(int i = 0; i < (int)numFaceZones; i++)
    {
    output->SetDataSet(0, output->GetNumberOfDataSets(0),
                       this->GetFaceZoneMesh(timeState, i));
    }
  for(int i = 0; i < (int)numCellZones; i++)
    {
    output->SetDataSet(0, output->GetNumberOfDataSets(0),
                       this->GetCellZoneMesh(timeState, i));
    }

  //clear timestep data
  this->TimeStepData->value.clear();
}
