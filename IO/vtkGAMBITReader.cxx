/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGAMBITReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// Thanks to Jean M. Favre (CSCS, Swiss Center for Scientific Computing) who 
// developed this class.
// Please address all comments to Jean Favre (jfavre at cscs.ch)

#include "vtkGAMBITReader.h"

#include "vtkObjectFactory.h"
#include "vtkErrorCode.h"
#include "vtkUnstructuredGrid.h"
#include "vtkPointData.h"
#include "vtkCellData.h"
#include "vtkDoubleArray.h"
#include "vtkIntArray.h"
#include "vtkCellArray.h"

vtkCxxRevisionMacro(vtkGAMBITReader, "1.2");
vtkStandardNewMacro(vtkGAMBITReader);

vtkGAMBITReader::vtkGAMBITReader()
{
  this->FileName  = NULL;
  this->NumberOfNodeFields = 0;
  this->NumberOfCellFields = 0;
  this->FileStream = NULL;
}

vtkGAMBITReader::~vtkGAMBITReader()
{
  if (this->FileName)
    {
    delete [] this->FileName;
    }
}

void vtkGAMBITReader::Execute()
{
  vtkDebugMacro( << "Reading GAMBIT Neutral file");

  // If ExecuteInformation() failed the FileStream will be NULL and
  // ExecuteInformation() will have spit out an error.
  if ( this->FileStream == NULL )
    {
    return;
    }

  this->ReadFile();

  return;
}

void vtkGAMBITReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "File Name: " 
     << (this->FileName ? this->FileName : "(none)") << "\n";

  os << indent << "Number Of Nodes: " << this->NumberOfNodes << endl;
  os << indent << "Number Of Node Fields: " 
     << this->NumberOfNodeFields << endl;

  os << indent << "Number Of Cells: " << this->NumberOfCells << endl;
  os << indent << "Number Of Cell Fields: " 
     << this->NumberOfCellFields << endl;
}


void vtkGAMBITReader::ReadFile()
{
  this->ReadGeometry();

  // yes, but, we cannot find any examples containing data.
  // GAMBIT users seem to say that they use the Fluent solver and do not
  // use Gambit as an output format, thus no data when used as input to solver
  if(this->NumberOfNodeFields)
    {
    this->ReadNodeData();
    }

  if(this->NumberOfCellFields)
    {
    this->ReadCellData(); 
    }

  delete this->FileStream;
  this->FileStream = NULL;
}

void vtkGAMBITReader::ReadNodeData()
{
  vtkWarningMacro("Not implemented due to lack of examples");
}

void vtkGAMBITReader::ReadCellData()
{
  vtkWarningMacro("Not implemented due to lack of examples");
}

void vtkGAMBITReader::ExecuteInformation()
{
  if ( !this->FileName )
    {
    this->NumberOfNodes = 0;
    this->NumberOfCells = 0;
    this->NumberOfNodeFields = 0;
    this->NumberOfCellFields = 0;

    vtkErrorMacro("No filename specified");
    return;
    }
  
  this->FileStream = new ifstream(this->FileName, ios::in);

  if (this->FileStream->fail())
    {
    this->SetErrorCode(vtkErrorCode::FileNotFoundError);
    delete this->FileStream;
    this->FileStream = NULL;
    vtkErrorMacro("Specified filename not found");
    return;
    }

  char c='\0', buf[128];

  this->FileStream->get(buf, 128, '\n'); this->FileStream->get(c);
  this->FileStream->get(buf, 128, '\n'); this->FileStream->get(c);
  this->FileStream->get(buf, 128, '\n'); this->FileStream->get(c);
  this->FileStream->get(buf, 128, '\n'); this->FileStream->get(c);
  this->FileStream->get(buf, 128, '\n'); this->FileStream->get(c);
  this->FileStream->get(buf, 128, '\n'); this->FileStream->get(c);

  *(this->FileStream) >> this->NumberOfNodes;
  *(this->FileStream) >> this->NumberOfCells;
  *(this->FileStream) >> this->NumberOfElementGroups;
  *(this->FileStream) >> this->NumberOfBoundaryConditionSets;
  *(this->FileStream) >> this->NumberOfCoordinateDirections;
  *(this->FileStream) >> this->NumberOfVelocityComponents;
  this->FileStream->get(c);

  // read here the end of section
  this->FileStream->get(buf, 128, '\n'); this->FileStream->get(c);
  if(strncmp(buf, "ENDOFSECTION", 12))
    {
    vtkErrorMacro(<<"Error reading file");
    }
  vtkDebugMacro(
  << "\nNumberOfNodes " << this->NumberOfNodes
  << "\nNumberOfCells " << this->NumberOfCells
  << "\nNumberOfElementGroups " <<  this->NumberOfElementGroups
  << "\nNumberOfBoundaryConditionSets " << this->NumberOfBoundaryConditionSets
  << "\nNumberOfCoordinateDirections " << this->NumberOfCoordinateDirections
  << "\nNumberOfVelocityComponents " << this->NumberOfVelocityComponents);
}

