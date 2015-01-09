/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLHyperOctreeReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

//TODO:
// Add support for timesteps
// Add streaming support.

#include "vtkXMLHyperOctreeReader.h"

#include "vtkDataArray.h"
#include "vtkObjectFactory.h"
#include "vtkHyperOctree.h"
#include "vtkHyperOctreeCursor.h"
#include "vtkXMLDataElement.h"
#include "vtkXMLDataParser.h"
#include "vtkInformation.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkIntArray.h"
#include "vtkFieldData.h"

vtkStandardNewMacro(vtkXMLHyperOctreeReader);

//----------------------------------------------------------------------------
vtkXMLHyperOctreeReader::vtkXMLHyperOctreeReader()
{
}

//----------------------------------------------------------------------------
vtkXMLHyperOctreeReader::~vtkXMLHyperOctreeReader()
{
}

//----------------------------------------------------------------------------
void vtkXMLHyperOctreeReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
vtkHyperOctree* vtkXMLHyperOctreeReader::GetOutput()
{
  return this->GetOutput(0);
}

//----------------------------------------------------------------------------
vtkHyperOctree* vtkXMLHyperOctreeReader::GetOutput(int idx)
{
  return vtkHyperOctree::SafeDownCast(this->GetOutputDataObject(idx));
}

//----------------------------------------------------------------------------
const char* vtkXMLHyperOctreeReader::GetDataSetName()
{
  return "HyperOctree";
}

//----------------------------------------------------------------------------
void vtkXMLHyperOctreeReader::SetupEmptyOutput()
{
  this->GetCurrentOutput()->Initialize();
}

//----------------------------------------------------------------------------
int vtkXMLHyperOctreeReader::FillOutputPortInformation(int, vtkInformation *info)
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkHyperOctree");
  return 1;
}

//----------------------------------------------------------------------------
vtkIdType vtkXMLHyperOctreeReader::GetNumberOfPoints()
{
  vtkIdType numPts = 0;
  vtkDataSet* output = vtkDataSet::SafeDownCast(this->GetCurrentOutput());
  if (output)
    {
    numPts = output->GetNumberOfPoints();
    }
  return numPts;
}

//----------------------------------------------------------------------------
vtkIdType vtkXMLHyperOctreeReader::GetNumberOfCells()
{
  vtkIdType numCells = 0;
  vtkDataSet* output = vtkDataSet::SafeDownCast(this->GetCurrentOutput());
  if (output)
    {
    numCells = output->GetNumberOfCells();
    }
  return numCells;
}

//----------------------------------------------------------------------------
int vtkXMLHyperOctreeReader::ReadArrayForPoints(vtkXMLDataElement* da,
                                                vtkAbstractArray* outArray)
{
  vtkIdType components = outArray->GetNumberOfComponents();
  vtkIdType numberOfTuples = this->GetNumberOfPoints();
  outArray->SetNumberOfTuples(numberOfTuples);
  return this->ReadArrayValues(da, 0, outArray, 0, numberOfTuples*components);
}

//----------------------------------------------------------------------------
int vtkXMLHyperOctreeReader::ReadArrayForCells(vtkXMLDataElement* da,
                                               vtkAbstractArray* outArray)
{
  vtkIdType components = outArray->GetNumberOfComponents();
  vtkIdType numberOfTuples = this->GetNumberOfCells();
  outArray->SetNumberOfTuples(numberOfTuples);
  return this->ReadArrayValues(da, 0, outArray, 0, numberOfTuples*components);

}

