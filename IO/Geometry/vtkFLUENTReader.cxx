/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkFLUENTReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// Thanks to Brian W. Dotson & Terry E. Jordan (Department of Energy, National
// Energy Technology Laboratory) & Douglas McCorkle (Iowa State University)
// who developed this class.
//
// Please address all comments to Brian Dotson (brian.dotson@netl.doe.gov) &
// Terry Jordan (terry.jordan@sa.netl.doe.gov)
// & Doug McCorkle (mccdo@iastate.edu)

#include "vtkFLUENTReader.h"
#include "vtkDataArraySelection.h"
#include "vtkErrorCode.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkUnstructuredGrid.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkFieldData.h"
#include "vtkPointData.h"
#include "vtkCellData.h"
#include "vtkByteSwap.h"
#include "vtkIdTypeArray.h"
#include "vtkFloatArray.h"
#include "vtkIntArray.h"
#include "vtkByteSwap.h"
#include "vtkCellArray.h"
#include "vtkHexahedron.h"
#include "vtkDoubleArray.h"
#include "vtkPoints.h"
#include "vtkTriangle.h"
#include "vtkQuad.h"
#include "vtkTetra.h"
#include "vtkWedge.h"
#include "vtkPyramid.h"
#include "vtkConvexPointSet.h"

#include <string>
#include <map>
#include <vector>
#include <set>
#include "fstream"
#include <sstream>
#include <algorithm>

#include <cctype>
#include <sys/stat.h>

vtkStandardNewMacro(vtkFLUENTReader);

#define VTK_FILE_BYTE_ORDER_BIG_ENDIAN 0
#define VTK_FILE_BYTE_ORDER_LITTLE_ENDIAN 1

//Structures
struct vtkFLUENTReader::Cell
{
  int type;
  int zone;
  std::vector< int > faces;
  int parent;
  int child;
  std::vector< int > nodes;
};

struct vtkFLUENTReader::Face
{
  int type;
  unsigned int zone;
  std::vector< int > nodes;
  int c0;
  int c1;
  int periodicShadow;
  int parent;
  int child;
  int interfaceFaceParent;
  int interfaceFaceChild;
  int ncgParent;
  int ncgChild;
};

struct vtkFLUENTReader::ScalarDataChunk
{
  int subsectionId;
  unsigned int zoneId;
  std::vector< double > scalarData;
};

struct vtkFLUENTReader::VectorDataChunk
{
  int subsectionId;
  unsigned int zoneId;
  std::vector< double > iComponentData;
  std::vector< double > jComponentData;
  std::vector< double > kComponentData;
};

struct vtkFLUENTReader::stdString
{
  std::string value;
};
struct vtkFLUENTReader::intVector
{
  std::vector<int> value;
};
struct vtkFLUENTReader::doubleVector
{
  std::vector<double> value;
};
struct vtkFLUENTReader::stringVector
{
  std::vector< std::string > value;
};
struct vtkFLUENTReader::cellVector
{
  std::vector< Cell > value;
};
struct vtkFLUENTReader::faceVector
{
  std::vector< Face > value;
};
struct vtkFLUENTReader::stdMap
{
  std::map< int, std::string > value;
};
struct vtkFLUENTReader::scalarDataVector
{
  std::vector< ScalarDataChunk > value;
};
struct vtkFLUENTReader::vectorDataVector
{
  std::vector< VectorDataChunk > value;
};
struct vtkFLUENTReader::intVectorVector
{
  std::vector< std::vector< int > > value;
};

//----------------------------------------------------------------------------
vtkFLUENTReader::vtkFLUENTReader()
{
  this->SwapBytes = 0;
  this->SetNumberOfInputPorts(0);
  this->FileName  = NULL;
  this->Points = vtkPoints::New();
  this->Triangle = vtkTriangle::New();
  this->Tetra = vtkTetra::New();
  this->Quad = vtkQuad::New();
  this->Hexahedron = vtkHexahedron::New();
  this->Pyramid = vtkPyramid::New();
  this->Wedge = vtkWedge::New();
  this->ConvexPointSet = vtkConvexPointSet::New();

  this->CaseBuffer = new stdString;
  this->DataBuffer = new stdString;
  this->Cells = new cellVector;
  this->Faces = new faceVector;
  this->VariableNames = new stdMap;
  this->CellZones = new intVector;
  this->ScalarDataChunks = new scalarDataVector;
  this->VectorDataChunks = new vectorDataVector;
  this->SubSectionZones = new intVectorVector;
  this->SubSectionIds = new intVector;
  this->SubSectionSize = new intVector;
  this->ScalarVariableNames = new stringVector;
  this->ScalarSubSectionIds = new intVector;
  this->VectorVariableNames = new stringVector;
  this->VectorSubSectionIds = new intVector;
  this->FluentCaseFile = new ifstream;
  this->FluentDataFile = new ifstream;

  this->NumberOfCells=0;

  this->CellDataArraySelection = vtkDataArraySelection::New();
  this->SetDataByteOrderToLittleEndian();
}

//----------------------------------------------------------------------------
vtkFLUENTReader::~vtkFLUENTReader()
{
  this->Points->Delete();
  this->Triangle->Delete();
  this->Tetra->Delete();
  this->Quad->Delete();
  this->Hexahedron->Delete();
  this->Pyramid->Delete();
  this->Wedge->Delete();
  this->ConvexPointSet->Delete();

  delete this->CaseBuffer;
  delete this->DataBuffer;
  delete this->Cells;
  delete this->Faces;
  delete this->VariableNames;
  delete this->CellZones;
  delete this->ScalarDataChunks;
  delete this->VectorDataChunks;
  delete this->SubSectionZones;
  delete this->SubSectionIds;
  delete this->SubSectionSize;
  delete this->ScalarVariableNames;
  delete this->ScalarSubSectionIds;
  delete this->VectorVariableNames;
  delete this->VectorSubSectionIds;
  delete this->FluentCaseFile;
  delete this->FluentDataFile;

  this->CellDataArraySelection->Delete();

  delete[] this->FileName;
}

//----------------------------------------------------------------------------
int vtkFLUENTReader::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **vtkNotUsed(inputVector),
  vtkInformationVector *outputVector)
{
  if (!this->FileName)
  {
    vtkErrorMacro("FileName has to be specified!");
    return 0;
  }

  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  vtkMultiBlockDataSet *output = vtkMultiBlockDataSet::SafeDownCast(
    outInfo->Get(vtkMultiBlockDataSet::DATA_OBJECT()));

  output->SetNumberOfBlocks(
    static_cast< unsigned int >(this->CellZones->value.size()) );
  //vtkUnstructuredGrid *Grid[CellZones.size()];

  std::vector< vtkUnstructuredGrid * > grid;
  grid.resize(this->CellZones->value.size());

  for(int test=0; test < (int)this->CellZones->value.size(); test++)
  {
    grid[test] = vtkUnstructuredGrid::New();
  }

  for (int i = 0; i < (int)this->Cells->value.size(); i++)
  {
    int location =
      std::find(this->CellZones->value.begin(),
                   this->CellZones->value.end(),
                   this->Cells->value[i].zone) -
      this->CellZones->value.begin();
                                    ;
    if (this->Cells->value[i].type == 1 )
    {
      for (int j = 0; j < 3; j++)
      {
        this->Triangle->GetPointIds()->SetId(j, this->Cells->value[i].nodes[j]);
      }
      grid[location]->InsertNextCell(this->Triangle->GetCellType(),
                                     this->Triangle->GetPointIds());
    }
    else if (this->Cells->value[i].type == 2 )
    {
      for (int j = 0; j < 4; j++)
      {
        this->Tetra->GetPointIds()->SetId(j, Cells->value[i].nodes[j]);
      }
      grid[location]->InsertNextCell(this->Tetra->GetCellType(),
                                     this->Tetra->GetPointIds());
    }
    else if (this->Cells->value[i].type == 3 )
    {
      for (int j = 0; j < 4; j++)
      {
        this->Quad->GetPointIds()->SetId(j, this->Cells->value[i].nodes[j]);
      }
      grid[location]->InsertNextCell(this->Quad->GetCellType(),
                                     this->Quad->GetPointIds());
    }
    else if (this->Cells->value[i].type == 4 )
    {
      for (int j = 0; j < 8; j++)
      {
        this->Hexahedron->GetPointIds()->
              SetId(j, this->Cells->value[i].nodes[j]);
      }
      grid[location]->InsertNextCell(this->Hexahedron->GetCellType(),
                                     this->Hexahedron->GetPointIds());
    }
    else if (this->Cells->value[i].type == 5 )
    {
      for (int j = 0; j < 5; j++)
      {
        this->Pyramid->GetPointIds()->SetId(j, this->Cells->value[i].nodes[j]);
      }
      grid[location]->InsertNextCell(this->Pyramid->GetCellType(),
                                     this->Pyramid->GetPointIds());
    }
    else if (this->Cells->value[i].type == 6 )
    {
      for (int j = 0; j < 6; j++)
      {
        this->Wedge->GetPointIds()->SetId(j, this->Cells->value[i].nodes[j]);
      }
      grid[location]->InsertNextCell(this->Wedge->GetCellType(),
                                     this->Wedge->GetPointIds());
    }
    else if (this->Cells->value[i].type == 7 )
    {
      this->ConvexPointSet->GetPointIds()->
            SetNumberOfIds(static_cast<vtkIdType>(this->Cells->value[i].nodes.size()));
      for (int j = 0; j < (int)this->Cells->value[i].nodes.size(); j++)
      {
        this->ConvexPointSet->GetPointIds()->
              SetId(j, this->Cells->value[i].nodes[j]);
      }
      grid[location]->InsertNextCell(this->ConvexPointSet->GetCellType(),
                                     this->ConvexPointSet->GetPointIds());
    }
  }
//  this->Cells->value.clear();

  //Scalar Data
  for (int l = 0; l < (int)this->ScalarDataChunks->value.size(); l++)
  {
    int location =
      std::find(this->CellZones->value.begin(),
                   this->CellZones->value.end(),
                   this->ScalarDataChunks->value[l].zoneId) -
      this->CellZones->value.begin();

    vtkDoubleArray *v = vtkDoubleArray::New();
    for (int m = 0; m <
         (int)this->ScalarDataChunks->value[l].scalarData.size(); m++)
    {
      v->InsertValue(m, this->ScalarDataChunks->value[l].scalarData[m]);
    }
    //v->SetName(this->ScalarVariableNames->
    //           value[l/this->CellZones->value.size()].c_str());
    v->SetName(this->VariableNames->
      value[this->ScalarDataChunks->value[l].subsectionId].c_str());
    grid[location]->GetCellData()->AddArray(v);
    v->Delete();
  }
  this->ScalarDataChunks->value.clear();

  //Vector Data
  for (int l = 0; l < (int)this->VectorDataChunks->value.size(); l++)
  {
    int location =
      std::find(this->CellZones->value.begin(),
                   this->CellZones->value.end(),
                   this->VectorDataChunks->value[l].zoneId) -
      this->CellZones->value.begin();
    vtkDoubleArray *v = vtkDoubleArray::New();
    v->SetNumberOfComponents(3);
    for (int m = 0;
         m < (int)this->VectorDataChunks->value[l].iComponentData.size(); m++)
    {
      v->InsertComponent(m, 0,
                         this->VectorDataChunks->value[l].iComponentData[m]);
      v->InsertComponent(m, 1,
                         this->VectorDataChunks->value[l].jComponentData[m]);
      v->InsertComponent(m, 2,
                         this->VectorDataChunks->value[l].kComponentData[m]);
    }
    //v->SetName(this->VectorVariableNames->
    //           value[l/this->CellZones->value.size()].c_str());
    v->SetName(this->VariableNames->
      value[this->VectorDataChunks->value[l].subsectionId].c_str());
    grid[location]->GetCellData()->AddArray(v);
    v->Delete();
  }
  this->VectorDataChunks->value.clear();

  for(int addTo = 0; addTo < (int)this->CellZones->value.size(); addTo++)
  {
    grid[addTo]->SetPoints(Points);
    output->SetBlock(addTo, grid[addTo]);
    grid[addTo]->Delete();
  }
  return 1;
}

//----------------------------------------------------------------------------
void vtkFLUENTReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "File Name: "
     << (this->FileName ? this->FileName : "(none)") << endl;
  os << indent << "Number Of Cells: " << this->NumberOfCells << endl;
}

//----------------------------------------------------------------------------
int vtkFLUENTReader::RequestInformation(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **vtkNotUsed(inputVector),
  vtkInformationVector *vtkNotUsed(outputVector))
{
  if (!this->FileName)
  {
    vtkErrorMacro("FileName has to be specified!");
    return 0;
  }

  if(!this->OpenCaseFile(this->FileName))
  {
    vtkErrorMacro("Unable to open cas file.");
    return 0;
  }

  int dat_file_opened = this->OpenDataFile(this->FileName);
  if(!dat_file_opened)
  {
    vtkWarningMacro("Unable to open dat file.");
  }

  this->LoadVariableNames();
  this->ParseCaseFile();  // Reads Necessary Information from the .cas file.
  this->CleanCells();  //  Removes unnecessary faces from the cells.
  this->PopulateCellNodes();
  this->GetNumberOfCellZones();
  this->NumberOfScalars = 0;
  this->NumberOfVectors = 0;
  if(dat_file_opened)
  {
    this->ParseDataFile();
  }
  for (int i = 0; i < (int)this->SubSectionIds->value.size(); i++)
  {
    if (this->SubSectionSize->value[i] == 1)
    {
      this->CellDataArraySelection->AddArray(this->VariableNames->value[this->
                                             SubSectionIds->value[i]].c_str());
      this->ScalarVariableNames->value.
        push_back(this->VariableNames->value[this->SubSectionIds->value[i]]);
      this->ScalarSubSectionIds->value.push_back(this->SubSectionIds->value[i]);
    }
    else if (this->SubSectionSize->value[i] == 3)
    {
      this->CellDataArraySelection->
            AddArray(this->VariableNames->
                     value[this->SubSectionIds->value[i]].c_str());
      this->VectorVariableNames->value.push_back(this->VariableNames->value[
                                            this->SubSectionIds->value[i]]);
      this->VectorSubSectionIds->value.push_back(this->SubSectionIds->value[i]);
    }
  }
  this->NumberOfCells = (int)this->Cells->value.size();
  return 1;
}

//----------------------------------------------------------------------------
bool vtkFLUENTReader::OpenCaseFile(const char *filename)
{
#ifdef _WIN32
  //this->FluentCaseFile->open(filename, ios::in | ios::binary);
  this->FluentCaseFile = new ifstream(filename, ios::in | ios::binary);
#else
  //this->FluentCaseFile->open(filename, ios::in);
  this->FluentCaseFile = new ifstream(filename, ios::in);
#endif

  if (!this->FluentCaseFile->fail())
  {
    return true;
  }
  else
  {
    return false;
  }
}

//----------------------------------------------------------------------------
int vtkFLUENTReader::GetNumberOfCellArrays()
{
  return this->CellDataArraySelection->GetNumberOfArrays();
}

//----------------------------------------------------------------------------
const char* vtkFLUENTReader::GetCellArrayName(int index)
{
  return this->CellDataArraySelection->GetArrayName(index);
}

//----------------------------------------------------------------------------
int vtkFLUENTReader::GetCellArrayStatus(const char* name)
{
  return this->CellDataArraySelection->ArrayIsEnabled(name);
}

//----------------------------------------------------------------------------
void vtkFLUENTReader::SetCellArrayStatus(const char* name, int status)
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

//----------------------------------------------------------------------------
void vtkFLUENTReader::EnableAllCellArrays()
{
  this->CellDataArraySelection->EnableAllArrays();
}

//----------------------------------------------------------------------------
void vtkFLUENTReader::DisableAllCellArrays()
{
  this->CellDataArraySelection->DisableAllArrays();
}

//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
bool vtkFLUENTReader::OpenDataFile(const char *filename)
{
  std::string dfilename(filename);
  dfilename.erase(dfilename.length()-3, 3);
  dfilename.append("dat");

#ifdef _WIN32
  //this->FluentDataFile->open(dfilename.c_str(), ios::in | ios::binary);
  this->FluentDataFile = new ifstream(dfilename.c_str(), ios::in | ios::binary);
#else
  //this->FluentDataFile->open(dfilename.c_str(), ios::in);
  this->FluentDataFile = new ifstream(dfilename.c_str(), ios::in);
#endif

  if (this->FluentDataFile->fail())
  {
    return false;
  }
  else
  {
    return true;
  }
}

//----------------------------------------------------------------------------
int vtkFLUENTReader::GetCaseChunk ()
{
  this->CaseBuffer->value = "";  // Clear buffer

  //
  // Look for beginning of chunk
  //
  while(this->FluentCaseFile->peek() != '(')
  {
    this->FluentCaseFile->get();
    if (this->FluentCaseFile->eof())
    {
      return 0;
    }
  }

  //
  // Figure out whether this is a binary or ascii chunk.
  // If the index is 3 digits or more, then binary, otherwise ascii.
  //
  std::string index;
  while(this->FluentCaseFile->peek() != ' ')
  {
    //index.push_back(this->FluentCaseFile->peek());
        index += this->FluentCaseFile->peek();
        //this->CaseBuffer->value.push_back(this->FluentCaseFile->get());
        this->CaseBuffer->value += this->FluentCaseFile->get();
    if (this->FluentCaseFile->eof())
    {
      return 0;
    }
  }

  index.erase(0,1);  // Get rid of the "("

  //
  //  Grab the chunk and put it in buffer.
  //  You have to look for the end of section std::string if it is
  //  a binary chunk.
  //

  if (index.size() > 2)
  {  // Binary Chunk
    char end[120];
    strcpy(end, "End of Binary Section   ");
    strcat(end, index.c_str());
    strcat(end, ")");
    size_t len = strlen(end);

    // Load the case buffer enough to start comparing to the end std::string.
    while (this->CaseBuffer->value.size() < len)
    {
      //this->CaseBuffer->value.push_back(this->FluentCaseFile->get());
      this->CaseBuffer->value += this->FluentCaseFile->get();
    }

    //while (CaseBuffer.compare(CaseBuffer.size()-strlen(end),
    //strlen(end), end))
    while (strcmp(this->CaseBuffer->value.c_str()+
                   (this->CaseBuffer->value.size()-len), end))
    {
      //this->CaseBuffer->value.push_back(this->FluentCaseFile->get());
      this->CaseBuffer->value += this->FluentCaseFile->get();
    }

  }
  else
  {  // Ascii Chunk
    int level = 0;
    while ((this->FluentCaseFile->peek() != ')') || (level != 0) )
    {
      //this->CaseBuffer->value.push_back(this->FluentCaseFile->get());
      this->CaseBuffer->value += this->FluentCaseFile->get();
      if (this->CaseBuffer->value.at(this->CaseBuffer->value.length()-1) == '(')
      {
        level++;
      }
      if (this->CaseBuffer->value.at(this->CaseBuffer->value.length()-1) == ')')
      {
        level--;
      }
      if (this->FluentCaseFile->eof())
      {
        return 0;
      }
    }
    //this->CaseBuffer->value.push_back(this->FluentCaseFile->get());
    this->CaseBuffer->value += this->FluentCaseFile->get();
  }
  return 1;
}

//----------------------------------------------------------------------------
int vtkFLUENTReader::GetCaseIndex()
{
  std::string sindex;

  int i = 1;
  while (this->CaseBuffer->value.at(i) != ' ')
  {
    //sindex.push_back(this->CaseBuffer->value.at(i++));
    sindex += this->CaseBuffer->value.at(i++);
  }
  return atoi(sindex.c_str());
}

//----------------------------------------------------------------------------
void vtkFLUENTReader::GetNumberOfCellZones()
{
  int match;

  for (int i = 0; i < (int)this->Cells->value.size(); i++)
  {
    if (this->CellZones->value.size() == 0)
    {
      this->CellZones->value.push_back(this->Cells->value[i].zone);
    }
    else
    {
      match = 0;
      for (int j = 0; j < (int)this->CellZones->value.size(); j++)
      {
        if (this->CellZones->value[j] == this->Cells->value[i].zone)
        {
          match = 1;
        }
      }
      if (match == 0)
      {
        this->CellZones->value.push_back(this->Cells->value[i].zone);
      }
    }
  }
}


//----------------------------------------------------------------------------
int vtkFLUENTReader::GetDataIndex()
{
  std::string sindex;

  int i = 1;
  while (this->DataBuffer->value.at(i) != ' ')
  {
    //sindex.push_back(this->DataBuffer->value.at(i++));
    sindex += this->DataBuffer->value.at(i++);
  }
  return atoi(sindex.c_str());
}

//----------------------------------------------------------------------------
int vtkFLUENTReader::GetDataChunk ()
{
  this->DataBuffer->value = "";  // Clear buffer
  //
  // Look for beginning of chunk
  //
  while(this->FluentDataFile->peek() != '(')
  {
    this->FluentDataFile->get();
    if (this->FluentDataFile->eof())
    {
      return 0;
    }
  }

  //
  // Figure out whether this is a binary or ascii chunk.
  // If the index is 3 digits or more, then binary, otherwise ascii.
  //
  std::string index;
  while(this->FluentDataFile->peek() != ' ')
  {
    //index.push_back(this->FluentDataFile->peek());
    index += this->FluentDataFile->peek();
    //this->DataBuffer->value.push_back(this->FluentDataFile->get());
    this->DataBuffer->value += this->FluentDataFile->get();
    if (this->FluentDataFile->eof())
    {
      return 0;
    }
  }

  index.erase(0,1);  // Get rid of the "("

  //
  //  Grab the chunk and put it in buffer.
  //  You have to look for the end of section std::string if it is
  //  a binary chunk.
  //
  if (index.size() > 3)
  {  // Binary Chunk
    //it may be in our best interest to do away with the index portion of the
    //"end" string - we have found a dataset, that although errant, does work
    //fine in ensight and the index does not match - maybe just an end string
    //that contains "End of Binary Section" and and a search to relocate the
    //file pointer to the "))" entry.
    char end[120];
    strcpy(end, "End of Binary Section   ");
    //strcat(end, index.c_str());
    //strcat(end, ")");
    size_t len = strlen(end);

    // Load the data buffer enough to start comparing to the end std::string.
    while (this->DataBuffer->value.size() < len)
    {
      //this->DataBuffer->value.push_back(this->FluentDataFile->get());
      this->DataBuffer->value += this->FluentDataFile->get();
    }

    //while (DataBuffer.compare(DataBuffer.size()-strlen(end),
    //strlen(end), end))
    while (strcmp(this->DataBuffer->value.c_str()+
                  (this->DataBuffer->value.size()-len), end))
    {
      //this->DataBuffer->value.push_back(this->FluentDataFile->get());
      this->DataBuffer->value += this->FluentDataFile->get();
    }

  }
  else
  {  // Ascii Chunk
    int level = 0;
    while ((this->FluentDataFile->peek() != ')') || (level != 0) )
    {
      //this->DataBuffer->value.push_back(this->FluentDataFile->get());
      this->DataBuffer->value += this->FluentDataFile->get();
      if (this->DataBuffer->value.at(this->DataBuffer->value.length()-1) == '(')
      {
        level++;
      }
      if (this->DataBuffer->value.at(this->DataBuffer->value.length()-1) == ')')
      {
        level--;
      }
      if (this->FluentDataFile->eof())
      {
        return 0;
      }
    }
    //this->DataBuffer->value.push_back(this->FluentDataFile->get());
    this->DataBuffer->value += this->FluentDataFile->get();
  }

  return 1;
}