void vtkGAMBITReader::ReadGeometry()
{
  vtkUnstructuredGrid *output = (vtkUnstructuredGrid *)this->GetOutput();

  vtkDoubleArray *coords = vtkDoubleArray::New();
  coords->SetNumberOfComponents(3);
  // allocate one more pt and store node id=0
  coords->SetNumberOfTuples(this->NumberOfNodes+1);

  this->ReadXYZCoords(coords);
  this->ReadCellConnectivity();
  if(this->NumberOfElementGroups > 0)
    {
    this->ReadMaterialTypes();
    }
  if(this->NumberOfBoundaryConditionSets > 0)
    {
    this->ReadBoundaryConditionSets();
    }
  vtkPoints *points = vtkPoints::New();
  points->SetData(coords);
  coords->Delete();
  
  output->SetPoints(points);
  points->Delete();
}

void vtkGAMBITReader::ReadBoundaryConditionSets()
{
  int bcs, f, itype, nentry, nvalues;
  int isUsable=0;
  int node, elt, eltype, facenumber;
  char c, buf[128];

  vtkUnstructuredGrid *output = (vtkUnstructuredGrid *)this->GetOutput();

  // no idea about how to treat element/cell, so we allocate a single array

  vtkIntArray *bcscalar = vtkIntArray::New();
  bcscalar->SetNumberOfComponents(1);
  bcscalar->SetNumberOfTuples(this->NumberOfNodes+1);
  bcscalar->SetName("Boundary Condition");
  int *ptr = bcscalar->GetPointer(0);
  // initialise with null values. When set later, will set to 1
  memset((void*)ptr,0,sizeof(int)*this->NumberOfNodes);

  for(bcs=1; bcs <= this->NumberOfBoundaryConditionSets; bcs++)
  {
    this->FileStream->get(buf, 128, '\n'); this->FileStream->get(c);
    this->FileStream->get(buf, 128, '\n'); this->FileStream->get(c);
    sscanf(&buf[32],"%10d%10d%10d", &itype, &nentry, &nvalues);
    vtkDebugMacro(
      << "\nitype " << itype
      << "\tnentry " << nentry
      << "\tnvalues " <<  nvalues);
    // I have no example o how nvalues is used....So no implementation.
    if(itype == 0) // nodes
      {
      isUsable = 1;
      for(f=0; f < nentry; f++)
        {
        *(this->FileStream) >> node;
        bcscalar->SetValue(node, 1);
        }
      this->FileStream->get(c);
      // read here the end of section
      this->FileStream->get(buf, 128, '\n'); this->FileStream->get(c);
      if(strncmp(buf, "ENDOFSECTION", 12))
        {
        vtkErrorMacro(<<"Error reading ENDOFSECTION tag at end of group");
        }
      }
    else // element/cell are parsed but nothing is done with the info read
      {
      for(f=0; f < nentry; f++)
        {
        *(this->FileStream) >> elt >> eltype >> facenumber;
        }
      this->FileStream->get(c);
      // read here the end of section
      this->FileStream->get(buf, 128, '\n'); this->FileStream->get(c);
      if(strncmp(buf, "ENDOFSECTION", 12))
        {
        vtkErrorMacro(<<"Error reading ENDOFSECTION tag at end of group");
        }
      }
  }
  vtkDebugMacro(<< "All BCS read succesfully");
  if(isUsable)
    {
    output->GetPointData()->AddArray(bcscalar);
    if (!output->GetPointData()->GetScalars())
      {
      output->GetPointData()->SetScalars(bcscalar);
      }
    }
  bcscalar->Delete();
}