//----------------------------------------------------------------------------
void vtkXMLHyperOctreeReader::ReadXMLData()
{

  //1) vtkXMLReader grandparent class checks if this timestep needs to SetupOutputData, and if so initializes the output.
  //2) vtkXMLDataReader parent class reads fielddata
  this->Superclass::ReadXMLData();

  //3) For Other XML readers, vtkXMLUnstructuredDataReader or vtkXMLStructuredDataReader parent classes use pipeline info to determines what pieces to read and then ReadPieceData is called to read only part of the data. Since HyperOctree is not streamed yet, I'll just read the whole file here instead.

  vtkXMLDataElement *ePrimary =
    this->XMLParser->GetRootElement()->GetNestedElement(0);

  int dimension;
  double size[3];
  double origin[3];

  if (!ePrimary->GetScalarAttribute("Dimension", dimension))
    {
    dimension = 3;
    }

  if (ePrimary->GetVectorAttribute("Size", 3, size) != 3)
    {
    size[0] = 1;
    size[1] = 1;
    size[2] = 1;
    }

  if (ePrimary->GetVectorAttribute("Origin", 3, origin) != 3)
    {
    origin[0] = 0;
    origin[1] = 0;
    origin[2] = 0;
    }

  vtkHyperOctree *output = vtkHyperOctree::SafeDownCast(
      this->GetCurrentOutput());
  output->SetDimension(dimension);
  output->SetSize(size);
  output->SetOrigin(origin);

  // Find the topology element, which defines the structure of the HyperOctree
  // Rebuild the HyperOctree from that.
  // This needs to happen before ReadPieceData so that numPoints and numCells
  // will be defined.
  int numNested = ePrimary->GetNumberOfNestedElements();
  for (int i = 0; i < numNested; ++i)
    {
    vtkXMLDataElement* eNested = ePrimary->GetNestedElement(i);
    if (strcmp(eNested->GetName(), "Topology") == 0)
      {
      this->ReadTopology(eNested);
      break;
      }
    }

  //Read the pointdata and celldata attribute data.
  //We only have one piece so this will suffice.
  this->ReadPieceData();
}

//----------------------------------------------------------------------------
void vtkXMLHyperOctreeReader::ReadTopology(vtkXMLDataElement *elem)
{

  float progressRange[2] = { 0.f, 0.f };
  this->GetProgressRange(progressRange);
  //Part spent reading and reconstructing assumed to be roughly equal.
  float fractions[3] = { 0.f, 0.5f, 1.f };
  this->SetProgressRange(progressRange, 0, fractions);

  //Find the topology array and read it into a vtkIntArray
  int numNested = elem->GetNumberOfNestedElements();
  if (numNested != 1)
    {
    return;
    }

  vtkXMLDataElement* tElem = elem->GetNestedElement(0);

  // Since topology is a vtkIntArray.
  vtkAbstractArray* a = this->CreateArray(tElem);
  vtkDataArray *tda = vtkDataArray::SafeDownCast(a);
  if (!tda)
    {
    if (a)
      {
      a->Delete();
      }
    return;
    }

  int numTuples;
  if (!tElem->GetScalarAttribute("NumberOfTuples", numTuples))
    {
    tda->Delete();
    return;
    }

  tda->SetNumberOfTuples(numTuples);
  if (!this->ReadArrayValues(tElem, 0, tda, 0, numTuples* tda->GetNumberOfComponents())

   /* this->ReadData(tElem, tda->GetVoidPointer(0), tda->GetDataType(),
                      0, numTuples*tda->GetNumberOfComponents())*/)
    {
    tda->Delete();
    return;
    }

  vtkIntArray *ta = vtkIntArray::SafeDownCast(tda);
  if (!ta)
    {
    tda->Delete();
    return;
    }

  this->SetProgressRange(progressRange, 1, fractions);

  //Restore the topology from the vtkIntArray. Do it recursively, cell by cell.
  vtkHyperOctreeCursor *cursor=vtkHyperOctree::SafeDownCast(
    this->GetCurrentOutput())->NewCellCursor();
  cursor->ToRoot();
  //Where in the array we need to read from next.
  this->ArrayIndex = 0;
  if (!this->BuildNextCell(ta, cursor, cursor->GetNumberOfChildren()))
    {
    vtkErrorMacro( << "Problem reading topology. ");
    ta->Delete();
    return ;
    }

  //Cleanup
  cursor->Delete();
  ta->Delete();
}

//----------------------------------------------------------------------------
int vtkXMLHyperOctreeReader::BuildNextCell(
  vtkIntArray *ta, vtkHyperOctreeCursor *cursor, int nchildren)
{

  int nodeType = ta->GetValue(this->ArrayIndex);

  if (nodeType == 1)
    {
    //leaf, stop now
    }
/*
  else if (nodeType == 2)
    {
    //terminal node
    //subdivide but stop there
    vtkHyperOctree::SafeDownCast(this->GetCurrentOutput())->SubdivideLeaf(cursor);
    }
*/
  else
    {
    //internal node
    //subdivide
    vtkHyperOctree::SafeDownCast(this->GetCurrentOutput())->SubdivideLeaf(cursor);
    //then keep going down
    int i = 0;
    while (i < nchildren)
      {
      cursor->ToChild(i);

      this->ArrayIndex++;
      if (!this->BuildNextCell(ta, cursor, nchildren))
        {
        //IO failure somewhere below
        return 0;
        }

      cursor->ToParent();
      ++i;
      }
    }
  return 1;
}