void vtkFLUENTReader::LoadVariableNames()
{
  this->VariableNames->value[1]  = "PRESSURE";
  this->VariableNames->value[2]  = "MOMENTUM";
  this->VariableNames->value[3]  = "TEMPERATURE";
  this->VariableNames->value[4]  = "ENTHALPY";
  this->VariableNames->value[5]  = "TKE";
  this->VariableNames->value[6]  = "TED";
  this->VariableNames->value[7]  = "SPECIES";
  this->VariableNames->value[8]  = "G";
  this->VariableNames->value[9]  = "WSWIRL";
  this->VariableNames->value[10] = "DPMS_MASS";
  this->VariableNames->value[11] = "DPMS_MOM";
  this->VariableNames->value[12] = "DPMS_ENERGY";
  this->VariableNames->value[13] = "DPMS_SPECIES";
  this->VariableNames->value[14] = "DVOLUME_DT";
  this->VariableNames->value[15] = "BODY_FORCES";
  this->VariableNames->value[16] = "FMEAN";
  this->VariableNames->value[17] = "FVAR";
  this->VariableNames->value[18] = "MASS_FLUX";
  this->VariableNames->value[19] = "WALL_SHEAR";
  this->VariableNames->value[20] = "BOUNDARY_HEAT_FLUX";
  this->VariableNames->value[21] = "BOUNDARY_RAD_HEAT_FLUX";
  this->VariableNames->value[22] = "OLD_PRESSURE";
  this->VariableNames->value[23] = "POLLUT";
  this->VariableNames->value[24] = "DPMS_P1_S";
  this->VariableNames->value[25] = "DPMS_P1_AP";
  this->VariableNames->value[26] = "WALL_GAS_TEMPERATURE";
  this->VariableNames->value[27] = "DPMS_P1_DIFF";
  this->VariableNames->value[28] = "DR_SURF";
  this->VariableNames->value[29] = "W_M1";
  this->VariableNames->value[30] = "W_M2";
  this->VariableNames->value[31] = "DPMS_BURNOUT";

  this->VariableNames->value[32] = "DPMS_CONCENTRATION";
  this->VariableNames->value[33] = "PDF_MW";
  this->VariableNames->value[34] = "DPMS_WSWIRL";
  this->VariableNames->value[35] = "YPLUS";
  this->VariableNames->value[36] = "YPLUS_UTAU";
  this->VariableNames->value[37] = "WALL_SHEAR_SWIRL";
  this->VariableNames->value[38] = "WALL_T_INNER";
  this->VariableNames->value[39] = "POLLUT0";
  this->VariableNames->value[40] = "POLLUT1";
  this->VariableNames->value[41] = "WALL_G_INNER";
  this->VariableNames->value[42] = "PREMIXC";
  this->VariableNames->value[43] = "PREMIXC_T";
  this->VariableNames->value[44] = "PREMIXC_RATE";
  this->VariableNames->value[45] = "POLLUT2";
  this->VariableNames->value[46] = "POLLUT3";
  this->VariableNames->value[47] = "MASS_FLUX_M1";
  this->VariableNames->value[48] = "MASS_FLUX_M2";
  this->VariableNames->value[49] = "GRID_FLUX";
  this->VariableNames->value[50] = "DO_I";
  this->VariableNames->value[51] = "DO_RECON_I";
  this->VariableNames->value[52] = "DO_ENERGY_SOURCE";
  this->VariableNames->value[53] = "DO_IRRAD";
  this->VariableNames->value[54] = "DO_QMINUS";
  this->VariableNames->value[55] = "DO_IRRAD_OLD";
  this->VariableNames->value[56] = "DO_IWX=56";
  this->VariableNames->value[57] = "DO_IWY";
  this->VariableNames->value[58] = "DO_IWZ";
  this->VariableNames->value[59] = "MACH";
  this->VariableNames->value[60] = "SLIP_U";
  this->VariableNames->value[61] = "SLIP_V";
  this->VariableNames->value[62] = "SLIP_W";
  this->VariableNames->value[63] = "SDR";
  this->VariableNames->value[64] = "SDR_M1";
  this->VariableNames->value[65] = "SDR_M2";
  this->VariableNames->value[66] = "POLLUT4";
  this->VariableNames->value[67] = "GRANULAR_TEMPERATURE";
  this->VariableNames->value[68] = "GRANULAR_TEMPERATURE_M1";
  this->VariableNames->value[69] = "GRANULAR_TEMPERATURE_M2";
  this->VariableNames->value[70] = "VFLUX";
  this->VariableNames->value[80] = "VFLUX_M1";
  this->VariableNames->value[90] = "VFLUX_M2";
  this->VariableNames->value[91] = "DO_QNET";
  this->VariableNames->value[92] = "DO_QTRANS";
  this->VariableNames->value[93] = "DO_QREFL";
  this->VariableNames->value[94] = "DO_QABS";
  this->VariableNames->value[95] = "POLLUT5";
  this->VariableNames->value[96] = "WALL_DIST";
  this->VariableNames->value[97] = "SOLAR_SOURCE";
  this->VariableNames->value[98] = "SOLAR_QREFL";
  this->VariableNames->value[99] = "SOLAR_QABS";
  this->VariableNames->value[100] = "SOLAR_QTRANS";
  this->VariableNames->value[101] = "DENSITY";
  this->VariableNames->value[102] = "MU_LAM";
  this->VariableNames->value[103] = "MU_TURB";
  this->VariableNames->value[104] = "CP";
  this->VariableNames->value[105] = "KTC";
  this->VariableNames->value[106] = "VGS_DTRM";
  this->VariableNames->value[107] = "VGF_DTRM";
  this->VariableNames->value[108] = "RSTRESS";
  this->VariableNames->value[109] = "THREAD_RAD_FLUX";
  this->VariableNames->value[110] = "SPE_Q";
  this->VariableNames->value[111] = "X_VELOCITY";
  this->VariableNames->value[112] = "Y_VELOCITY";
  this->VariableNames->value[113] = "Z_VELOCITY";
  this->VariableNames->value[114] = "WALL_VELOCITY";
  this->VariableNames->value[115] = "X_VELOCITY_M1";
  this->VariableNames->value[116] = "Y_VELOCITY_M1";
  this->VariableNames->value[117] = "Z_VELOCITY_M1";
  this->VariableNames->value[118] = "PHASE_MASS";
  this->VariableNames->value[119] = "TKE_M1";
  this->VariableNames->value[120] = "TED_M1";
  this->VariableNames->value[121] = "POLLUT6";
  this->VariableNames->value[122] = "X_VELOCITY_M2";
  this->VariableNames->value[123] = "Y_VELOCITY_M2";
  this->VariableNames->value[124] = "Z_VELOCITY_M2";
  this->VariableNames->value[126] = "TKE_M2";
  this->VariableNames->value[127] = "TED_M2";
  this->VariableNames->value[128] = "RUU";
  this->VariableNames->value[129] = "RVV";
  this->VariableNames->value[130] = "RWW";
  this->VariableNames->value[131] = "RUV";
  this->VariableNames->value[132] = "RVW";
  this->VariableNames->value[133] = "RUW";
  this->VariableNames->value[134] = "DPMS_EROSION";
  this->VariableNames->value[135] = "DPMS_ACCRETION";
  this->VariableNames->value[136] = "FMEAN2";
  this->VariableNames->value[137] = "FVAR2";
  this->VariableNames->value[138] = "ENTHALPY_M1";
  this->VariableNames->value[139] = "ENTHALPY_M2";
  this->VariableNames->value[140] = "FMEAN_M1";
  this->VariableNames->value[141] = "FMEAN_M2";
  this->VariableNames->value[142] = "FVAR_M1";
  this->VariableNames->value[143] = "FVAR_M2";
  this->VariableNames->value[144] = "FMEAN2_M1";
  this->VariableNames->value[145] = "FMEAN2_M2";
  this->VariableNames->value[146] = "FVAR2_M1";
  this->VariableNames->value[147] = "FVAR2_M2";
  this->VariableNames->value[148] = "PREMIXC_M1";
  this->VariableNames->value[149] = "PREMIXC_M2";
  this->VariableNames->value[150] = "VOF";
  this->VariableNames->value[151] = "VOF_1";
  this->VariableNames->value[152] = "VOF_2";
  this->VariableNames->value[153] = "VOF_3";
  this->VariableNames->value[154] = "VOF_4";
  this->VariableNames->value[160] = "VOF_M1";
  this->VariableNames->value[161] = "VOF_1_M1";
  this->VariableNames->value[162] = "VOF_2_M1";
  this->VariableNames->value[163] = "VOF_3_M1";
  this->VariableNames->value[164] = "VOF_4_M1";
  this->VariableNames->value[170] = "VOF_M2";
  this->VariableNames->value[171] = "VOF_1_M2";
  this->VariableNames->value[172] = "VOF_2_M2";
  this->VariableNames->value[173] = "VOF_3_M2";
  this->VariableNames->value[174] = "VOF_4_M2";
  this->VariableNames->value[180] = "VOLUME_M2";
  this->VariableNames->value[181] = "WALL_GRID_VELOCITY";
  this->VariableNames->value[182] = "POLLUT7";
  this->VariableNames->value[183] = "POLLUT8";
  this->VariableNames->value[184] = "POLLUT9";
  this->VariableNames->value[185] = "POLLUT10";
  this->VariableNames->value[186] = "POLLUT11";
  this->VariableNames->value[187] = "POLLUT12";
  this->VariableNames->value[188] = "POLLUT13";
  this->VariableNames->value[190] = "SV_T_AUX";
  this->VariableNames->value[191] = "SV_T_AP_AUX";
  this->VariableNames->value[192] = "TOTAL_PRESSURE";
  this->VariableNames->value[193] = "TOTAL_TEMPERATURE";
  this->VariableNames->value[194] = "NRBC_DC";
  this->VariableNames->value[195] = "DP_TMFR";

  this->VariableNames->value[200] = "Y_00";
  this->VariableNames->value[201] = "Y_01";
  this->VariableNames->value[202] = "Y_02";
  this->VariableNames->value[203] = "Y_03";
  this->VariableNames->value[204] = "Y_04";
  this->VariableNames->value[205] = "Y_05";
  this->VariableNames->value[206] = "Y_06";
  this->VariableNames->value[207] = "Y_07";
  this->VariableNames->value[208] = "Y_08";
  this->VariableNames->value[209] = "Y_09";
  this->VariableNames->value[210] = "Y_10";
  this->VariableNames->value[211] = "Y_11";
  this->VariableNames->value[212] = "Y_12";
  this->VariableNames->value[213] = "Y_13";
  this->VariableNames->value[214] = "Y_14";
  this->VariableNames->value[215] = "Y_15";
  this->VariableNames->value[216] = "Y_16";
  this->VariableNames->value[217] = "Y_17";
  this->VariableNames->value[218] = "Y_18";
  this->VariableNames->value[219] = "Y_19";
  this->VariableNames->value[220] = "Y_20";
  this->VariableNames->value[221] = "Y_21";
  this->VariableNames->value[222] = "Y_22";
  this->VariableNames->value[223] = "Y_23";
  this->VariableNames->value[224] = "Y_24";
  this->VariableNames->value[225] = "Y_25";
  this->VariableNames->value[226] = "Y_26";
  this->VariableNames->value[227] = "Y_27";
  this->VariableNames->value[228] = "Y_28";
  this->VariableNames->value[229] = "Y_29";
  this->VariableNames->value[230] = "Y_30";
  this->VariableNames->value[231] = "Y_31";
  this->VariableNames->value[232] = "Y_32";
  this->VariableNames->value[233] = "Y_33";
  this->VariableNames->value[234] = "Y_34";
  this->VariableNames->value[235] = "Y_35";
  this->VariableNames->value[236] = "Y_36";
  this->VariableNames->value[237] = "Y_37";
  this->VariableNames->value[238] = "Y_38";
  this->VariableNames->value[239] = "Y_39";
  this->VariableNames->value[240] = "Y_40";
  this->VariableNames->value[241] = "Y_41";
  this->VariableNames->value[242] = "Y_42";
  this->VariableNames->value[243] = "Y_43";
  this->VariableNames->value[244] = "Y_44";
  this->VariableNames->value[245] = "Y_45";
  this->VariableNames->value[246] = "Y_46";
  this->VariableNames->value[247] = "Y_47";
  this->VariableNames->value[248] = "Y_48";
  this->VariableNames->value[249] = "Y_49";

  this->VariableNames->value[250] = "Y_M1_00";
  this->VariableNames->value[251] = "Y_M1_01";
  this->VariableNames->value[252] = "Y_M1_02";
  this->VariableNames->value[253] = "Y_M1_03";
  this->VariableNames->value[254] = "Y_M1_04";
  this->VariableNames->value[255] = "Y_M1_05";
  this->VariableNames->value[256] = "Y_M1_06";
  this->VariableNames->value[257] = "Y_M1_07";
  this->VariableNames->value[258] = "Y_M1_08";
  this->VariableNames->value[259] = "Y_M1_09";
  this->VariableNames->value[260] = "Y_M1_10";
  this->VariableNames->value[261] = "Y_M1_11";
  this->VariableNames->value[262] = "Y_M1_12";
  this->VariableNames->value[263] = "Y_M1_13";
  this->VariableNames->value[264] = "Y_M1_14";
  this->VariableNames->value[265] = "Y_M1_15";
  this->VariableNames->value[266] = "Y_M1_16";
  this->VariableNames->value[267] = "Y_M1_17";
  this->VariableNames->value[268] = "Y_M1_18";
  this->VariableNames->value[269] = "Y_M1_19";
  this->VariableNames->value[270] = "Y_M1_20";
  this->VariableNames->value[271] = "Y_M1_21";
  this->VariableNames->value[272] = "Y_M1_22";
  this->VariableNames->value[273] = "Y_M1_23";
  this->VariableNames->value[274] = "Y_M1_24";
  this->VariableNames->value[275] = "Y_M1_25";
  this->VariableNames->value[276] = "Y_M1_26";
  this->VariableNames->value[277] = "Y_M1_27";
  this->VariableNames->value[278] = "Y_M1_28";
  this->VariableNames->value[279] = "Y_M1_29";
  this->VariableNames->value[280] = "Y_M1_30";
  this->VariableNames->value[281] = "Y_M1_31";
  this->VariableNames->value[282] = "Y_M1_32";
  this->VariableNames->value[283] = "Y_M1_33";
  this->VariableNames->value[284] = "Y_M1_34";
  this->VariableNames->value[285] = "Y_M1_35";
  this->VariableNames->value[286] = "Y_M1_36";
  this->VariableNames->value[287] = "Y_M1_37";
  this->VariableNames->value[288] = "Y_M1_38";
  this->VariableNames->value[289] = "Y_M1_39";
  this->VariableNames->value[290] = "Y_M1_40";
  this->VariableNames->value[291] = "Y_M1_41";
  this->VariableNames->value[292] = "Y_M1_42";
  this->VariableNames->value[293] = "Y_M1_43";
  this->VariableNames->value[294] = "Y_M1_44";
  this->VariableNames->value[295] = "Y_M1_45";
  this->VariableNames->value[296] = "Y_M1_46";
  this->VariableNames->value[297] = "Y_M1_47";
  this->VariableNames->value[298] = "Y_M1_48";
  this->VariableNames->value[299] = "Y_M1_49";

  this->VariableNames->value[300] = "Y_M2_00";
  this->VariableNames->value[301] = "Y_M2_01";
  this->VariableNames->value[302] = "Y_M2_02";
  this->VariableNames->value[303] = "Y_M2_03";
  this->VariableNames->value[304] = "Y_M2_04";
  this->VariableNames->value[305] = "Y_M2_05";
  this->VariableNames->value[306] = "Y_M2_06";
  this->VariableNames->value[307] = "Y_M2_07";
  this->VariableNames->value[308] = "Y_M2_08";
  this->VariableNames->value[309] = "Y_M2_09";
  this->VariableNames->value[310] = "Y_M2_10";
  this->VariableNames->value[311] = "Y_M2_11";
  this->VariableNames->value[312] = "Y_M2_12";
  this->VariableNames->value[313] = "Y_M2_13";
  this->VariableNames->value[314] = "Y_M2_14";
  this->VariableNames->value[315] = "Y_M2_15";
  this->VariableNames->value[316] = "Y_M2_16";
  this->VariableNames->value[317] = "Y_M2_17";
  this->VariableNames->value[318] = "Y_M2_18";
  this->VariableNames->value[319] = "Y_M2_19";
  this->VariableNames->value[320] = "Y_M2_20";
  this->VariableNames->value[321] = "Y_M2_21";
  this->VariableNames->value[322] = "Y_M2_22";
  this->VariableNames->value[323] = "Y_M2_23";
  this->VariableNames->value[324] = "Y_M2_24";
  this->VariableNames->value[325] = "Y_M2_25";
  this->VariableNames->value[326] = "Y_M2_26";
  this->VariableNames->value[327] = "Y_M2_27";
  this->VariableNames->value[328] = "Y_M2_28";
  this->VariableNames->value[329] = "Y_M2_29";
  this->VariableNames->value[330] = "Y_M2_30";
  this->VariableNames->value[331] = "Y_M2_31";
  this->VariableNames->value[332] = "Y_M2_32";
  this->VariableNames->value[333] = "Y_M2_33";
  this->VariableNames->value[334] = "Y_M2_34";
  this->VariableNames->value[335] = "Y_M2_35";
  this->VariableNames->value[336] = "Y_M2_36";
  this->VariableNames->value[337] = "Y_M2_37";
  this->VariableNames->value[338] = "Y_M2_38";
  this->VariableNames->value[339] = "Y_M2_39";
  this->VariableNames->value[340] = "Y_M2_40";
  this->VariableNames->value[341] = "Y_M2_41";
  this->VariableNames->value[342] = "Y_M2_42";
  this->VariableNames->value[343] = "Y_M2_43";
  this->VariableNames->value[344] = "Y_M2_44";
  this->VariableNames->value[345] = "Y_M2_45";
  this->VariableNames->value[346] = "Y_M2_46";
  this->VariableNames->value[347] = "Y_M2_47";
  this->VariableNames->value[348] = "Y_M2_48";
  this->VariableNames->value[349] = "Y_M2_49";

  this->VariableNames->value[350] = "DR_SURF_00";
  this->VariableNames->value[351] = "DR_SURF_01";
  this->VariableNames->value[352] = "DR_SURF_02";
  this->VariableNames->value[353] = "DR_SURF_03";
  this->VariableNames->value[354] = "DR_SURF_04";
  this->VariableNames->value[355] = "DR_SURF_05";
  this->VariableNames->value[356] = "DR_SURF_06";
  this->VariableNames->value[357] = "DR_SURF_07";
  this->VariableNames->value[358] = "DR_SURF_08";
  this->VariableNames->value[359] = "DR_SURF_09";
  this->VariableNames->value[360] = "DR_SURF_10";
  this->VariableNames->value[361] = "DR_SURF_11";
  this->VariableNames->value[362] = "DR_SURF_12";
  this->VariableNames->value[363] = "DR_SURF_13";
  this->VariableNames->value[364] = "DR_SURF_14";
  this->VariableNames->value[365] = "DR_SURF_15";
  this->VariableNames->value[366] = "DR_SURF_16";
  this->VariableNames->value[367] = "DR_SURF_17";
  this->VariableNames->value[368] = "DR_SURF_18";
  this->VariableNames->value[369] = "DR_SURF_19";
  this->VariableNames->value[370] = "DR_SURF_20";
  this->VariableNames->value[371] = "DR_SURF_21";
  this->VariableNames->value[372] = "DR_SURF_22";
  this->VariableNames->value[373] = "DR_SURF_23";
  this->VariableNames->value[374] = "DR_SURF_24";
  this->VariableNames->value[375] = "DR_SURF_25";
  this->VariableNames->value[376] = "DR_SURF_26";
  this->VariableNames->value[377] = "DR_SURF_27";
  this->VariableNames->value[378] = "DR_SURF_28";
  this->VariableNames->value[379] = "DR_SURF_29";
  this->VariableNames->value[380] = "DR_SURF_30";
  this->VariableNames->value[381] = "DR_SURF_31";
  this->VariableNames->value[382] = "DR_SURF_32";
  this->VariableNames->value[383] = "DR_SURF_33";
  this->VariableNames->value[384] = "DR_SURF_34";
  this->VariableNames->value[385] = "DR_SURF_35";
  this->VariableNames->value[386] = "DR_SURF_36";
  this->VariableNames->value[387] = "DR_SURF_37";
  this->VariableNames->value[388] = "DR_SURF_38";
  this->VariableNames->value[389] = "DR_SURF_39";
  this->VariableNames->value[390] = "DR_SURF_40";
  this->VariableNames->value[391] = "DR_SURF_41";
  this->VariableNames->value[392] = "DR_SURF_42";
  this->VariableNames->value[393] = "DR_SURF_43";
  this->VariableNames->value[394] = "DR_SURF_44";
  this->VariableNames->value[395] = "DR_SURF_45";
  this->VariableNames->value[396] = "DR_SURF_46";
  this->VariableNames->value[397] = "DR_SURF_47";
  this->VariableNames->value[398] = "DR_SURF_48";
  this->VariableNames->value[399] = "DR_SURF_49";

  this->VariableNames->value[400] = "PRESSURE_MEAN";
  this->VariableNames->value[401] = "PRESSURE_RMS";
  this->VariableNames->value[402] = "X_VELOCITY_MEAN";
  this->VariableNames->value[403] = "X_VELOCITY_RMS";
  this->VariableNames->value[404] = "Y_VELOCITY_MEAN";
  this->VariableNames->value[405] = "Y_VELOCITY_RMS";
  this->VariableNames->value[406] = "Z_VELOCITY_MEAN";
  this->VariableNames->value[407] = "Z_VELOCITY_RMS";
  this->VariableNames->value[408] = "TEMPERATURE_MEAN";
  this->VariableNames->value[409] = "TEMPERATURE_RMS";
  this->VariableNames->value[410] = "VOF_MEAN";
  this->VariableNames->value[411] = "VOF_RMS";
  this->VariableNames->value[412] = "PRESSURE_M1";
  this->VariableNames->value[413] = "PRESSURE_M2";
  this->VariableNames->value[414] = "GRANULAR_TEMPERATURE_MEAN";
  this->VariableNames->value[415] = "GRANULAR_TEMPERATURE_RMS";

  this->VariableNames->value[450] = "DPMS_Y_00";
  this->VariableNames->value[451] = "DPMS_Y_01";
  this->VariableNames->value[452] = "DPMS_Y_02";
  this->VariableNames->value[453] = "DPMS_Y_03";
  this->VariableNames->value[454] = "DPMS_Y_04";
  this->VariableNames->value[455] = "DPMS_Y_05";
  this->VariableNames->value[456] = "DPMS_Y_06";
  this->VariableNames->value[457] = "DPMS_Y_07";
  this->VariableNames->value[458] = "DPMS_Y_08";
  this->VariableNames->value[459] = "DPMS_Y_09";
  this->VariableNames->value[460] = "DPMS_Y_10";
  this->VariableNames->value[461] = "DPMS_Y_11";
  this->VariableNames->value[462] = "DPMS_Y_12";
  this->VariableNames->value[463] = "DPMS_Y_13";
  this->VariableNames->value[464] = "DPMS_Y_14";
  this->VariableNames->value[465] = "DPMS_Y_15";
  this->VariableNames->value[466] = "DPMS_Y_16";
  this->VariableNames->value[467] = "DPMS_Y_17";
  this->VariableNames->value[468] = "DPMS_Y_18";
  this->VariableNames->value[469] = "DPMS_Y_19";
  this->VariableNames->value[470] = "DPMS_Y_20";
  this->VariableNames->value[471] = "DPMS_Y_21";
  this->VariableNames->value[472] = "DPMS_Y_22";
  this->VariableNames->value[473] = "DPMS_Y_23";
  this->VariableNames->value[474] = "DPMS_Y_24";
  this->VariableNames->value[475] = "DPMS_Y_25";
  this->VariableNames->value[476] = "DPMS_Y_26";
  this->VariableNames->value[477] = "DPMS_Y_27";
  this->VariableNames->value[478] = "DPMS_Y_28";
  this->VariableNames->value[479] = "DPMS_Y_29";
  this->VariableNames->value[480] = "DPMS_Y_30";
  this->VariableNames->value[481] = "DPMS_Y_31";
  this->VariableNames->value[482] = "DPMS_Y_32";
  this->VariableNames->value[483] = "DPMS_Y_33";
  this->VariableNames->value[484] = "DPMS_Y_34";
  this->VariableNames->value[485] = "DPMS_Y_35";
  this->VariableNames->value[486] = "DPMS_Y_36";
  this->VariableNames->value[487] = "DPMS_Y_37";
  this->VariableNames->value[488] = "DPMS_Y_38";
  this->VariableNames->value[489] = "DPMS_Y_39";
  this->VariableNames->value[490] = "DPMS_Y_40";
  this->VariableNames->value[491] = "DPMS_Y_41";
  this->VariableNames->value[492] = "DPMS_Y_42";
  this->VariableNames->value[493] = "DPMS_Y_43";
  this->VariableNames->value[494] = "DPMS_Y_44";
  this->VariableNames->value[495] = "DPMS_Y_45";
  this->VariableNames->value[496] = "DPMS_Y_46";
  this->VariableNames->value[497] = "DPMS_Y_47";
  this->VariableNames->value[498] = "DPMS_Y_48";
  this->VariableNames->value[499] = "DPMS_Y_49";

  this->VariableNames->value[500] = "NUT";
  this->VariableNames->value[501] = "NUT_M1";
  this->VariableNames->value[502] = "NUT_M2";
  this->VariableNames->value[503] = "RUU_M1";
  this->VariableNames->value[504] = "RVV_M1";
  this->VariableNames->value[505] = "RWW_M1";
  this->VariableNames->value[506] = "RUV_M1";
  this->VariableNames->value[507] = "RVW_M1";
  this->VariableNames->value[508] = "RUW_M1";
  this->VariableNames->value[509] = "RUU_M2";
  this->VariableNames->value[510] = "RVV_M2";
  this->VariableNames->value[511] = "RWW_M2";
  this->VariableNames->value[512] = "RUV_M2";
  this->VariableNames->value[513] = "RVW_M2";
  this->VariableNames->value[514] = "RUW_M2";
  this->VariableNames->value[515] = "ENERGY_M1";
  this->VariableNames->value[516] = "ENERGY_M2";
  this->VariableNames->value[517] = "DENSITY_M1";
  this->VariableNames->value[518] = "DENSITY_M2";
  this->VariableNames->value[519] = "DPMS_PDF_1";
  this->VariableNames->value[520] = "DPMS_PDF_2";
  this->VariableNames->value[521] = "V2";
  this->VariableNames->value[522] = "V2_M1";
  this->VariableNames->value[523] = "V2_M2";
  this->VariableNames->value[524] = "FEL";
  this->VariableNames->value[525] = "FEL_M1";
  this->VariableNames->value[526] = "FEL_M2";
  this->VariableNames->value[527] = "LKE";
  this->VariableNames->value[528] = "LKE_M1";
  this->VariableNames->value[529] = "LKE_M2";
  this->VariableNames->value[530] = "SHELL_CELL_T";
  this->VariableNames->value[531] = "SHELL_FACE_T";
  this->VariableNames->value[532] = "SHELL_CELL_ENERGY_M1";
  this->VariableNames->value[533] = "SHELL_CELL_ENERGY_M2";
  this->VariableNames->value[540] = "DPMS_TKE";
  this->VariableNames->value[541] = "DPMS_D";
  this->VariableNames->value[542] = "DPMS_O";
  this->VariableNames->value[543] = "DPMS_TKE_RUU";
  this->VariableNames->value[544] = "DPMS_TKE_RVV";
  this->VariableNames->value[545] = "DPMS_TKE_RWW";
  this->VariableNames->value[546] = "DPMS_TKE_RUV";
  this->VariableNames->value[547] = "DPMS_TKE_RVW";
  this->VariableNames->value[548] = "DPMS_TKE_RUW";
  this->VariableNames->value[549] = "DPMS_DS_MASS";
  this->VariableNames->value[550] = "DPMS_DS_ENERGY";
  this->VariableNames->value[551] = "DPMS_DS_TKE";
  this->VariableNames->value[552] = "DPMS_DS_D";
  this->VariableNames->value[553] = "DPMS_DS_O";
  this->VariableNames->value[554] = "DPMS_DS_TKE_RUU";
  this->VariableNames->value[555] = "DPMS_DS_TKE_RVV";
  this->VariableNames->value[556] = "DPMS_DS_TKE_RWW";
  this->VariableNames->value[557] = "DPMS_DS_TKE_RUV";
  this->VariableNames->value[558] = "DPMS_DS_TKE_RVW";
  this->VariableNames->value[559] = "DPMS_DS_TKE_RUW";
  this->VariableNames->value[560] = "DPMS_DS_PDF_1";
  this->VariableNames->value[561] = "DPMS_DS_PDF_2";
  this->VariableNames->value[562] = "DPMS_DS_EMISS";
  this->VariableNames->value[563] = "DPMS_DS_ABS";
  this->VariableNames->value[564] = "DPMS_DS_SCAT";
  this->VariableNames->value[565] = "DPMS_DS_BURNOUT";
  this->VariableNames->value[566] = "DPMS_DS_MOM";
  this->VariableNames->value[567] = "DPMS_DS_WSWIRL";
  this->VariableNames->value[580] = "MU_TURB_L";
  this->VariableNames->value[581] = "MU_TURB_S";
  this->VariableNames->value[582] = "TKE_TRANS";
  this->VariableNames->value[583] = "TKE_TRANS_M1";
  this->VariableNames->value[584] = "TKE_TRANS_M2";
  this->VariableNames->value[585] = "MU_TURB_W";
  this->VariableNames->value[600] = "DELH";
  this->VariableNames->value[601] = "DPMS_MOM_AP";
  this->VariableNames->value[602] = "DPMS_WSWIRL_AP";
  this->VariableNames->value[603] = "X_PULL";
  this->VariableNames->value[604] = "Y_PULL";
  this->VariableNames->value[605] = "Z_PULL";
  this->VariableNames->value[606] = "LIQF";
  this->VariableNames->value[610] = "PDFT_QBAR";
  this->VariableNames->value[611] = "PDFT_PHI";
  this->VariableNames->value[612] = "PDFT_Q_TA";
  this->VariableNames->value[613] = "PDFT_SVOL_TA";
  this->VariableNames->value[614] = "PDFT_MASS_TA";
  this->VariableNames->value[615] = "PDFT_T4_TA";
  this->VariableNames->value[620] = "MICRO_MIX_FVAR1 ";
  this->VariableNames->value[621] = "MICRO_MIX_FVAR2 ";
  this->VariableNames->value[622] = "MICRO_MIX_FVAR3 ";
  this->VariableNames->value[623] = "MICRO_MIX_FVAR1_M1 ";
  this->VariableNames->value[624] = "MICRO_MIX_FVAR2_M1 ";
  this->VariableNames->value[625] = "MICRO_MIX_FVAR3_M1 ";
  this->VariableNames->value[626] = "MICRO_MIX_FVAR1_M2 ";
  this->VariableNames->value[627] = "MICRO_MIX_FVAR2_M2 ";
  this->VariableNames->value[628] = "MICRO_MIX_FVAR3_M2 ";
  this->VariableNames->value[630] = "SCAD_LES ";
  this->VariableNames->value[635] = "UFLA_Y    ";
  this->VariableNames->value[636] = "UFLA_Y_M1 ";
  this->VariableNames->value[637] = "UFLA_Y_M2 ";
  this->VariableNames->value[645] = "CREV_MASS";
  this->VariableNames->value[646] = "CREV_ENRG";
  this->VariableNames->value[647] = "CREV_MOM";
  this->VariableNames->value[650] = "ACOUSTICS_MODEL";
  this->VariableNames->value[651] = "AC_RECEIVERS_DATA";
  this->VariableNames->value[652] = "SV_DPDT_RMS";
  this->VariableNames->value[653] = "SV_PRESSURE_M1";
  this->VariableNames->value[654] = "AC_PERIODIC_INDEX";
  this->VariableNames->value[655] = "AC_PERIODIC_PS";
  this->VariableNames->value[656] = "AC_F_NORMAL";
  this->VariableNames->value[657] = "AC_F_CENTROID";
  this->VariableNames->value[660] = "IGNITE";
  this->VariableNames->value[661] = "IGNITE_M1";
  this->VariableNames->value[662] = "IGNITE_M2";
  this->VariableNames->value[663] = "IGNITE_RATE";

  this->VariableNames->value[680] = "WALL_SHEAR_MEAN";
  this->VariableNames->value[681] = "UV_MEAN";
  this->VariableNames->value[682] = "UW_MEAN";
  this->VariableNames->value[683] = "VW_MEAN";
  this->VariableNames->value[684] = "UT_MEAN";
  this->VariableNames->value[685] = "VT_MEAN";
  this->VariableNames->value[686] = "WT_MEAN";
  this->VariableNames->value[687] = "BOUNDARY_HEAT_FLUX_MEAN";

  this->VariableNames->value[700] = "UDS_00";
  this->VariableNames->value[701] = "UDS_01";
  this->VariableNames->value[702] = "UDS_02";
  this->VariableNames->value[703] = "UDS_03";
  this->VariableNames->value[704] = "UDS_04";
  this->VariableNames->value[705] = "UDS_05";
  this->VariableNames->value[706] = "UDS_06";
  this->VariableNames->value[707] = "UDS_07";
  this->VariableNames->value[708] = "UDS_08";
  this->VariableNames->value[709] = "UDS_09";
  this->VariableNames->value[710] = "UDS_10";
  this->VariableNames->value[711] = "UDS_11";
  this->VariableNames->value[712] = "UDS_12";
  this->VariableNames->value[713] = "UDS_13";
  this->VariableNames->value[714] = "UDS_14";
  this->VariableNames->value[715] = "UDS_15";
  this->VariableNames->value[716] = "UDS_16";
  this->VariableNames->value[717] = "UDS_17";
  this->VariableNames->value[718] = "UDS_18";
  this->VariableNames->value[719] = "UDS_19";
  this->VariableNames->value[720] = "UDS_20";
  this->VariableNames->value[721] = "UDS_21";
  this->VariableNames->value[722] = "UDS_22";
  this->VariableNames->value[723] = "UDS_23";
  this->VariableNames->value[724] = "UDS_24";
  this->VariableNames->value[725] = "UDS_25";
  this->VariableNames->value[726] = "UDS_26";
  this->VariableNames->value[727] = "UDS_27";
  this->VariableNames->value[728] = "UDS_28";
  this->VariableNames->value[729] = "UDS_29";
  this->VariableNames->value[730] = "UDS_30";
  this->VariableNames->value[731] = "UDS_31";
  this->VariableNames->value[732] = "UDS_32";
  this->VariableNames->value[733] = "UDS_33";
  this->VariableNames->value[734] = "UDS_34";
  this->VariableNames->value[735] = "UDS_35";
  this->VariableNames->value[736] = "UDS_36";
  this->VariableNames->value[737] = "UDS_37";
  this->VariableNames->value[738] = "UDS_38";
  this->VariableNames->value[739] = "UDS_39";
  this->VariableNames->value[740] = "UDS_40";
  this->VariableNames->value[741] = "UDS_41";
  this->VariableNames->value[742] = "UDS_42";
  this->VariableNames->value[743] = "UDS_43";
  this->VariableNames->value[744] = "UDS_44";
  this->VariableNames->value[745] = "UDS_45";
  this->VariableNames->value[746] = "UDS_46";
  this->VariableNames->value[747] = "UDS_47";
  this->VariableNames->value[748] = "UDS_48";
  this->VariableNames->value[749] = "UDS_49";

  this->VariableNames->value[750] = "UDS_M1_00";
  this->VariableNames->value[751] = "UDS_M1_01";
  this->VariableNames->value[752] = "UDS_M1_02";
  this->VariableNames->value[753] = "UDS_M1_03";
  this->VariableNames->value[754] = "UDS_M1_04";
  this->VariableNames->value[755] = "UDS_M1_05";
  this->VariableNames->value[756] = "UDS_M1_06";
  this->VariableNames->value[757] = "UDS_M1_07";
  this->VariableNames->value[758] = "UDS_M1_08";
  this->VariableNames->value[759] = "UDS_M1_09";
  this->VariableNames->value[760] = "UDS_M1_10";
  this->VariableNames->value[761] = "UDS_M1_11";
  this->VariableNames->value[762] = "UDS_M1_12";
  this->VariableNames->value[763] = "UDS_M1_13";
  this->VariableNames->value[764] = "UDS_M1_14";
  this->VariableNames->value[765] = "UDS_M1_15";
  this->VariableNames->value[766] = "UDS_M1_16";
  this->VariableNames->value[767] = "UDS_M1_17";
  this->VariableNames->value[768] = "UDS_M1_18";
  this->VariableNames->value[769] = "UDS_M1_19";
  this->VariableNames->value[770] = "UDS_M1_20";
  this->VariableNames->value[771] = "UDS_M1_21";
  this->VariableNames->value[772] = "UDS_M1_22";
  this->VariableNames->value[773] = "UDS_M1_23";
  this->VariableNames->value[774] = "UDS_M1_24";
  this->VariableNames->value[775] = "UDS_M1_25";
  this->VariableNames->value[776] = "UDS_M1_26";
  this->VariableNames->value[777] = "UDS_M1_27";
  this->VariableNames->value[778] = "UDS_M1_28";
  this->VariableNames->value[779] = "UDS_M1_29";
  this->VariableNames->value[780] = "UDS_M1_30";
  this->VariableNames->value[781] = "UDS_M1_31";
  this->VariableNames->value[782] = "UDS_M1_32";
  this->VariableNames->value[783] = "UDS_M1_33";
  this->VariableNames->value[784] = "UDS_M1_34";
  this->VariableNames->value[785] = "UDS_M1_35";
  this->VariableNames->value[786] = "UDS_M1_36";
  this->VariableNames->value[787] = "UDS_M1_37";
  this->VariableNames->value[788] = "UDS_M1_38";
  this->VariableNames->value[789] = "UDS_M1_39";
  this->VariableNames->value[790] = "UDS_M1_40";
  this->VariableNames->value[791] = "UDS_M1_41";
  this->VariableNames->value[792] = "UDS_M1_42";
  this->VariableNames->value[793] = "UDS_M1_43";
  this->VariableNames->value[794] = "UDS_M1_44";
  this->VariableNames->value[795] = "UDS_M1_45";
  this->VariableNames->value[796] = "UDS_M1_46";
  this->VariableNames->value[797] = "UDS_M1_47";
  this->VariableNames->value[798] = "UDS_M1_48";
  this->VariableNames->value[799] = "UDS_M1_49";

  this->VariableNames->value[800] = "UDS_M2_00";
  this->VariableNames->value[801] = "UDS_M2_01";
  this->VariableNames->value[802] = "UDS_M2_02";
  this->VariableNames->value[803] = "UDS_M2_03";
  this->VariableNames->value[804] = "UDS_M2_04";
  this->VariableNames->value[805] = "UDS_M2_05";
  this->VariableNames->value[806] = "UDS_M2_06";
  this->VariableNames->value[807] = "UDS_M2_07";
  this->VariableNames->value[808] = "UDS_M2_08";
  this->VariableNames->value[809] = "UDS_M2_09";
  this->VariableNames->value[810] = "UDS_M2_10";
  this->VariableNames->value[811] = "UDS_M2_11";
  this->VariableNames->value[812] = "UDS_M2_12";
  this->VariableNames->value[813] = "UDS_M2_13";
  this->VariableNames->value[814] = "UDS_M2_14";
  this->VariableNames->value[815] = "UDS_M2_15";
  this->VariableNames->value[816] = "UDS_M2_16";
  this->VariableNames->value[817] = "UDS_M2_17";
  this->VariableNames->value[818] = "UDS_M2_18";
  this->VariableNames->value[819] = "UDS_M2_19";
  this->VariableNames->value[820] = "UDS_M2_20";
  this->VariableNames->value[821] = "UDS_M2_21";
  this->VariableNames->value[822] = "UDS_M2_22";
  this->VariableNames->value[823] = "UDS_M2_23";
  this->VariableNames->value[824] = "UDS_M2_24";
  this->VariableNames->value[825] = "UDS_M2_25";
  this->VariableNames->value[826] = "UDS_M2_26";
  this->VariableNames->value[827] = "UDS_M2_27";
  this->VariableNames->value[828] = "UDS_M2_28";
  this->VariableNames->value[829] = "UDS_M2_29";
  this->VariableNames->value[830] = "UDS_M2_30";
  this->VariableNames->value[831] = "UDS_M2_31";
  this->VariableNames->value[832] = "UDS_M2_32";
  this->VariableNames->value[833] = "UDS_M2_33";
  this->VariableNames->value[834] = "UDS_M2_34";
  this->VariableNames->value[835] = "UDS_M2_35";
  this->VariableNames->value[836] = "UDS_M2_36";
  this->VariableNames->value[837] = "UDS_M2_37";
  this->VariableNames->value[838] = "UDS_M2_38";
  this->VariableNames->value[839] = "UDS_M2_39";
  this->VariableNames->value[840] = "UDS_M2_40";
  this->VariableNames->value[841] = "UDS_M2_41";
  this->VariableNames->value[842] = "UDS_M2_42";
  this->VariableNames->value[843] = "UDS_M2_43";
  this->VariableNames->value[844] = "UDS_M2_44";
  this->VariableNames->value[845] = "UDS_M2_45";
  this->VariableNames->value[846] = "UDS_M2_46";
  this->VariableNames->value[847] = "UDS_M2_47";
  this->VariableNames->value[848] = "UDS_M2_48";
  this->VariableNames->value[849] = "UDS_M2_49";

  this->VariableNames->value[850] = "DPMS_DS_Y_00";
  this->VariableNames->value[851] = "DPMS_DS_Y_01";
  this->VariableNames->value[852] = "DPMS_DS_Y_02";
  this->VariableNames->value[853] = "DPMS_DS_Y_03";
  this->VariableNames->value[854] = "DPMS_DS_Y_04";
  this->VariableNames->value[855] = "DPMS_DS_Y_05";
  this->VariableNames->value[856] = "DPMS_DS_Y_06";
  this->VariableNames->value[857] = "DPMS_DS_Y_07";
  this->VariableNames->value[858] = "DPMS_DS_Y_08";
  this->VariableNames->value[859] = "DPMS_DS_Y_09";
  this->VariableNames->value[860] = "DPMS_DS_Y_10";
  this->VariableNames->value[861] = "DPMS_DS_Y_11";
  this->VariableNames->value[862] = "DPMS_DS_Y_12";
  this->VariableNames->value[863] = "DPMS_DS_Y_13";
  this->VariableNames->value[864] = "DPMS_DS_Y_14";
  this->VariableNames->value[865] = "DPMS_DS_Y_15";
  this->VariableNames->value[866] = "DPMS_DS_Y_16";
  this->VariableNames->value[867] = "DPMS_DS_Y_17";
  this->VariableNames->value[868] = "DPMS_DS_Y_18";
  this->VariableNames->value[869] = "DPMS_DS_Y_19";
  this->VariableNames->value[870] = "DPMS_DS_Y_20";
  this->VariableNames->value[871] = "DPMS_DS_Y_21";
  this->VariableNames->value[872] = "DPMS_DS_Y_22";
  this->VariableNames->value[873] = "DPMS_DS_Y_23";
  this->VariableNames->value[874] = "DPMS_DS_Y_24";
  this->VariableNames->value[875] = "DPMS_DS_Y_25";
  this->VariableNames->value[876] = "DPMS_DS_Y_26";
  this->VariableNames->value[877] = "DPMS_DS_Y_27";
  this->VariableNames->value[878] = "DPMS_DS_Y_28";
  this->VariableNames->value[879] = "DPMS_DS_Y_29";
  this->VariableNames->value[880] = "DPMS_DS_Y_30";
  this->VariableNames->value[881] = "DPMS_DS_Y_31";
  this->VariableNames->value[882] = "DPMS_DS_Y_32";
  this->VariableNames->value[883] = "DPMS_DS_Y_33";
  this->VariableNames->value[884] = "DPMS_DS_Y_34";
  this->VariableNames->value[885] = "DPMS_DS_Y_35";
  this->VariableNames->value[886] = "DPMS_DS_Y_36";
  this->VariableNames->value[887] = "DPMS_DS_Y_37";
  this->VariableNames->value[888] = "DPMS_DS_Y_38";
  this->VariableNames->value[889] = "DPMS_DS_Y_39";
  this->VariableNames->value[890] = "DPMS_DS_Y_40";
  this->VariableNames->value[891] = "DPMS_DS_Y_41";
  this->VariableNames->value[892] = "DPMS_DS_Y_42";
  this->VariableNames->value[893] = "DPMS_DS_Y_43";
  this->VariableNames->value[894] = "DPMS_DS_Y_44";
  this->VariableNames->value[895] = "DPMS_DS_Y_45";
  this->VariableNames->value[896] = "DPMS_DS_Y_46";
  this->VariableNames->value[897] = "DPMS_DS_Y_47";
  this->VariableNames->value[898] = "DPMS_DS_Y_48";
  this->VariableNames->value[899] = "DPMS_DS_Y_49";

  this->VariableNames->value[910] = "GRANULAR_PRESSURE";
  this->VariableNames->value[911] = "DPMS_DS_P1_S";
  this->VariableNames->value[912] = "DPMS_DS_P1_AP";
  this->VariableNames->value[913] = "DPMS_DS_P1_DIFF";

  this->VariableNames->value[920] = "DPMS_DS_SURFACE_SPECIES_00";
  this->VariableNames->value[921] = "DPMS_DS_SURFACE_SPECIES_01";
  this->VariableNames->value[922] = "DPMS_DS_SURFACE_SPECIES_02";
  this->VariableNames->value[923] = "DPMS_DS_SURFACE_SPECIES_03";
  this->VariableNames->value[924] = "DPMS_DS_SURFACE_SPECIES_04";
  this->VariableNames->value[925] = "DPMS_DS_SURFACE_SPECIES_05";
  this->VariableNames->value[926] = "DPMS_DS_SURFACE_SPECIES_06";
  this->VariableNames->value[927] = "DPMS_DS_SURFACE_SPECIES_07";
  this->VariableNames->value[928] = "DPMS_DS_SURFACE_SPECIES_08";
  this->VariableNames->value[929] = "DPMS_DS_SURFACE_SPECIES_09";
  this->VariableNames->value[930] = "DPMS_DS_SURFACE_SPECIES_10";
  this->VariableNames->value[931] = "DPMS_DS_SURFACE_SPECIES_11";
  this->VariableNames->value[932] = "DPMS_DS_SURFACE_SPECIES_12";
  this->VariableNames->value[933] = "DPMS_DS_SURFACE_SPECIES_13";
  this->VariableNames->value[934] = "DPMS_DS_SURFACE_SPECIES_14";
  this->VariableNames->value[935] = "DPMS_DS_SURFACE_SPECIES_15";
  this->VariableNames->value[936] = "DPMS_DS_SURFACE_SPECIES_16";
  this->VariableNames->value[937] = "DPMS_DS_SURFACE_SPECIES_17";
  this->VariableNames->value[938] = "DPMS_DS_SURFACE_SPECIES_18";
  this->VariableNames->value[939] = "DPMS_DS_SURFACE_SPECIES_19";
  this->VariableNames->value[940] = "DPMS_DS_SURFACE_SPECIES_20";
  this->VariableNames->value[941] = "DPMS_DS_SURFACE_SPECIES_21";
  this->VariableNames->value[942] = "DPMS_DS_SURFACE_SPECIES_22";
  this->VariableNames->value[943] = "DPMS_DS_SURFACE_SPECIES_23";
  this->VariableNames->value[944] = "DPMS_DS_SURFACE_SPECIES_24";
  this->VariableNames->value[945] = "DPMS_DS_SURFACE_SPECIES_25";
  this->VariableNames->value[946] = "DPMS_DS_SURFACE_SPECIES_26";
  this->VariableNames->value[947] = "DPMS_DS_SURFACE_SPECIES_27";
  this->VariableNames->value[948] = "DPMS_DS_SURFACE_SPECIES_28";
  this->VariableNames->value[949] = "DPMS_DS_SURFACE_SPECIES_29";
  this->VariableNames->value[950] = "DPMS_DS_SURFACE_SPECIES_30";
  this->VariableNames->value[951] = "DPMS_DS_SURFACE_SPECIES_31";
  this->VariableNames->value[952] = "DPMS_DS_SURFACE_SPECIES_32";
  this->VariableNames->value[953] = "DPMS_DS_SURFACE_SPECIES_33";
  this->VariableNames->value[954] = "DPMS_DS_SURFACE_SPECIES_34";
  this->VariableNames->value[955] = "DPMS_DS_SURFACE_SPECIES_35";
  this->VariableNames->value[956] = "DPMS_DS_SURFACE_SPECIES_36";
  this->VariableNames->value[957] = "DPMS_DS_SURFACE_SPECIES_37";
  this->VariableNames->value[958] = "DPMS_DS_SURFACE_SPECIES_38";
  this->VariableNames->value[959] = "DPMS_DS_SURFACE_SPECIES_39";
  this->VariableNames->value[960] = "DPMS_DS_SURFACE_SPECIES_40";
  this->VariableNames->value[961] = "DPMS_DS_SURFACE_SPECIES_41";
  this->VariableNames->value[962] = "DPMS_DS_SURFACE_SPECIES_42";
  this->VariableNames->value[963] = "DPMS_DS_SURFACE_SPECIES_43";
  this->VariableNames->value[964] = "DPMS_DS_SURFACE_SPECIES_44";
  this->VariableNames->value[965] = "DPMS_DS_SURFACE_SPECIES_45";
  this->VariableNames->value[966] = "DPMS_DS_SURFACE_SPECIES_46";
  this->VariableNames->value[967] = "DPMS_DS_SURFACE_SPECIES_47";
  this->VariableNames->value[968] = "DPMS_DS_SURFACE_SPECIES_48";
  this->VariableNames->value[969] = "DPMS_DS_SURFACE_SPECIES_49";
  this->VariableNames->value[970] = "UDM_I";

  this->VariableNames->value[1000] = "Y_MEAN_00";
  this->VariableNames->value[1001] = "Y_MEAN_01";
  this->VariableNames->value[1002] = "Y_MEAN_02";
  this->VariableNames->value[1003] = "Y_MEAN_03";
  this->VariableNames->value[1004] = "Y_MEAN_04";
  this->VariableNames->value[1005] = "Y_MEAN_05";
  this->VariableNames->value[1006] = "Y_MEAN_06";
  this->VariableNames->value[1007] = "Y_MEAN_07";
  this->VariableNames->value[1008] = "Y_MEAN_08";
  this->VariableNames->value[1009] = "Y_MEAN_09";
  this->VariableNames->value[1010] = "Y_MEAN_10";
  this->VariableNames->value[1011] = "Y_MEAN_11";
  this->VariableNames->value[1012] = "Y_MEAN_12";
  this->VariableNames->value[1013] = "Y_MEAN_13";
  this->VariableNames->value[1014] = "Y_MEAN_14";
  this->VariableNames->value[1015] = "Y_MEAN_15";
  this->VariableNames->value[1016] = "Y_MEAN_16";
  this->VariableNames->value[1017] = "Y_MEAN_17";
  this->VariableNames->value[1018] = "Y_MEAN_18";
  this->VariableNames->value[1019] = "Y_MEAN_19";
  this->VariableNames->value[1020] = "Y_MEAN_20";
  this->VariableNames->value[1021] = "Y_MEAN_21";
  this->VariableNames->value[1022] = "Y_MEAN_22";
  this->VariableNames->value[1023] = "Y_MEAN_23";
  this->VariableNames->value[1024] = "Y_MEAN_24";
  this->VariableNames->value[1025] = "Y_MEAN_25";
  this->VariableNames->value[1026] = "Y_MEAN_26";
  this->VariableNames->value[1027] = "Y_MEAN_27";
  this->VariableNames->value[1028] = "Y_MEAN_28";
  this->VariableNames->value[1029] = "Y_MEAN_29";
  this->VariableNames->value[1030] = "Y_MEAN_30";
  this->VariableNames->value[1031] = "Y_MEAN_31";
  this->VariableNames->value[1032] = "Y_MEAN_32";
  this->VariableNames->value[1033] = "Y_MEAN_33";
  this->VariableNames->value[1034] = "Y_MEAN_34";
  this->VariableNames->value[1035] = "Y_MEAN_35";
  this->VariableNames->value[1036] = "Y_MEAN_36";
  this->VariableNames->value[1037] = "Y_MEAN_37";
  this->VariableNames->value[1038] = "Y_MEAN_38";
  this->VariableNames->value[1039] = "Y_MEAN_39";
  this->VariableNames->value[1040] = "Y_MEAN_40";
  this->VariableNames->value[1041] = "Y_MEAN_41";
  this->VariableNames->value[1042] = "Y_MEAN_42";
  this->VariableNames->value[1043] = "Y_MEAN_43";
  this->VariableNames->value[1044] = "Y_MEAN_44";
  this->VariableNames->value[1045] = "Y_MEAN_45";
  this->VariableNames->value[1046] = "Y_MEAN_46";
  this->VariableNames->value[1047] = "Y_MEAN_47";
  this->VariableNames->value[1048] = "Y_MEAN_48";
  this->VariableNames->value[1049] = "Y_MEAN_49";

  this->VariableNames->value[1050] = "Y_RMS_00";
  this->VariableNames->value[1051] = "Y_RMS_01";
  this->VariableNames->value[1052] = "Y_RMS_02";
  this->VariableNames->value[1053] = "Y_RMS_03";
  this->VariableNames->value[1054] = "Y_RMS_04";
  this->VariableNames->value[1055] = "Y_RMS_05";
  this->VariableNames->value[1056] = "Y_RMS_06";
  this->VariableNames->value[1057] = "Y_RMS_07";
  this->VariableNames->value[1058] = "Y_RMS_08";
  this->VariableNames->value[1059] = "Y_RMS_09";
  this->VariableNames->value[1060] = "Y_RMS_10";
  this->VariableNames->value[1061] = "Y_RMS_11";
  this->VariableNames->value[1062] = "Y_RMS_12";
  this->VariableNames->value[1063] = "Y_RMS_13";
  this->VariableNames->value[1064] = "Y_RMS_14";
  this->VariableNames->value[1065] = "Y_RMS_15";
  this->VariableNames->value[1066] = "Y_RMS_16";
  this->VariableNames->value[1067] = "Y_RMS_17";
  this->VariableNames->value[1068] = "Y_RMS_18";
  this->VariableNames->value[1069] = "Y_RMS_19";
  this->VariableNames->value[1070] = "Y_RMS_20";
  this->VariableNames->value[1071] = "Y_RMS_21";
  this->VariableNames->value[1072] = "Y_RMS_22";
  this->VariableNames->value[1073] = "Y_RMS_23";
  this->VariableNames->value[1074] = "Y_RMS_24";
  this->VariableNames->value[1075] = "Y_RMS_25";
  this->VariableNames->value[1076] = "Y_RMS_26";
  this->VariableNames->value[1077] = "Y_RMS_27";
  this->VariableNames->value[1078] = "Y_RMS_28";
  this->VariableNames->value[1079] = "Y_RMS_29";
  this->VariableNames->value[1080] = "Y_RMS_30";
  this->VariableNames->value[1081] = "Y_RMS_31";
  this->VariableNames->value[1082] = "Y_RMS_32";
  this->VariableNames->value[1083] = "Y_RMS_33";
  this->VariableNames->value[1084] = "Y_RMS_34";
  this->VariableNames->value[1085] = "Y_RMS_35";
  this->VariableNames->value[1086] = "Y_RMS_36";
  this->VariableNames->value[1087] = "Y_RMS_37";
  this->VariableNames->value[1088] = "Y_RMS_38";
  this->VariableNames->value[1089] = "Y_RMS_39";
  this->VariableNames->value[1090] = "Y_RMS_40";
  this->VariableNames->value[1091] = "Y_RMS_41";
  this->VariableNames->value[1092] = "Y_RMS_42";
  this->VariableNames->value[1093] = "Y_RMS_43";
  this->VariableNames->value[1094] = "Y_RMS_44";
  this->VariableNames->value[1095] = "Y_RMS_45";
  this->VariableNames->value[1096] = "Y_RMS_46";
  this->VariableNames->value[1097] = "Y_RMS_47";
  this->VariableNames->value[1098] = "Y_RMS_48";
  this->VariableNames->value[1099] = "Y_RMS_49";


  this->VariableNames->value[1200] = "SITE_F_00";
  this->VariableNames->value[1201] = "SITE_F_01";
  this->VariableNames->value[1202] = "SITE_F_02";
  this->VariableNames->value[1203] = "SITE_F_03";
  this->VariableNames->value[1204] = "SITE_F_04";
  this->VariableNames->value[1205] = "SITE_F_05";
  this->VariableNames->value[1206] = "SITE_F_06";
  this->VariableNames->value[1207] = "SITE_F_07";
  this->VariableNames->value[1208] = "SITE_F_08";
  this->VariableNames->value[1209] = "SITE_F_09";
  this->VariableNames->value[1210] = "SITE_F_10";
  this->VariableNames->value[1211] = "SITE_F_11";
  this->VariableNames->value[1212] = "SITE_F_12";
  this->VariableNames->value[1213] = "SITE_F_13";
  this->VariableNames->value[1214] = "SITE_F_14";
  this->VariableNames->value[1215] = "SITE_F_15";
  this->VariableNames->value[1216] = "SITE_F_16";
  this->VariableNames->value[1217] = "SITE_F_17";
  this->VariableNames->value[1218] = "SITE_F_18";
  this->VariableNames->value[1219] = "SITE_F_19";
  this->VariableNames->value[1220] = "SITE_F_20";
  this->VariableNames->value[1221] = "SITE_F_21";
  this->VariableNames->value[1222] = "SITE_F_22";
  this->VariableNames->value[1223] = "SITE_F_23";
  this->VariableNames->value[1224] = "SITE_F_24";
  this->VariableNames->value[1225] = "SITE_F_25";
  this->VariableNames->value[1226] = "SITE_F_26";
  this->VariableNames->value[1227] = "SITE_F_27";
  this->VariableNames->value[1228] = "SITE_F_28";
  this->VariableNames->value[1229] = "SITE_F_29";
  this->VariableNames->value[1230] = "SITE_F_30";
  this->VariableNames->value[1231] = "SITE_F_31";
  this->VariableNames->value[1232] = "SITE_F_32";
  this->VariableNames->value[1233] = "SITE_F_33";
  this->VariableNames->value[1234] = "SITE_F_34";
  this->VariableNames->value[1235] = "SITE_F_35";
  this->VariableNames->value[1236] = "SITE_F_36";
  this->VariableNames->value[1237] = "SITE_F_37";
  this->VariableNames->value[1238] = "SITE_F_38";
  this->VariableNames->value[1239] = "SITE_F_39";
  this->VariableNames->value[1240] = "SITE_F_40";
  this->VariableNames->value[1241] = "SITE_F_41";
  this->VariableNames->value[1242] = "SITE_F_42";
  this->VariableNames->value[1243] = "SITE_F_43";
  this->VariableNames->value[1244] = "SITE_F_44";
  this->VariableNames->value[1245] = "SITE_F_45";
  this->VariableNames->value[1246] = "SITE_F_46";
  this->VariableNames->value[1247] = "SITE_F_47";
  this->VariableNames->value[1248] = "SITE_F_48";
  this->VariableNames->value[1249] = "SITE_F_49";

  this->VariableNames->value[1250] = "CREV_Y_00";
  this->VariableNames->value[1251] = "CREV_Y_01";
  this->VariableNames->value[1252] = "CREV_Y_02";
  this->VariableNames->value[1253] = "CREV_Y_03";
  this->VariableNames->value[1254] = "CREV_Y_04";
  this->VariableNames->value[1255] = "CREV_Y_05";
  this->VariableNames->value[1256] = "CREV_Y_06";
  this->VariableNames->value[1257] = "CREV_Y_07";
  this->VariableNames->value[1258] = "CREV_Y_08";
  this->VariableNames->value[1259] = "CREV_Y_09";
  this->VariableNames->value[1260] = "CREV_Y_10";
  this->VariableNames->value[1261] = "CREV_Y_11";
  this->VariableNames->value[1262] = "CREV_Y_12";
  this->VariableNames->value[1263] = "CREV_Y_13";
  this->VariableNames->value[1264] = "CREV_Y_14";
  this->VariableNames->value[1265] = "CREV_Y_15";
  this->VariableNames->value[1266] = "CREV_Y_16";
  this->VariableNames->value[1267] = "CREV_Y_17";
  this->VariableNames->value[1268] = "CREV_Y_18";
  this->VariableNames->value[1269] = "CREV_Y_19";
  this->VariableNames->value[1270] = "CREV_Y_20";
  this->VariableNames->value[1271] = "CREV_Y_21";
  this->VariableNames->value[1272] = "CREV_Y_22";
  this->VariableNames->value[1273] = "CREV_Y_23";
  this->VariableNames->value[1274] = "CREV_Y_24";
  this->VariableNames->value[1275] = "CREV_Y_25";
  this->VariableNames->value[1276] = "CREV_Y_26";
  this->VariableNames->value[1277] = "CREV_Y_27";
  this->VariableNames->value[1278] = "CREV_Y_28";
  this->VariableNames->value[1279] = "CREV_Y_29";
  this->VariableNames->value[1280] = "CREV_Y_30";
  this->VariableNames->value[1281] = "CREV_Y_31";
  this->VariableNames->value[1282] = "CREV_Y_32";
  this->VariableNames->value[1283] = "CREV_Y_33";
  this->VariableNames->value[1284] = "CREV_Y_34";
  this->VariableNames->value[1285] = "CREV_Y_35";
  this->VariableNames->value[1286] = "CREV_Y_36";
  this->VariableNames->value[1287] = "CREV_Y_37";
  this->VariableNames->value[1288] = "CREV_Y_38";
  this->VariableNames->value[1289] = "CREV_Y_39";
  this->VariableNames->value[1290] = "CREV_Y_40";
  this->VariableNames->value[1291] = "CREV_Y_41";
  this->VariableNames->value[1292] = "CREV_Y_42";
  this->VariableNames->value[1293] = "CREV_Y_43";
  this->VariableNames->value[1294] = "CREV_Y_44";
  this->VariableNames->value[1295] = "CREV_Y_45";
  this->VariableNames->value[1296] = "CREV_Y_46";
  this->VariableNames->value[1297] = "CREV_Y_47";
  this->VariableNames->value[1298] = "CREV_Y_48";
  this->VariableNames->value[1299] = "CREV_Y_49";

  this->VariableNames->value[1301] = "WSB";
  this->VariableNames->value[1302] = "WSN";
  this->VariableNames->value[1303] = "WSR";
  this->VariableNames->value[1304] = "WSB_M1";
  this->VariableNames->value[1305] = "WSB_M2";
  this->VariableNames->value[1306] = "WSN_M1";
  this->VariableNames->value[1307] = "WSN_M2";
  this->VariableNames->value[1308] = "WSR_M1";
  this->VariableNames->value[1309] = "WSR_M2";
  this->VariableNames->value[1310] = "MASGEN";
  this->VariableNames->value[1311] = "NUCRAT";
  this->VariableNames->value[1330] = "TEMPERATURE_M1";
  this->VariableNames->value[1331] = "TEMPERATURE_M2";

  this->VariableNames->value[1350] = "SURF_F_00";
  this->VariableNames->value[1351] = "SURF_F_01";
  this->VariableNames->value[1352] = "SURF_F_02";
  this->VariableNames->value[1353] = "SURF_F_03";
  this->VariableNames->value[1354] = "SURF_F_04";
  this->VariableNames->value[1355] = "SURF_F_05";
  this->VariableNames->value[1356] = "SURF_F_06";
  this->VariableNames->value[1357] = "SURF_F_07";
  this->VariableNames->value[1358] = "SURF_F_08";
  this->VariableNames->value[1359] = "SURF_F_09";
  this->VariableNames->value[1360] = "SURF_F_10";
  this->VariableNames->value[1361] = "SURF_F_11";
  this->VariableNames->value[1362] = "SURF_F_12";
  this->VariableNames->value[1363] = "SURF_F_13";
  this->VariableNames->value[1364] = "SURF_F_14";
  this->VariableNames->value[1365] = "SURF_F_15";
  this->VariableNames->value[1366] = "SURF_F_16";
  this->VariableNames->value[1367] = "SURF_F_17";
  this->VariableNames->value[1368] = "SURF_F_18";
  this->VariableNames->value[1369] = "SURF_F_19";
  this->VariableNames->value[1370] = "SURF_F_20";
  this->VariableNames->value[1371] = "SURF_F_21";
  this->VariableNames->value[1372] = "SURF_F_22";
  this->VariableNames->value[1373] = "SURF_F_23";
  this->VariableNames->value[1374] = "SURF_F_24";
  this->VariableNames->value[1375] = "SURF_F_25";
  this->VariableNames->value[1376] = "SURF_F_26";
  this->VariableNames->value[1377] = "SURF_F_27";
  this->VariableNames->value[1378] = "SURF_F_28";
  this->VariableNames->value[1379] = "SURF_F_29";
  this->VariableNames->value[1380] = "SURF_F_30";
  this->VariableNames->value[1381] = "SURF_F_31";
  this->VariableNames->value[1382] = "SURF_F_32";
  this->VariableNames->value[1383] = "SURF_F_33";
  this->VariableNames->value[1384] = "SURF_F_34";
  this->VariableNames->value[1385] = "SURF_F_35";
  this->VariableNames->value[1386] = "SURF_F_36";
  this->VariableNames->value[1387] = "SURF_F_37";
  this->VariableNames->value[1388] = "SURF_F_38";
  this->VariableNames->value[1389] = "SURF_F_39";
  this->VariableNames->value[1390] = "SURF_F_40";
  this->VariableNames->value[1391] = "SURF_F_41";
  this->VariableNames->value[1392] = "SURF_F_42";
  this->VariableNames->value[1393] = "SURF_F_43";
  this->VariableNames->value[1394] = "SURF_F_44";
  this->VariableNames->value[1395] = "SURF_F_45";
  this->VariableNames->value[1396] = "SURF_F_46";
  this->VariableNames->value[1397] = "SURF_F_47";
  this->VariableNames->value[1398] = "SURF_F_48";
  this->VariableNames->value[1399] = "SURF_F_49";

  this->VariableNames->value[7700] = "PB_DISC_00";
  this->VariableNames->value[7701] = "PB_DISC_01";
  this->VariableNames->value[7702] = "PB_DISC_02";
  this->VariableNames->value[7703] = "PB_DISC_03";
  this->VariableNames->value[7704] = "PB_DISC_04";
  this->VariableNames->value[7705] = "PB_DISC_05";
  this->VariableNames->value[7706] = "PB_DISC_06";
  this->VariableNames->value[7707] = "PB_DISC_07";
  this->VariableNames->value[7708] = "PB_DISC_08";
  this->VariableNames->value[7709] = "PB_DISC_09";
  this->VariableNames->value[7710] = "PB_DISC_10";
  this->VariableNames->value[7711] = "PB_DISC_11";
  this->VariableNames->value[7712] = "PB_DISC_12";
  this->VariableNames->value[7713] = "PB_DISC_13";
  this->VariableNames->value[7714] = "PB_DISC_14";
  this->VariableNames->value[7715] = "PB_DISC_15";
  this->VariableNames->value[7716] = "PB_DISC_16";
  this->VariableNames->value[7717] = "PB_DISC_17";
  this->VariableNames->value[7718] = "PB_DISC_18";
  this->VariableNames->value[7719] = "PB_DISC_19";
  this->VariableNames->value[7720] = "PB_DISC_20";
  this->VariableNames->value[7721] = "PB_DISC_21";
  this->VariableNames->value[7722] = "PB_DISC_22";
  this->VariableNames->value[7723] = "PB_DISC_23";
  this->VariableNames->value[7724] = "PB_DISC_24";
  this->VariableNames->value[7725] = "PB_DISC_25";
  this->VariableNames->value[7726] = "PB_DISC_26";
  this->VariableNames->value[7727] = "PB_DISC_27";
  this->VariableNames->value[7728] = "PB_DISC_28";
  this->VariableNames->value[7729] = "PB_DISC_29";
  this->VariableNames->value[7730] = "PB_DISC_30";
  this->VariableNames->value[7731] = "PB_DISC_31";
  this->VariableNames->value[7732] = "PB_DISC_32";
  this->VariableNames->value[7733] = "PB_DISC_33";
  this->VariableNames->value[7734] = "PB_DISC_34";
  this->VariableNames->value[7735] = "PB_DISC_35";
  this->VariableNames->value[7736] = "PB_DISC_36";
  this->VariableNames->value[7737] = "PB_DISC_37";
  this->VariableNames->value[7738] = "PB_DISC_38";
  this->VariableNames->value[7739] = "PB_DISC_39";
  this->VariableNames->value[7740] = "PB_DISC_40";
  this->VariableNames->value[7741] = "PB_DISC_41";
  this->VariableNames->value[7742] = "PB_DISC_42";
  this->VariableNames->value[7743] = "PB_DISC_43";
  this->VariableNames->value[7744] = "PB_DISC_44";
  this->VariableNames->value[7745] = "PB_DISC_45";
  this->VariableNames->value[7746] = "PB_DISC_46";
  this->VariableNames->value[7747] = "PB_DISC_47";
  this->VariableNames->value[7748] = "PB_DISC_48";
  this->VariableNames->value[7749] = "PB_DISC_49";

  this->VariableNames->value[7750] = "PB_DISC_M1_00";
  this->VariableNames->value[7751] = "PB_DISC_M1_01";
  this->VariableNames->value[7752] = "PB_DISC_M1_02";
  this->VariableNames->value[7753] = "PB_DISC_M1_03";
  this->VariableNames->value[7754] = "PB_DISC_M1_04";
  this->VariableNames->value[7755] = "PB_DISC_M1_05";
  this->VariableNames->value[7756] = "PB_DISC_M1_06";
  this->VariableNames->value[7757] = "PB_DISC_M1_07";
  this->VariableNames->value[7758] = "PB_DISC_M1_08";
  this->VariableNames->value[7759] = "PB_DISC_M1_09";
  this->VariableNames->value[7760] = "PB_DISC_M1_10";
  this->VariableNames->value[7761] = "PB_DISC_M1_11";
  this->VariableNames->value[7762] = "PB_DISC_M1_12";
  this->VariableNames->value[7763] = "PB_DISC_M1_13";
  this->VariableNames->value[7764] = "PB_DISC_M1_14";
  this->VariableNames->value[7765] = "PB_DISC_M1_15";
  this->VariableNames->value[7766] = "PB_DISC_M1_16";
  this->VariableNames->value[7767] = "PB_DISC_M1_17";
  this->VariableNames->value[7768] = "PB_DISC_M1_18";
  this->VariableNames->value[7769] = "PB_DISC_M1_19";
  this->VariableNames->value[7770] = "PB_DISC_M1_20";
  this->VariableNames->value[7771] = "PB_DISC_M1_21";
  this->VariableNames->value[7772] = "PB_DISC_M1_22";
  this->VariableNames->value[7773] = "PB_DISC_M1_23";
  this->VariableNames->value[7774] = "PB_DISC_M1_24";
  this->VariableNames->value[7775] = "PB_DISC_M1_25";
  this->VariableNames->value[7776] = "PB_DISC_M1_26";
  this->VariableNames->value[7777] = "PB_DISC_M1_27";
  this->VariableNames->value[7778] = "PB_DISC_M1_28";
  this->VariableNames->value[7779] = "PB_DISC_M1_29";
  this->VariableNames->value[7780] = "PB_DISC_M1_30";
  this->VariableNames->value[7781] = "PB_DISC_M1_31";
  this->VariableNames->value[7782] = "PB_DISC_M1_32";
  this->VariableNames->value[7783] = "PB_DISC_M1_33";
  this->VariableNames->value[7784] = "PB_DISC_M1_34";
  this->VariableNames->value[7785] = "PB_DISC_M1_35";
  this->VariableNames->value[7786] = "PB_DISC_M1_36";
  this->VariableNames->value[7787] = "PB_DISC_M1_37";
  this->VariableNames->value[7788] = "PB_DISC_M1_38";
  this->VariableNames->value[7789] = "PB_DISC_M1_39";
  this->VariableNames->value[7790] = "PB_DISC_M1_40";
  this->VariableNames->value[7791] = "PB_DISC_M1_41";
  this->VariableNames->value[7792] = "PB_DISC_M1_42";
  this->VariableNames->value[7793] = "PB_DISC_M1_43";
  this->VariableNames->value[7794] = "PB_DISC_M1_44";
  this->VariableNames->value[7795] = "PB_DISC_M1_45";
  this->VariableNames->value[7796] = "PB_DISC_M1_46";
  this->VariableNames->value[7797] = "PB_DISC_M1_47";
  this->VariableNames->value[7798] = "PB_DISC_M1_48";
  this->VariableNames->value[7799] = "PB_DISC_M1_49";

  this->VariableNames->value[7800] = "PB_DISC_M2_00";
  this->VariableNames->value[7801] = "PB_DISC_M2_01";
  this->VariableNames->value[7802] = "PB_DISC_M2_02";
  this->VariableNames->value[7803] = "PB_DISC_M2_03";
  this->VariableNames->value[7804] = "PB_DISC_M2_04";
  this->VariableNames->value[7805] = "PB_DISC_M2_05";
  this->VariableNames->value[7806] = "PB_DISC_M2_06";
  this->VariableNames->value[7807] = "PB_DISC_M2_07";
  this->VariableNames->value[7808] = "PB_DISC_M2_08";
  this->VariableNames->value[7809] = "PB_DISC_M2_09";
  this->VariableNames->value[7810] = "PB_DISC_M2_10";
  this->VariableNames->value[7811] = "PB_DISC_M2_11";
  this->VariableNames->value[7812] = "PB_DISC_M2_12";
  this->VariableNames->value[7813] = "PB_DISC_M2_13";
  this->VariableNames->value[7814] = "PB_DISC_M2_14";
  this->VariableNames->value[7815] = "PB_DISC_M2_15";
  this->VariableNames->value[7816] = "PB_DISC_M2_16";
  this->VariableNames->value[7817] = "PB_DISC_M2_17";
  this->VariableNames->value[7818] = "PB_DISC_M2_18";
  this->VariableNames->value[7819] = "PB_DISC_M2_19";
  this->VariableNames->value[7820] = "PB_DISC_M2_20";
  this->VariableNames->value[7821] = "PB_DISC_M2_21";
  this->VariableNames->value[7822] = "PB_DISC_M2_22";
  this->VariableNames->value[7823] = "PB_DISC_M2_23";
  this->VariableNames->value[7824] = "PB_DISC_M2_24";
  this->VariableNames->value[7825] = "PB_DISC_M2_25";
  this->VariableNames->value[7826] = "PB_DISC_M2_26";
  this->VariableNames->value[7827] = "PB_DISC_M2_27";
  this->VariableNames->value[7828] = "PB_DISC_M2_28";
  this->VariableNames->value[7829] = "PB_DISC_M2_29";
  this->VariableNames->value[7830] = "PB_DISC_M2_30";
  this->VariableNames->value[7831] = "PB_DISC_M2_31";
  this->VariableNames->value[7832] = "PB_DISC_M2_32";
  this->VariableNames->value[7833] = "PB_DISC_M2_33";
  this->VariableNames->value[7834] = "PB_DISC_M2_34";
  this->VariableNames->value[7835] = "PB_DISC_M2_35";
  this->VariableNames->value[7836] = "PB_DISC_M2_36";
  this->VariableNames->value[7837] = "PB_DISC_M2_37";
  this->VariableNames->value[7838] = "PB_DISC_M2_38";
  this->VariableNames->value[7839] = "PB_DISC_M2_39";
  this->VariableNames->value[7840] = "PB_DISC_M2_40";
  this->VariableNames->value[7841] = "PB_DISC_M2_41";
  this->VariableNames->value[7842] = "PB_DISC_M2_42";
  this->VariableNames->value[7843] = "PB_DISC_M2_43";
  this->VariableNames->value[7844] = "PB_DISC_M2_44";
  this->VariableNames->value[7845] = "PB_DISC_M2_45";
  this->VariableNames->value[7846] = "PB_DISC_M2_46";
  this->VariableNames->value[7847] = "PB_DISC_M2_47";
  this->VariableNames->value[7848] = "PB_DISC_M2_48";
  this->VariableNames->value[7849] = "PB_DISC_M2_49";

  this->VariableNames->value[7850] = "PB_QMOM_00";
  this->VariableNames->value[7851] = "PB_QMOM_01";
  this->VariableNames->value[7852] = "PB_QMOM_02";
  this->VariableNames->value[7853] = "PB_QMOM_03";
  this->VariableNames->value[7854] = "PB_QMOM_04";
  this->VariableNames->value[7855] = "PB_QMOM_05";
  this->VariableNames->value[7856] = "PB_QMOM_06";
  this->VariableNames->value[7857] = "PB_QMOM_07";
  this->VariableNames->value[7858] = "PB_QMOM_08";
  this->VariableNames->value[7859] = "PB_QMOM_09";
  this->VariableNames->value[7860] = "PB_QMOM_10";
  this->VariableNames->value[7861] = "PB_QMOM_11";
  this->VariableNames->value[7862] = "PB_QMOM_12";
  this->VariableNames->value[7863] = "PB_QMOM_13";
  this->VariableNames->value[7864] = "PB_QMOM_14";
  this->VariableNames->value[7865] = "PB_QMOM_15";
  this->VariableNames->value[7866] = "PB_QMOM_16";
  this->VariableNames->value[7867] = "PB_QMOM_17";
  this->VariableNames->value[7868] = "PB_QMOM_18";
  this->VariableNames->value[7869] = "PB_QMOM_19";
  this->VariableNames->value[7870] = "PB_QMOM_20";
  this->VariableNames->value[7871] = "PB_QMOM_21";
  this->VariableNames->value[7872] = "PB_QMOM_22";
  this->VariableNames->value[7873] = "PB_QMOM_23";
  this->VariableNames->value[7874] = "PB_QMOM_24";
  this->VariableNames->value[7875] = "PB_QMOM_25";
  this->VariableNames->value[7876] = "PB_QMOM_26";
  this->VariableNames->value[7877] = "PB_QMOM_27";
  this->VariableNames->value[7878] = "PB_QMOM_28";
  this->VariableNames->value[7879] = "PB_QMOM_29";
  this->VariableNames->value[7880] = "PB_QMOM_30";
  this->VariableNames->value[7881] = "PB_QMOM_31";
  this->VariableNames->value[7882] = "PB_QMOM_32";
  this->VariableNames->value[7883] = "PB_QMOM_33";
  this->VariableNames->value[7884] = "PB_QMOM_34";
  this->VariableNames->value[7885] = "PB_QMOM_35";
  this->VariableNames->value[7886] = "PB_QMOM_36";
  this->VariableNames->value[7887] = "PB_QMOM_37";
  this->VariableNames->value[7888] = "PB_QMOM_38";
  this->VariableNames->value[7889] = "PB_QMOM_39";
  this->VariableNames->value[7890] = "PB_QMOM_40";
  this->VariableNames->value[7891] = "PB_QMOM_41";
  this->VariableNames->value[7892] = "PB_QMOM_42";
  this->VariableNames->value[7893] = "PB_QMOM_43";
  this->VariableNames->value[7894] = "PB_QMOM_44";
  this->VariableNames->value[7895] = "PB_QMOM_45";
  this->VariableNames->value[7896] = "PB_QMOM_46";
  this->VariableNames->value[7897] = "PB_QMOM_47";
  this->VariableNames->value[7898] = "PB_QMOM_48";
  this->VariableNames->value[7899] = "PB_QMOM_49";

  this->VariableNames->value[7900] = "PB_QMOM_M1_00";
  this->VariableNames->value[7901] = "PB_QMOM_M1_01";
  this->VariableNames->value[7902] = "PB_QMOM_M1_02";
  this->VariableNames->value[7903] = "PB_QMOM_M1_03";
  this->VariableNames->value[7904] = "PB_QMOM_M1_04";
  this->VariableNames->value[7905] = "PB_QMOM_M1_05";
  this->VariableNames->value[7906] = "PB_QMOM_M1_06";
  this->VariableNames->value[7907] = "PB_QMOM_M1_07";
  this->VariableNames->value[7908] = "PB_QMOM_M1_08";
  this->VariableNames->value[7909] = "PB_QMOM_M1_09";
  this->VariableNames->value[7910] = "PB_QMOM_M1_10";
  this->VariableNames->value[7911] = "PB_QMOM_M1_11";
  this->VariableNames->value[7912] = "PB_QMOM_M1_12";
  this->VariableNames->value[7913] = "PB_QMOM_M1_13";
  this->VariableNames->value[7914] = "PB_QMOM_M1_14";
  this->VariableNames->value[7915] = "PB_QMOM_M1_15";
  this->VariableNames->value[7916] = "PB_QMOM_M1_16";
  this->VariableNames->value[7917] = "PB_QMOM_M1_17";
  this->VariableNames->value[7918] = "PB_QMOM_M1_18";
  this->VariableNames->value[7919] = "PB_QMOM_M1_19";
  this->VariableNames->value[7920] = "PB_QMOM_M1_20";
  this->VariableNames->value[7921] = "PB_QMOM_M1_21";
  this->VariableNames->value[7922] = "PB_QMOM_M1_22";
  this->VariableNames->value[7923] = "PB_QMOM_M1_23";
  this->VariableNames->value[7924] = "PB_QMOM_M1_24";
  this->VariableNames->value[7925] = "PB_QMOM_M1_25";
  this->VariableNames->value[7926] = "PB_QMOM_M1_26";
  this->VariableNames->value[7927] = "PB_QMOM_M1_27";
  this->VariableNames->value[7928] = "PB_QMOM_M1_28";
  this->VariableNames->value[7929] = "PB_QMOM_M1_29";
  this->VariableNames->value[7930] = "PB_QMOM_M1_30";
  this->VariableNames->value[7931] = "PB_QMOM_M1_31";
  this->VariableNames->value[7932] = "PB_QMOM_M1_32";
  this->VariableNames->value[7933] = "PB_QMOM_M1_33";
  this->VariableNames->value[7934] = "PB_QMOM_M1_34";
  this->VariableNames->value[7935] = "PB_QMOM_M1_35";
  this->VariableNames->value[7936] = "PB_QMOM_M1_36";
  this->VariableNames->value[7937] = "PB_QMOM_M1_37";
  this->VariableNames->value[7938] = "PB_QMOM_M1_38";
  this->VariableNames->value[7939] = "PB_QMOM_M1_39";
  this->VariableNames->value[7940] = "PB_QMOM_M1_40";
  this->VariableNames->value[7941] = "PB_QMOM_M1_41";
  this->VariableNames->value[7942] = "PB_QMOM_M1_42";
  this->VariableNames->value[7943] = "PB_QMOM_M1_43";
  this->VariableNames->value[7944] = "PB_QMOM_M1_44";
  this->VariableNames->value[7945] = "PB_QMOM_M1_45";
  this->VariableNames->value[7946] = "PB_QMOM_M1_46";
  this->VariableNames->value[7947] = "PB_QMOM_M1_47";
  this->VariableNames->value[7948] = "PB_QMOM_M1_48";
  this->VariableNames->value[7949] = "PB_QMOM_M1_49";

  this->VariableNames->value[7950] = "PB_QMOM_M2_00";
  this->VariableNames->value[7951] = "PB_QMOM_M2_01";
  this->VariableNames->value[7952] = "PB_QMOM_M2_02";
  this->VariableNames->value[7953] = "PB_QMOM_M2_03";
  this->VariableNames->value[7954] = "PB_QMOM_M2_04";
  this->VariableNames->value[7955] = "PB_QMOM_M2_05";
  this->VariableNames->value[7956] = "PB_QMOM_M2_06";
  this->VariableNames->value[7957] = "PB_QMOM_M2_07";
  this->VariableNames->value[7958] = "PB_QMOM_M2_08";
  this->VariableNames->value[7959] = "PB_QMOM_M2_09";
  this->VariableNames->value[7960] = "PB_QMOM_M2_10";
  this->VariableNames->value[7961] = "PB_QMOM_M2_11";
  this->VariableNames->value[7962] = "PB_QMOM_M2_12";
  this->VariableNames->value[7963] = "PB_QMOM_M2_13";
  this->VariableNames->value[7964] = "PB_QMOM_M2_14";
  this->VariableNames->value[7965] = "PB_QMOM_M2_15";
  this->VariableNames->value[7966] = "PB_QMOM_M2_16";
  this->VariableNames->value[7967] = "PB_QMOM_M2_17";
  this->VariableNames->value[7968] = "PB_QMOM_M2_18";
  this->VariableNames->value[7969] = "PB_QMOM_M2_19";
  this->VariableNames->value[7970] = "PB_QMOM_M2_20";
  this->VariableNames->value[7971] = "PB_QMOM_M2_21";
  this->VariableNames->value[7972] = "PB_QMOM_M2_22";
  this->VariableNames->value[7973] = "PB_QMOM_M2_23";
  this->VariableNames->value[7974] = "PB_QMOM_M2_24";
  this->VariableNames->value[7975] = "PB_QMOM_M2_25";
  this->VariableNames->value[7976] = "PB_QMOM_M2_26";
  this->VariableNames->value[7977] = "PB_QMOM_M2_27";
  this->VariableNames->value[7978] = "PB_QMOM_M2_28";
  this->VariableNames->value[7979] = "PB_QMOM_M2_29";
  this->VariableNames->value[7980] = "PB_QMOM_M2_30";
  this->VariableNames->value[7981] = "PB_QMOM_M2_31";
  this->VariableNames->value[7982] = "PB_QMOM_M2_32";
  this->VariableNames->value[7983] = "PB_QMOM_M2_33";
  this->VariableNames->value[7984] = "PB_QMOM_M2_34";
  this->VariableNames->value[7985] = "PB_QMOM_M2_35";
  this->VariableNames->value[7986] = "PB_QMOM_M2_36";
  this->VariableNames->value[7987] = "PB_QMOM_M2_37";
  this->VariableNames->value[7988] = "PB_QMOM_M2_38";
  this->VariableNames->value[7989] = "PB_QMOM_M2_39";
  this->VariableNames->value[7990] = "PB_QMOM_M2_40";
  this->VariableNames->value[7991] = "PB_QMOM_M2_41";
  this->VariableNames->value[7992] = "PB_QMOM_M2_42";
  this->VariableNames->value[7993] = "PB_QMOM_M2_43";
  this->VariableNames->value[7994] = "PB_QMOM_M2_44";
  this->VariableNames->value[7995] = "PB_QMOM_M2_45";
  this->VariableNames->value[7996] = "PB_QMOM_M2_46";
  this->VariableNames->value[7997] = "PB_QMOM_M2_47";
  this->VariableNames->value[7998] = "PB_QMOM_M2_48";
  this->VariableNames->value[7999] = "PB_QMOM_M2_49";

  this->VariableNames->value[8000] = "PB_SMM_00";
  this->VariableNames->value[8001] = "PB_SMM_01";
  this->VariableNames->value[8002] = "PB_SMM_02";
  this->VariableNames->value[8003] = "PB_SMM_03";
  this->VariableNames->value[8004] = "PB_SMM_04";
  this->VariableNames->value[8005] = "PB_SMM_05";
  this->VariableNames->value[8006] = "PB_SMM_06";
  this->VariableNames->value[8007] = "PB_SMM_07";
  this->VariableNames->value[8008] = "PB_SMM_08";
  this->VariableNames->value[8009] = "PB_SMM_09";
  this->VariableNames->value[8010] = "PB_SMM_10";
  this->VariableNames->value[8011] = "PB_SMM_11";
  this->VariableNames->value[8012] = "PB_SMM_12";
  this->VariableNames->value[8013] = "PB_SMM_13";
  this->VariableNames->value[8014] = "PB_SMM_14";
  this->VariableNames->value[8015] = "PB_SMM_15";
  this->VariableNames->value[8016] = "PB_SMM_16";
  this->VariableNames->value[8017] = "PB_SMM_17";
  this->VariableNames->value[8018] = "PB_SMM_18";
  this->VariableNames->value[8019] = "PB_SMM_19";
  this->VariableNames->value[8020] = "PB_SMM_20";
  this->VariableNames->value[8021] = "PB_SMM_21";
  this->VariableNames->value[8022] = "PB_SMM_22";
  this->VariableNames->value[8023] = "PB_SMM_23";
  this->VariableNames->value[8024] = "PB_SMM_24";
  this->VariableNames->value[8025] = "PB_SMM_25";
  this->VariableNames->value[8026] = "PB_SMM_26";
  this->VariableNames->value[8027] = "PB_SMM_27";
  this->VariableNames->value[8028] = "PB_SMM_28";
  this->VariableNames->value[8029] = "PB_SMM_29";
  this->VariableNames->value[8030] = "PB_SMM_30";
  this->VariableNames->value[8031] = "PB_SMM_31";
  this->VariableNames->value[8032] = "PB_SMM_32";
  this->VariableNames->value[8033] = "PB_SMM_33";
  this->VariableNames->value[8034] = "PB_SMM_34";
  this->VariableNames->value[8035] = "PB_SMM_35";
  this->VariableNames->value[8036] = "PB_SMM_36";
  this->VariableNames->value[8037] = "PB_SMM_37";
  this->VariableNames->value[8038] = "PB_SMM_38";
  this->VariableNames->value[8039] = "PB_SMM_39";
  this->VariableNames->value[8040] = "PB_SMM_40";
  this->VariableNames->value[8041] = "PB_SMM_41";
  this->VariableNames->value[8042] = "PB_SMM_42";
  this->VariableNames->value[8043] = "PB_SMM_43";
  this->VariableNames->value[8044] = "PB_SMM_44";
  this->VariableNames->value[8045] = "PB_SMM_45";
  this->VariableNames->value[8046] = "PB_SMM_46";
  this->VariableNames->value[8047] = "PB_SMM_47";
  this->VariableNames->value[8048] = "PB_SMM_48";
  this->VariableNames->value[8049] = "PB_SMM_49";

  this->VariableNames->value[8050] = "PB_SMM_M1_00";
  this->VariableNames->value[8051] = "PB_SMM_M1_01";
  this->VariableNames->value[8052] = "PB_SMM_M1_02";
  this->VariableNames->value[8053] = "PB_SMM_M1_03";
  this->VariableNames->value[8054] = "PB_SMM_M1_04";
  this->VariableNames->value[8055] = "PB_SMM_M1_05";
  this->VariableNames->value[8056] = "PB_SMM_M1_06";
  this->VariableNames->value[8057] = "PB_SMM_M1_07";
  this->VariableNames->value[8058] = "PB_SMM_M1_08";
  this->VariableNames->value[8059] = "PB_SMM_M1_09";
  this->VariableNames->value[8060] = "PB_SMM_M1_10";
  this->VariableNames->value[8061] = "PB_SMM_M1_11";
  this->VariableNames->value[8062] = "PB_SMM_M1_12";
  this->VariableNames->value[8063] = "PB_SMM_M1_13";
  this->VariableNames->value[8064] = "PB_SMM_M1_14";
  this->VariableNames->value[8065] = "PB_SMM_M1_15";
  this->VariableNames->value[8066] = "PB_SMM_M1_16";
  this->VariableNames->value[8067] = "PB_SMM_M1_17";
  this->VariableNames->value[8068] = "PB_SMM_M1_18";
  this->VariableNames->value[8069] = "PB_SMM_M1_19";
  this->VariableNames->value[8070] = "PB_SMM_M1_20";
  this->VariableNames->value[8071] = "PB_SMM_M1_21";
  this->VariableNames->value[8072] = "PB_SMM_M1_22";
  this->VariableNames->value[8073] = "PB_SMM_M1_23";
  this->VariableNames->value[8074] = "PB_SMM_M1_24";
  this->VariableNames->value[8075] = "PB_SMM_M1_25";
  this->VariableNames->value[8076] = "PB_SMM_M1_26";
  this->VariableNames->value[8077] = "PB_SMM_M1_27";
  this->VariableNames->value[8078] = "PB_SMM_M1_28";
  this->VariableNames->value[8079] = "PB_SMM_M1_29";
  this->VariableNames->value[8080] = "PB_SMM_M1_30";
  this->VariableNames->value[8081] = "PB_SMM_M1_31";
  this->VariableNames->value[8082] = "PB_SMM_M1_32";
  this->VariableNames->value[8083] = "PB_SMM_M1_33";
  this->VariableNames->value[8084] = "PB_SMM_M1_34";
  this->VariableNames->value[8085] = "PB_SMM_M1_35";
  this->VariableNames->value[8086] = "PB_SMM_M1_36";
  this->VariableNames->value[8087] = "PB_SMM_M1_37";
  this->VariableNames->value[8088] = "PB_SMM_M1_38";
  this->VariableNames->value[8089] = "PB_SMM_M1_39";
  this->VariableNames->value[8090] = "PB_SMM_M1_40";
  this->VariableNames->value[8091] = "PB_SMM_M1_41";
  this->VariableNames->value[8092] = "PB_SMM_M1_42";
  this->VariableNames->value[8093] = "PB_SMM_M1_43";
  this->VariableNames->value[8094] = "PB_SMM_M1_44";
  this->VariableNames->value[8095] = "PB_SMM_M1_45";
  this->VariableNames->value[8096] = "PB_SMM_M1_46";
  this->VariableNames->value[8097] = "PB_SMM_M1_47";
  this->VariableNames->value[8098] = "PB_SMM_M1_48";
  this->VariableNames->value[8099] = "PB_SMM_M1_49";

  this->VariableNames->value[8100] = "PB_SMM_M2_00";
  this->VariableNames->value[8101] = "PB_SMM_M2_01";
  this->VariableNames->value[8102] = "PB_SMM_M2_02";
  this->VariableNames->value[8103] = "PB_SMM_M2_03";
  this->VariableNames->value[8104] = "PB_SMM_M2_04";
  this->VariableNames->value[8105] = "PB_SMM_M2_05";
  this->VariableNames->value[8106] = "PB_SMM_M2_06";
  this->VariableNames->value[8107] = "PB_SMM_M2_07";
  this->VariableNames->value[8108] = "PB_SMM_M2_08";
  this->VariableNames->value[8109] = "PB_SMM_M2_09";
  this->VariableNames->value[8110] = "PB_SMM_M2_10";
  this->VariableNames->value[8111] = "PB_SMM_M2_11";
  this->VariableNames->value[8112] = "PB_SMM_M2_12";
  this->VariableNames->value[8113] = "PB_SMM_M2_13";
  this->VariableNames->value[8114] = "PB_SMM_M2_14";
  this->VariableNames->value[8115] = "PB_SMM_M2_15";
  this->VariableNames->value[8116] = "PB_SMM_M2_16";
  this->VariableNames->value[8117] = "PB_SMM_M2_17";
  this->VariableNames->value[8118] = "PB_SMM_M2_18";
  this->VariableNames->value[8119] = "PB_SMM_M2_19";
  this->VariableNames->value[8120] = "PB_SMM_M2_20";
  this->VariableNames->value[8121] = "PB_SMM_M2_21";
  this->VariableNames->value[8122] = "PB_SMM_M2_22";
  this->VariableNames->value[8123] = "PB_SMM_M2_23";
  this->VariableNames->value[8124] = "PB_SMM_M2_24";
  this->VariableNames->value[8125] = "PB_SMM_M2_25";
  this->VariableNames->value[8126] = "PB_SMM_M2_26";
  this->VariableNames->value[8127] = "PB_SMM_M2_27";
  this->VariableNames->value[8128] = "PB_SMM_M2_28";
  this->VariableNames->value[8129] = "PB_SMM_M2_29";
  this->VariableNames->value[8130] = "PB_SMM_M2_30";
  this->VariableNames->value[8131] = "PB_SMM_M2_31";
  this->VariableNames->value[8132] = "PB_SMM_M2_32";
  this->VariableNames->value[8133] = "PB_SMM_M2_33";
  this->VariableNames->value[8134] = "PB_SMM_M2_34";
  this->VariableNames->value[8135] = "PB_SMM_M2_35";
  this->VariableNames->value[8136] = "PB_SMM_M2_36";
  this->VariableNames->value[8137] = "PB_SMM_M2_37";
  this->VariableNames->value[8138] = "PB_SMM_M2_38";
  this->VariableNames->value[8139] = "PB_SMM_M2_39";
  this->VariableNames->value[8140] = "PB_SMM_M2_40";
  this->VariableNames->value[8141] = "PB_SMM_M2_41";
  this->VariableNames->value[8142] = "PB_SMM_M2_42";
  this->VariableNames->value[8143] = "PB_SMM_M2_43";
  this->VariableNames->value[8144] = "PB_SMM_M2_44";
  this->VariableNames->value[8145] = "PB_SMM_M2_45";
  this->VariableNames->value[8146] = "PB_SMM_M2_46";
  this->VariableNames->value[8147] = "PB_SMM_M2_47";
  this->VariableNames->value[8148] = "PB_SMM_M2_48";
  this->VariableNames->value[8149] = "PB_SMM_M2_49";

}

