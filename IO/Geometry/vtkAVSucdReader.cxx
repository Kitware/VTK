/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAVSucdReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// Thanks to Guenole Harel and Emmanuel Colin (Supelec engineering school,
// France) and Jean M. Favre (CSCS, Switzerland) who co-developed this class.
// Thanks to Isabelle Surin (isabelle.surin at cea.fr, CEA-DAM, France) who
// supervised the internship of the first two authors.  Thanks to Daniel
// Aguilera (daniel.aguilera at cea.fr, CEA-DAM, France) who contributed code
// und advice.  Please address all comments to Jean Favre (jfavre at cscs.ch)

#include "vtkAVSucdReader.h"
#include "vtkType.h"
#include "vtkDataArraySelection.h"
#include "vtkErrorCode.h"
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

vtkStandardNewMacro(vtkAVSucdReader);

//----------------------------------------------------------------------------
vtkAVSucdReader::vtkAVSucdReader()
{
  this->FileName  = NULL;
  this->ByteOrder = FILE_BIG_ENDIAN;
  this->BinaryFile = 0;
  this->NumberOfNodeFields = 0;
  this->NumberOfCellFields = 0;
  this->NumberOfFields = 0;
  this->NumberOfNodeComponents = 0;
  this->NumberOfCellComponents = 0;
  this->FileStream = NULL;
  this->DecrementNodeIds = 0;
  this->NumberOfNodes = 0;
  this->NumberOfCells = 0;

  this->NodeDataInfo = NULL;
  this->CellDataInfo = NULL;
  this->PointDataArraySelection = vtkDataArraySelection::New();
  this->CellDataArraySelection = vtkDataArraySelection::New();

  this->SetNumberOfInputPorts(0);
}

//----------------------------------------------------------------------------
vtkAVSucdReader::~vtkAVSucdReader()
{
  delete [] this->FileName;
  delete [] this->NodeDataInfo;
  delete [] this->CellDataInfo;

  this->CellDataArraySelection->Delete();
  this->PointDataArraySelection->Delete();
}


//----------------------------------------------------------------------------
void vtkAVSucdReader::SetByteOrderToBigEndian()
{
  this->ByteOrder = FILE_BIG_ENDIAN;
}


//----------------------------------------------------------------------------
void vtkAVSucdReader::SetByteOrderToLittleEndian()
{
  this->ByteOrder = FILE_LITTLE_ENDIAN;
}

//----------------------------------------------------------------------------
const char *vtkAVSucdReader::GetByteOrderAsString()
{
  if ( this->ByteOrder ==  FILE_LITTLE_ENDIAN)
    {
    return "LittleEndian";
    }
  else
    {
    return "BigEndian";
    }
}

//----------------------------------------------------------------------------
int vtkAVSucdReader::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **vtkNotUsed(inputVector),
  vtkInformationVector *outputVector)
{
  // get the info object
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the ouptut
  vtkUnstructuredGrid *output = vtkUnstructuredGrid::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkDebugMacro( << "Reading AVS UCD file");

  // If ExecuteInformation() failed FileStream will be NULL and
  // ExecuteInformation() will have spit out an error.
  if ( this->FileStream )
    {
    this->ReadFile(output);
    }

  return 1;
}

//----------------------------------------------------------------------------
void vtkAVSucdReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "File Name: "
     << (this->FileName ? this->FileName : "(none)") << "\n";

  os << indent << "Number Of Nodes: " << this->NumberOfNodes << endl;
  os << indent << "Number Of Node Fields: "
     << this->NumberOfNodeFields << endl;
  os << indent << "Number Of Node Components: "
     << this->NumberOfNodeComponents << endl;

  os << indent << "Number Of Cells: " << this->NumberOfCells << endl;
  os << indent << "Number Of Cell Fields: "
     << this->NumberOfCellFields << endl;
  os << indent << "Number Of Cell Components: "
     << this->NumberOfCellComponents << endl;

  os << indent << "Byte Order: " << this->ByteOrder << endl;
  os << indent << "Binary File: " << (this->BinaryFile ? "True\n" : "False\n");
  os << indent << "Number of Fields: " << this->NumberOfFields << endl;
}


//----------------------------------------------------------------------------
void vtkAVSucdReader::ReadFile(vtkUnstructuredGrid *output)
{
  this->ReadGeometry(output);

  if(this->NumberOfNodeFields)
    {
    this->ReadNodeData(output);
    }

  if(this->NumberOfCellFields)
    {
    this->ReadCellData(output);
    }

  delete this->FileStream;
  this->FileStream = NULL;
}