void vtkGAMBITReader::ReadMaterialTypes()
{
  int grp, f, flag, id, nbelts, elt, mat, nbflags;
  char c, buf[128];
  vtkUnstructuredGrid *output = (vtkUnstructuredGrid *)this->GetOutput();

  vtkIntArray *materials = vtkIntArray::New();
  materials->SetNumberOfComponents(1);
  materials->SetNumberOfTuples(this->NumberOfCells);
  materials->SetName("Material Type");

  for(grp=1; grp <= this->NumberOfElementGroups; grp++)
  {
    this->FileStream->get(buf, 128, '\n'); this->FileStream->get(c);
    this->FileStream->get(buf, 128, '\n'); this->FileStream->get(c);
    sscanf(buf,"GROUP:%10d ELEMENTS: %10d MATERIAL: %10d NFLAGS:%10d", &id, &nbelts, &mat, &nbflags);

    vtkDebugMacro(
      << "\nid " << id
      << "\tnbelts " << nbelts
      << "\tmat " <<  mat
      << "\tnbflags " << nbflags);

    this->FileStream->get(buf, 128, '\n'); this->FileStream->get(c);
    for(f=0; f < nbflags; f++)
      {
      *(this->FileStream) >> flag;
      }
    this->FileStream->get(c);
    for(f=0; f < nbelts; f++)
      {
      *(this->FileStream) >> elt;
      materials->SetValue(elt-1, mat);
      }
    this->FileStream->get(c);
    // read here the end of section
    this->FileStream->get(buf, 128, '\n'); this->FileStream->get(c);
    if(strncmp(buf, "ENDOFSECTION", 12))
      {
      vtkErrorMacro(<<"Error reading ENDOFSECTION tag at end of group");
      }
    }
    vtkDebugMacro(<< "All groups read succesfully");

  output->GetCellData()->AddArray(materials);
  if (!output->GetCellData()->GetScalars())
    {
    output->GetCellData()->SetScalars(materials);
    }
  materials->Delete();
}

void vtkGAMBITReader::ReadCellConnectivity()
{
  int i, k;
  vtkIdType list[27];
  char c, buf[128];
  vtkUnstructuredGrid *output = (vtkUnstructuredGrid *)this->GetOutput();

  output->Allocate();

  this->FileStream->get(buf, 128, '\n'); this->FileStream->get(c);

  for(i=1; i <= this->NumberOfCells; i++)
    {
    int id;  // no check is done to see that they are monotonously increasing
    int ntype, ndp;
    *(this->FileStream) >> id >> ntype >> ndp;

    switch(ntype){
    case EDGE:
      {
      for(k=0; k < 2; k++)
        {
        *(this->FileStream) >> list[k];
        }
      output->InsertNextCell(VTK_LINE, 2, list);
      }
    break;
    case TRI:
      {
      for(k=0; k < 3; k++)
        {
        *(this->FileStream) >> list[k];
        }
      output->InsertNextCell(VTK_TRIANGLE, 3, list);
      }
    break;
    case QUAD:
      {
      for(k=0; k < 4; k++)
        {
        *(this->FileStream) >> list[k];
        }
      output->InsertNextCell(VTK_QUAD, 4, list);
      }
    break;
    case TETRA:
      {
      for(k=0; k < 4; k++)
        {
        *(this->FileStream) >> list[k];
        }
      output->InsertNextCell(VTK_TETRA, 4, list);
      }
    break;
    case PYRAMID:
      {
      for(k=0; k < 5; k++)
        {
        *(this->FileStream) >> list[k];
        }
      output->InsertNextCell(VTK_PYRAMID, 5, list);
      }
    break;
    case PRISM:
      {
      for(k=0; k < 6; k++)
        {
        *(this->FileStream) >> list[k];
        }
      output->InsertNextCell(VTK_WEDGE, 6, list);
      }
    break;
    case BRICK:
      {
      for(k=0; k < 8; k++)
        {
        *(this->FileStream) >> list[k];
        }
      output->InsertNextCell(VTK_HEXAHEDRON, 8, list);
      }
    break;
    default:
      {
      vtkErrorMacro( << "cell type: " << ntype << " is not supported\n");
      return;
      }
    }  // for all cell, read the indices 
  }
  // read here the end of section
  this->FileStream->get(c); this->FileStream->get(buf, 128, '\n'); this->FileStream->get(c);
  if(strncmp(buf, "ENDOFSECTION", 12))
    {
    vtkErrorMacro(<<"Error reading ENDOFSECTION tag at end of connectivity");
    }
}

void vtkGAMBITReader::ReadXYZCoords(vtkDoubleArray *coords)
{
  int i;
  double *ptr = coords->GetPointer(0);
  char c, buf[64];
  ptr[0] = ptr[1] = ptr[2] = 0.0;

  int id;  // no check is done to see that they are monotonously increasing
  this->FileStream->get(buf, 64, '\n'); this->FileStream->get(c);

  if(this->NumberOfCoordinateDirections == 3)
    {
    for(i=1; i <= this->NumberOfNodes; i++)
      {
      *(this->FileStream) >> id;
      *(this->FileStream) >> ptr[3*i] >> ptr[3*i+1] >> ptr[3*i+2];
      }
    }
  else
    {
    for(i=1; i <= this->NumberOfNodes; i++)
      {
      *(this->FileStream) >> id;
      *(this->FileStream) >> ptr[3*i] >> ptr[3*i+1];
      ptr[3*i+2] = 0.0;
      }
    }
  this->FileStream->get(c); this->FileStream->get(buf, 128, '\n'); this->FileStream->get(c);
  if(strncmp(buf, "ENDOFSECTION", 12))
    {
    vtkErrorMacro("Error reading ENDOFSECTION tag at end of coordinates section");
    }
}