//----------------------------------------------------------------------------
void vtkFLUENTReader::ParseCaseFile()
{
  this->FluentCaseFile->clear();
  this->FluentCaseFile->seekg (0, ios::beg);

  while (this->GetCaseChunk())
  {

    int index = this->GetCaseIndex();
    switch (index)
    {
      case 0:
        break;
      case 1:
        break;
      case 2:
        this->GridDimension = this->GetDimension();
        break;
      case 4:
        this->GetLittleEndianFlag();
        break;
      case 10:
        this->GetNodesAscii();
        break;
      case 12:
        this->GetCellsAscii();
        break;
      case 13:
        this->GetFacesAscii();
        break;
      case 18:
        this->GetPeriodicShadowFacesAscii();
        break;
      case 37:
        this->GetSpeciesVariableNames();
        break;
      case 38:
        break;
      case 39:
        break;
      case 40:
        break;
      case 41:
        break;
      case 45:
        break;
      case 58:
        this->GetCellTreeAscii();
        break;
      case 59:
        this->GetFaceTreeAscii();
        break;
      case 61:
        this->GetInterfaceFaceParentsAscii();
        break;
      case 62:
        this->GetNonconformalGridInterfaceFaceInformationAscii();
        break;
      case 63:
        break;
      case 64:
        break;
      case 2010:
        this->GetNodesSinglePrecision();
        break;
      case 3010:
        this->GetNodesDoublePrecision();
        break;
      case 2012:
        this->GetCellsBinary();
        break;
      case 3012:
        this->GetCellsBinary();  // Should be the same as single precision..
                           //only grabbing ints.
        break;
      case 2013:
        this->GetFacesBinary();
        break;
      case 3013:
        this->GetFacesBinary();
        break;
      case 2018:
        this->GetPeriodicShadowFacesBinary();
        break;
      case 3018:
        this->GetPeriodicShadowFacesBinary();
        break;
      case 2040:
        break;
      case 3040:
        break;
      case 2041:
        break;
      case 3041:
        break;
      case 2058:
        this->GetCellTreeBinary();
        break;
      case 3058:
        this->GetCellTreeBinary();
        break;
      case 2059:
        this->GetFaceTreeBinary();
        break;
      case 3059:
        this->GetFaceTreeBinary();
        break;
      case 2061:
        this->GetInterfaceFaceParentsBinary();
        break;
      case 3061:
        this->GetInterfaceFaceParentsBinary();
        break;
      case 2062:
        this->GetNonconformalGridInterfaceFaceInformationBinary();
        break;
      case 3062:
        this->GetNonconformalGridInterfaceFaceInformationBinary();
        break;
      case 2063:
        break;
      case 3063:
        break;
      default:
        //cout << "Undefined Section = " << index << endl;
        break;
    }
  }
}