//----------------------------------------------------------------------------
int vtkAVSucdReader::RequestInformation(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **vtkNotUsed(inputVector),
  vtkInformationVector *vtkNotUsed(outputVector))
{
  int i, j, k, *ncomp_list;

  // first open file in binary mode to check the first byte.
  if ( !this->FileName )
    {
    vtkErrorMacro("No filename specified");
    return 0;
    }

#ifdef _WIN32
    this->FileStream = new ifstream(this->FileName, ios::in | ios::binary);
#else
    this->FileStream = new ifstream(this->FileName, ios::in);
#endif
  if (this->FileStream->fail())
    {
    this->SetErrorCode(vtkErrorCode::FileNotFoundError);
    delete this->FileStream;
    this->FileStream = NULL;
    vtkErrorMacro("Specified filename not found");
    return 0;
    }

  char magic_number='\0';
  this->FileStream->get(magic_number);
  this->FileStream->putback(magic_number);
  if(magic_number != 7)
    { // most likely an ASCII file
    this->BinaryFile = 0;
    delete this->FileStream; // close file to reopen it later
    this->FileStream = NULL;

    this->FileStream = new ifstream(this->FileName, ios::in);
    char c='\0';
    while (!FileStream->eof())
      {
      // skip leading whitespace
      while(isspace(FileStream->peek()))
        {
        FileStream->get(c);
        }
      // skip comment lines
      if (FileStream->peek() == '#')
        {
        while(this->FileStream->get(c))
          {
          if (c == '\n')
            {
            break;
            }
          }
        }
      else
        {
        break;
        }
      }

    *(this->FileStream) >> this->NumberOfNodes;
    *(this->FileStream) >> this->NumberOfCells;
    *(this->FileStream) >> this->NumberOfNodeFields;
    *(this->FileStream) >> this->NumberOfCellFields;
    *(this->FileStream) >> this->NumberOfFields;
    }
  else
    {
    this->BinaryFile = 1;

    // Here we first need to check if the file is little-endian or big-endian
    // We will read the variables once, with the given endian-ness set up in
    // the class constructor. If trueFileLength does not match
    // calculatedFileLength, then we will toggle the endian-ness and re-swap
    // the variables. We try at most twice, since there are only two endian-nesses.
    this->FileStream->seekg(0L, ios::end);
    vtkTypeUInt64 trueFileLength = this->FileStream->tellg();
    vtkTypeUInt64 calculatedFileLength = 0; // not known yet

    unsigned int attempts = 0;
    while (attempts < 2)
      {
      // restart at beginning of file
      this->FileStream->seekg(0L, ios::beg);

      this->FileStream->read(&magic_number, 1);

      this->ReadIntBlock(1, &this->NumberOfNodes);
      this->ReadIntBlock(1, &this->NumberOfCells);
      this->ReadIntBlock(1, &this->NumberOfNodeFields);
      this->ReadIntBlock(1, &this->NumberOfCellFields);
      this->ReadIntBlock(1, &this->NumberOfFields);
      this->ReadIntBlock(1, &this->NlistNodes);

      vtkDebugMacro( << this->NumberOfNodes << " "
                     << this->NumberOfCells << " "
                     << this->NumberOfNodeFields << " "
                     << this->NumberOfCellFields << " "
                     << this->NumberOfFields << " "
                     << this->NlistNodes << endl);

      // If we've guessed the wrong endianness, these values will be nonsense,
      // and the arithmetic below could easily caused (undefined) signed overflow,
      // so convert everything into uint64.
      vtkTypeUInt64 numNodes = this->NumberOfNodes;
      vtkTypeUInt64 numCells = this->NumberOfCells;
      vtkTypeUInt64 numNodeFields = this->NumberOfNodeFields;
      vtkTypeUInt64 numCellFields = this->NumberOfCellFields;
      vtkTypeUInt64 numFields = this->NumberOfFields;
      vtkTypeUInt64 numListNodes = this->NlistNodes;

      calculatedFileLength  = 1 + 6*4;
      calculatedFileLength += 16 * numCells + 4 * numListNodes;
      calculatedFileLength += 3*4 * numNodes;
      if(numNodeFields)
        {
        calculatedFileLength += 2052 + numNodeFields*(12 + 4 * numNodes + 4);
        }

      if(numCellFields)
        {
        calculatedFileLength += 2052 + numCellFields*(12 + 4 * numCells + 4);
        }

      if(numFields)
        {
        calculatedFileLength += 2052 + numFields*(4 * 5);
        }

      vtkDebugMacro( << "TFL = " << trueFileLength
                     << "\tCFL = " << calculatedFileLength << endl);

      // We tried. Count our trys.
      attempts++;

      if(trueFileLength == calculatedFileLength)
        {
        // Endianness assumption was correct.
        break;
        }
      else
        {
        // If the lengths don't match, then either:
        // we tried the wrong endian-ness or the file is corrupt.
        // Switch to opposite of what was previously set in constructor.
        if(this->ByteOrder == FILE_LITTLE_ENDIAN)
          {
          this->ByteOrder = FILE_BIG_ENDIAN;
          }
        else if(this->ByteOrder == FILE_BIG_ENDIAN)
          {
          this->ByteOrder = FILE_LITTLE_ENDIAN;
          }
        }
      } // end of while loop

    if(trueFileLength != calculatedFileLength)
      {
      vtkErrorMacro("Calculated file length inconsistent with actual length; file corrupt?");
      return 0;
      }

    const long base_offset = 1 + 6*4;
    char buffer1[1024], buffer2[1024], label[32];

    long offset = base_offset + 16 * this->NumberOfCells +
      4 * this->NlistNodes + 3 * 4 * this->NumberOfNodes;

    if(this->NumberOfNodeFields)
      {
      this->FileStream->seekg(offset,ios::beg);
      this->FileStream->read(buffer1, sizeof(buffer1));
      this->FileStream->read(buffer2, sizeof(buffer2)); // read 2nd array of 1024 bytes
      this->ReadIntBlock(1, &this->NumberOfNodeComponents);

      ncomp_list = new int[this->NumberOfNodeFields];
      this->ReadIntBlock(this->NumberOfNodeFields, ncomp_list);

      this->NodeDataInfo = new DataInfo[this->NumberOfNodeComponents];

      float *mx = new float[this->NumberOfNodeFields];
      // read now the minimums for node_data
      this->ReadFloatBlock(this->NumberOfNodeFields, mx);
      k=0;
      for(i=0; i < this->NumberOfNodeComponents; i++)
        {
        for(j=0; j < ncomp_list[i]; j++)
          {
          this->NodeDataInfo[i].min[j] = mx[k];
          }
        k++;
        }
      // read now the maximums for node_data
      this->ReadFloatBlock(this->NumberOfNodeFields, mx);
      k=0;
      for(i=0; i < this->NumberOfNodeComponents; i++)
        {
        for(j=0; j < ncomp_list[i]; j++)
          {
          this->NodeDataInfo[i].max[j] = mx[k];
          }
        k++;
        }
      delete [] mx;

      offset +=  1024 + 1024 + 4 + 3 * 4 * this->NumberOfNodeFields;

      k = 0;
      for(i=0; i < this->NumberOfNodeComponents; i++)
        {
        this->GetLabel(buffer1, i, label);
        vtkDebugMacro( << i+1 << " :found ND label = " << label
                       << " [" << ncomp_list[i] << "]" <<endl);
        this->PointDataArraySelection->AddArray(label);
        this->NodeDataInfo[i].foffset = offset + k * 4 * this->NumberOfNodes;
        this->NodeDataInfo[i].veclen = ncomp_list[i];
        k += ncomp_list[i];
        }
      delete [] ncomp_list;
      }

    if(this->NumberOfCellFields)
      {
      offset += 4 * this->NumberOfNodes * this->NumberOfNodeFields +
        4 * this->NumberOfNodeFields;
      this->FileStream->seekg(offset,ios::beg);
      this->FileStream->read(buffer1, sizeof(buffer1));

      this->FileStream->read(buffer2, sizeof(buffer2)); // read 2nd array of 1024 bytes
      this->ReadIntBlock(1, &this->NumberOfCellComponents);

      ncomp_list = new int[this->NumberOfCellFields];
      this->ReadIntBlock(this->NumberOfCellFields, ncomp_list);

      this->CellDataInfo = new DataInfo[this->NumberOfCellComponents];

      float *mx = new float[this->NumberOfCellFields];
      // read now the minimums for cell_data
      this->ReadFloatBlock(this->NumberOfCellFields, mx);
      k=0;
      for(i=0; i < this->NumberOfCellFields; i++)
        {
        for(j=0; j < ncomp_list[i]; j++)
          {
          this->CellDataInfo[i].min[j] = mx[k];
          };
        k++;
        }
      // read now the maximums for cell_data
      this->ReadFloatBlock(this->NumberOfCellFields, mx);
      k=0;
      for(i=0; i < this->NumberOfCellFields; i++)
        {
        for(j=0; j < ncomp_list[i]; j++)
          {
          this->CellDataInfo[i].max[j] = mx[k];
          }
        k++;
        }
      delete [] mx;

      offset += 1024 + 1024 + 4 + 3 * 4 * this->NumberOfCellFields;

      k = 0;
      for(i=0; i < this->NumberOfCellComponents; i++)
        {
        this->GetLabel(buffer1, i, label);
        vtkDebugMacro( << i+1 << " :found CD label = " << label << " ["
                       << ncomp_list[i] << "]" << endl);
        this->CellDataArraySelection->AddArray(label);
        this->CellDataInfo[i].foffset = offset + k * 4 * this->NumberOfCells;
        this->CellDataInfo[i].veclen = ncomp_list[i];
        k += ncomp_list[i];
        }
      delete [] ncomp_list;
      }

    if(this->NumberOfFields)
      {
      offset += 4 * this->NumberOfCells * this->NumberOfCellFields +
        4 * this->NumberOfCellFields;
      this->FileStream->seekg(offset,ios::beg);
      this->FileStream->read(buffer1, sizeof(buffer1));
      vtkDebugMacro(<< buffer1 << endl);

      //offset += 1024 + 1024 + 4 + 3 * 4 * this->NumberOfFields;

      for(i=0; i < this->NumberOfFields; i++)
        {
        this->GetLabel(buffer1, i, label);
        vtkDebugMacro( << "found MD label = " << label << endl);
        }
      }
    } // end of Binary part
  for(i=0; i < this->NumberOfNodeComponents; i++)
    {
    vtkDebugMacro( << endl << this->PointDataArraySelection->GetArrayName(i)
                   << endl
                   << "offset = " << this->NodeDataInfo[i].foffset << endl
                   << "load = " << this->PointDataArraySelection->GetArraySetting(i) << endl
                   << "veclen = " << this->NodeDataInfo[i].veclen);
    }

  for(i=0; i < this->NumberOfCellComponents; i++)
    {
    vtkDebugMacro( << endl << this->CellDataArraySelection->GetArrayName(i)
                   << endl
                   << "offset = " << this->CellDataInfo[i].foffset << endl
                   << "load = " << this->CellDataArraySelection->GetArraySetting(i) << endl
                   << "veclen = " << this->CellDataInfo[i].veclen);
    }

  vtkDebugMacro( << "end of ExecuteInformation\n");

  return 1;
}

