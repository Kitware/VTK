/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestQuadraturePointStatistics.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This example demonstrates the capabilities of vtkQuadraturePointInterpolator 
// vtkQuadraturePointsGenerator and the class required to suppport their 
// addition.
// 
// The command line arguments are:
// -I        => run in interactive mode; unless this is used, the program will
//              not allow interaction and exit
// -D <path> => path to the data; the data should be in <path>/Data/
#include "vtkTesting.h"
#include "vtkUnstructuredGrid.h"
#include "vtkTable.h"
#include "vtkXMLUnstructuredGridReader.h"
#include "vtkUnstructuredGridReader.h"
#include "vtkPointData.h"
#include "vtkQuadratureSchemeDictionaryGenerator.h"
#include "vtkQuadraturePointInterpolator.h"
#include "vtkQuadraturePointStatistics.h"
#include "vtkDoubleArray.h"
#include "vtkDataObject.h"

#include "vtkstd/string"
using vtkstd::string;


// Compare doubles.
bool Equal(double l, double r);
// Test a column of the table.
int TestColumn(double *column, double *expected, char *name);

int TestQuadraturePointStatistics(int argc,char *argv[])
{
  vtkTesting *testHelper=vtkTesting::New();
  testHelper->AddArguments(argc,const_cast<const char **>(argv));
  if (!testHelper->IsFlagSpecified("-D"))
    {
    cerr << "Error: -D /path/to/data was not specified.";
    return 1;
    }
  string dataRoot=testHelper->GetDataRoot();
  string tempDir=testHelper->GetTempDirectory();
  string inputFileName=dataRoot+"/Data/Quadratic/CylinderQuadratic.vtk";
  testHelper->Delete();

  // Raed, xml or legacy file.
  vtkUnstructuredGrid *input=0;
  vtkXMLUnstructuredGridReader *xusgr=vtkXMLUnstructuredGridReader::New();
  xusgr->SetFileName(inputFileName.c_str());
  vtkUnstructuredGridReader *lusgr=vtkUnstructuredGridReader::New();
  lusgr->SetFileName(inputFileName.c_str());
  if (xusgr->CanReadFile(inputFileName.c_str()))
    {
    input=xusgr->GetOutput();
    input->Update();
    input->Register(0);
    }
  else if (lusgr->IsFileValid("unstructured_grid"))
    {
    lusgr->SetFileName(inputFileName.c_str());
    input=lusgr->GetOutput();
    input->Update();
    input->Register(0);
    }
  xusgr->Delete();
  lusgr->Delete();
  if (input==0)
    {
    cerr << "Error: Could not read file " << inputFileName << "." << endl;
    return 1;
    }
  // Add a quadrature scheme dictionary to the data set. This filter is
  // solely for our convinience. Typically we would expect that users
  // provide there own in XML format and use the readers or to generate
  // them on the fly.
  vtkQuadratureSchemeDictionaryGenerator *dictGen=vtkQuadratureSchemeDictionaryGenerator::New();
  dictGen->SetInput(input);
  input->Delete();
  // Interpolate fields to the quadrature points. This generates new field data
  // arrays, but not a set of points.
  vtkQuadraturePointInterpolator *fieldInterp=vtkQuadraturePointInterpolator::New();
  fieldInterp->SetInput(dictGen->GetOutput());
  dictGen->Delete();
  // Connect the statistics filter.
  vtkQuadraturePointStatistics *stats=vtkQuadraturePointStatistics::New();
  stats->SetInput(fieldInterp->GetOutput());
  fieldInterp->Delete();
  stats->Update();
  // Get the columns of table of statistics produced.
  vtkTable *results=stats->GetOutput();
  vtkDoubleArray *ss=vtkDoubleArray::SafeDownCast(results->GetColumn(1));
  vtkDoubleArray *vsm=vtkDoubleArray::SafeDownCast(results->GetColumn(2));
  vtkDoubleArray *vs0=vtkDoubleArray::SafeDownCast(results->GetColumn(3));
  vtkDoubleArray *vs1=vtkDoubleArray::SafeDownCast(results->GetColumn(4));
  vtkDoubleArray *vs2=vtkDoubleArray::SafeDownCast(results->GetColumn(5));
  // Expected results.
  double expected[5][3]={
      { 3.059852414448038e-02,  9.956630332424743e-01,  4.029730492116645e-01},
      {-2.269918310038044e-01,  2.024122131787856e-01, -4.004585517533307e-04},
      {-2.021326110317450e-01,  2.234015215692812e-01,  4.329055382852992e-05},
      {-9.956377843500491e-01, -3.021884798540561e-02, -4.023756660384976e-01},
      {-4.848191252082387e+01,  5.931853206950250e+03,  2.031073976434023e+03}};

  int passFlag=0x1;
  // Test and display.
  passFlag&=TestColumn(ss->GetPointer(0) ,expected[0],ss->GetName() );
  passFlag&=TestColumn(vsm->GetPointer(0),expected[1],vsm->GetName());
  passFlag&=TestColumn(vs0->GetPointer(0),expected[2],vs0->GetName());
  passFlag&=TestColumn(vs1->GetPointer(0),expected[3],vs1->GetName());
  passFlag&=TestColumn(vs2->GetPointer(0),expected[4],vs2->GetName());

  stats->Delete();
  return !passFlag;
}

//-----------------------------------------------------------------------------
int TestColumn(double *column, double *expected, char *name)
{
  if (!Equal(column[0],expected[0]) 
    || !Equal(column[1],expected[1])
    || !Equal(column[2],expected[2]))
    {
    cerr << "Test of column " << name << " failed." << endl;
    cerr.precision(15);
    cerr.setf(ios::scientific,ios::floatfield);
    cerr << column[0] << " == " << expected[0] << endl;
    cerr << column[1] << " == " << expected[1] << endl;
    cerr << column[2] << " == " << expected[2] << endl;
    return 0;
    }
  return 1;
}


//-----------------------------------------------------------------------------
bool Equal(double l, double r)
{
  double d=fabs(fabs(l)-fabs(r))/fabs(l<r?r:l);
  if (d<1E-13) return true;
  return false;
}