//----------------------------------------------------------------------------
int vtkFLUENTReader::GetDimension()
{
  size_t start = this->CaseBuffer->value.find('(', 1);
  std::string info = this->CaseBuffer->value.substr(start+4, 1 );
  return atoi(info.c_str());
}

//----------------------------------------------------------------------------
void vtkFLUENTReader::GetLittleEndianFlag()
{
  size_t start = this->CaseBuffer->value.find('(', 1);
  size_t end = this->CaseBuffer->value.find(')',1);
  std::string info = this->CaseBuffer->value.substr(start+1,end-start-1 );
  int flag;
  sscanf(info.c_str(), "%d", &flag);

  if (flag == 60)
  {
    this->SetDataByteOrderToLittleEndian();
  }
  else
  {
    this->SetDataByteOrderToBigEndian();
  }
}

//----------------------------------------------------------------------------
void vtkFLUENTReader::GetNodesAscii()
{
  size_t start = this->CaseBuffer->value.find('(', 1);
  size_t end = this->CaseBuffer->value.find(')',1);
  std::string info = this->CaseBuffer->value.substr(start+1,end-start-1 );
  unsigned int zoneId, firstIndex, lastIndex;
  int type, nd;
  sscanf(info.c_str(), "%x %x %x %d %d", &zoneId, &firstIndex, &lastIndex,
                                         &type, &nd);

  if (this->CaseBuffer->value.at(5) == '0')
  {
    this->Points->Allocate(lastIndex);
  }
  else
  {
    size_t dstart = this->CaseBuffer->value.find('(', 5);
    size_t dend = this->CaseBuffer->value.find(')', dstart+1);
    std::string pdata = this->CaseBuffer->
                           value.substr(dstart+1, dend-start-1);
    std::stringstream pdatastream(pdata);

    double x, y, z;
    if (this->GridDimension == 3)
    {
      for (unsigned int i = firstIndex; i <= lastIndex; i++)
      {
        pdatastream >> x;
        pdatastream >> y;
        pdatastream >> z;
        this->Points->InsertPoint(i-1, x, y, z);
      }
    }
    else
    {
      for (unsigned int i = firstIndex; i <= lastIndex; i++)
      {
        pdatastream >> x;
        pdatastream >> y;
        this->Points->InsertPoint(i-1, x, y, 0.0);
      }
    }
  }
}