//----------------------------------------------------------------------------
void vtkAVSucdReader::GetCellDataRange(int cellComp, int index, float *min, float *max)
{
  if (index >= this->CellDataInfo[cellComp].veclen || index < 0)
    {
    index = 0;  // if wrong index, set it to zero
    }
  *min = this->CellDataInfo[cellComp].min[index];
  *max = this->CellDataInfo[cellComp].max[index];
}

//----------------------------------------------------------------------------
void vtkAVSucdReader::GetNodeDataRange(int nodeComp, int index, float *min, float *max)
{
  if (index >= this->NodeDataInfo[nodeComp].veclen || index < 0)
    {
    index = 0;  // if wrong index, set it to zero
    }
  *min = this->NodeDataInfo[nodeComp].min[index];
  *max = this->NodeDataInfo[nodeComp].max[index];
}

//----------------------------------------------------------------------------
void vtkAVSucdReader::ReadGeometry(vtkUnstructuredGrid *output)
{
  // add a material array
  vtkIntArray *materials = vtkIntArray::New();
  materials->SetNumberOfTuples(this->NumberOfCells);
  materials->SetName("Material Id");

  vtkFloatArray *coords = vtkFloatArray::New();
  coords->SetNumberOfComponents(3);
  coords->SetNumberOfTuples(this->NumberOfNodes);

  if (this->BinaryFile)
    {
    int *types = new int[this->NumberOfCells];
    if(types == NULL)
      {
      vtkErrorMacro(<< "Error allocating types memory\n");
      }

    vtkIdTypeArray *listcells = vtkIdTypeArray::New();
    // this array contains a list of NumberOfCells tuples
    // each tuple is 1 integer, i.e. the number of indices following it (N)
    // followed by these N integers
    listcells->SetNumberOfValues(this->NumberOfCells + this->NlistNodes);

    this->ReadBinaryCellTopology(materials, types, listcells);
    this->ReadXYZCoords(coords);

    vtkCellArray *cells = vtkCellArray::New();
    cells->SetCells(this->NumberOfCells, listcells);
    listcells->Delete();

    output->SetCells(types, cells);
    cells->Delete();
    delete [] types;
    }
  else
    {
    this->ReadXYZCoords(coords);
    this->ReadASCIICellTopology(materials, output);
    }

  vtkPoints *points = vtkPoints::New();
  points->SetData(coords);
  coords->Delete();

  output->SetPoints(points);
  points->Delete();

  // now add the material array
  output->GetCellData()->AddArray(materials);
  if (!output->GetCellData()->GetScalars())
    {
    output->GetCellData()->SetScalars(materials);
    }
  materials->Delete();
}


