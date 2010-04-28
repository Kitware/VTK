/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLPolyhedronMeshWriter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkXMLPolyhedronMeshWriter.h"

#include "vtkPolyhedron.h"
#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkErrorCode.h"
#include "vtkInformation.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnstructuredGrid.h"
#define vtkOffsetsManager_DoNotInclude
#include "vtkOffsetsManagerArray.h"
#undef vtkOffsetsManager_DoNotInclude
#include "vtkSmartPointer.h"

#include <assert.h>

vtkStandardNewMacro(vtkXMLPolyhedronMeshWriter);

//----------------------------------------------------------------------------
vtkXMLPolyhedronMeshWriter::vtkXMLPolyhedronMeshWriter()
{
}

//----------------------------------------------------------------------------
vtkXMLPolyhedronMeshWriter::~vtkXMLPolyhedronMeshWriter()
{
}

//----------------------------------------------------------------------------
void vtkXMLPolyhedronMeshWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

//----------------------------------------------------------------------------
vtkUnstructuredGrid* vtkXMLPolyhedronMeshWriter::GetInput()
{
  return static_cast<vtkUnstructuredGrid*>(this->Superclass::GetInput());
}

//----------------------------------------------------------------------------
int vtkXMLPolyhedronMeshWriter::ProcessRequest(vtkInformation* request,
                                               vtkInformationVector** inputVector,
                                               vtkInformationVector* outputVector)
{
  
  // Todo:
  // sweep through the original input to keep only the polyhedron cells.
  //vtkSmartPointer<vtkUnstructuredGrid> cleanPolyhedronMesh = 
  //  vtkSmartPointer<vtkUnstructuredGrid>::New();
  //
  //Add polyhedron to cleanPolyhedronMesh
  //
  //this->SetInput(cleanPolyhedronMesh);
  
  return this->Superclass::ProcessRequest(request, inputVector, outputVector);
}

//----------------------------------------------------------------------------
const char* vtkXMLPolyhedronMeshWriter::GetDataSetName()
{
  return "UnstructuredGrid";
}

//----------------------------------------------------------------------------
const char* vtkXMLPolyhedronMeshWriter::GetDefaultFileExtension()
{
  return "vth";
}

//----------------------------------------------------------------------------
void vtkXMLPolyhedronMeshWriter::WriteInlinePieceAttributes()
{
  this->Superclass::WriteInlinePieceAttributes();
  if (this->ErrorCode == vtkErrorCode::OutOfDiskSpaceError)
    {
    return;
    }

  this->WriteScalarAttribute("NumberOfFaces", this->GetNumberOfInputFaces());
}

//----------------------------------------------------------------------------
void vtkXMLPolyhedronMeshWriter::WriteInlinePiece(vtkIndent indent)
{
  vtkUnstructuredGrid* input = this->GetInput();
  
  // Split progress range by the approximate fraction of data written
  // by each step in this method.
  float progressRange[2] = {0,0};
  this->GetProgressRange(progressRange);
  float fractions[3];
  this->CalculateSuperclassFraction(fractions);
  
  // Set the range of progress for superclass.
  this->SetProgressRange(progressRange, 0, fractions);
  
  // Let the superclass write its data.
  this->Superclass::WriteInlinePiece(indent);
  if (this->ErrorCode == vtkErrorCode::OutOfDiskSpaceError)
    {
    return;
    }
  
  // Set range of progress for the cell specifications.
  this->SetProgressRange(progressRange, 1, fractions);
  
  // Write the cell specifications.
  this->WriteFacesInline("Faces", input, indent);
}

//----------------------------------------------------------------------------
void vtkXMLPolyhedronMeshWriter::
ConstructArrays(vtkIdTypeArray * connectivityArray, vtkIdTypeArray * offsetArray)
{
  vtkUnstructuredGrid* input = this->GetInput();
  
  connectivityArray->Resize(0);
  offsetArray->Resize(0);
  vtkIdType offset = 0;
  
  // for each cell, one element to store number of faces
  for (vtkIdType i = 0; i < input->GetNumberOfCells(); i++)
    {
    vtkPolyhedron * cell = vtkPolyhedron::SafeDownCast(input->GetCell(i));
    if (!cell)
      {
      continue;
      }
    connectivityArray->InsertNextValue(cell->GetNumberOfFaces());
    offset++;
    
    vtkIdType* ids = cell->GetFaces();
    ids++;    
    // for each face, one element to store number of points, then 
    for (vtkIdType j = 0; j < cell->GetNumberOfFaces(); j++)
      {
      vtkCell * face = cell->GetFace(j);
      connectivityArray->InsertNextValue(face->GetNumberOfPoints());
      offset++;
      ids++;
      for (vtkIdType k = 0; k < face->GetNumberOfPoints(); k++)
        {
        connectivityArray->InsertNextValue(*ids);
        offset++;
        ids++;
        }
      }
    
    offsetArray->InsertNextValue(offset);
    }
}

//----------------------------------------------------------------------------
vtkIdType vtkXMLPolyhedronMeshWriter::GetNumberOfInputFaces()
{
  vtkUnstructuredGrid* input = this->GetInput();
  vtkIdType numberOfFaces = 0;  
  for (vtkIdType i = 0; i < this->GetInput()->GetNumberOfCells(); i++)
    {
    vtkPolyhedron * cell = vtkPolyhedron::SafeDownCast(input->GetCell(i));
    if (cell)
      {
      numberOfFaces += cell->GetNumberOfFaces();
      }
    }
  
  return numberOfFaces;
}