//----------------------------------------------------------------------------
void vtkFLUENTReader::GetNodesSinglePrecision()
{
  size_t start = this->CaseBuffer->value.find('(', 1);
  size_t end = this->CaseBuffer->value.find(')',1);
  std::string info = this->CaseBuffer->value.substr(start+1,end-start-1 );
  unsigned int zoneId, firstIndex, lastIndex;
  int type;
  sscanf(info.c_str(), "%x %x %x %d", &zoneId, &firstIndex, &lastIndex, &type);

  size_t dstart = this->CaseBuffer->value.find('(', 7);
  size_t ptr = dstart + 1;

  double x, y, z;
  if (this->GridDimension == 3)
  {
    for (unsigned int i = firstIndex; i <= lastIndex; i++)
    {
      x = this->GetCaseBufferFloat( static_cast< int >(ptr) );
      ptr = ptr + 4;

      y = this->GetCaseBufferFloat( static_cast< int >(ptr) );
      ptr = ptr + 4;

      z = this->GetCaseBufferFloat( static_cast< int >(ptr) );
      ptr = ptr + 4;
      this->Points->InsertPoint(i-1, x, y, z);
    }
  }
  else
  {
    for (unsigned int i = firstIndex; i <= lastIndex; i++)
    {
      x = this->GetCaseBufferFloat( static_cast< int >(ptr) );
      ptr = ptr + 4;

      y = this->GetCaseBufferFloat( static_cast< int >(ptr) );
      ptr = ptr + 4;

      z = 0.0;

      this->Points->InsertPoint(i-1, x, y, z);
    }
  }
}