//----------------------------------------------------------------------------
void vtkAVSucdReader::ReadBinaryCellTopology(vtkIntArray *materials,
                                             int *types,
                                             vtkIdTypeArray *listcells)
{
  int i, j, k2=0;
  int *mat = materials->GetPointer(0);
  vtkIdType *list = listcells->GetPointer(0);
  int *ctype = new int[4 * this->NumberOfCells];
  if(ctype == NULL)
    {
    vtkErrorMacro(<< "Error allocating ctype memory");
    return;
    }

  this->FileStream->seekg(6*4 + 1,ios::beg);
  this->ReadIntBlock(4 * this->NumberOfCells, ctype);

  int *topology_list = new int[this->NlistNodes];
  if(topology_list == NULL)
    {
    vtkErrorMacro(<< "Error allocating topology_list memory");
    return;
    }

  this->ReadIntBlock(this->NlistNodes, topology_list);
  this->UpdateProgress(0.25);

  for(i=0; i < this->NumberOfCells; i++)
    {
    *list++ = ctype[4*i+2];
    if(ctype[4*i+3] == vtkAVSucdReader::PYR)
      { //UCD ordering is 0,1,2,3,4 => VTK ordering is 1,2,3,4,0
      *list++ = topology_list[++k2] - 1;
      *list++ = topology_list[++k2] - 1;
      *list++ = topology_list[++k2] - 1;
      *list++ = topology_list[++k2] - 1;
      *list++ = topology_list[k2-4] - 1;
      k2++;
      }
     else
      {
       for(j=0; j < ctype[4*i+2]; j++)
         {
         *list++ = topology_list[k2++] - 1;
         }
      }
    }

  delete [] topology_list;

  for(i=0; i < this->NumberOfCells; i++)
    {
    *mat++ = ctype[4*i+1];
    switch(ctype[4*i+3])
      {
      case vtkAVSucdReader::PT:    types[i] = VTK_VERTEX;     break;
      case vtkAVSucdReader::LINE:  types[i] = VTK_LINE;       break;
      case vtkAVSucdReader::TRI:   types[i] = VTK_TRIANGLE;   break;
      case vtkAVSucdReader::QUAD:  types[i] = VTK_QUAD;       break;
      case vtkAVSucdReader::TET:   types[i] = VTK_TETRA;      break;
      case vtkAVSucdReader::PYR:   types[i] = VTK_PYRAMID;    break;
      case vtkAVSucdReader::PRISM: types[i] = VTK_WEDGE;      break;
      case vtkAVSucdReader::HEX:   types[i] = VTK_HEXAHEDRON; break;
      default:
        vtkErrorMacro( << "cell type: " << ctype[4*i+3] << "not supported\n");
        delete [] ctype;
        return;
      }
    }
  delete [] ctype;
}