//----------------------------------------------------------------------------
vtkIdType vtkXMLPolyhedronMeshWriter::GetSizeOfFaceConnectivityArray()
{
  vtkUnstructuredGrid* input = this->GetInput();
  vtkIdType connectSize = 0;
  
  // for each cell, one element to store number of faces
  for (vtkIdType i = 0; i < input->GetNumberOfCells(); i++)
    {
    vtkPolyhedron * cell = vtkPolyhedron::SafeDownCast(input->GetCell(i));
    if (!cell)
      {
      continue;
      }
    connectSize += 1;
    // for each face, one element to store number of points, then 
    for (vtkIdType j = 0; j < cell->GetNumberOfFaces(); j++)
      {
      vtkCell * face = cell->GetFace(j);
      connectSize += face->GetNumberOfPoints() + 1;
      }
    }
  
  return connectSize;
}

//----------------------------------------------------------------------------
void vtkXMLPolyhedronMeshWriter::CalculateSuperclassFraction(float* fractions)
{
  vtkUnstructuredGrid* input = this->GetInput();
  
  // The super-superclass will write point/cell data and point specifications.
  int pdArrays = input->GetPointData()->GetNumberOfArrays();
  int cdArrays = input->GetCellData()->GetNumberOfArrays();
  vtkIdType pdSize = pdArrays*this->GetNumberOfInputPoints();
  vtkIdType cdSize = cdArrays*this->GetNumberOfInputCells();
  vtkIdType pointsSize = this->GetNumberOfInputPoints();
  
  // The superclass will write cell specifications.
  vtkIdType cellConnectSize;
  if(input->GetCells()==0)
    {
    cellConnectSize=0;
    }
  else
    {
    cellConnectSize = (input->GetCells()->GetData()->GetNumberOfTuples() -
                      input->GetNumberOfCells());
    }
  vtkIdType cellOffsetSize = input->GetNumberOfCells();
  vtkIdType cellTypesSize = input->GetNumberOfCells();

  // This class will write faces specification
  vtkIdType faceConnectSize = 0;
  // for each cell
  for (vtkIdType i = 0; i < input->GetNumberOfCells(); i++)
    {
    vtkCell * cell = input->GetCell(i);
    if (!cell)
      {
      continue;
      }
    faceConnectSize += 1;
    // for each face
    for (vtkIdType j = 0; j < cell->GetNumberOfFaces(); j++)
      {
      vtkCell * face = cell->GetFace(j);
      if (!face)
        {
        continue;
        }
      faceConnectSize += face->GetNumberOfPoints() + 1;
      }
    }
  vtkIdType faceOffsetSize = this->GetNumberOfInputFaces();
  vtkIdType faceTypesSize = this->GetNumberOfInputFaces();
  
  vtkIdType parentTotal = pdSize + cdSize + pointsSize
                        + cellConnectSize + cellOffsetSize + cellTypesSize;

  vtkIdType total       = parentTotal 
                        + faceConnectSize + faceOffsetSize + faceTypesSize;
  if(total == 0)
    {
    total = 1;
    }
  
  fractions[0] = 0;
  fractions[1] = float(parentTotal)/total;
  fractions[2] = 1;
}

//----------------------------------------------------------------------------
void vtkXMLPolyhedronMeshWriter::
WriteFacesInline(const char* name, vtkUnstructuredGrid* input, vtkIndent indent)
{
  ostream& os = *(this->Stream);
  os << indent << "<" << name << ">\n";
  
  // Split progress by cell connectivity, offset, and type arrays.
  float progressRange[2] = {0,0};
  this->GetProgressRange(progressRange);
  float fractions[4];
  this->CalculateFaceFractions(fractions);

  vtkSmartPointer<vtkIdTypeArray> connectivityArray = 
    vtkSmartPointer<vtkIdTypeArray>::New();
  connectivityArray->SetName("connectivity");
  
  vtkSmartPointer<vtkIdTypeArray> offsetArray =
    vtkSmartPointer<vtkIdTypeArray>::New();
  offsetArray->SetName("offsets");
  
  this->ConstructArrays(connectivityArray, offsetArray);
  
  // Set the range of progress for the connectivity array.
  this->SetProgressRange(progressRange, 0, fractions);
  
  // Write the connectivity array.
  this->WriteArrayInline(connectivityArray, indent.GetNextIndent());
  if (this->ErrorCode == vtkErrorCode::OutOfDiskSpaceError)
    {
    return;
    }
  
  // Set the range of progress for the offsets array.
  this->SetProgressRange(progressRange, 1, fractions);
  
  // Write the offsets array.
  this->WriteArrayInline(offsetArray, indent.GetNextIndent());
  if (this->ErrorCode == vtkErrorCode::OutOfDiskSpaceError)
    {
    return;
    }
  
  os << indent << "</" << name << ">\n";
  os.flush();
  if (os.fail())
    {
    this->SetErrorCode(vtkErrorCode::OutOfDiskSpaceError);
    }
}


//----------------------------------------------------------------------------
void vtkXMLPolyhedronMeshWriter::CalculateFaceFractions(float* fractions)
{
  // Calculate the fraction of cell specification data contributed by
  // each of the connectivity, offset, and type arrays.
  vtkIdType connectSize = this->GetSizeOfFaceConnectivityArray();
  vtkIdType offsetSize = this->GetNumberOfInputFaces();
  vtkIdType total = connectSize+offsetSize;
  if(total == 0)
    {
    total = 1;
    }
  fractions[0] = 0;
  fractions[1] = float(connectSize)/total;
  fractions[2] = 1;
  fractions[3] = 1;
}