//----------------------------------------------------------------------------
void vtkFLUENTReader::GetNodesDoublePrecision()
{
  size_t start = this->CaseBuffer->value.find('(', 1);
  size_t end = this->CaseBuffer->value.find(')',1);
  std::string info = this->CaseBuffer->value.substr(start+1,end-start-1 );
  unsigned int zoneId, firstIndex, lastIndex;
  int type;
  sscanf(info.c_str(), "%x %x %x %d", &zoneId, &firstIndex, &lastIndex, &type);

  size_t dstart = this->CaseBuffer->value.find('(', 7);
  size_t ptr = dstart+1;

  if (this->GridDimension == 3)
  {
    for (unsigned int i = firstIndex; i <= lastIndex; i++)
    {
      double x = this->GetCaseBufferDouble( static_cast< int >(ptr) );
      ptr = ptr + 8;

      double y = this->GetCaseBufferDouble( static_cast< int >(ptr) );
      ptr = ptr + 8;

      double z = this->GetCaseBufferDouble( static_cast< int >(ptr) );
      ptr = ptr + 8;
      this->Points->InsertPoint(i-1, x, y, z);
    }
  }
  else
  {
    for (unsigned int i = firstIndex; i <= lastIndex; i++)
    {
      double x = this->GetCaseBufferDouble( static_cast< int >(ptr) );
      ptr = ptr + 8;

      double y = this->GetCaseBufferDouble( static_cast< int >(ptr) );
      ptr = ptr + 8;

      this->Points->InsertPoint(i-1, x, y, 0.0);
    }
  }
}

//----------------------------------------------------------------------------
void vtkFLUENTReader::GetCellsAscii()
{
  if (this->CaseBuffer->value.at(5) == '0')
  { // Cell Info
    size_t start = this->CaseBuffer->value.find('(', 1);
    size_t end = this->CaseBuffer->value.find(')',1);
    std::string info = this->CaseBuffer->value.substr(start+1,end-start-1 );
    unsigned int zoneId, firstIndex, lastIndex;
    int type;
    sscanf(info.c_str(), "%x %x %x %d", &zoneId, &firstIndex, &lastIndex,
                                        &type);
    this->Cells->value.resize(lastIndex);
  }
  else
  { // Cell Definitions
    size_t start = this->CaseBuffer->value.find('(', 1);
    size_t end = this->CaseBuffer->value.find(')',1);
    std::string info = this->CaseBuffer->value.substr(start+1,end-start-1 );
    unsigned int zoneId, firstIndex, lastIndex;
    int type, elementType;
    sscanf(info.c_str(), "%x %x %x %d %d", &zoneId, &firstIndex, &lastIndex,
                                           &type, &elementType);

    if (elementType == 0)
    {
      size_t dstart = this->CaseBuffer->value.find('(', 5);
      size_t dend = this->CaseBuffer->value.find(')', dstart+1);
      std::string pdata = this->CaseBuffer->
                                   value.substr(dstart+1, dend-start-1);
      std::stringstream pdatastream(pdata);
      for (unsigned int i = firstIndex; i <= lastIndex; i++)
      {
        pdatastream >> this->Cells->value[i-1].type;
        this->Cells->value[i-1].zone = zoneId;
        this->Cells->value[i-1].parent = 0;
        this->Cells->value[i-1].child  = 0;
      }
    }
    else
    {
      for (unsigned int i = firstIndex; i <= lastIndex; i++)
      {
        this->Cells->value[i-1].type = elementType;
        this->Cells->value[i-1].zone = zoneId;
        this->Cells->value[i-1].parent = 0;
        this->Cells->value[i-1].child  = 0;
      }
    }
  }
}

//----------------------------------------------------------------------------
void vtkFLUENTReader::GetCellsBinary()
{
  size_t start = this->CaseBuffer->value.find('(', 1);
  size_t end = this->CaseBuffer->value.find(')',1);
  std::string info = this->CaseBuffer->value.substr(start+1,end-start-1 );
  unsigned int zoneId, firstIndex, lastIndex, type, elementType;
  sscanf(info.c_str(), "%x %x %x %x %x", &zoneId, &firstIndex, &lastIndex,
                                         &type, &elementType);

  if (elementType == 0)
  {
    size_t dstart = this->CaseBuffer->value.find('(', 7);
    size_t ptr = dstart + 1;
    for (unsigned int i = firstIndex; i <= lastIndex; i++)
    {
      this->Cells->value[i-1].type =
        this->GetCaseBufferInt( static_cast< int >(ptr) );
      ptr = ptr +4;
      this->Cells->value[i-1].zone = zoneId;
      this->Cells->value[i-1].parent = 0;
      this->Cells->value[i-1].child  = 0;
    }
  }
  else
  {
    for (unsigned int i = firstIndex; i <= lastIndex; i++)
    {
      this->Cells->value[i-1].type = elementType;
      this->Cells->value[i-1].zone = zoneId;
      this->Cells->value[i-1].parent = 0;
      this->Cells->value[i-1].child  = 0;
    }
  }
}

//----------------------------------------------------------------------------
void vtkFLUENTReader::GetFacesAscii()
{

  if (this->CaseBuffer->value.at(5) == '0')
  { // Face Info
    size_t start = this->CaseBuffer->value.find('(', 1);
    size_t end = this->CaseBuffer->value.find(')',1);
    std::string info = this->CaseBuffer->value.substr(start+1,end-start-1 );
    unsigned int zoneId, firstIndex, lastIndex, bcType;
    sscanf(info.c_str(), "%x %x %x %x", &zoneId, &firstIndex, &lastIndex,
                                        &bcType);

    this->Faces->value.resize(lastIndex);
  }
  else
  { // Face Definitions
    size_t start = this->CaseBuffer->value.find('(', 1);
    size_t end = this->CaseBuffer->value.find(')',1);
    std::string info = this->CaseBuffer->value.substr(start+1,end-start-1 );
    unsigned int zoneId, firstIndex, lastIndex, bcType, faceType;
    sscanf(info.c_str(), "%x %x %x %x %x", &zoneId, &firstIndex, &lastIndex,
                                           &bcType, &faceType);

    size_t dstart = this->CaseBuffer->value.find('(', 7);
    size_t dend = this->CaseBuffer->value.find(')', dstart+1);
    std::string pdata = this->CaseBuffer->
                                 value.substr(dstart+1, dend-start-1);
    std::stringstream pdatastream(pdata);

    int numberOfNodesInFace = 0;
    for (unsigned int i = firstIndex; i <= lastIndex; i++)
    {
      if (faceType == 0 || faceType == 5)
      {
        pdatastream >> numberOfNodesInFace;
      }
      else
      {
        numberOfNodesInFace = faceType;
      }
      this->Faces->value[i-1].nodes.resize(numberOfNodesInFace);
      for (int j = 0; j<numberOfNodesInFace; j++)
      {
        pdatastream >> hex >> this->Faces->value[i-1].nodes[j];
        this->Faces->value[i-1].nodes[j]--;
      }
      pdatastream >> hex >> this->Faces->value[i-1].c0;
      pdatastream >> hex >> this->Faces->value[i-1].c1;
      this->Faces->value[i-1].c0--;
      this->Faces->value[i-1].c1--;
      this->Faces->value[i-1].type = numberOfNodesInFace;
      this->Faces->value[i-1].zone = zoneId;
      this->Faces->value[i-1].periodicShadow = 0;
      this->Faces->value[i-1].parent = 0;
      this->Faces->value[i-1].child = 0;
      this->Faces->value[i-1].interfaceFaceParent = 0;
      this->Faces->value[i-1].ncgParent = 0;
      this->Faces->value[i-1].ncgChild = 0;
      this->Faces->value[i-1].interfaceFaceChild = 0;
      if (this->Faces->value[i-1].c0 >= 0)
      {
        this->Cells->value[this->Faces->value[i-1].c0].faces.push_back(i-1);
      }
      if (this->Faces->value[i-1].c1 >= 0)
      {
        this->Cells->value[this->Faces->value[i-1].c1].faces.push_back(i-1);
      }
    }
  }
}

//----------------------------------------------------------------------------
void vtkFLUENTReader::GetFacesBinary()
{
  size_t start = this->CaseBuffer->value.find('(', 1);
  size_t end = this->CaseBuffer->value.find(')',1);
  std::string info = this->CaseBuffer->value.substr(start+1,end-start-1 );
  unsigned int zoneId, firstIndex, lastIndex, bcType, faceType;
  sscanf(info.c_str(), "%x %x %x %x %x", &zoneId, &firstIndex, &lastIndex,
                                         &bcType, &faceType);
  size_t dstart = this->CaseBuffer->value.find('(', 7);
  int numberOfNodesInFace = 0;
  size_t ptr = dstart + 1;
  for (unsigned int i = firstIndex; i <= lastIndex; i++)
  {
    if ((faceType == 0) || (faceType == 5))
    {
      numberOfNodesInFace = this->GetCaseBufferInt( static_cast< int >(ptr) );
      ptr = ptr + 4;
    }
    else
    {
      numberOfNodesInFace = faceType;
    }

    this->Faces->value[i-1].nodes.resize(numberOfNodesInFace);

    for (int k = 0; k<numberOfNodesInFace; k++)
    {
      this->Faces->value[i-1].nodes[k] =
        this->GetCaseBufferInt(static_cast< int >(ptr));
      this->Faces->value[i-1].nodes[k]--;
      ptr = ptr + 4;
    }

    this->Faces->value[i-1].c0 =
      this->GetCaseBufferInt( static_cast< int >(ptr) );
    ptr = ptr + 4;
    this->Faces->value[i-1].c1 =
        this->GetCaseBufferInt( static_cast< int >(ptr) );
    ptr = ptr + 4;
    this->Faces->value[i-1].c0--;
    this->Faces->value[i-1].c1--;
    this->Faces->value[i-1].type = numberOfNodesInFace;
    this->Faces->value[i-1].zone = zoneId;
    this->Faces->value[i-1].periodicShadow = 0;
    this->Faces->value[i-1].parent = 0;
    this->Faces->value[i-1].child = 0;
    this->Faces->value[i-1].interfaceFaceParent = 0;
    this->Faces->value[i-1].ncgParent = 0;
    this->Faces->value[i-1].ncgChild = 0;
    this->Faces->value[i-1].interfaceFaceChild = 0;
    if (this->Faces->value[i-1].c0 >= 0)
    {
      this->Cells->value[this->Faces->value[i-1].c0].faces.push_back(i-1);
    }
    if (this->Faces->value[i-1].c1 >= 0)
    {
      this->Cells->value[this->Faces->value[i-1].c1].faces.push_back(i-1);
    }
  }
}