//----------------------------------------------------------------------------
void vtkAVSucdReader::ReadASCIICellTopology(vtkIntArray *materials,
                                            vtkUnstructuredGrid *output)
{
  int i, k;
  vtkIdType list[8];
  int *mat = materials->GetPointer(0);
  char ctype[5];

  output->Allocate();
  for(i=0; i < this->NumberOfCells; i++)
    {
    int id;  // no check is done to see that they are monotonously increasing
    *(this->FileStream) >> id;
    *(this->FileStream) >> mat[i];
    *(this->FileStream) >> ctype;
    vtkDebugMacro( << mat[i] << ", " << ctype );
    if(!strcmp(ctype, "pt"))
      {
      for(k=0; k < 1; k++)
        {
        *(this->FileStream) >> list[k];
        if(this->DecrementNodeIds)
          {
          list[k]--;
          }
        }
      output->InsertNextCell(VTK_VERTEX, 1, list);
      }
    else if(!strcmp(ctype, "line"))
      {
      for(k=0; k < 2; k++)
        {
        *(this->FileStream) >> list[k];
        if(this->DecrementNodeIds)
          {
          list[k]--;
          }
        }
      output->InsertNextCell(VTK_LINE, 2, list);
      }
    else if(!strcmp(ctype, "tri"))
      {
      for(k=0; k < 3; k++)
        {
        *(this->FileStream) >> list[k];
        if(this->DecrementNodeIds)
          {
          list[k]--;
          }
        }
      output->InsertNextCell(VTK_TRIANGLE, 3, list);
      }
    else if(!strcmp(ctype, "quad"))
      {
      for(k=0; k < 4; k++)
        {
        *(this->FileStream) >> list[k];
        if(this->DecrementNodeIds)
          {
          list[k]--;
          }
        }
      output->InsertNextCell(VTK_QUAD, 4, list);
      }
    else if(!strcmp(ctype, "tet"))
      {
      for(k=0; k < 4; k++)
        {
        *(this->FileStream) >> list[k];
        if(this->DecrementNodeIds)
          {
          list[k]--;
          }
        }
      output->InsertNextCell(VTK_TETRA, 4, list);
      }
    else if(!strcmp(ctype, "pyr"))
      {
      for(k=0; k < 5; k++)
        {
        *(this->FileStream) >> list[k];
        if(this->DecrementNodeIds)
          {
          list[k]--;
          }
        }
      int tmp;
      tmp = list[0];
      list[0] = list[1]; list[1] = list[2]; list[2] = list[3];
      list[3] = list[4]; list[4] = tmp;
      output->InsertNextCell(VTK_PYRAMID, 5, list);
      }
    else if(!strcmp(ctype, "prism"))
      {
      for(k=0; k < 6; k++)
        {
        *(this->FileStream) >> list[k];
        if(this->DecrementNodeIds)
          {
          list[k]--;
          }
        }
      output->InsertNextCell(VTK_WEDGE, 6, list);
      }
    else if(!strcmp(ctype, "hex"))
      {
      for(k=0; k < 8; k++)
        {
        *(this->FileStream) >> list[k];
        if(this->DecrementNodeIds)
          {
          list[k]--;
          }
        }
      output->InsertNextCell(VTK_HEXAHEDRON, 8, list);
      }
    else
      {
      vtkErrorMacro( << "cell type: " << ctype << " is not supported\n");
      return;
      }
    }  // for all cell, read the indices
}


