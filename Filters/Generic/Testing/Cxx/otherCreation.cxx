/*=========================================================================

  Program:   Visualization Toolkit
  Module:    otherCreation.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME
// .SECTION Description
// this program tests the creation of the BridgeDataSet

#include "vtkBridgeDataSet.h"
#include "vtkGenericAdaptorCell.h"
#include "vtkUnstructuredGrid.h"
#include "vtkMath.h"
#include "vtkIndent.h"
#include "vtkCellTypes.h"
#include "vtkGenericCellIterator.h"
#include "vtkGenericPointIterator.h"
#include "vtkGenericAttributeCollection.h"
#include "vtkTetra.h"
#include "vtkTriangle.h"
#include "vtkLine.h"
#include "vtkVertex.h"
#include "vtkDoubleArray.h"
#include "vtkPointData.h"
#include "vtkGenericAttribute.h"
#include <assert.h>
#include <string>
#include <vtksys/ios/sstream>

//-----------------------------------------------------------------------------
// Description:
// Display message for a test result and return the test value
int TestAssertion(ostream &strm,
                  vtkIndent indent,
                  const char *label,
                  int assertion);

int TestEmpty(ostream &strm);

//-----------------------------------------------------------------------------
// a dataset with points but no cells, and no pointdata and no celldata
int TestWithPoints(ostream &strm);

//-----------------------------------------------------------------------------
// a dataset with points and cells, and no pointdata and no celldata
int TestWithPointsAndCells(ostream &strm);

//-----------------------------------------------------------------------------
// a dataset with points and cells, pointdata but no celldata
int TestWithPointsAndCellsAndPointData(ostream &strm);

//-----------------------------------------------------------------------------
// Description:
// Display message for a test result and return the test value
int TestAssertion(ostream &strm,
                  vtkIndent indent,
                  const char *label,
                  int assertion)
{
  strm<<indent<<"Test `"<<label<<"\': ";
  if(assertion)
    {
    strm<<"passed."<<endl;
    }
  else
    {
    strm<<"FAILED!"<<endl;
    }
  return assertion;
}

//-----------------------------------------------------------------------------
// Description:
// Call TestAssertion() and return with 1 if it fails, do nothing oterwise.
// void TestAssertion(ostream &strm,
//                    vtkIndent indent,
//                    const *char label,
//                    int assertion);
#define MacroTest(strm,indent,label,assertion) if(!TestAssertion(strm,indent,label,assertion)) return 1

//-----------------------------------------------------------------------------
int TestEmpty(ostream &strm)
{
  vtkIndent indent;

  // actual test
  strm << "Test vtkBridgeDataSet Start" << endl;

  strm<<"Create an empty vtkUnstructuredGrid"<<endl;
  vtkUnstructuredGrid *g=vtkUnstructuredGrid::New();
  strm<<"Empty unstructured grid created"<<endl;

  strm<<"Create a vtkBridgeDataSet"<<endl;
  vtkBridgeDataSet *ds=vtkBridgeDataSet::New();
  strm<<"vtkBridgeDataSet created"<<endl;

  strm<<"Init the vtkBridgeDataSet with the empty unstructured grid"<<endl;
  ds->SetDataSet(g);
  strm<<"vtkBridgeDataSet initialized with the empty unstructured grid"<<endl;

  MacroTest(strm,indent,"number of points",ds->GetNumberOfPoints()==0);
  MacroTest(strm,indent,"number of cells -1",ds->GetNumberOfCells(-1)==0);
  MacroTest(strm,indent,"number of cells  0",ds->GetNumberOfCells(0)==0);
  MacroTest(strm,indent,"number of cells  1",ds->GetNumberOfCells(1)==0);
  MacroTest(strm,indent,"number of cells  2",ds->GetNumberOfCells(2)==0);
  MacroTest(strm,indent,"number of cells  3",ds->GetNumberOfCells(3)==0);
  MacroTest(strm,indent,"cell dimension",ds->GetCellDimension()==-1);

  strm<<"GetCellTypes() start"<<endl;
  vtkCellTypes *types=vtkCellTypes::New();
  ds->GetCellTypes(types);
  MacroTest(strm,indent,"cell types",types->GetNumberOfTypes()==0);
  types->Delete();
  strm<<"GetCellTypes() end"<<endl;

  strm<<"NewCellIterator() start"<<endl;
  vtkGenericCellIterator *it=ds->NewCellIterator(-1);
  MacroTest(strm,indent,"empty cell iterator -1 exists",it!=0);
  it->Begin();
  MacroTest(strm,indent,"empty cell iterator -1",it->IsAtEnd());
  it->Delete();
  it=ds->NewCellIterator(0);
  MacroTest(strm,indent,"empty cell iterator 0 exists",it!=0);
  it->Begin();
  MacroTest(strm,indent,"empty cell iterator 0",it->IsAtEnd());
  it->Delete();
  it=ds->NewCellIterator(1);
  MacroTest(strm,indent,"empty cell iterator 1 exists",it!=0);
  it->Begin();
  MacroTest(strm,indent,"empty cell iterator 1",it->IsAtEnd());
  it->Delete();
  it=ds->NewCellIterator(2);
  MacroTest(strm,indent,"empty cell iterator 2 exists",it!=0);
  it->Begin();
  MacroTest(strm,indent,"empty cell iterator 2",it->IsAtEnd());
  it->Delete();
  it=ds->NewCellIterator(3);
  MacroTest(strm,indent,"empty cell iterator 3 exists",it!=0);
  it->Begin();
  MacroTest(strm,indent,"empty cell iterator 3",it->IsAtEnd());
  it->Delete();
  strm<<"NewCellIterator() end"<<endl;

  strm<<"NewPointIterator() start"<<endl;
  vtkGenericPointIterator *pit=ds->NewPointIterator();
  MacroTest(strm,indent,"empty point iterator exists",pit!=0);
  pit->Begin();
  MacroTest(strm,indent,"empty point iterator",pit->IsAtEnd());
  pit->Delete();
  strm<<"NewPointIterator() end"<<endl;

  double bounds[6];
  double *b;
  double center[3];
  double *c;
  const double epsilon=0.000001; // 10^{-6}

  strm<<"GetBounds() start"<<endl;

  b=ds->GetBounds();
  MacroTest(strm,indent,"volatile bounds exist",b!=0);
  MacroTest(strm,indent,"default volatile bounds",!vtkMath::AreBoundsInitialized(b));

  ds->GetBounds(bounds);
  MacroTest(strm,indent,"default bounds",!vtkMath::AreBoundsInitialized(bounds));

  c=ds->GetCenter();
  MacroTest(strm,indent,"volatile center exists",c!=0);
  MacroTest(strm,indent,"default volatile center",(fabs(c[0])<epsilon)&&(fabs(c[1])<epsilon)&&(fabs(c[2])<epsilon));
  ds->GetCenter(center);
  MacroTest(strm,indent,"volatile center",(fabs(center[0])<epsilon)&&(fabs(center[1])<epsilon)&&(fabs(center[2])<epsilon));

  MacroTest(strm,indent,"diagonal length",fabs(ds->GetLength()-2*sqrt(3.0))<epsilon);

  strm<<"GetBounds() end"<<endl;

  vtkGenericAttributeCollection *attributes;
  attributes=ds->GetAttributes();
  MacroTest(strm,indent,"attributes exist",attributes!=0);
  MacroTest(strm,indent,"empty attributes",attributes->IsEmpty());
  MacroTest(strm,indent,"empty attributes",attributes->GetNumberOfAttributes()==0);
  MacroTest(strm,indent,"empty attributes",attributes->GetNumberOfComponents()==0);
  MacroTest(strm,indent,"empty attributes",attributes->GetMaxNumberOfComponents()==0);

#if 0
   strm<<"NewBoundaryIterator() start"<<endl;
  it=ds->NewBoundaryIterator(-1,0);
  MacroTest(strm,indent,"empty boundary iterator -1,false exists",it!=0);
  it->Begin();
  MacroTest(strm,indent,"empty boundary iterator -1,false",it->IsAtEnd());
  it->Delete();
  it=ds->NewBoundaryIterator(0,0);
  MacroTest(strm,indent,"empty boundary iterator 0,false exists",it!=0);
  it->Begin();
  MacroTest(strm,indent,"empty cell iterator 0,false",it->IsAtEnd());
  it->Delete();
  it=ds->NewBoundaryIterator(1,0);
  MacroTest(strm,indent,"empty boundary iterator 1,false exists",it!=0);
  it->Begin();
  MacroTest(strm,indent,"empty cell iterator 1,false",it->IsAtEnd());
  it->Delete();
  it=ds->NewBoundaryIterator(2,0);
  MacroTest(strm,indent,"empty boundary iterator 2,false exists",it!=0);
  it->Begin();
  MacroTest(strm,indent,"empty cell iterator 2,false",it->IsAtEnd());
  it->Delete();
  it=ds->NewBoundaryIterator(3,0);
  MacroTest(strm,indent,"empty boundary iterator 3,false exists",it!=0);
  it->Begin();
  MacroTest(strm,indent,"empty cell iterator 3,false",it->IsAtEnd());
  it->Delete();
  it=ds->NewBoundaryIterator(-1,1);
  MacroTest(strm,indent,"empty boundary iterator -1,true exists",it!=0);
  it->Begin();
  MacroTest(strm,indent,"empty boundary iterator -1,true",it->IsAtEnd());
  it->Delete();
  it=ds->NewBoundaryIterator(0,1);
  MacroTest(strm,indent,"empty boundary iterator 0,true exists",it!=0);
  it->Begin();
  MacroTest(strm,indent,"empty cell iterator 0,true",it->IsAtEnd());
  it->Delete();
  it=ds->NewBoundaryIterator(1,1);
  MacroTest(strm,indent,"empty boundary iterator 1,true exists",it!=0);
  it->Begin();
  MacroTest(strm,indent,"empty cell iterator 1,true",it->IsAtEnd());
  it->Delete();
  it=ds->NewBoundaryIterator(2,1);
  MacroTest(strm,indent,"empty boundary iterator 2,true exists",it!=0);
  it->Begin();
  MacroTest(strm,indent,"empty cell iterator 2,true",it->IsAtEnd());
  it->Delete();
  it=ds->NewBoundaryIterator(3,1);
  MacroTest(strm,indent,"empty boundary iterator 3,true exists",it!=0);
  it->Begin();
  MacroTest(strm,indent,"empty cell iterator 3,true",it->IsAtEnd());
  it->Delete();
  strm<<"NewBoundaryIterator() end"<<endl;
#endif
  strm<<"Delete the vtkBridgeDataSet"<<endl;
  ds->Delete();
  strm<<"vtkBridgeDataSet deleted"<<endl;

  strm<<"Delete the empty vtkUnstructuredGrid"<<endl;
  g->Delete();
  strm<<"Empty vtkUnstructuredGrid deleted"<<endl;

  strm << "Test vtkBridgeDataSet creation End" << endl;

  // Do the same thing for:
  // 1. a dataset with points but no cells, and no pointdata and no celldata
  // 2. a dataset with points and cells, and no pointdata and no celldata
  // 3. a dataset with points and cells, and pointdata but  no celldata
  // 4. a dataset with points and cells, and celldata but not pointdata
  // 5. a dataset with points and cells, and pointdata but celldata

  return 0;
}

//-----------------------------------------------------------------------------
// a dataset with points but no cells, and no pointdata and no celldata
int TestWithPoints(ostream &strm)
{
  vtkIndent indent;
  vtkPoints *pts;

  // actual test
  strm << "Test vtkBridgeDataSet Start" << endl;

  strm<<"Create an empty vtkUnstructuredGrid"<<endl;
  vtkUnstructuredGrid *g=vtkUnstructuredGrid::New();
  strm<<"Empty unstructured grid created"<<endl;

  pts=vtkPoints::New();
  pts->InsertNextPoint(-1,-2,-3);
  pts->InsertNextPoint(4,5,6);
  strm<<"Add points to the vtkUnstructuredGrid"<<endl;
  g->SetPoints(pts);
  strm<<"Points added to the vtkUnstructuredGrid"<<endl;

  strm<<"Create a vtkBridgeDataSet"<<endl;
  vtkBridgeDataSet *ds=vtkBridgeDataSet::New();
  strm<<"vtkBridgeDataSet created"<<endl;

  strm<<"Init the vtkBridgeDataSet with the unstructured grid"<<endl;
  ds->SetDataSet(g);
  strm<<"vtkBridgeDataSet initialized with the unstructured grid"<<endl;

  MacroTest(strm,indent,"number of points",ds->GetNumberOfPoints()==2);
  MacroTest(strm,indent,"number of cells -1",ds->GetNumberOfCells(-1)==0);
  MacroTest(strm,indent,"number of cells  0",ds->GetNumberOfCells(0)==0);
  MacroTest(strm,indent,"number of cells  1",ds->GetNumberOfCells(1)==0);
  MacroTest(strm,indent,"number of cells  2",ds->GetNumberOfCells(2)==0);
  MacroTest(strm,indent,"number of cells  3",ds->GetNumberOfCells(3)==0);
  MacroTest(strm,indent,"cell dimension",ds->GetCellDimension()==-1);

  strm<<"GetCellTypes() start"<<endl;
  vtkCellTypes *types=vtkCellTypes::New();
  ds->GetCellTypes(types);
  MacroTest(strm,indent,"cell types",types->GetNumberOfTypes()==0);
  types->Delete();
  strm<<"GetCellTypes() end"<<endl;

  strm<<"NewCellIterator() start"<<endl;
  vtkGenericCellIterator *it=ds->NewCellIterator(-1);
  MacroTest(strm,indent,"empty cell iterator -1 exists",it!=0);
  it->Begin();
  MacroTest(strm,indent,"empty cell iterator -1",it->IsAtEnd());
  it->Delete();
  it=ds->NewCellIterator(0);
  MacroTest(strm,indent,"empty cell iterator 0 exists",it!=0);
  it->Begin();
  MacroTest(strm,indent,"empty cell iterator 0",it->IsAtEnd());
  it->Delete();
  it=ds->NewCellIterator(1);
  MacroTest(strm,indent,"empty cell iterator 1 exists",it!=0);
  it->Begin();
  MacroTest(strm,indent,"empty cell iterator 1",it->IsAtEnd());
  it->Delete();
  it=ds->NewCellIterator(2);
  MacroTest(strm,indent,"empty cell iterator 2 exists",it!=0);
  it->Begin();
  MacroTest(strm,indent,"empty cell iterator 2",it->IsAtEnd());
  it->Delete();
  it=ds->NewCellIterator(3);
  MacroTest(strm,indent,"empty cell iterator 3 exists",it!=0);
  it->Begin();
  MacroTest(strm,indent,"empty cell iterator 3",it->IsAtEnd());
  it->Delete();
  strm<<"NewCellIterator() end"<<endl;

  double x[3];

  strm<<"NewPointIterator() start"<<endl;
  vtkGenericPointIterator *pit=ds->NewPointIterator();
  MacroTest(strm,indent,"point iterator exists",pit!=0);
  pit->Begin();
  MacroTest(strm,indent,"point iterator",!pit->IsAtEnd());
  pit->GetPosition(x);
  MacroTest(strm,indent,"point iterator",(x[0]==-1)&&(x[1]==-2)&&(x[2]==-3));
  MacroTest(strm,indent,"point iterator",pit->GetId()==0);
  pit->Next();
  MacroTest(strm,indent,"point iterator",!pit->IsAtEnd());
  pit->GetPosition(x);
  MacroTest(strm,indent,"point iterator",(x[0]==4)&&(x[1]==5)&&(x[2]==6));
  MacroTest(strm,indent,"point iterator",pit->GetId()==1);
  pit->Next();
  MacroTest(strm,indent,"point iterator",pit->IsAtEnd());
  pit->Delete();
  strm<<"NewPointIterator() end"<<endl;

  double bounds[6];
  double *b;
  double center[3];
  double *c;
  const double epsilon=0.000001; // 10^{-6}

  strm<<"GetBounds() start"<<endl;

  b=ds->GetBounds();
  MacroTest(strm,indent,"volatile bounds exist",b!=0);

  //strm<<"bounds=("<<b[0]<<','<<b[1]<<','<<b[2]<<','<<b[3]<<','<<b[4]<<','<<b[5]<<')'<<endl;

  MacroTest(strm,indent,"valid volatile bounds",(b[0]==-1)&&(b[1]==4)&&(b[2]==-2)&&(b[3]==5)&&(b[4]==-3)&&(b[5]==6));

  ds->GetBounds(bounds);
  MacroTest(strm,indent,"valid bounds",(bounds[0]==-1)&&(bounds[1]==4)&&(bounds[2]==-2)&&(bounds[3]==5)&&(bounds[4]==-3)&&(bounds[5]==6));

  c=ds->GetCenter();
  MacroTest(strm,indent,"volatile center exists",c!=0);
  MacroTest(strm,indent,"volatile center",(fabs(c[0]-1.5)<epsilon)&&(fabs(c[1]-1.5)<epsilon)&&(fabs(c[2]-1.5)<epsilon));
  ds->GetCenter(center);
  MacroTest(strm,indent,"valid center",(fabs(center[0]-1.5)<epsilon)&&(fabs(center[1]-1.5)<epsilon)&&(fabs(center[2]-1.5)<epsilon));
  MacroTest(strm,indent,"diagonal length",fabs(ds->GetLength()-sqrt(155.0))<epsilon);
  strm<<"GetBounds() end"<<endl;

  vtkGenericAttributeCollection *attributes;
  attributes=ds->GetAttributes();
  MacroTest(strm,indent,"attributes exist",attributes!=0);
  MacroTest(strm,indent,"empty attributes",attributes->IsEmpty());
  MacroTest(strm,indent,"empty attributes",attributes->GetNumberOfAttributes()==0);
  MacroTest(strm,indent,"empty attributes",attributes->GetNumberOfComponents()==0);
  MacroTest(strm,indent,"empty attributes",attributes->GetMaxNumberOfComponents()==0);

#if 0
   strm<<"NewBoundaryIterator() start"<<endl;
  it=ds->NewBoundaryIterator(-1,0);
  MacroTest(strm,indent,"empty boundary iterator -1,false exists",it!=0);
  it->Begin();
  MacroTest(strm,indent,"empty boundary iterator -1,false",it->IsAtEnd());
  it->Delete();
  it=ds->NewBoundaryIterator(0,0);
  MacroTest(strm,indent,"empty boundary iterator 0,false exists",it!=0);
  it->Begin();
  MacroTest(strm,indent,"empty cell iterator 0,false",it->IsAtEnd());
  it->Delete();
  it=ds->NewBoundaryIterator(1,0);
  MacroTest(strm,indent,"empty boundary iterator 1,false exists",it!=0);
  it->Begin();
  MacroTest(strm,indent,"empty cell iterator 1,false",it->IsAtEnd());
  it->Delete();
  it=ds->NewBoundaryIterator(2,0);
  MacroTest(strm,indent,"empty boundary iterator 2,false exists",it!=0);
  it->Begin();
  MacroTest(strm,indent,"empty cell iterator 2,false",it->IsAtEnd());
  it->Delete();
  it=ds->NewBoundaryIterator(3,0);
  MacroTest(strm,indent,"empty boundary iterator 3,false exists",it!=0);
  it->Begin();
  MacroTest(strm,indent,"empty cell iterator 3,false",it->IsAtEnd());
  it->Delete();
  it=ds->NewBoundaryIterator(-1,1);
  MacroTest(strm,indent,"empty boundary iterator -1,true exists",it!=0);
  it->Begin();
  MacroTest(strm,indent,"empty boundary iterator -1,true",it->IsAtEnd());
  it->Delete();
  it=ds->NewBoundaryIterator(0,1);
  MacroTest(strm,indent,"empty boundary iterator 0,true exists",it!=0);
  it->Begin();
  MacroTest(strm,indent,"empty cell iterator 0,true",it->IsAtEnd());
  it->Delete();
  it=ds->NewBoundaryIterator(1,1);
  MacroTest(strm,indent,"empty boundary iterator 1,true exists",it!=0);
  it->Begin();
  MacroTest(strm,indent,"empty cell iterator 1,true",it->IsAtEnd());
  it->Delete();
  it=ds->NewBoundaryIterator(2,1);
  MacroTest(strm,indent,"empty boundary iterator 2,true exists",it!=0);
  it->Begin();
  MacroTest(strm,indent,"empty cell iterator 2,true",it->IsAtEnd());
  it->Delete();
  it=ds->NewBoundaryIterator(3,1);
  MacroTest(strm,indent,"empty boundary iterator 3,true exists",it!=0);
  it->Begin();
  MacroTest(strm,indent,"empty cell iterator 3,true",it->IsAtEnd());
  it->Delete();
  strm<<"NewBoundaryIterator() end"<<endl;
#endif
  pts->Delete();

  strm<<"Delete the vtkBridgeDataSet"<<endl;
  ds->Delete();
  strm<<"vtkBridgeDataSet deleted"<<endl;

  strm<<"Delete the vtkUnstructuredGrid"<<endl;
  g->Delete();
  strm<<"vtkUnstructuredGrid deleted"<<endl;

  strm << "Test vtkBridgeDataSet creation End" << endl;

  // Do the same thing for:
  // 2. a dataset with points and cells, and no pointdata and no celldata
  // 3. a dataset with points and cells, and pointdata but  no celldata
  // 4. a dataset with points and cells, and celldata but not pointdata
  // 5. a dataset with points and cells, and pointdata but celldata

  return 0;
}

//-----------------------------------------------------------------------------
// a dataset with points and cells, and no pointdata and no celldata
int TestWithPointsAndCells(ostream &strm)
{
  vtkIndent indent;
  vtkPoints *pts;

  // actual test
  strm << "----------------------------------------------------------" << endl;
  strm << "TestWithPointsAndCells Start" << endl;
  strm << "----------------------------------------------------------" << endl;

  strm<<"Create an empty vtkUnstructuredGrid"<<endl;
  vtkUnstructuredGrid *g=vtkUnstructuredGrid::New();
  strm<<"Empty unstructured grid created"<<endl;

  pts=vtkPoints::New();
  pts->InsertNextPoint(0,0,0);   // 0
  pts->InsertNextPoint(1,-1,0);  // 1
  pts->InsertNextPoint(1,1,0);   // 2
  pts->InsertNextPoint(0.5,0,1); // 3

  pts->InsertNextPoint(2,-1,0);  // 4
  pts->InsertNextPoint(3,0,0);   // 5
  pts->InsertNextPoint(2,1,0);   // 6

  pts->InsertNextPoint(4,0,0);   // 7
  pts->InsertNextPoint(5,0,0);   // 8

  pts->InsertNextPoint(6,0,0);   // 9

  pts->InsertNextPoint(10,0,0);   // 10 0
  pts->InsertNextPoint(11,-1,0);  // 11 1,4
  pts->InsertNextPoint(11,1,0);   // 12 2,6
  pts->InsertNextPoint(10.5,0,1); // 13

  pts->InsertNextPoint(12,0,0);   // 14 // 5,7

  pts->InsertNextPoint(13,0,0);   // 15 // 8,9

  pts->InsertNextPoint(14,0,0);   // extra point


  strm<<"Add points to the vtkUnstructuredGrid"<<endl;
  g->SetPoints(pts);
  strm<<"Points added to the vtkUnstructuredGrid"<<endl;

  vtkTetra *tetra=vtkTetra::New();
  tetra->GetPointIds()->SetId(0,0);
  tetra->GetPointIds()->SetId(1,1);
  tetra->GetPointIds()->SetId(2,2);
  tetra->GetPointIds()->SetId(3,3);

  g->InsertNextCell(tetra->GetCellType(),tetra->GetPointIds());
  tetra->Delete();

  vtkTriangle *triangle=vtkTriangle::New();
  triangle->GetPointIds()->SetId(0,4);
  triangle->GetPointIds()->SetId(1,5);
  triangle->GetPointIds()->SetId(2,6);

  g->InsertNextCell(triangle->GetCellType(),triangle->GetPointIds());
  triangle->Delete();

  vtkLine *line=vtkLine::New();
  line->GetPointIds()->SetId(0,7);
  line->GetPointIds()->SetId(1,8);

  g->InsertNextCell(line->GetCellType(),line->GetPointIds());
  line->Delete();

  vtkVertex *vertex=vtkVertex::New();
  vertex->GetPointIds()->SetId(0,9);

  g->InsertNextCell(vertex->GetCellType(),vertex->GetPointIds());
  vertex->Delete();

  tetra=vtkTetra::New();
  tetra->GetPointIds()->SetId(0,10);
  tetra->GetPointIds()->SetId(1,11);
  tetra->GetPointIds()->SetId(2,12);
  tetra->GetPointIds()->SetId(3,13);

  g->InsertNextCell(tetra->GetCellType(),tetra->GetPointIds());
  tetra->Delete();

  triangle=vtkTriangle::New();
  triangle->GetPointIds()->SetId(0,11);
  triangle->GetPointIds()->SetId(1,14);
  triangle->GetPointIds()->SetId(2,12);

  g->InsertNextCell(triangle->GetCellType(),triangle->GetPointIds());
  triangle->Delete();

  line=vtkLine::New();
  line->GetPointIds()->SetId(0,14);
  line->GetPointIds()->SetId(1,15);

  g->InsertNextCell(line->GetCellType(),line->GetPointIds());
  line->Delete();

  vertex=vtkVertex::New();
  vertex->GetPointIds()->SetId(0,15);

  g->InsertNextCell(vertex->GetCellType(),vertex->GetPointIds());
  vertex->Delete();

  strm<<"Create a vtkBridgeDataSet"<<endl;
  vtkBridgeDataSet *ds=vtkBridgeDataSet::New();
  strm<<"vtkBridgeDataSet created"<<endl;

  strm<<"Init the vtkBridgeDataSet with the unstructured grid"<<endl;
  ds->SetDataSet(g);
  strm<<"vtkBridgeDataSet initialized with the unstructured grid"<<endl;

  MacroTest(strm,indent,"number of points",ds->GetNumberOfPoints()==17);
  MacroTest(strm,indent,"number of cells -1",ds->GetNumberOfCells(-1)==8);
  MacroTest(strm,indent,"number of cells  0",ds->GetNumberOfCells(0)==2);
  MacroTest(strm,indent,"number of cells  1",ds->GetNumberOfCells(1)==2);
  MacroTest(strm,indent,"number of cells  2",ds->GetNumberOfCells(2)==2);
  MacroTest(strm,indent,"number of cells  3",ds->GetNumberOfCells(3)==2);
  MacroTest(strm,indent,"cell dimension",ds->GetCellDimension()==-1);

  strm<<"GetCellTypes() start"<<endl;
  vtkCellTypes *types=vtkCellTypes::New();
  ds->GetCellTypes(types);
  MacroTest(strm,indent,"cell types",types->GetNumberOfTypes()==4);
  types->Delete();
  strm<<"GetCellTypes() end"<<endl;

  strm<<"NewCellIterator() start"<<endl;

  int itNum=-1;
  int itCount=4;
  int i;
  int count;
  std::string s;
  vtksys_ios::ostringstream ost;
  vtkGenericAdaptorCell *cab=0;

  while(itNum<itCount)
    {
    vtkGenericCellIterator *it=ds->NewCellIterator(itNum);
    ost << "empty cell iterator " << itNum << " exists";
    s=ost.str();
    const char *cstring=s.c_str();
    MacroTest(strm,indent,cstring,it!=0);
    it->Begin();
    i=0;
    count=ds->GetNumberOfCells(itNum);
    while(i<count)
      {
      ost.str("");
      ost<<"not finished cell iterator "<<itNum;
      s=ost.str();
      cstring=s.c_str();
      MacroTest(strm,indent,cstring,!it->IsAtEnd());
      ++i;
      cab=it->GetCell();
      MacroTest(strm,indent,"cell at iterator position is set",cab!=0);
      it->Next();
      }
    ost.str("");
    ost<<"Finished cell iterator "<<itNum;
    s=ost.str();
    cstring=s.c_str();
    MacroTest(strm,indent,cstring,it->IsAtEnd());
    it->Delete();
    ++itNum;
    }
  strm<<"NewCellIterator() end"<<endl;

  double x[3];
  double y[3];

  strm<<"NewPointIterator() start"<<endl;
  vtkGenericPointIterator *pit=ds->NewPointIterator();
  MacroTest(strm,indent,"point iterator exists",pit!=0);
  pit->Begin();

  i=0;
  count=ds->GetNumberOfPoints();
  while(i<count)
    {
    MacroTest(strm,indent,"not finished point iterator",!pit->IsAtEnd());
    pit->GetPosition(x);
    pts->GetPoint(i,y);
    MacroTest(strm,indent,"point iterator position",(x[0]==y[0])&&(x[1]==y[1])&&(x[2]==y[2]));
    MacroTest(strm,indent,"point iterator id",pit->GetId()==i);
    ++i;
    pit->Next();
    }
  pit->Delete();
  strm<<"NewPointIterator() end"<<endl;


  strm<<" cell::GetPointIterator() start"<<endl;
  vtkGenericCellIterator *it=ds->NewCellIterator(-1);
  it->Begin();
  count=0;
  pit=ds->NewPointIterator();
  int count2=0;
  while(!it->IsAtEnd())
    {
    cab=it->GetCell();
    cab->GetPointIterator(pit);
    pit->Begin();
    switch(count)
      {
      case 0: // tetra
        count2=0;
        while(!pit->IsAtEnd())
          {
          MacroTest(strm,indent,"point iterator id",pit->GetId()==count2);
          pit->GetPosition(x);
          pts->GetPoint(pit->GetId(),y);
          MacroTest(strm,indent,"point iterator position",(x[0]==y[0])&&(x[1]==y[1])&&(x[2]==y[2]));
          pit->Next();

          count2++;
          }
        break;
      case 1: // triangle
        count2=4;
        while(!pit->IsAtEnd())
          {
          MacroTest(strm,indent,"point iterator id",pit->GetId()==count2);
          pit->GetPosition(x);
          pts->GetPoint(pit->GetId(),y);
          MacroTest(strm,indent,"point iterator position",(x[0]==y[0])&&(x[1]==y[1])&&(x[2]==y[2]));
          pit->Next();

          count2++;
          }
        break;
      case 2: // line
        count2=7;
        while(!pit->IsAtEnd())
          {
          MacroTest(strm,indent,"point iterator id",pit->GetId()==count2);
          pit->GetPosition(x);
          pts->GetPoint(pit->GetId(),y);
          MacroTest(strm,indent,"point iterator position",(x[0]==y[0])&&(x[1]==y[1])&&(x[2]==y[2]));
          pit->Next();

          count2++;
          }
        break;
      case 3: // vertex
        count2=9;
        while(!pit->IsAtEnd())
          {
          MacroTest(strm,indent,"point iterator id",pit->GetId()==count2);
          pit->GetPosition(x);
          pts->GetPoint(pit->GetId(),y);
          MacroTest(strm,indent,"point iterator position",(x[0]==y[0])&&(x[1]==y[1])&&(x[2]==y[2]));
          pit->Next();

          count2++;
          }
        break;
      case 4: // tetra
        count2=10;
        while(!pit->IsAtEnd())
          {
          MacroTest(strm,indent,"point iterator id",pit->GetId()==count2);
          pit->GetPosition(x);
          pts->GetPoint(pit->GetId(),y);
          MacroTest(strm,indent,"point iterator position",(x[0]==y[0])&&(x[1]==y[1])&&(x[2]==y[2]));
          pit->Next();

          count2++;
          }
        break;
      case 5: // triangle
        count2=0;
        while(!pit->IsAtEnd())
          {
          switch(count2)
            {
            case 0:
              MacroTest(strm,indent,"point iterator id",pit->GetId()==11);
              break;
            case 1:
              MacroTest(strm,indent,"point iterator id",pit->GetId()==14);
              break;
            case 2:
              MacroTest(strm,indent,"point iterator id",pit->GetId()==12);
              break;
            default:
              MacroTest(strm,indent,"impossible case",0);
              break;
            }
          pit->GetPosition(x);
          pts->GetPoint(pit->GetId(),y);
          MacroTest(strm,indent,"point iterator position",(x[0]==y[0])&&(x[1]==y[1])&&(x[2]==y[2]));
          pit->Next();

          count2++;
          }
        break;
      case 6: // line
        count2=14;
        while(!pit->IsAtEnd())
          {
          MacroTest(strm,indent,"point iterator id",pit->GetId()==count2);
          pit->GetPosition(x);
          pts->GetPoint(pit->GetId(),y);
          MacroTest(strm,indent,"point iterator position",(x[0]==y[0])&&(x[1]==y[1])&&(x[2]==y[2]));
          pit->Next();

          count2++;
          }
        break;
      case 7: // vertex
        count2=15;
        while(!pit->IsAtEnd())
          {
          MacroTest(strm,indent,"point iterator id",pit->GetId()==count2);
          pit->GetPosition(x);
          pts->GetPoint(pit->GetId(),y);
          MacroTest(strm,indent,"point iterator position",(x[0]==y[0])&&(x[1]==y[1])&&(x[2]==y[2]));
          pit->Next();

          count2++;
          }
        break;
      default:
        MacroTest(strm,indent,"impossible case",0);
        break;
      }
    ++count;
    it->Next();
    }
  pit->Delete();
  it->Delete();
  strm<<" cell::GetPointIterator() end"<<endl;


  double bounds[6];
  double *b;
  double center[3];
  double *c;
  const double epsilon=0.000001; // 10^{-6}

  strm<<"GetBounds() start"<<endl;

  b=ds->GetBounds();
  MacroTest(strm,indent,"volatile bounds exist",b!=0);

  strm<<"bounds=("<<b[0]<<','<<b[1]<<','<<b[2]<<','<<b[3]<<','<<b[4]<<','<<b[5]<<')'<<endl;

  MacroTest(strm,indent,"valid volatile bounds",(b[0]==0)&&(b[1]==14)&&(b[2]==-1)&&(b[3]==1)&&(b[4]==0)&&(b[5]==1));

  ds->GetBounds(bounds);
  MacroTest(strm,indent,"valid bounds",(bounds[0]==0)&&(bounds[1]==14)&&(bounds[2]==-1)&&(bounds[3]==1)&&(bounds[4]==0)&&(bounds[5]==1));

  c=ds->GetCenter();
  MacroTest(strm,indent,"volatile center exists",c!=0);
  MacroTest(strm,indent,"volatile center",(fabs(c[0]-7)<epsilon)&&(fabs(c[1])<epsilon)&&(fabs(c[2]-0.5)<epsilon));
  ds->GetCenter(center);
  MacroTest(strm,indent,"valid center",(fabs(center[0]-7)<epsilon)&&(fabs(center[1])<epsilon)&&(fabs(center[2]-0.5)<epsilon));
  MacroTest(strm,indent,"diagonal length",fabs(ds->GetLength()-sqrt(201.0))<epsilon);
  strm<<"GetBounds() end"<<endl;

  vtkGenericAttributeCollection *attributes=0;
  attributes=ds->GetAttributes();
  MacroTest(strm,indent,"attributes exist",attributes!=0);
  MacroTest(strm,indent,"empty attributes",attributes->IsEmpty());
  MacroTest(strm,indent,"empty attributes",attributes->GetNumberOfAttributes()==0);
  MacroTest(strm,indent,"empty attributes",attributes->GetNumberOfComponents()==0);
  MacroTest(strm,indent,"empty attributes",attributes->GetMaxNumberOfComponents()==0);

  strm<<"vtkBridgeCell::GetBoundaryIterator() test start"<<endl;

  // iterate over dataset cell
  // for each cell, get the boundaries of each dimension less than the cell
  // dimension


//  int i;
//  int count;
//  std::string s;
//  vtkOStrStreamWrapper *ost=0;
//  vtkGenericAdaptorCell *cab=0;

  int dim;

  it=ds->NewCellIterator(-1);
  MacroTest(strm,indent,"cell iterator on all data set cells exists" ,it!=0);

  it->Begin();

  vtkGenericCellIterator *boundaries=ds->NewCellIterator(-1); // just for creation
  MacroTest(strm,indent,"boundaries exists" ,boundaries!=0);

  i=0;
  count=ds->GetNumberOfCells(-1);

  vtkGenericAdaptorCell *cab2;

  while(i<count)
    {
    MacroTest(strm,indent,"not finished cell iterator",!it->IsAtEnd());
    cab=it->GetCell();
    dim=cab->GetDimension();

    int currentDim=dim-1;

    while(currentDim>=-1)
      {
      cab->GetBoundaryIterator(boundaries,currentDim);
      boundaries->Begin();
      while(!boundaries->IsAtEnd())
        {
        cab2=boundaries->GetCell();
        MacroTest(strm,indent,"the cell at iterator position is set",cab2!=0);
        boundaries->Next();
        }
      --currentDim;
      }
    ++i;
    it->Next();
    }
  boundaries->Delete();
  it->Delete();


  strm<<"vtkBridgeCell::GetBoundaryIterator() test end"<<endl;


#if 0
   strm<<"NewBoundaryIterator() start"<<endl;
  it=ds->NewBoundaryIterator(-1,0);
  MacroTest(strm,indent,"empty boundary iterator -1,false exists",it!=0);
  it->Begin();
  MacroTest(strm,indent,"empty boundary iterator -1,false",it->IsAtEnd());
  it->Delete();
  it=ds->NewBoundaryIterator(0,0);
  MacroTest(strm,indent,"empty boundary iterator 0,false exists",it!=0);
  it->Begin();
  MacroTest(strm,indent,"empty cell iterator 0,false",it->IsAtEnd());
  it->Delete();
  it=ds->NewBoundaryIterator(1,0);
  MacroTest(strm,indent,"empty boundary iterator 1,false exists",it!=0);
  it->Begin();
  MacroTest(strm,indent,"empty cell iterator 1,false",it->IsAtEnd());
  it->Delete();
  it=ds->NewBoundaryIterator(2,0);
  MacroTest(strm,indent,"empty boundary iterator 2,false exists",it!=0);
  it->Begin();
  MacroTest(strm,indent,"empty cell iterator 2,false",it->IsAtEnd());
  it->Delete();
  it=ds->NewBoundaryIterator(3,0);
  MacroTest(strm,indent,"empty boundary iterator 3,false exists",it!=0);
  it->Begin();
  MacroTest(strm,indent,"empty cell iterator 3,false",it->IsAtEnd());
  it->Delete();
  it=ds->NewBoundaryIterator(-1,1);
  MacroTest(strm,indent,"empty boundary iterator -1,true exists",it!=0);
  it->Begin();
  MacroTest(strm,indent,"empty boundary iterator -1,true",it->IsAtEnd());
  it->Delete();
  it=ds->NewBoundaryIterator(0,1);
  MacroTest(strm,indent,"empty boundary iterator 0,true exists",it!=0);
  it->Begin();
  MacroTest(strm,indent,"empty cell iterator 0,true",it->IsAtEnd());
  it->Delete();
  it=ds->NewBoundaryIterator(1,1);
  MacroTest(strm,indent,"empty boundary iterator 1,true exists",it!=0);
  it->Begin();
  MacroTest(strm,indent,"empty cell iterator 1,true",it->IsAtEnd());
  it->Delete();
  it=ds->NewBoundaryIterator(2,1);
  MacroTest(strm,indent,"empty boundary iterator 2,true exists",it!=0);
  it->Begin();
  MacroTest(strm,indent,"empty cell iterator 2,true",it->IsAtEnd());
  it->Delete();
  it=ds->NewBoundaryIterator(3,1);
  MacroTest(strm,indent,"empty boundary iterator 3,true exists",it!=0);
  it->Begin();
  MacroTest(strm,indent,"empty cell iterator 3,true",it->IsAtEnd());
  it->Delete();
  strm<<"NewBoundaryIterator() end"<<endl;
#endif

  pts->Delete();

  strm<<"Delete the vtkBridgeDataSet"<<endl;
  ds->Delete();
  strm<<"vtkBridgeDataSet deleted"<<endl;

  strm<<"Delete the vtkUnstructuredGrid"<<endl;
  g->Delete();
  strm<<"vtkUnstructuredGrid deleted"<<endl;

  strm << "Test vtkBridgeDataSet creation End" << endl;

  // Do the same thing for:
  // 3. a dataset with points and cells, and pointdata but  no celldata
  // 4. a dataset with points and cells, and celldata but not pointdata
  // 5. a dataset with points and cells, and pointdata but celldata

  return 0;
}

//-----------------------------------------------------------------------------
// a dataset with points and cells, pointdata but no celldata
int TestWithPointsAndCellsAndPointData(ostream &strm)
{
  vtkIndent indent;
  vtkPoints *pts;

  // actual test
  strm << "----------------------------------------------------------" << endl;
  strm << "TestWithPointsAndCellsAndPointData Start" << endl;
  strm << "----------------------------------------------------------" << endl;

  strm<<"Create an empty vtkUnstructuredGrid"<<endl;
  vtkUnstructuredGrid *g=vtkUnstructuredGrid::New();
  strm<<"Empty unstructured grid created"<<endl;

  pts=vtkPoints::New();
  pts->InsertNextPoint(0,0,0);   // 0
  pts->InsertNextPoint(1,-1,0);  // 1
  pts->InsertNextPoint(1,1,0);   // 2
  pts->InsertNextPoint(0.5,0,1); // 3

  pts->InsertNextPoint(2,-1,0);  // 4
  pts->InsertNextPoint(3,0,0);   // 5
  pts->InsertNextPoint(2,1,0);   // 6

  pts->InsertNextPoint(4,0,0);   // 7
  pts->InsertNextPoint(5,0,0);   // 8

  pts->InsertNextPoint(6,0,0);   // 9

  pts->InsertNextPoint(10,0,0);   // 10 0
  pts->InsertNextPoint(11,-1,0);  // 11 1,4
  pts->InsertNextPoint(11,1,0);   // 12 2,6
  pts->InsertNextPoint(10.5,0,1); // 13

  pts->InsertNextPoint(12,0,0);   // 14 // 5,7

  pts->InsertNextPoint(13,0,0);   // 15 // 8,9

  pts->InsertNextPoint(14,0,0);   // extra point


  strm<<"Add points to the vtkUnstructuredGrid"<<endl;
  g->SetPoints(pts);
  strm<<"Points added to the vtkUnstructuredGrid"<<endl;

  vtkTetra *tetra=vtkTetra::New();
  tetra->GetPointIds()->SetId(0,0);
  tetra->GetPointIds()->SetId(1,1);
  tetra->GetPointIds()->SetId(2,2);
  tetra->GetPointIds()->SetId(3,3);

  g->InsertNextCell(tetra->GetCellType(),tetra->GetPointIds());
  tetra->Delete();

  vtkTriangle *triangle=vtkTriangle::New();
  triangle->GetPointIds()->SetId(0,4);
  triangle->GetPointIds()->SetId(1,5);
  triangle->GetPointIds()->SetId(2,6);

  g->InsertNextCell(triangle->GetCellType(),triangle->GetPointIds());
  triangle->Delete();

  vtkLine *line=vtkLine::New();
  line->GetPointIds()->SetId(0,7);
  line->GetPointIds()->SetId(1,8);

  g->InsertNextCell(line->GetCellType(),line->GetPointIds());
  line->Delete();

  vtkVertex *vertex=vtkVertex::New();
  vertex->GetPointIds()->SetId(0,9);

  g->InsertNextCell(vertex->GetCellType(),vertex->GetPointIds());
  vertex->Delete();

  tetra=vtkTetra::New();
  tetra->GetPointIds()->SetId(0,10);
  tetra->GetPointIds()->SetId(1,11);
  tetra->GetPointIds()->SetId(2,12);
  tetra->GetPointIds()->SetId(3,13);

  g->InsertNextCell(tetra->GetCellType(),tetra->GetPointIds());
  tetra->Delete();

  triangle=vtkTriangle::New();
  triangle->GetPointIds()->SetId(0,11);
  triangle->GetPointIds()->SetId(1,14);
  triangle->GetPointIds()->SetId(2,12);

  g->InsertNextCell(triangle->GetCellType(),triangle->GetPointIds());
  triangle->Delete();

  line=vtkLine::New();
  line->GetPointIds()->SetId(0,14);
  line->GetPointIds()->SetId(1,15);

  g->InsertNextCell(line->GetCellType(),line->GetPointIds());
  line->Delete();

  vertex=vtkVertex::New();
  vertex->GetPointIds()->SetId(0,15);

  g->InsertNextCell(vertex->GetCellType(),vertex->GetPointIds());
  vertex->Delete();

  strm<<"Add point data to the vtkUnstructuredGrid"<<endl;
  int m=0;
  vtkDoubleArray *attrib= vtkDoubleArray::New();
  while(m<17)
    {
    attrib->InsertNextValue(m+100);
    ++m;
    }

  assert(g->GetPointData()!=0);
  g->GetPointData()->SetScalars(attrib);
  attrib->Delete();
  attrib=0;
  strm<<"Point data added to the vtkUnstructuredGrid"<<endl;


  strm<<"Create a vtkBridgeDataSet"<<endl;
  vtkBridgeDataSet *ds=vtkBridgeDataSet::New();
  strm<<"vtkBridgeDataSet created"<<endl;

  strm<<"Init the vtkBridgeDataSet with the unstructured grid"<<endl;
  ds->SetDataSet(g);
  strm<<"vtkBridgeDataSet initialized with the unstructured grid"<<endl;

  MacroTest(strm,indent,"number of points",ds->GetNumberOfPoints()==17);
  MacroTest(strm,indent,"number of cells -1",ds->GetNumberOfCells(-1)==8);
  MacroTest(strm,indent,"number of cells  0",ds->GetNumberOfCells(0)==2);
  MacroTest(strm,indent,"number of cells  1",ds->GetNumberOfCells(1)==2);
  MacroTest(strm,indent,"number of cells  2",ds->GetNumberOfCells(2)==2);
  MacroTest(strm,indent,"number of cells  3",ds->GetNumberOfCells(3)==2);
  MacroTest(strm,indent,"cell dimension",ds->GetCellDimension()==-1);

  strm<<"GetCellTypes() start"<<endl;
  vtkCellTypes *types=vtkCellTypes::New();
  ds->GetCellTypes(types);
  MacroTest(strm,indent,"cell types",types->GetNumberOfTypes()==4);
  types->Delete();
  strm<<"GetCellTypes() end"<<endl;

  strm<<"NewCellIterator() start"<<endl;

  int itNum=-1;
  int itCount=4;
  int i;
  int count;
  std::string s;
  vtksys_ios::ostringstream ost;
  vtkGenericAdaptorCell *cab;

  while(itNum<itCount)
    {
    vtkGenericCellIterator *it=ds->NewCellIterator(itNum);
    ost<<"empty cell iterator "<<itNum<<" exists";
    s=ost.str();
    const char *cstring=s.c_str();
    MacroTest(strm,indent,cstring,it!=0);
    it->Begin();
    i=0;
    count=ds->GetNumberOfCells(itNum);
    while(i<count)
      {
      ost.str("");
      ost<<"not finished cell iterator "<<itNum;
      s=ost.str();
      cstring=s.c_str();
      MacroTest(strm,indent,cstring,!it->IsAtEnd());
      ++i;
      cab=it->GetCell();
      MacroTest(strm,indent,"cell at current position is set",cab!=0);
      it->Next();
      }
    ost.str("");
    ost<<"Finished cell iterator "<<itNum;
    s=ost.str();
    cstring=s.c_str();
    MacroTest(strm,indent,cstring,it->IsAtEnd());
    it->Delete();
    ++itNum;
    }
  strm<<"NewCellIterator() end"<<endl;

  double x[3];
  double y[3];

  strm<<"NewPointIterator() start"<<endl;
  vtkGenericPointIterator *pit=ds->NewPointIterator();
  MacroTest(strm,indent,"point iterator exists",pit!=0);
  pit->Begin();

  i=0;
  count=ds->GetNumberOfPoints();
  while(i<count)
    {
    MacroTest(strm,indent,"not finished point iterator",!pit->IsAtEnd());
    pit->GetPosition(x);
    pts->GetPoint(i,y);
    MacroTest(strm,indent,"point iterator position",(x[0]==y[0])&&(x[1]==y[1])&&(x[2]==y[2]));
    MacroTest(strm,indent,"point iterator id",pit->GetId()==i);
    ++i;
    pit->Next();
    }
  pit->Delete();
  strm<<"NewPointIterator() end"<<endl;

  double bounds[6];
  double *b;
  double center[3];
  double *c=0;
  const double epsilon=0.000001; // 10^{-6}

  strm<<"GetBounds() start"<<endl;

  b=ds->GetBounds();
  MacroTest(strm,indent,"volatile bounds exist",b!=0);

  strm<<"bounds=("<<b[0]<<','<<b[1]<<','<<b[2]<<','<<b[3]<<','<<b[4]<<','<<b[5]<<')'<<endl;

  MacroTest(strm,indent,"valid volatile bounds",(b[0]==0)&&(b[1]==14)&&(b[2]==-1)&&(b[3]==1)&&(b[4]==0)&&(b[5]==1));

  ds->GetBounds(bounds);
  MacroTest(strm,indent,"valid bounds",(bounds[0]==0)&&(bounds[1]==14)&&(bounds[2]==-1)&&(bounds[3]==1)&&(bounds[4]==0)&&(bounds[5]==1));

  c=ds->GetCenter();
  MacroTest(strm,indent,"volatile center exists",c!=0);
  MacroTest(strm,indent,"volatile center",(fabs(c[0]-7)<epsilon)&&(fabs(c[1])<epsilon)&&(fabs(c[2]-0.5)<epsilon));
  ds->GetCenter(center);
  MacroTest(strm,indent,"valid center",(fabs(center[0]-7)<epsilon)&&(fabs(center[1])<epsilon)&&(fabs(center[2]-0.5)<epsilon));
  MacroTest(strm,indent,"diagonal length",fabs(ds->GetLength()-sqrt(201.0))<epsilon);
  strm<<"GetBounds() end"<<endl;

  vtkGenericAttributeCollection *attributes=0;
  attributes=ds->GetAttributes();
  MacroTest(strm,indent,"attributes exist",attributes!=0);
  MacroTest(strm,indent,"not empty attributes",!attributes->IsEmpty());
  MacroTest(strm,indent,"one attribute",attributes->GetNumberOfAttributes()==1);
  MacroTest(strm,indent,"one scalar attribute",attributes->GetNumberOfComponents()==1);
  MacroTest(strm,indent,"one scalar attribute",attributes->GetMaxNumberOfComponents()==1);

  vtkGenericAttribute *attribute=0;
  attribute=attributes->GetAttribute(0);
  MacroTest(strm,indent,"attribute exists",attribute!=0);

  MacroTest(strm,indent,"attribute name does not exist",attribute->GetName()==0);


  int attribId;
  attribId=attributes->FindAttribute("");
  MacroTest(strm,indent,"attribute not found",attribId==-1);

  g->GetPointData()->GetScalars()->SetName("pressure");
  attribId=attributes->FindAttribute("pressure");
  strm<<"attribId="<<attribId<<endl;

  MacroTest(strm,indent,"attribute found",attribId==0);

  MacroTest(strm,indent,"attribute name exists",attribute->GetName()!=0);
  MacroTest(strm,indent,"valid attribute name",strcmp(attribute->GetName(),"pressure")==0);

  MacroTest(strm,indent,"attribute components",attribute->GetNumberOfComponents()==1);
  MacroTest(strm,indent,"attribute centering",attribute->GetCentering()==vtkPointCentered);
  MacroTest(strm,indent,"attribute type",attribute->GetComponentType()==VTK_DOUBLE);
  MacroTest(strm,indent,"attribute size",attribute->GetSize()==17);

  double *range=attribute->GetRange(0);
  double myRange[2];
  attribute->GetRange(0,myRange);

  MacroTest(strm,indent,"attribute component lower boundary",range[0]==100);
  MacroTest(strm,indent,"attribute component upper boundary",range[1]==116);
  MacroTest(strm,indent,"attribute component lower boundary",myRange[0]==100);
  MacroTest(strm,indent,"attribute component upper boundary",myRange[1]==116);

  MacroTest(strm,indent,"attribute max norm",fabs(attribute->GetMaxNorm()-116)<0.0001);




  strm<<"vtkBridgeCell::GetBoundaryIterator() test start"<<endl;

  // iterate over dataset cell
  // for each cell, get the boundaries of each dimension less than the cell
  // dimension


//  int i;
//  int count;
//  std::string s;
//  vtkOStrStreamWrapper *ost=0;
//  vtkGenericAdaptorCell *cab=0;

  int dim;

  vtkGenericCellIterator *it=ds->NewCellIterator(-1);
  MacroTest(strm,indent,"cell iterator on all data set cells exists" ,it!=0);

  it->Begin();

  vtkGenericCellIterator *boundaries=ds->NewCellIterator(-1); // just for creation
  MacroTest(strm,indent,"boundaries exists" ,boundaries!=0);

  i=0;
  count=ds->GetNumberOfCells(-1);

  vtkGenericAdaptorCell *cab2;

  while(i<count)
    {
    MacroTest(strm,indent,"not finished cell iterator",!it->IsAtEnd());
    cab=it->GetCell();
    dim=cab->GetDimension();

    int currentDim=dim-1;

    while(currentDim>=-1)
      {
      cab->GetBoundaryIterator(boundaries,currentDim);
      boundaries->Begin();
      while(!boundaries->IsAtEnd())
        {
        cab2=boundaries->GetCell();
        MacroTest(strm,indent,"the cell at iterator position is set",cab2!=0);
        boundaries->Next();
        }
      --currentDim;
      }
    ++i;
    it->Next();
    }
  boundaries->Delete();
  it->Delete();


  strm<<"vtkBridgeCell::GetBoundaryIterator() test end"<<endl;

   // Description:
  // Attribute at all points of cell `c'.
  // \pre c_exists: c!=0
  // \pre c_valid: !c->IsAtEnd()
  // \post result_exists: result!=0
  // \post valid_result: sizeof(result)==GetNumberOfComponents()*c->GetCell()->GetNumberOfPoints()

  strm<<"GetTuple() on cell iterator start"<<endl;
  it=ds->NewCellIterator(-1);

  // tetra1
  it->Begin();

  double *tuples=attribute->GetTuple(it);
  double myTuples[4];

  MacroTest(strm,indent,"tetra1, pt0",tuples[0]==100);
  MacroTest(strm,indent,"tetra1, pt1",tuples[1]==101);
  MacroTest(strm,indent,"tetra1, pt2",tuples[2]==102);
  MacroTest(strm,indent,"tetra1, pt3",tuples[3]==103);

  attribute->GetTuple(it,myTuples);
  MacroTest(strm,indent,"tetra1, pt0",myTuples[0]==100);
  MacroTest(strm,indent,"tetra1, pt1",myTuples[1]==101);
  MacroTest(strm,indent,"tetra1, pt2",myTuples[2]==102);
  MacroTest(strm,indent,"tetra1, pt3",myTuples[3]==103);

  // triangle1
  it->Next();

  tuples=attribute->GetTuple(it);

  MacroTest(strm,indent,"tri1, pt0",tuples[0]==104);
  MacroTest(strm,indent,"tri1, pt1",tuples[1]==105);
  MacroTest(strm,indent,"tri1, pt2",tuples[2]==106);

  attribute->GetTuple(it,myTuples);
  MacroTest(strm,indent,"tri1, pt0",myTuples[0]==104);
  MacroTest(strm,indent,"tri1, pt1",myTuples[1]==105);
  MacroTest(strm,indent,"tri1, pt2",myTuples[2]==106);

  // line1
  it->Next();

  tuples=attribute->GetTuple(it);

  MacroTest(strm,indent,"line1, pt0",tuples[0]==107);
  MacroTest(strm,indent,"line1, pt1",tuples[1]==108);

  attribute->GetTuple(it,myTuples);
  MacroTest(strm,indent,"line1, pt0",myTuples[0]==107);
  MacroTest(strm,indent,"line1, pt1",myTuples[1]==108);

  // vertex1
  it->Next();

  tuples=attribute->GetTuple(it);

  MacroTest(strm,indent,"vertex1, pt0",tuples[0]==109);

  attribute->GetTuple(it,myTuples);
  MacroTest(strm,indent,"vertex1, pt0",myTuples[0]==109);


   // tetra2
  it->Next();

  tuples=attribute->GetTuple(it);

  MacroTest(strm,indent,"tetra2, pt0",tuples[0]==110);
  MacroTest(strm,indent,"tetra2, pt1",tuples[1]==111);
  MacroTest(strm,indent,"tetra2, pt2",tuples[2]==112);
  MacroTest(strm,indent,"tetra2, pt3",tuples[3]==113);

  attribute->GetTuple(it,myTuples);
  MacroTest(strm,indent,"tetra2, pt0",myTuples[0]==110);
  MacroTest(strm,indent,"tetra2, pt1",myTuples[1]==111);
  MacroTest(strm,indent,"tetra2, pt2",myTuples[2]==112);
  MacroTest(strm,indent,"tetra2, pt3",myTuples[3]==113);

  // triangle2
  it->Next();

  tuples=attribute->GetTuple(it);

  MacroTest(strm,indent,"tri2, pt0",tuples[0]==111);
  MacroTest(strm,indent,"tri2, pt1",tuples[1]==114);
  MacroTest(strm,indent,"tri2, pt2",tuples[2]==112);

  attribute->GetTuple(it,myTuples);
  MacroTest(strm,indent,"tri2, pt0",myTuples[0]==111);
  MacroTest(strm,indent,"tri2, pt1",myTuples[1]==114);
  MacroTest(strm,indent,"tri2, pt2",myTuples[2]==112);

  // line1
  it->Next();

  tuples=attribute->GetTuple(it);

  MacroTest(strm,indent,"line2, pt0",tuples[0]==114);
  MacroTest(strm,indent,"line2, pt1",tuples[1]==115);

  attribute->GetTuple(it,myTuples);
  MacroTest(strm,indent,"line2, pt0",myTuples[0]==114);
  MacroTest(strm,indent,"line2, pt1",myTuples[1]==115);

  // vertex2
  it->Next();

  tuples=attribute->GetTuple(it);

  MacroTest(strm,indent,"vertex2, pt0",tuples[0]==115);

  attribute->GetTuple(it,myTuples);
  MacroTest(strm,indent,"vertex2, pt0",myTuples[0]==115);

  it->Delete();
  strm<<"GetTuple() on cell iterator end"<<endl;
  strm<<"GetTuple() on point iterator start"<<endl;
  pit=ds->NewPointIterator();
  pit->Begin();
  m=100;
  while(!pit->IsAtEnd())
    {
    tuples=attribute->GetTuple(pit);
    MacroTest(strm,indent,"valid point tuple",tuples[0]==m);
    attribute->GetTuple(pit,myTuples);
    MacroTest(strm,indent,"valid point tuple",myTuples[0]==m);
    pit->Next();
    ++m;
    }

  pit->Delete();
  strm<<"GetTuple() on point iterator end"<<endl;

  strm<<"GetComponent() on cell iterator start"<<endl;
  it=ds->NewCellIterator(-1);

  // tetra1
  it->Begin();

  attribute->GetComponent(0,it,myTuples);
  MacroTest(strm,indent,"tetra1, pt0",myTuples[0]==100);
  MacroTest(strm,indent,"tetra1, pt1",myTuples[1]==101);
  MacroTest(strm,indent,"tetra1, pt2",myTuples[2]==102);
  MacroTest(strm,indent,"tetra1, pt3",myTuples[3]==103);

  // triangle1
  it->Next();

  attribute->GetComponent(0,it,myTuples);
  MacroTest(strm,indent,"tri1, pt0",myTuples[0]==104);
  MacroTest(strm,indent,"tri1, pt1",myTuples[1]==105);
  MacroTest(strm,indent,"tri1, pt2",myTuples[2]==106);

  // line1
  it->Next();

  attribute->GetComponent(0,it,myTuples);
  MacroTest(strm,indent,"line1, pt0",myTuples[0]==107);
  MacroTest(strm,indent,"line1, pt1",myTuples[1]==108);

  // vertex1
  it->Next();

  attribute->GetComponent(0,it,myTuples);
  MacroTest(strm,indent,"vertex1, pt0",myTuples[0]==109);


   // tetra2
  it->Next();

  attribute->GetComponent(0,it,myTuples);
  MacroTest(strm,indent,"tetra2, pt0",myTuples[0]==110);
  MacroTest(strm,indent,"tetra2, pt1",myTuples[1]==111);
  MacroTest(strm,indent,"tetra2, pt2",myTuples[2]==112);
  MacroTest(strm,indent,"tetra2, pt3",myTuples[3]==113);

  // triangle2
  it->Next();

  attribute->GetComponent(0,it,myTuples);
  MacroTest(strm,indent,"tri2, pt0",myTuples[0]==111);
  MacroTest(strm,indent,"tri2, pt1",myTuples[1]==114);
  MacroTest(strm,indent,"tri2, pt2",myTuples[2]==112);

  // line1
  it->Next();

  attribute->GetComponent(0,it,myTuples);
  MacroTest(strm,indent,"line2, pt0",myTuples[0]==114);
  MacroTest(strm,indent,"line2, pt1",myTuples[1]==115);

  // vertex2
  it->Next();

  attribute->GetComponent(0,it,myTuples);
  MacroTest(strm,indent,"vertex2, pt0",myTuples[0]==115);

  it->Delete();
  strm<<"GetComponent() on cell iterator end"<<endl;

  strm<<"GetComponent() on point iterator start"<<endl;
  pit=ds->NewPointIterator();
  pit->Begin();
  m=100;
  while(!pit->IsAtEnd())
    {
    MacroTest(strm,indent,"valid point tuple",attribute->GetComponent(0,pit)==m);
    pit->Next();
    ++m;
    }

  pit->Delete();
  strm<<"GetComponent() on point iterator end"<<endl;


  // InterpolateTuple()
  strm<<"InterpolateTuple() start"<<endl;
  it=ds->NewCellIterator(-1);

  // tetra1
  it->Begin();

  double pcoords[3];

  pcoords[0]=0;
  pcoords[1]=0;
  pcoords[2]=0;
  it->GetCell()->InterpolateTuple(attribute,pcoords,myTuples);
  MacroTest(strm,indent,"valid interpolation p0",myTuples[0]==100);

  pcoords[0]=1;
  pcoords[1]=0;
  pcoords[2]=0;
  it->GetCell()->InterpolateTuple(attribute,pcoords,myTuples);
  MacroTest(strm,indent,"valid interpolation p1",myTuples[0]==101);

  pcoords[0]=0;
  pcoords[1]=1;
  pcoords[2]=0;
  it->GetCell()->InterpolateTuple(attribute,pcoords,myTuples);
  MacroTest(strm,indent,"valid interpolation p2",myTuples[0]==102);

  pcoords[0]=0;
  pcoords[1]=0;
  pcoords[2]=1;
  it->GetCell()->InterpolateTuple(attribute,pcoords,myTuples);
  MacroTest(strm,indent,"valid interpolation p3",myTuples[0]==103);

  pcoords[0]=0.5;
  pcoords[1]=0;
  pcoords[2]=0;
  it->GetCell()->InterpolateTuple(attribute,pcoords,myTuples);
  MacroTest(strm,indent,"valid interpolation mid p0p1",myTuples[0]==100.5);

  pcoords[0]=0;
  pcoords[1]=0.5;
  pcoords[2]=0;
  it->GetCell()->InterpolateTuple(attribute,pcoords,myTuples);
  MacroTest(strm,indent,"valid interpolation mid p0p2",myTuples[0]==101);

  pcoords[0]=0;
  pcoords[1]=0;
  pcoords[2]=0.5;
  it->GetCell()->InterpolateTuple(attribute,pcoords,myTuples);
  MacroTest(strm,indent,"valid interpolation mid p0p3",myTuples[0]==101.5);

  pcoords[0]=0.5;
  pcoords[1]=0.5;
  pcoords[2]=0;
  it->GetCell()->InterpolateTuple(attribute,pcoords,myTuples);
  MacroTest(strm,indent,"valid interpolation mid p1p2",myTuples[0]==101.5);

  pcoords[0]=0.5;
  pcoords[1]=0;
  pcoords[2]=0.5;
  it->GetCell()->InterpolateTuple(attribute,pcoords,myTuples);
  MacroTest(strm,indent,"valid interpolation mid p1p3",myTuples[0]==102);

  pcoords[0]=0;
  pcoords[1]=0.5;
  pcoords[2]=0.5;
  it->GetCell()->InterpolateTuple(attribute,pcoords,myTuples);
  MacroTest(strm,indent,"valid interpolation mid p2p3",myTuples[0]==102.5);

  it->Delete();
  strm<<"InterpolateTuple() end"<<endl;

#if 0
   strm<<"NewBoundaryIterator() start"<<endl;
  it=ds->NewBoundaryIterator(-1,0);
  MacroTest(strm,indent,"empty boundary iterator -1,false exists",it!=0);
  it->Begin();
  MacroTest(strm,indent,"empty boundary iterator -1,false",it->IsAtEnd());
  it->Delete();
  it=ds->NewBoundaryIterator(0,0);
  MacroTest(strm,indent,"empty boundary iterator 0,false exists",it!=0);
  it->Begin();
  MacroTest(strm,indent,"empty cell iterator 0,false",it->IsAtEnd());
  it->Delete();
  it=ds->NewBoundaryIterator(1,0);
  MacroTest(strm,indent,"empty boundary iterator 1,false exists",it!=0);
  it->Begin();
  MacroTest(strm,indent,"empty cell iterator 1,false",it->IsAtEnd());
  it->Delete();
  it=ds->NewBoundaryIterator(2,0);
  MacroTest(strm,indent,"empty boundary iterator 2,false exists",it!=0);
  it->Begin();
  MacroTest(strm,indent,"empty cell iterator 2,false",it->IsAtEnd());
  it->Delete();
  it=ds->NewBoundaryIterator(3,0);
  MacroTest(strm,indent,"empty boundary iterator 3,false exists",it!=0);
  it->Begin();
  MacroTest(strm,indent,"empty cell iterator 3,false",it->IsAtEnd());
  it->Delete();
  it=ds->NewBoundaryIterator(-1,1);
  MacroTest(strm,indent,"empty boundary iterator -1,true exists",it!=0);
  it->Begin();
  MacroTest(strm,indent,"empty boundary iterator -1,true",it->IsAtEnd());
  it->Delete();
  it=ds->NewBoundaryIterator(0,1);
  MacroTest(strm,indent,"empty boundary iterator 0,true exists",it!=0);
  it->Begin();
  MacroTest(strm,indent,"empty cell iterator 0,true",it->IsAtEnd());
  it->Delete();
  it=ds->NewBoundaryIterator(1,1);
  MacroTest(strm,indent,"empty boundary iterator 1,true exists",it!=0);
  it->Begin();
  MacroTest(strm,indent,"empty cell iterator 1,true",it->IsAtEnd());
  it->Delete();
  it=ds->NewBoundaryIterator(2,1);
  MacroTest(strm,indent,"empty boundary iterator 2,true exists",it!=0);
  it->Begin();
  MacroTest(strm,indent,"empty cell iterator 2,true",it->IsAtEnd());
  it->Delete();
  it=ds->NewBoundaryIterator(3,1);
  MacroTest(strm,indent,"empty boundary iterator 3,true exists",it!=0);
  it->Begin();
  MacroTest(strm,indent,"empty cell iterator 3,true",it->IsAtEnd());
  it->Delete();
  strm<<"NewBoundaryIterator() end"<<endl;
#endif

  pts->Delete();

  strm<<"Delete the vtkBridgeDataSet"<<endl;
  ds->Delete();
  strm<<"vtkBridgeDataSet deleted"<<endl;

  strm<<"Delete the vtkUnstructuredGrid"<<endl;
  g->Delete();
  strm<<"vtkUnstructuredGrid deleted"<<endl;

  strm << "Test vtkBridgeDataSet creation End" << endl;

  // Do the same thing for:
  // 4. a dataset with points and cells, and celldata but not pointdata
  // 5. a dataset with points and cells, and pointdata but celldata

  return 0;
}

int otherCreation(int vtkNotUsed(argc),
                  char *vtkNotUsed(argv)[])
{
  vtksys_ios::ostringstream vtkmsg_with_warning_C4701;

  if (TestEmpty(cout))
    {
    return 1;
    }

  if (TestWithPoints(cout))
    {
    return 1;
    }

  if (TestWithPointsAndCells(cout))
    {
    return 1;
    }

  if (TestWithPointsAndCellsAndPointData(cout))
    {
    return 1;
    }

  return 0;
}