//----------------------------------------------------------------------------
void vtkFLUENTReader::GetPeriodicShadowFacesAscii()
{
  size_t start = this->CaseBuffer->value.find('(', 1);
  size_t end = this->CaseBuffer->value.find(')',1);
  std::string info = this->CaseBuffer->value.substr(start+1,end-start-1 );
  unsigned int firstIndex, lastIndex, periodicZone, shadowZone;
  sscanf(info.c_str(), "%x %x %x %x", &firstIndex, &lastIndex, &periodicZone,
                                      &shadowZone);

  size_t dstart = this->CaseBuffer->value.find('(', 7);
  size_t dend = this->CaseBuffer->value.find(')', dstart+1);
  std::string pdata = this->CaseBuffer->value.substr(dstart+1, dend-start-1);
  std::stringstream pdatastream(pdata);

  int faceIndex1, faceIndex2;
  for (unsigned int i = firstIndex; i <= lastIndex; i++)
  {
    pdatastream >> hex >> faceIndex1;
    pdatastream >> hex >> faceIndex2;
    this->Faces->value[faceIndex1].periodicShadow = 1;
  }
}

//----------------------------------------------------------------------------
void vtkFLUENTReader::GetPeriodicShadowFacesBinary()
{
  size_t start = this->CaseBuffer->value.find('(', 1);
  size_t end = this->CaseBuffer->value.find(')',1);
  std::string info = this->CaseBuffer->value.substr(start+1,end-start-1 );
  unsigned int firstIndex, lastIndex, periodicZone, shadowZone;
  sscanf(info.c_str(), "%x %x %x %x", &firstIndex, &lastIndex, &periodicZone,
                                      &shadowZone);

  size_t dstart = this->CaseBuffer->value.find('(', 7);
  size_t ptr = dstart + 1;

  //int faceIndex1, faceIndex2;
  for (unsigned int i = firstIndex; i <= lastIndex; i++)
  {
    //faceIndex1 = this->GetCaseBufferInt(ptr);
    this->GetCaseBufferInt( static_cast< int >(ptr) );
    ptr = ptr + 4;
    //faceIndex2 = this->GetCaseBufferInt(ptr);
    this->GetCaseBufferInt( static_cast< int >(ptr) );
    ptr = ptr + 4;
  }
}

//----------------------------------------------------------------------------
void vtkFLUENTReader::GetCellTreeAscii()
{
  size_t start = this->CaseBuffer->value.find('(', 1);
  size_t end = this->CaseBuffer->value.find(')',1);
  std::string info = this->CaseBuffer->value.substr(start+1,end-start-1 );
  unsigned int cellId0, cellId1, parentZoneId, childZoneId;
  sscanf(info.c_str(), "%x %x %x %x", &cellId0, &cellId1, &parentZoneId,
                                      &childZoneId);

  size_t dstart = this->CaseBuffer->value.find('(', 7);
  size_t dend = this->CaseBuffer->value.find(')', dstart+1);
  std::string pdata = this->CaseBuffer->value.substr(dstart+1, dend-start-1);
  std::stringstream pdatastream(pdata);

  int numberOfKids, kid;
  for (unsigned int i = cellId0; i <= cellId1; i++)
  {
    this->Cells->value[i-1].parent = 1;
    pdatastream >> hex >> numberOfKids;
    for (int j = 0; j < numberOfKids; j++)
    {
      pdatastream >> hex >> kid;
      this->Cells->value[kid-1].child = 1;
    }
  }
}

//----------------------------------------------------------------------------
void vtkFLUENTReader::GetCellTreeBinary()
{

  size_t start = this->CaseBuffer->value.find('(', 1);
  size_t end = this->CaseBuffer->value.find(')',1);
  std::string info = this->CaseBuffer->value.substr(start+1,end-start-1 );
  unsigned int cellId0, cellId1, parentZoneId, childZoneId;
  sscanf(info.c_str(), "%x %x %x %x", &cellId0, &cellId1, &parentZoneId,
                                      &childZoneId);

  size_t dstart = this->CaseBuffer->value.find('(', 7);
  size_t ptr = dstart + 1;

  int numberOfKids, kid;
  for (unsigned int i = cellId0; i <= cellId1; i++)
  {
    this->Cells->value[i-1].parent = 1;
    numberOfKids = this->GetCaseBufferInt( static_cast< int >(ptr) );
    ptr = ptr + 4;
    for (int j = 0; j < numberOfKids; j++)
    {
      kid = this->GetCaseBufferInt( static_cast< int >(ptr) );
      ptr = ptr + 4;
      this->Cells->value[kid-1].child = 1;
    }
  }
}

//----------------------------------------------------------------------------
void vtkFLUENTReader::GetFaceTreeAscii()
{
  size_t start = this->CaseBuffer->value.find('(', 1);
  size_t end = this->CaseBuffer->value.find(')',1);
  std::string info = this->CaseBuffer->value.substr(start+1,end-start-1 );
  unsigned int faceId0, faceId1, parentZoneId, childZoneId;
  sscanf(info.c_str(), "%x %x %x %x", &faceId0, &faceId1, &parentZoneId,
                                      &childZoneId);

  size_t dstart = this->CaseBuffer->value.find('(', 7);
  size_t dend = this->CaseBuffer->value.find(')', dstart+1);
  std::string pdata = this->CaseBuffer->value.substr(dstart+1, dend-start-1);
  std::stringstream pdatastream(pdata);

  int numberOfKids, kid;
  for (unsigned int i = faceId0; i <= faceId1; i++)
  {
    this->Faces->value[i-1].parent = 1;
    pdatastream >> hex >> numberOfKids;
    for (int j = 0; j < numberOfKids; j++)
    {
      pdatastream >> hex >> kid;
      this->Faces->value[kid-1].child = 1;
    }
  }
}

//----------------------------------------------------------------------------
void vtkFLUENTReader::GetFaceTreeBinary()
{
  size_t start = this->CaseBuffer->value.find('(', 1);
  size_t end = this->CaseBuffer->value.find(')',1);
  std::string info = this->CaseBuffer->value.substr(start+1,end-start-1 );
  unsigned int faceId0, faceId1, parentZoneId, childZoneId;
  sscanf(info.c_str(), "%x %x %x %x", &faceId0, &faceId1, &parentZoneId,
                                      &childZoneId);

  size_t dstart = this->CaseBuffer->value.find('(', 7);
  size_t ptr = dstart + 1;

  int numberOfKids, kid;
  for (unsigned int i = faceId0; i <= faceId1; i++)
  {
    this->Faces->value[i-1].parent = 1;
    numberOfKids = this->GetCaseBufferInt( static_cast< int >(ptr) );
    ptr = ptr + 4;
    for (int j = 0; j < numberOfKids; j++)
    {
      kid = this->GetCaseBufferInt( static_cast< int >(ptr) );
      ptr = ptr + 4;
      this->Faces->value[kid-1].child = 1;
    }
  }
}

//----------------------------------------------------------------------------
void vtkFLUENTReader::GetInterfaceFaceParentsAscii()
{
  size_t start = this->CaseBuffer->value.find('(', 1);
  size_t end = this->CaseBuffer->value.find(')',1);
  std::string info = this->CaseBuffer->value.substr(start+1,end-start-1 );
  unsigned int faceId0, faceId1;
  sscanf(info.c_str(), "%x %x", &faceId0, &faceId1);

  size_t dstart = this->CaseBuffer->value.find('(', 7);
  size_t dend = this->CaseBuffer->value.find(')', dstart+1);
  std::string pdata = this->CaseBuffer->value.substr(dstart+1, dend-start-1);
  std::stringstream pdatastream(pdata);

  int parentId0, parentId1;
  for (unsigned int i = faceId0; i <= faceId1; i++)
  {
    pdatastream >> hex >> parentId0;
    pdatastream >> hex >> parentId1;
    this->Faces->value[parentId0-1].interfaceFaceParent = 1;
    this->Faces->value[parentId1-1].interfaceFaceParent = 1;
    this->Faces->value[i-1].interfaceFaceChild = 1;
  }

}

//----------------------------------------------------------------------------
void vtkFLUENTReader::GetInterfaceFaceParentsBinary()
{
  size_t start = this->CaseBuffer->value.find('(', 1);
  size_t end = this->CaseBuffer->value.find(')',1);
  std::string info = this->CaseBuffer->value.substr(start+1,end-start-1 );
  unsigned int faceId0, faceId1;
  sscanf(info.c_str(), "%x %x", &faceId0, &faceId1);

  size_t dstart = this->CaseBuffer->value.find('(', 7);
  size_t ptr = dstart + 1;

  int parentId0, parentId1;
  for (unsigned int i = faceId0; i <= faceId1; i++)
  {
    parentId0 = this->GetCaseBufferInt( static_cast< int >(ptr) );
    ptr = ptr + 4;
    parentId1 = this->GetCaseBufferInt( static_cast< int >(ptr) );
    ptr = ptr + 4;
    this->Faces->value[parentId0-1].interfaceFaceParent = 1;
    this->Faces->value[parentId1-1].interfaceFaceParent = 1;
    this->Faces->value[i-1].interfaceFaceChild = 1;
  }

}

//----------------------------------------------------------------------------
void vtkFLUENTReader::GetNonconformalGridInterfaceFaceInformationAscii()
{
  size_t start = this->CaseBuffer->value.find('(', 1);
  size_t end = this->CaseBuffer->value.find(')',1);
  std::string info = this->CaseBuffer->value.substr(start+1,end-start-1 );
  int kidId, parentId, numberOfFaces;
  sscanf(info.c_str(), "%d %d %d", &kidId, &parentId, &numberOfFaces);

  size_t dstart = this->CaseBuffer->value.find('(', 7);
  size_t dend = this->CaseBuffer->value.find(')', dstart+1);
  std::string pdata = this->CaseBuffer->value.substr(dstart+1, dend-start-1);
  std::stringstream pdatastream(pdata);

  int child, parent;
  for (int i = 0; i < numberOfFaces; i++)
  {
    pdatastream >> hex >> child;
    pdatastream >> hex >> parent;
    this->Faces->value[child-1].ncgChild = 1;
    this->Faces->value[parent-1].ncgParent = 1;
  }

}

//----------------------------------------------------------------------------
void vtkFLUENTReader::GetNonconformalGridInterfaceFaceInformationBinary()
{
  size_t start = this->CaseBuffer->value.find('(', 1);
  size_t end = this->CaseBuffer->value.find(')',1);
  std::string info = CaseBuffer->value.substr(start+1,end-start-1 );
  int kidId, parentId, numberOfFaces;
  sscanf(info.c_str(), "%d %d %d", &kidId, &parentId, &numberOfFaces);

  size_t dstart = this->CaseBuffer->value.find('(', 7);
  size_t ptr = dstart + 1;

  int child, parent;
  for (int i = 0; i < numberOfFaces; i++)
  {
    child = this->GetCaseBufferInt( static_cast< int >(ptr) );
    ptr = ptr + 4;
    parent = this->GetCaseBufferInt( static_cast< int >(ptr) );
    ptr = ptr + 4;
    this->Faces->value[child-1].ncgChild = 1;
    this->Faces->value[parent-1].ncgParent = 1;
  }

}

//----------------------------------------------------------------------------
void vtkFLUENTReader::CleanCells()
{

  std::vector<int> t;
  for (int i = 0; i < (int)Cells->value.size(); i++)
  {

    if ( ((this->Cells->value[i].type == 1)&&
          (this->Cells->value[i].faces.size() != 3))||
         ((this->Cells->value[i].type == 2)&&
          (this->Cells->value[i].faces.size() != 4))||
         ((this->Cells->value[i].type == 3)&&
          (this->Cells->value[i].faces.size() != 4))||
         ((this->Cells->value[i].type == 4)&&
          (this->Cells->value[i].faces.size() != 6))||
         ((this->Cells->value[i].type == 5)&&
          (this->Cells->value[i].faces.size() != 5))||
         ((this->Cells->value[i].type == 6)&&
          (this->Cells->value[i].faces.size() != 5)) )
    {

      // Copy faces
      t.clear();
      for (int j = 0; j < (int)this->Cells->value[i].faces.size(); j++)
      {
        t.push_back(this->Cells->value[i].faces[j]);
      }

      // Clear Faces
      this->Cells->value[i].faces.clear();

      // Copy the faces that are not flagged back into the cell
      for (int j = 0; j < (int)t.size(); j++)
      {
        if ( (this->Faces->value[t[j]].child == 0 ) &&
             (this->Faces->value[t[j]].ncgChild == 0 ) &&
             (this->Faces->value[t[j]].interfaceFaceChild == 0 ))
        {
          this->Cells->value[i].faces.push_back(t[j]);
        }
      }
    }
  }
}

//----------------------------------------------------------------------------
void vtkFLUENTReader::PopulateCellNodes()
{
  for (int i = 0; i < (int)this->Cells->value.size(); i++)
  {
    switch (this->Cells->value[i].type)
    {
      case 1:  // Triangle
        this->PopulateTriangleCell(i);
        break;

      case 2:  // Tetrahedron
        this->PopulateTetraCell(i);
        break;

      case 3:  // Quadrilateral
        this->PopulateQuadCell(i);
        break;

      case 4:  // Hexahedral
        this->PopulateHexahedronCell(i);
        break;

      case 5:  // Pyramid
        this->PopulatePyramidCell(i);
        break;

      case 6:  // Wedge
        this->PopulateWedgeCell(i);
        break;

      case 7:  // Polyhedron
        this->PopulatePolyhedronCell(i);
        break;
    }
  }
}

//----------------------------------------------------------------------------
int vtkFLUENTReader::GetCaseBufferInt(int ptr)
{
  union mix_i
  {
    int i;
    char c[4];
  } mi = {1};

  for (int j = 0; j < 4; j++)
  {
    if (this->GetSwapBytes())
    {
      mi.c[3 - j] = this->CaseBuffer->value.at(ptr+j);
    }
    else
    {
      mi.c[j] = this->CaseBuffer->value.at(ptr+j);
    }
  }
  return mi.i;
}

//----------------------------------------------------------------------------
float vtkFLUENTReader::GetCaseBufferFloat(int ptr)
{
  union mix_f
  {
    float f;
    char c[4];
  } mf = {1.0};

  for (int j = 0; j < 4; j++)
  {
    if (this->GetSwapBytes())
    {
      mf.c[3 - j] = this->CaseBuffer->value.at(ptr+j);
    }
    else
    {
      mf.c[j] = this->CaseBuffer->value.at(ptr+j);
    }
  }
  return mf.f;
}

//----------------------------------------------------------------------------
double vtkFLUENTReader::GetCaseBufferDouble(int ptr)
{
  union mix_i
  {
    double d;
    char c[8];
  } md = {1.0};

  for (int j = 0; j < 8; j++)
  {
    if (this->GetSwapBytes())
    {
      md.c[7 - j] = this->CaseBuffer->value.at(ptr+j);
    }
    else
    {
      md.c[j] = this->CaseBuffer->value.at(ptr+j);
    }
  }
  return md.d;
}

//----------------------------------------------------------------------------
void vtkFLUENTReader::PopulateTriangleCell(int i)
{
  this->Cells->value[i].nodes.resize(3);
  if (this->Faces->value[this->Cells->value[i].faces[0]].c0 == i)
  {
    this->Cells->value[i].nodes[0] =
      this->Faces->value[this->Cells->value[i].faces[0]].nodes[0];
    this->Cells->value[i].nodes[1] =
      this->Faces->value[this->Cells->value[i].faces[0]].nodes[1];
  }
  else
  {
    this->Cells->value[i].nodes[1] =
      this->Faces->value[this->Cells->value[i].faces[0]].nodes[0];
    this->Cells->value[i].nodes[0] =
      this->Faces->value[this->Cells->value[i].faces[0]].nodes[1];
  }

  if (this->Faces->value[this->Cells->value[i].faces[1]].nodes[0]!=
      this->Cells->value[i].nodes[0]
      && this->Faces->value[this->Cells->value[i].faces[1]].nodes[0]!=
         this->Cells->value[i].nodes[1])
  {
    this->Cells->value[i].nodes[2] =
      this->Faces->value[this->Cells->value[i].faces[1]].nodes[0];
  }
  else
  {
    this->Cells->value[i].nodes[2] =
      this->Faces->value[this->Cells->value[i].faces[1]].nodes[1];
  }
}

//----------------------------------------------------------------------------
void vtkFLUENTReader::PopulateTetraCell(int i)
{
  this->Cells->value[i].nodes.resize(4);

  if (this->Faces->value[this->Cells->value[i].faces[0]].c0 == i)
  {
    this->Cells->value[i].nodes[0] =
      this->Faces->value[this->Cells->value[i].faces[0]].nodes[0];
    this->Cells->value[i].nodes[1] =
      this->Faces->value[this->Cells->value[i].faces[0]].nodes[1];
    this->Cells->value[i].nodes[2] =
      this->Faces->value[this->Cells->value[i].faces[0]].nodes[2];
  }
  else
  {
    this->Cells->value[i].nodes[2] =
      this->Faces->value[this->Cells->value[i].faces[0]].nodes[0];
    this->Cells->value[i].nodes[1] =
      this->Faces->value[this->Cells->value[i].faces[0]].nodes[1];
    this->Cells->value[i].nodes[0] =
      this->Faces->value[this->Cells->value[i].faces[0]].nodes[2];
  }

  if (this->Faces->value[this->Cells->value[i].faces[1]].nodes[0]!=
        this->Cells->value[i].nodes[0]
      && this->Faces->value[this->Cells->value[i].faces[1]].nodes[0]!=
         this->Cells->value[i].nodes[1]
      && this->Faces->value[this->Cells->value[i].faces[1]].nodes[0] !=
         this->Cells->value[i].nodes[2] )
  {
    this->Cells->value[i].nodes[3] =
      this->Faces->value[this->Cells->value[i].faces[1]].nodes[0];
  }
  else if (this->Faces->value[this->Cells->value[i].faces[1]].nodes[1] !=
              this->Cells->value[i].nodes[0] &&
            this->Faces->value[this->Cells->value[i].faces[1]].nodes[1] !=
              this->Cells->value[i].nodes[1] &&
            this->Faces->value[this->Cells->value[i].faces[1]].nodes[1] !=
              this->Cells->value[i].nodes[2])
  {
    this->Cells->value[i].nodes[3] =
      this->Faces->value[this->Cells->value[i].faces[1]].nodes[1];
  }
  else
  {
    this->Cells->value[i].nodes[3] =
      this->Faces->value[this->Cells->value[i].faces[1]].nodes[2];
  }
}

//----------------------------------------------------------------------------
void vtkFLUENTReader::PopulateQuadCell(int i)
{
  this->Cells->value[i].nodes.resize(4);

  if (this->Faces->value[this->Cells->value[i].faces[0]].c0 == i)
  {
    this->Cells->value[i].nodes[0] =
      this->Faces->value[this->Cells->value[i].faces[0]].nodes[0];
    this->Cells->value[i].nodes[1] =
      this->Faces->value[this->Cells->value[i].faces[0]].nodes[1];
  }
  else
  {
    this->Cells->value[i].nodes[1] =
      this->Faces->value[this->Cells->value[i].faces[0]].nodes[0];
    this->Cells->value[i].nodes[0] =
      this->Faces->value[this->Cells->value[i].faces[0]].nodes[1];
  }

  if ((this->Faces->value[this->Cells->value[i].faces[1]].nodes[0] !=
         this->Cells->value[i].nodes[0] &&
       this->Faces->value[this->Cells->value[i].faces[1]].nodes[0] !=
         this->Cells->value[i].nodes[1]) &&
       (this->Faces->value[this->Cells->value[i].faces[1]].nodes[1] !=
         this->Cells->value[i].nodes[0] &&
       this->Faces->value[this->Cells->value[i].faces[1]].nodes[1] !=
         this->Cells->value[i].nodes[1]))
  {
    if (this->Faces->value[this->Cells->value[i].faces[1]].c0 == i)
    {
      this->Cells->value[i].nodes[2] =
        this->Faces->value[this->Cells->value[i].faces[1]].nodes[0];
      this->Cells->value[i].nodes[3] =
        this->Faces->value[this->Cells->value[i].faces[1]].nodes[1];
    }
    else
    {
      this->Cells->value[i].nodes[3] =
        this->Faces->value[this->Cells->value[i].faces[1]].nodes[0];
      this->Cells->value[i].nodes[2] =
        this->Faces->value[this->Cells->value[i].faces[1]].nodes[1];
    }
  }
  else if ((this->Faces->value[this->Cells->value[i].faces[2]].nodes[0] !=
              this->Cells->value[i].nodes[0] &&
            this->Faces->value[this->Cells->value[i].faces[2]].nodes[0] !=
             this->Cells->value[i].nodes[1]) &&
            (this->Faces->value[this->Cells->value[i].faces[2]].nodes[1] !=
              this->Cells->value[i].nodes[0] &&
            this->Faces->value[this->Cells->value[i].faces[2]].nodes[1] !=
              this->Cells->value[i].nodes[1]))
  {
    if (this->Faces->value[this->Cells->value[i].faces[2]].c0 == i)
    {
      this->Cells->value[i].nodes[2] =
        this->Faces->value[this->Cells->value[i].faces[2]].nodes[0];
      this->Cells->value[i].nodes[3] =
        this->Faces->value[this->Cells->value[i].faces[2]].nodes[1];
    }
    else
    {
      this->Cells->value[i].nodes[3] =
        this->Faces->value[this->Cells->value[i].faces[2]].nodes[0];
      this->Cells->value[i].nodes[2] =
        this->Faces->value[this->Cells->value[i].faces[2]].nodes[1];
    }
  }
  else
  {
    if (this->Faces->value[this->Cells->value[i].faces[3]].c0 == i)
    {
      this->Cells->value[i].nodes[2] =
        this->Faces->value[this->Cells->value[i].faces[3]].nodes[0];
      this->Cells->value[i].nodes[3] =
        this->Faces->value[this->Cells->value[i].faces[3]].nodes[1];
    }
    else
    {
      this->Cells->value[i].nodes[3] =
        this->Faces->value[this->Cells->value[i].faces[3]].nodes[0];
      this->Cells->value[i].nodes[2] =
        this->Faces->value[this->Cells->value[i].faces[3]].nodes[1];
    }
  }
}