//----------------------------------------------------------------------------
void vtkAVSucdReader::ReadXYZCoords(vtkFloatArray *coords)
{
  int i;
  float *ptr = coords->GetPointer(0);
  if (this->BinaryFile)
    {
    float *cs = new float[this->NumberOfNodes];

    // read X coordinates from file and stuff into coordinates array
    this->ReadFloatBlock(this->NumberOfNodes, cs);
    for(i=0; i < this->NumberOfNodes; i++)
      {
      ptr[3*i] = cs[i];
      }

    // read Y coordinates from file and stuff into coordinates array
    this->ReadFloatBlock(this->NumberOfNodes, cs);
    for(i=0; i < this->NumberOfNodes; i++)
      {
      ptr[3*i+1] = cs[i];
      }

    // read Z coordinates from file and stuff into coordinates array
    this->ReadFloatBlock(this->NumberOfNodes, cs);
    for(i=0; i < this->NumberOfNodes; i++)
      {
      ptr[3*i+2] = cs[i];
      }
    // end of stuffing all coordinates
    delete [] cs;
    }  // end of binary read
  else
    {
    int id;  // no check is done to see that they are monotonously increasing
    // read here the first node and check if its id number is 0

    *(this->FileStream) >> id;
    i=0;
    *(this->FileStream) >> ptr[3*i] >> ptr[3*i+1] >> ptr[3*i+2];
    if(id)
      {
      this->DecrementNodeIds = 1;
      }

    for(i=1; i < this->NumberOfNodes; i++)
      {
      *(this->FileStream) >> id;
      *(this->FileStream) >> ptr[3*i] >> ptr[3*i+1] >> ptr[3*i+2];
      }
    } // end of ASCII read
}


//----------------------------------------------------------------------------
void vtkAVSucdReader::ReadNodeData(vtkUnstructuredGrid *output)
{
  int i, j, n;
  float *ptr;
  vtkDebugMacro( << "Begin of ReadNodeData()\n");
  if(this->BinaryFile)
    {
    for (i=0; i < this->NumberOfNodeComponents; i++)
      {
      if(this->PointDataArraySelection->GetArraySetting(i))
        {
        vtkFloatArray *scalars = vtkFloatArray::New();
        scalars->SetNumberOfComponents(this->NodeDataInfo[i].veclen);
        scalars->SetNumberOfTuples(this->NumberOfNodes);
        scalars->SetName(PointDataArraySelection->GetArrayName(i));
        this->FileStream->seekg(this->NodeDataInfo[i].foffset, ios::beg);
        if(1) // this->NodeDataInfo[i].veclen == 1)
          {
          ptr = scalars->GetPointer(0);
          this->ReadFloatBlock(this->NumberOfNodes *
                               this->NodeDataInfo[i].veclen, ptr);
          }
        else
          {
          ptr = new float[this->NodeDataInfo[i].veclen];
          for(n=0; n < this->NumberOfNodes; n++)
            {
            this->ReadFloatBlock(this->NodeDataInfo[i].veclen, ptr);
            for(j=0; j < this->NodeDataInfo[i].veclen; j++)
              {
              scalars->SetComponent(n, j, ptr[j]);
              }
            }
          delete [] ptr;
          }

        output->GetPointData()->AddArray(scalars);
        if (!output->GetPointData()->GetScalars())
          {
          output->GetPointData()->SetScalars(scalars);
          }
        scalars->Delete();
        }
      }
    //
    // Don't know how to use the information below, so skip reading it
    // int *node_active_list = new int[this->NumberOfNodeFields];
    // this->ReadIntArray(node_active_list, this->NumberOfNodeFields);
    // delete [] node_active_list;
    //
    } // end of binary read
  else
    {
    float value;
    int id;
    char buf1[128], c='\0', buf2[128];

    *(this->FileStream) >> this->NumberOfNodeComponents;
    this->NodeDataInfo = new DataInfo[this->NumberOfNodeComponents];
    for(i=0; i < this->NumberOfNodeComponents; i++)
      {
      *(this->FileStream) >> this->NodeDataInfo[i].veclen;
      }
    this->FileStream->get(c); // one more newline to catch

    vtkFloatArray **scalars = new
      vtkFloatArray * [this->NumberOfNodeComponents];
    for(i=0; i < this->NumberOfNodeComponents; i++)
      {
      j=0;
      while(this->FileStream->get(c) && c != ',')
        {
        buf1[j++] = c;
        }
      buf1[j] = '\0';
      // finish here to read the line
      this->FileStream->get(buf2, 128, '\n'); this->FileStream->get(c);

      scalars[i] = vtkFloatArray::New();
      scalars[i]->SetNumberOfComponents(this->NodeDataInfo[i].veclen);
      scalars[i]->SetNumberOfTuples(this->NumberOfNodes);
      scalars[i]->SetName(buf1);
      }
    for(n=0; n < this->NumberOfNodes; n++)
      {
      *(this->FileStream) >> id;
      for(i=0; i < this->NumberOfNodeComponents; i++)
        {
        for(j=0; j < this->NodeDataInfo[i].veclen; j++)
          {
          *(this->FileStream) >> value;
          scalars[i]->SetComponent(n, j, value);
          }
        }
      }
    for(i=0; i < this->NumberOfNodeComponents; i++)
      {
      output->GetPointData()->AddArray(scalars[i]);
      if (!output->GetPointData()->GetScalars())
        {
        output->GetPointData()->SetScalars(scalars[i]);
        }
      scalars[i]->Delete();
      }
      delete[] scalars;
    } // end of ASCII read
  vtkDebugMacro( << "End of ReadNodeData()\n");
}