//----------------------------------------------------------------------------
void vtkFLUENTReader::PopulateHexahedronCell(int i)
{
  this->Cells->value[i].nodes.resize(8);

  if (this->Faces->value[this->Cells->value[i].faces[0]].c0 == i)
  {
    for (int j = 0; j < 4; j++)
    {
      this->Cells->value[i].nodes[j] =
        this->Faces->value[this->Cells->value[i].faces[0]].nodes[j];
    }
  }
  else
  {
    for (int j = 3; j >=0; j--)
    {
      this->Cells->value[i].nodes[3-j]=
        this->Faces->value[this->Cells->value[i].faces[0]].nodes[j];
    }
  }

  //  Look for opposite face of hexahedron
  for (int j = 1; j < 6; j++)
  {
    int flag = 0;
    for (int k = 0; k < 4; k++)
    {
      if ( (this->Cells->value[i].nodes[0] ==
              this->Faces->value[this->Cells->value[i].faces[j]].nodes[k]) ||
           (this->Cells->value[i].nodes[1] ==
              this->Faces->value[this->Cells->value[i].faces[j]].nodes[k]) ||
           (this->Cells->value[i].nodes[2] ==
              this->Faces->value[this->Cells->value[i].faces[j]].nodes[k]) ||
           (this->Cells->value[i].nodes[3] ==
              this->Faces->value[this->Cells->value[i].faces[j]].nodes[k]) )
      {
        flag = 1;
      }
    }
    if (flag == 0)
    {
      if (this->Faces->value[this->Cells->value[i].faces[j]].c1 == i)
      {
        for (int k = 4; k < 8; k++)
        {
          this->Cells->value[i].nodes[k] =
            this->Faces->value[this->Cells->value[i].faces[j]].nodes[k-4];
        }
      }
      else
      {
        for (int k = 7; k >= 4; k--)
        {
          this->Cells->value[i].nodes[k] =
            this->Faces->value[this->Cells->value[i].faces[j]].nodes[7-k];
        }
      }
    }
  }

  //  Find the face with points 0 and 1 in them.
  int f01[4] = {-1, -1, -1, -1};
  for (int j = 1; j < 6; j++)
  {
    int flag0 = 0;
    int flag1 = 0;
    for (int k = 0; k < 4; k++)
    {
      if (this->Cells->value[i].nodes[0] ==
          this->Faces->value[this->Cells->value[i].faces[j]].nodes[k])
      {
        flag0 = 1;
      }
      if (this->Cells->value[i].nodes[1] ==
          this->Faces->value[this->Cells->value[i].faces[j]].nodes[k])
      {
        flag1 = 1;
      }
    }
    if ((flag0 == 1) && (flag1 == 1))
    {
      if (this->Faces->value[this->Cells->value[i].faces[j]].c0 == i)
      {
        for (int k=0; k<4; k++)
        {
          f01[k] = this->Faces->value[this->Cells->value[i].faces[j]].nodes[k];
        }
      }
      else
      {
        for (int k=3; k>=0; k--)
        {
          f01[k] = this->Faces->value[this->Cells->value[i].faces[j]].nodes[k];
        }
      }
    }
  }

  //  Find the face with points 0 and 3 in them.
  int f03[4] =  {-1, -1, -1, -1};
  for (int j = 1; j < 6; j++)
  {
    int flag0 = 0;
    int flag1 = 0;
    for (int k = 0; k < 4; k++)
    {
      if (this->Cells->value[i].nodes[0] ==
          this->Faces->value[this->Cells->value[i].faces[j]].nodes[k])
      {
        flag0 = 1;
      }
      if (this->Cells->value[i].nodes[3] ==
          this->Faces->value[this->Cells->value[i].faces[j]].nodes[k])
      {
        flag1 = 1;
      }
    }

    if ((flag0 == 1) && (flag1 == 1))
    {
      if (this->Faces->value[this->Cells->value[i].faces[j]].c0 == i)
      {
        for (int k=0; k<4; k++)
        {
          f03[k] = this->Faces->value[this->Cells->value[i].faces[j]].nodes[k];
        }
      }
      else
      {
        for (int k=3; k>=0; k--)
        {
          f03[k] = this->Faces->value[this->Cells->value[i].faces[j]].nodes[k];
        }
      }
    }
  }

  // What point is in f01 and f03 besides 0 ... this is point 4
  int p4 = 0;
  for (int k = 0; k < 4; k++)
  {
    if ( f01[k] != this->Cells->value[i].nodes[0])
    {
      for (int n = 0; n < 4; n++)
      {
        if (f01[k] == f03[n])
        {
          p4 = f01[k];
        }
      }
    }
  }

  // Since we know point 4 now we check to see if points
  //  4, 5, 6, and 7 are in the correct positions.
  int t[8];
  t[4] = this->Cells->value[i].nodes[4];
  t[5] = this->Cells->value[i].nodes[5];
  t[6] = this->Cells->value[i].nodes[6];
  t[7] = this->Cells->value[i].nodes[7];
  if (p4 == this->Cells->value[i].nodes[5])
  {
    this->Cells->value[i].nodes[5] = t[6];
    this->Cells->value[i].nodes[6] = t[7];
    this->Cells->value[i].nodes[7] = t[4];
    this->Cells->value[i].nodes[4] = t[5];
  }
  else if (p4 == Cells->value[i].nodes[6])
  {
    this->Cells->value[i].nodes[5] = t[7];
    this->Cells->value[i].nodes[6] = t[4];
    this->Cells->value[i].nodes[7] = t[5];
    this->Cells->value[i].nodes[4] = t[6];
  }
  else if (p4 == Cells->value[i].nodes[7])
  {
    this->Cells->value[i].nodes[5] = t[4];
    this->Cells->value[i].nodes[6] = t[5];
    this->Cells->value[i].nodes[7] = t[6];
    this->Cells->value[i].nodes[4] = t[7];
  }
  // else point 4 was lined up so everything was correct.
}

//----------------------------------------------------------------------------
void vtkFLUENTReader::PopulatePyramidCell(int i)
{
  this->Cells->value[i].nodes.resize(5);
  //  The quad face will be the base of the pyramid
  for (int j = 0; j < (int)this->Cells->value[i].faces.size(); j++)
  {
    if ( this->Faces->value[this->Cells->value[i].faces[j]].nodes.size() == 4)
    {
      if (this->Faces->value[this->Cells->value[i].faces[j]].c0 == i)
      {
        for (int k = 0; k < 4; k++)
        {
          this->Cells->value[i].nodes[k] =
            this->Faces->value[this->Cells->value[i].faces[j]].nodes[k];
        }
      }
      else
      {
        for (int k = 0; k < 4; k++)
        {
          this->Cells->value[i].nodes[3-k] =
            this->Faces->value[this->Cells->value[i].faces[j]].nodes[k];
        }
      }
    }
  }

  // Just need to find point 4
  for (int j = 0; j < (int)this->Cells->value[i].faces.size(); j++)
  {
    if ( this->Faces->value[this->Cells->value[i].faces[j]].nodes.size() == 3)
    {
      for (int k = 0; k < 3; k ++)
      {
        if ( (this->Faces->value[this->Cells->value[i].faces[j]].nodes[k] !=
                this->Cells->value[i].nodes[0]) &&
             (this->Faces->value[this->Cells->value[i].faces[j]].nodes[k] !=
                this->Cells->value[i].nodes[1]) &&
             (this->Faces->value[this->Cells->value[i].faces[j]].nodes[k] !=
                this->Cells->value[i].nodes[2]) &&
             (this->Faces->value[this->Cells->value[i].faces[j]].nodes[k] !=
                this->Cells->value[i].nodes[3]) )
        {
          this->Cells->value[i].nodes[4] =
            this->Faces->value[this->Cells->value[i].faces[j]].nodes[k];
        }
      }
    }
  }

}

//----------------------------------------------------------------------------
void vtkFLUENTReader::PopulateWedgeCell(int i)
{
  this->Cells->value[i].nodes.resize(6);

  //  Find the first triangle face and make it the base.
  int base = 0;
  int first = 0;
  for (int j = 0; j < (int)this->Cells->value[i].faces.size(); j++)
  {
    if ((this->Faces->value[this->Cells->value[i].faces[j]].type == 3) &&
        (first == 0))
    {
      base = this->Cells->value[i].faces[j];
      first = 1;
    }
  }

  //  Find the second triangle face and make it the top.
  int top = 0;
  int second = 0;
  for (int j = 0; j < (int)this->Cells->value[i].faces.size(); j++)
  {
    if ((this->Faces->value[this->Cells->value[i].faces[j]].type == 3) &&
        (second == 0) && (this->Cells->value[i].faces[j] != base))
    {
      top = this->Cells->value[i].faces[j];
      second = 1;
    }
  }

  // Load Base nodes into the nodes std::vector
  if (this->Faces->value[base].c0 == i)
  {
    for (int j = 0; j < 3; j++)
    {
      this->Cells->value[i].nodes[j] = this->Faces->value[base].nodes[j];
    }
  }
  else
  {
    for (int j = 2; j >=0; j--)
    {
      this->Cells->value[i].nodes[2-j] = this->Faces->value[base].nodes[j];
    }
  }
  // Load Top nodes into the nodes std::vector
  if (this->Faces->value[top].c1 == i)
  {
    for (int j = 3; j < 6; j++)
    {
      this->Cells->value[i].nodes[j] = this->Faces->value[top].nodes[j-3];
    }
  }
  else
  {
    for (int j = 3; j < 6; j++)
    {
      this->Cells->value[i].nodes[j] = this->Faces->value[top].nodes[5-j];
    }
  }

  //  Find the quad face with points 0 and 1 in them.
  int w01[4] = {-1, -1, -1, -1};
  for (int j = 0; j < (int)this->Cells->value[i].faces.size(); j++)
  {
    if (this->Cells->value[i].faces[j] != base &&
      this->Cells->value[i].faces[j] != top)
    {
      int wf0 = 0;
      int wf1 = 0;
      for (int k = 0; k < 4; k++)
      {
        if (this->Cells->value[i].nodes[0] ==
              this->Faces->value[this->Cells->value[i].faces[j]].nodes[k])
        {
          wf0 = 1;
        }
        if (this->Cells->value[i].nodes[1] ==
              this->Faces->value[this->Cells->value[i].faces[j]].nodes[k])
        {
          wf1 = 1;
        }
        if ((wf0 == 1) && (wf1 == 1))
        {
          for (int n=0; n<4; n++)
          {
            w01[n]=this->Faces->value[this->Cells->value[i].faces[j]].nodes[n];
          }
        }
      }
    }
  }

  //  Find the quad face with points 0 and 2 in them.
  int w02[4] = {-1, -1, -1, -1};
  for (int j = 0; j < (int)this->Cells->value[i].faces.size(); j++)
  {
    if (this->Cells->value[i].faces[j] != base &&
      this->Cells->value[i].faces[j] != top)
    {
      int wf0 = 0;
      int wf2 = 0;
      for (int k = 0; k < 4; k++)
      {
        if (this->Cells->value[i].nodes[0] ==
              this->Faces->value[this->Cells->value[i].faces[j]].nodes[k])
        {
          wf0 = 1;
        }
        if (this->Cells->value[i].nodes[2] ==
              this->Faces->value[this->Cells->value[i].faces[j]].nodes[k])
        {
          wf2 = 1;
        }
        if ((wf0 == 1) && (wf2 == 1))
        {
          for (int n=0; n<4; n++)
          {
            w02[n]=this->Faces->value[this->Cells->value[i].faces[j]].nodes[n];
          }
        }
      }
    }
  }

  // Point 3 is the point that is in both w01 and w02

  // What point is in f01 and f02 besides 0 ... this is point 3
  int p3 = 0;
  for (int k = 0; k < 4; k++)
  {
    if ( w01[k] != this->Cells->value[i].nodes[0])
    {
      for (int n = 0; n < 4; n++)
      {
        if (w01[k] == w02[n])
        {
          p3 = w01[k];
        }
      }
    }
  }

  // Since we know point 3 now we check to see if points
  //  3, 4, and 5 are in the correct positions.
  int t[6];
  t[3] = this->Cells->value[i].nodes[3];
  t[4] = this->Cells->value[i].nodes[4];
  t[5] = this->Cells->value[i].nodes[5];
  if (p3 == this->Cells->value[i].nodes[4])
  {
    this->Cells->value[i].nodes[3] = t[4];
    this->Cells->value[i].nodes[4] = t[5];
    this->Cells->value[i].nodes[5] = t[3];
  }
  else if (p3 == this->Cells->value[i].nodes[5])
  {
    this->Cells->value[i].nodes[3] = t[5];
    this->Cells->value[i].nodes[4] = t[3];
    this->Cells->value[i].nodes[5] = t[4];
  }
  // else point 3 was lined up so everything was correct.

}

//----------------------------------------------------------------------------
void vtkFLUENTReader::PopulatePolyhedronCell(int i)
{
  //  We can't set the size on the nodes std::vector because we
  //  are not sure how many we are going to have.
  //  All we have to do here is add the nodes from the faces into
  //  nodes std::vector within the cell.  All we have to check for is
  //  duplicate nodes.
  //
  //cout << "number of faces in cell = " << Cells[i].faces.size() << endl;

  for (int j = 0; j < (int)this->Cells->value[i].faces.size(); j++)
  {
    //cout << "number of nodes in face = " <<
    //Faces[Cells[i].faces[j]].nodes.size() << endl;
    int k;
    for(k=0; k < (int)this->Faces->value[this->Cells->value[i].faces[j]].
                            nodes.size(); k++)
    {
      int flag;
      flag = 0;
      // Is the node already in the cell?
      for (int n = 0; n < (int)Cells->value[i].nodes.size(); n++)
      {
        if (this->Cells->value[i].nodes[n] ==
              this->Faces->value[this->Cells->value[i].faces[j]].nodes[k])
        {
          flag = 1;
        }
      }
      if (flag == 0)
      {
        //No match - insert node into cell.
        this->Cells->value[i].nodes.
              push_back(this->Faces->
                        value[this->Cells->value[i].faces[j]].nodes[k]);
      }
    }
  }
}

//----------------------------------------------------------------------------
void vtkFLUENTReader::ParseDataFile()
{
  while (this->GetDataChunk())
  {
    int index = this->GetDataIndex();
    switch (index)
    {
      case 0:
        //cout << "Comment Section" << endl;
        break;

      case 4:
        //cout << "Machine Configuration Section" << endl;
        break;

      case 33:
        //cout << "Grid Size Section" << endl;
        break;

      case 37:
        //cout << "Variables Section" << endl;
        break;

      case 300:
        //cout << "Data Section" << endl;
        GetData(1);
        break;

      case 301:
        //cout << "Residuals Section" << endl;
        break;

      case 302:
        //cout << "Residuals Section" << endl;
        break;

      case 2300:
        //cout << "Single Precision Data Section" << endl;
        GetData(2);
        break;

      case 2301:
        //cout << "Single Precision Residuals Section" << endl;
        break;

      case 2302:
        //cout << "Single Precision Residuals Section" << endl;
        break;

      case 3300:
        //cout << "Single Precision Data Section" << endl;
        GetData(3);
        break;

      case 3301:
        //cout << "Single Precision Residuals Section" << endl;
        break;

      case 3302:
        //cout << "Single Precision Residuals Section" << endl;
        break;

      default:
        //cout << "Data Undefined Section = " << index << endl;
        break;
    }
  }
}

//----------------------------------------------------------------------------
int vtkFLUENTReader::GetDataBufferInt(int ptr)
{
  union mix_i
  {
    int i;
    char c[4];
  } mi = {1};

  for (int j = 0; j < 4; j++)
  {
    if (this->GetSwapBytes())
    {
      mi.c[3 - j] = this->DataBuffer->value.at(ptr+j);
    }
    else
    {
      mi.c[j] = this->DataBuffer->value.at(ptr+j);
    }
  }
  return mi.i;
}

//----------------------------------------------------------------------------
float vtkFLUENTReader::GetDataBufferFloat(int ptr)
{
  union mix_f
  {
    float f;
    char c[4];
  } mf = {1.0};

  for (int j = 0; j < 4; j++)
  {
    if (this->GetSwapBytes())
    {
      mf.c[3 - j] = this->DataBuffer->value.at(ptr+j);
    }
    else
    {
      mf.c[j] = this->DataBuffer->value.at(ptr+j);
    }
  }
  return mf.f;
}

//----------------------------------------------------------------------------
double vtkFLUENTReader::GetDataBufferDouble(int ptr)
{
  union mix_i
  {
    double d;
    char c[8];
  } md = {1.0};

  for (int j = 0; j < 8; j++)
  {
    if (this->GetSwapBytes())
    {
      md.c[7 - j] = this->DataBuffer->value.at(ptr+j);
    }
    else
    {
      md.c[j] = this->DataBuffer->value.at(ptr+j);
    }
  }
  return md.d;
}

//------------------------------------------------------------------------------
void vtkFLUENTReader::GetData(int dataType)
{
  size_t start = this->DataBuffer->value.find('(', 1);
  size_t end = this->DataBuffer->value.find(')',1);
  std::string info = this->DataBuffer->value.substr(start+1,end-start-1 );
  std::stringstream infostream(info);
  int subSectionId, zoneId, size, nTimeLevels, nPhases, firstId, lastId;
  infostream >> subSectionId >> zoneId >> size >> nTimeLevels >> nPhases >>
                firstId >> lastId;

  // Is this a cell zone?
  int zmatch = 0;
  for (int i = 0; i < (int)this->CellZones->value.size(); i++)
  {
    if (this->CellZones->value[i] == zoneId)
    {
      zmatch = 1;
    }
  }

  if (zmatch)
  {

    // Set up stream or pointer to data
    size_t dstart = this->DataBuffer->value.find('(', 7);
    size_t dend = this->DataBuffer->value.find(')', dstart+1);
    std::string pdata = this->DataBuffer->
                           value.substr(dstart+1, dend-dstart-2);
    std::stringstream pdatastream(pdata);
    size_t ptr = dstart + 1;

    // Is this a new variable?
    int match = 0;
    for (int i = 0; i < (int)this->SubSectionIds->value.size(); i++)
    {
      if (subSectionId == this->SubSectionIds->value[i])
      {
        match = 1;
      }
    }

    if ((match == 0) && (size < 4))
    { // new variable
      this->SubSectionIds->value.push_back(subSectionId);
      this->SubSectionSize->value.push_back(size);
      this->SubSectionZones->
            value.resize(this->SubSectionZones->value.size()+1);
      this->SubSectionZones->
            value[this->SubSectionZones->value.size()-1].push_back(zoneId);
    }

    if (size == 1)
    {
      this->NumberOfScalars++;
      this->ScalarDataChunks->
        value.resize(this->ScalarDataChunks->value.size() + 1);
      this->ScalarDataChunks->
            value[this->ScalarDataChunks->value.size()-1].subsectionId =
              subSectionId;
      this->ScalarDataChunks->
            value[this->ScalarDataChunks->value.size()-1].zoneId = zoneId;
      for (int i=firstId; i<=lastId; i++)
      {
        double temp;
        if (dataType == 1)
        {
          pdatastream >> temp;
        }
        else if (dataType == 2)
        {
          temp = this->GetDataBufferFloat( static_cast< int >(ptr) );
          ptr = ptr + 4;
        }
        else
        {
          temp = this->GetDataBufferDouble( static_cast< int >(ptr) );
          ptr = ptr + 8;
        }
        this->ScalarDataChunks->value[this->ScalarDataChunks->value.size()-1].
                                scalarData.push_back(temp);
      }
    }
    else if (size == 3)
    {
      this->NumberOfVectors++;
      this->VectorDataChunks->
        value.resize(this->VectorDataChunks->value.size() + 1);
      this->VectorDataChunks->
            value[this->VectorDataChunks->value.size()-1].subsectionId =
              subSectionId;
      this->VectorDataChunks->
            value[this->VectorDataChunks->value.size()-1].zoneId = zoneId;
      for (int i=firstId; i<=lastId; i++)
      {
        double tempx, tempy, tempz;

        if (dataType == 1)
        {
          pdatastream >> tempx;
          pdatastream >> tempy;
          pdatastream >> tempz;
        }
        else if (dataType == 2)
        {
          tempx = this->GetDataBufferFloat( static_cast< int >(ptr) );
          ptr = ptr + 4;
          tempy = this->GetDataBufferFloat( static_cast< int >(ptr) );
          ptr = ptr + 4;
          tempz = this->GetDataBufferFloat( static_cast< int >(ptr) );
          ptr = ptr + 4;
        }
        else
        {
          tempx = this->GetDataBufferDouble( static_cast< int >(ptr) );
          ptr = ptr + 8;
          tempy = this->GetDataBufferDouble( static_cast< int >(ptr) );
          ptr = ptr + 8;
          tempz = this->GetDataBufferDouble( static_cast< int >(ptr) );
          ptr = ptr + 8;
        }
        this->VectorDataChunks->value[this->VectorDataChunks->value.size()-1].
                                iComponentData.push_back(tempx);
        this->VectorDataChunks->value[this->VectorDataChunks->value.size()-1].
                                jComponentData.push_back(tempy);
        this->VectorDataChunks->value[this->VectorDataChunks->value.size()-1].
                                kComponentData.push_back(tempz);
      }
    }
    else
    {
      //cout << "Weird Variable Size = " << size << endl;
    }
  }
}
//----------------------------------------------------------------------------
void vtkFLUENTReader::SetDataByteOrderToBigEndian()
{
#ifndef VTK_WORDS_BIGENDIAN
  this->SwapBytesOn();
#else
  this->SwapBytesOff();
#endif
}
//----------------------------------------------------------------------------
void vtkFLUENTReader::SetDataByteOrderToLittleEndian()
{
#ifdef VTK_WORDS_BIGENDIAN
  this->SwapBytesOn();
#else
  this->SwapBytesOff();
#endif
}
//----------------------------------------------------------------------------
void vtkFLUENTReader::SetDataByteOrder(int byteOrder)
{
  if ( byteOrder == VTK_FILE_BYTE_ORDER_BIG_ENDIAN )
  {
    this->SetDataByteOrderToBigEndian();
  }
  else
  {
    this->SetDataByteOrderToLittleEndian();
  }
}
//----------------------------------------------------------------------------
int vtkFLUENTReader::GetDataByteOrder()
{
#ifdef VTK_WORDS_BIGENDIAN
  if ( this->SwapBytes )
  {
    return VTK_FILE_BYTE_ORDER_LITTLE_ENDIAN;
  }
  else
  {
    return VTK_FILE_BYTE_ORDER_BIG_ENDIAN;
  }
#else
  if ( this->SwapBytes )
  {
    return VTK_FILE_BYTE_ORDER_BIG_ENDIAN;
  }
  else
  {
    return VTK_FILE_BYTE_ORDER_LITTLE_ENDIAN;
  }
#endif
}
//----------------------------------------------------------------------------
const char *vtkFLUENTReader::GetDataByteOrderAsString()
{
#ifdef VTK_WORDS_BIGENDIAN
  if ( this->SwapBytes )
  {
    return "LittleEndian";
  }
  else
  {
    return "BigEndian";
  }
#else
  if ( this->SwapBytes )
  {
    return "BigEndian";
  }
  else
  {
    return "LittleEndian";
  }
#endif
}
//------------------------------------------------------------------------------
void vtkFLUENTReader::GetSpeciesVariableNames()
{
    //Locate the "(species (names" entry
  std::string variables = this->CaseBuffer->value;
  size_t startPos = variables.find("(species (names (") +17;
  if (startPos != std::string::npos)
  {
    variables.erase( 0, startPos);

    size_t endPos = variables.find(")");
    variables.erase(endPos);

    std::stringstream tokenizer(variables);

    int iterator = 0;

    while ( !tokenizer.eof() )
    {
      std::string temp;
      tokenizer >> temp;

      this->VariableNames->value[200 + iterator] = temp;
      this->VariableNames->value[250 + iterator] = "M1_" + temp;
      this->VariableNames->value[300 + iterator] = "M2_" + temp;
      this->VariableNames->value[450 + iterator] = "DPMS_" + temp;
      this->VariableNames->value[850 + iterator] = "DPMS_DS_" + temp;
      this->VariableNames->value[1000 + iterator] = "MEAN_" + temp;
      this->VariableNames->value[1050 + iterator] = "RMS_" + temp;
      this->VariableNames->value[1250 + iterator] = "CREV_" + temp;

      iterator++;
    }
  }
}