//----------------------------------------------------------------------------
void vtkAVSucdReader::ReadCellData(vtkUnstructuredGrid *output)
{
  int i, j, n;
  float *ptr;
  vtkDebugMacro( << "Begin of ReadCellData()\n");
  if(this->BinaryFile)
    {
    for (i=0; i < this->NumberOfCellComponents; i++)
      {
      if(this->CellDataArraySelection->GetArraySetting(i))
        {
        vtkFloatArray *scalars = vtkFloatArray::New();
        scalars->SetNumberOfComponents(this->CellDataInfo[i].veclen);
        scalars->SetNumberOfTuples(this->NumberOfCells);
        scalars->SetName(CellDataArraySelection->GetArrayName(i));
        this->FileStream->seekg(this->CellDataInfo[i].foffset, ios::beg);
        if(1) // this->CellDataInfo[i].veclen == 1)
          {
          ptr = scalars->GetPointer(0);
          this->ReadFloatBlock(this->NumberOfCells *
                               this->CellDataInfo[i].veclen, ptr);
          }
        else
          {
          ptr = new float[this->NumberOfCells];
          for(j=0; j < this->CellDataInfo[i].veclen; j++)
            {
            this->FileStream->seekg(this->CellDataInfo[i].foffset +
                            j*this->NumberOfCells,
                            ios::beg);
            this->ReadFloatBlock(this->NumberOfCells, ptr);

            for(n=0; n < this->NumberOfCells; n++)
              {
              scalars->SetComponent(n, j, ptr[n]);
              }
            }
          delete [] ptr;
          }

        output->GetCellData()->AddArray(scalars);
        if (!output->GetCellData()->GetScalars())
          {
          output->GetCellData()->SetScalars(scalars);
          }
        scalars->Delete();
        }
      }
    } // end of binary read
  else
    {
    float value;
    int id;
    char buf1[128], c='\0', buf2[128];

    *(this->FileStream) >> this->NumberOfCellComponents;
    this->CellDataInfo = new DataInfo[this->NumberOfCellComponents];

    for(i=0; i < this->NumberOfCellComponents; i++)
      {
      *(this->FileStream) >> this->CellDataInfo[i].veclen;
      }
    this->FileStream->get(c); // one more newline to catch

    vtkFloatArray **scalars = new
      vtkFloatArray * [this->NumberOfCellComponents];
    for(i=0; i < this->NumberOfCellComponents; i++)
      {
      j=0;
      while(this->FileStream->get(c) && c != ',')
        {
        buf1[j++] = c;
        }
      buf1[j] = '\0';
      // finish here to read the line
      this->FileStream->get(buf2, 128, '\n'); this->FileStream->get(c);

      scalars[i] = vtkFloatArray::New();
      scalars[i]->SetNumberOfComponents(this->CellDataInfo[i].veclen);
      scalars[i]->SetNumberOfTuples(this->NumberOfCells);
      scalars[i]->SetName(buf1);
      }
    for(n=0; n < this->NumberOfCells; n++)
      {
      *(this->FileStream) >> id;
      for(i=0; i < this->NumberOfCellComponents; i++)
        {
        for(j=0; j < this->CellDataInfo[i].veclen; j++)
          {
          *(this->FileStream) >> value;
          scalars[i]->SetComponent(n, j, value);
          }
        }
      }
    for(i=0; i < this->NumberOfCellComponents; i++)
      {
      output->GetCellData()->AddArray(scalars[i]);
      if (!output->GetCellData()->GetScalars())
        {
        output->GetCellData()->SetScalars(scalars[i]);
        }
      scalars[i]->Delete();
      }
      delete[] scalars;
    } // end of ASCII read
  vtkDebugMacro( << "End of ReadCellData()\n");
}

//----------------------------------------------------------------------------
const char* vtkAVSucdReader::GetPointArrayName(int index)
{
  return this->PointDataArraySelection->GetArrayName(index);
}

//----------------------------------------------------------------------------
int vtkAVSucdReader::GetPointArrayStatus(const char* name)
{
  return this->PointDataArraySelection->ArrayIsEnabled(name);
}

//----------------------------------------------------------------------------
void vtkAVSucdReader::SetPointArrayStatus(const char* name, int status)
{
  if(status)
    {
    this->PointDataArraySelection->EnableArray(name);
    }
  else
    {
    this->PointDataArraySelection->DisableArray(name);
    }
}

//----------------------------------------------------------------------------
const char* vtkAVSucdReader::GetCellArrayName(int index)
{
  return this->CellDataArraySelection->GetArrayName(index);
}

//----------------------------------------------------------------------------
int vtkAVSucdReader::GetCellArrayStatus(const char* name)
{
  return this->CellDataArraySelection->ArrayIsEnabled(name);
}


//----------------------------------------------------------------------------
void vtkAVSucdReader::SetCellArrayStatus(const char* name, int status)
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
int vtkAVSucdReader::GetNumberOfCellArrays()
{
  return this->CellDataArraySelection->GetNumberOfArrays();
}

//----------------------------------------------------------------------------
int vtkAVSucdReader::GetNumberOfPointArrays()
{
  return this->PointDataArraySelection->GetNumberOfArrays();
}

//----------------------------------------------------------------------------
void vtkAVSucdReader::EnableAllPointArrays()
{
    this->PointDataArraySelection->EnableAllArrays();
}

//----------------------------------------------------------------------------
void vtkAVSucdReader::DisableAllPointArrays()
{
    this->PointDataArraySelection->DisableAllArrays();
}

//----------------------------------------------------------------------------
void vtkAVSucdReader::EnableAllCellArrays()
{
    this->CellDataArraySelection->EnableAllArrays();
}

//----------------------------------------------------------------------------
void vtkAVSucdReader::DisableAllCellArrays()
{
    this->CellDataArraySelection->DisableAllArrays();
}

//----------------------------------------------------------------------------
int vtkAVSucdReader::GetLabel(char *string, int number, char *label)
{
  int   i, j, k, len;
  char  current;


  // check to make sure that structure is not NULL
  if (string == NULL)
    {
    vtkErrorMacro( << "String is null");
    return 0;
    }

  // search for the appropriate label
  k = 0;
  len = static_cast<int>(strlen(string));
  for(i = 0; i <= number; i++)
    {
    current = string[k++];
    j = 0;
    while (current != '.')
      {
      // build the label character by character
      label[j++] = current;
      current = string[k++];

      // the last character was found
      if (k > len)
        {
        // the nth label was not found, where n = number
        if (i < number)
          {
          return 0;
          }
        current = '.';
        }
      }  // end while
    label[j] = '\0';
    }
  // a valid label was found
  return 1;
}


//----------------------------------------------------------------------------
// Read a block of ints (ascii or binary) and return number read.
int vtkAVSucdReader::ReadIntBlock(int n, int *block)
{
  if (this->BinaryFile)
    {
    this->FileStream->read((char *)block, n * sizeof(int));
    int retVal = static_cast<int>(this->FileStream->gcount()) / sizeof(int);

    if (this->ByteOrder == FILE_LITTLE_ENDIAN)
      {
      vtkByteSwap::Swap4LERange(block, n);
      }
    else
      {
      vtkByteSwap::Swap4BERange(block, n);
      }
    return retVal;
    }
  else
    {
    int count = 0;
    for(int i=0; i<n; i++)
      {
      if (*(this->FileStream) >> block[i])
        {
        count++;
        }
      else
        {
        return 0;
        }
      }
    return count;
    }
}

//----------------------------------------------------------------------------
int vtkAVSucdReader::ReadFloatBlock(int n, float* block)
{
  if (this->BinaryFile)
    {
    this->FileStream->read((char *)block, n * sizeof(float));
    int retVal = static_cast<int>(this->FileStream->gcount()) / sizeof(int);
    if (this->ByteOrder == FILE_LITTLE_ENDIAN)
      {
      vtkByteSwap::Swap4LERange(block, n);
      }
    else
      {
      vtkByteSwap::Swap4BERange(block, n);
      }
    return retVal;
    }
  else
    {
    int count = 0;
    for(int i=0; i<n; i++)
      {
      if (*(this->FileStream) >> block[i])
        {
        count++;
        }
      else
        {
        return 0;
        }
      }
    return count;
    }
}
