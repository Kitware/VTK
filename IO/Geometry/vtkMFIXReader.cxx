/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMFIXReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// Thanks to Phil Nicoletti, Terry Jordan and Brian Dotson at the
// National Energy Technology Laboratory who developed this class.
// Please address all comments to Terry Jordan (terry.jordan@netl.doe.gov)
//

#include "vtkMFIXReader.h"

#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkErrorCode.h"
#include "vtkUnstructuredGrid.h"
#include "vtkPointData.h"
#include "vtkCellData.h"
#include "vtkDoubleArray.h"
#include "vtkIntArray.h"
#include "vtkCellArray.h"
#include "vtkHexahedron.h"
#include "vtkFloatArray.h"
#include <string>
#include "vtkDataArraySelection.h"
#include "vtkWedge.h"
#include "vtkQuad.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkStringArray.h"

vtkStandardNewMacro(vtkMFIXReader);

//----------------------------------------------------------------------------
vtkMFIXReader::vtkMFIXReader()
{
  this->FileName = NULL;
  this->NumberOfCells = 0;
  this->NumberOfPoints = 0;
  this->NumberOfCellFields = 0;
  this->RequestInformationFlag = 0;
  this->MakeMeshFlag = 0;
  this->Minimum = vtkFloatArray::New();
  this->Maximum = vtkFloatArray::New();
  this->VectorLength = vtkIntArray::New();
  this->CellDataArray = NULL;
  this->DimensionIc = 5;
  this->DimensionBc = 5;
  this->DimensionC = 5;
  this->DimensionIs = 5;
  this->NumberOfSPXFilesUsed = 9;
  this->NumberOfScalars = 0;
  this->BkEpsilon = false;
  this->NumberOfReactionRates = 0;
  this->FileExtension[0] = '1';
  this->FileExtension[1] = '2';
  this->FileExtension[2] = '3';
  this->FileExtension[3] = '4';
  this->FileExtension[4] = '5';
  this->FileExtension[5] = '6';
  this->FileExtension[6] = '7';
  this->FileExtension[7] = '8';
  this->FileExtension[8] = '9';
  this->FileExtension[9] = 'A';
  this->FileExtension[10] = 'B';
  this->VersionNumber = 0;

  this->CellDataArraySelection = vtkDataArraySelection::New();
  this->Points = vtkPoints::New();
  this->Mesh = vtkUnstructuredGrid::New();
  this->AHexahedron = vtkHexahedron::New();
  this->AQuad = vtkQuad::New();
  this->AWedge = vtkWedge::New();
  this->NMax = vtkIntArray::New();
  this->C = vtkDoubleArray::New();
  this->Dx = vtkDoubleArray::New();
  this->Dy = vtkDoubleArray::New();
  this->Dz = vtkDoubleArray::New();
  this->TempI = vtkIntArray::New();
  this->TempD = vtkDoubleArray::New();
  this->Flag = vtkIntArray::New();
  this->VariableNames = vtkStringArray::New();
  this->VariableComponents = vtkIntArray::New();
  this->VariableIndexToSPX = vtkIntArray::New();
  this->VariableTimesteps = vtkIntArray::New();
  this->VariableTimestepTable = vtkIntArray::New();
  this->SPXToNVarTable = vtkIntArray::New();
  this->VariableToSkipTable = vtkIntArray::New();
  this->SpxFileExists = vtkIntArray::New();
  this->SetNumberOfInputPorts(0);
  this->SPXTimestepIndexTable = vtkIntArray::New();

  // Time support:
  this->TimeStep = 0; // By default the file does not have timestep
  this->TimeStepRange[0] = 0;
  this->TimeStepRange[1] = 0;
  this->NumberOfTimeSteps = 1;
  this->TimeSteps = 0;
  this->CurrentTimeStep = 0;
  this->TimeStepWasReadOnce = 0;
}

//----------------------------------------------------------------------------
vtkMFIXReader::~vtkMFIXReader()
{
  delete [] this->FileName;

  if( this->CellDataArray )
  {
  for (int j = 0; j <= this->VariableNames->GetMaxId(); j++)
    {
    this->CellDataArray[j]->Delete();
    }
    delete [] this->CellDataArray;
  }

  this->CellDataArraySelection->Delete();
  this->Points->Delete();
  this->Mesh->Delete();
  this->AHexahedron->Delete();
  this->AWedge->Delete();
  this->AQuad->Delete();
  this->NMax->Delete();
  this->C->Delete();
  this->Dx->Delete();
  this->Dy->Delete();
  this->Dz->Delete();
  this->TempI->Delete();
  this->TempD->Delete();
  this->Flag->Delete();
  this->VariableNames->Delete();
  this->VariableComponents->Delete();
  this->VariableIndexToSPX->Delete();
  this->VariableTimesteps->Delete();
  this->VariableTimestepTable->Delete();
  this->SPXToNVarTable->Delete();
  this->VariableToSkipTable->Delete();
  this->SpxFileExists->Delete();
  this->Minimum->Delete();
  this->Maximum->Delete();
  this->VectorLength->Delete();
  this->SPXTimestepIndexTable->Delete();
}

//----------------------------------------------------------------------------
int vtkMFIXReader::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **vtkNotUsed(inputVector),
  vtkInformationVector *outputVector)
{
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  vtkUnstructuredGrid *output = vtkUnstructuredGrid::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkDebugMacro( << "Reading MFIX file");

  // Save the time value in the output data information.
  int length = outInfo->Length(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
  double* steps = outInfo->Get(vtkStreamingDemandDrivenPipeline::TIME_STEPS());

  if (outInfo->Has( vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP()))
    {
    // Get the requested time step. We only support requests of a single time
    // step in this reader right now
    double requestedTimeStep =
      outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP());


    //find the timestep with the closest value
    int cnt=0;
    int closestStep=0;
    double minDist=-1;
    for (cnt=0;cnt<length;cnt++)
      {
      double tdist=(steps[cnt]-requestedTimeStep>requestedTimeStep-steps[cnt])?
        steps[cnt]-requestedTimeStep:
        requestedTimeStep-steps[cnt];
      if (minDist<0 || tdist<minDist)
        {
        minDist=tdist;
        closestStep=cnt;
        }
      }
    this->CurrentTimeStep=closestStep;
    }
  else
    {
    this->CurrentTimeStep = this->TimeStep;
    }

  this->MakeMesh(output);
  output->GetInformation()->Set(vtkDataObject::DATA_TIME_STEP(), steps[this->CurrentTimeStep]);
  return 1;
}

//----------------------------------------------------------------------------
void vtkMFIXReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "File Name: "
     << (this->FileName ? this->FileName : "(none)") << "\n";
  os << indent << "Number Of Nodes: " << this->NumberOfPoints << endl;
  os << indent << "Number Of Cells: " << this->NumberOfCells << endl;
  os << indent << "Number Of Cell Fields: " << this->NumberOfCellFields << endl;
  os << indent << "Time Step Range: "
     << this->TimeStepRange[0] << " - " << this->TimeStepRange[1]
     << endl;
  os << indent << "Time Step: " << this->TimeStep << endl;
  os << indent << "Number of Time Steps: " << this->NumberOfTimeSteps << endl;
}

//----------------------------------------------------------------------------
void vtkMFIXReader::MakeMesh(vtkUnstructuredGrid *output)
{
  output->Allocate();

  if (this->MakeMeshFlag == 0)
    {
    //Points->SetNumberOfPoints((this->IMaximum2+1)
      // *(this->JMaximum2+1)*(this->KMaximum2+1));

    //
    //  Cartesian type mesh
    //
    if ( !strcmp(this->CoordinateSystem,"CARTESIAN") && (this->KMaximum2 != 1))
      {
      double pointX = -this->Dx->GetValue(0);
      double pointY = -this->Dy->GetValue(0);
      double pointZ = -this->Dz->GetValue(0);
      for (int k = 0; k <= this->KMaximum2; k++)
        {
        for (int j = 0; j <= this->JMaximum2; j++)
          {
          for (int i = 0; i <= this->IMaximum2; i++)
            {
            this->Points->InsertNextPoint( pointX, pointY, pointZ );
            if ( i == this->IMaximum2 )
              {
              pointX = pointX + this->Dx->GetValue(i-1);
              }
            else
              {
              pointX = pointX + this->Dx->GetValue(i);
              }
            }
          pointX = -this->Dx->GetValue(0);
          if ( j == this->JMaximum2)
            {
            pointY = pointY + this->Dy->GetValue(j-1);
            }
          else
            {
            pointY = pointY + this->Dy->GetValue(j);
            }
          }
        pointY = -this->Dy->GetValue(0);
        if ( k == this->KMaximum2)
          {
          pointZ = pointZ + this->Dz->GetValue(k-1);
          }
        else
          {
          pointZ = pointZ + this->Dz->GetValue(k);
          }
        }
      }
    else if (!strcmp(this->CoordinateSystem,"CARTESIAN") &&
                    (this->KMaximum2 == 1))
      {
      double pointX = -this->Dx->GetValue(0);
      double pointY = -this->Dy->GetValue(0);
      double pointZ = 0.0;
      for (int j = 0; j <= this->JMaximum2; j++)
        {
        for (int i = 0; i <= this->IMaximum2; i++)
          {
          this->Points->InsertNextPoint(pointX, pointY, pointZ );
          if ( i == this->IMaximum2 )
            {
            pointX = pointX + this->Dx->GetValue(i-1);
            }
          else
            {
            pointX = pointX + this->Dx->GetValue(i);
            }
          }
        pointX = -this->Dx->GetValue(0);
        if ( j == this->JMaximum2)
          {
          pointY = pointY + this->Dy->GetValue(j-1);
          }
        else
          {
          pointY = pointY + this->Dy->GetValue(j);
          }
        }
      }
    else if (!strcmp(this->CoordinateSystem,"CYLINDRICAL") &&
                    (this->KMaximum2 == 1))
      {
      double pointX = -this->Dx->GetValue(0);
      double pointY = -this->Dy->GetValue(0);
      double pointZ = 0.0;
      for (int j = 0; j <= this->JMaximum2; j++)
        {
        for (int i = 0; i <= this->IMaximum2; i++)
          {
          this->Points->InsertNextPoint( pointX, pointY, pointZ );
          if ( i == this->IMaximum2 )
            {
            pointX = pointX + this->Dx->GetValue(i-1);
            }
          else if ( i == 0 )
            {
            pointX = 0;
            }
          else
            {
            pointX = pointX + this->Dx->GetValue(i);
            }
          }
        pointX = -this->Dx->GetValue(0);
        if ( j == this->JMaximum2)
          {
          pointY = pointY + this->Dy->GetValue(j-1);
          }
        else
          {
          pointY = pointY + this->Dy->GetValue(j);
          }
        }
      }
    else
      {
      //
      //  Cylindrical Type Mesh
      //
      double pointX = -this->Dx->GetValue(0);
      double pointY = -this->Dy->GetValue(0);
      double pointZ = -this->Dz->GetValue(0);
      double radialX = 0.0;
      double radialY = 0.0;
      double radialZ = 0.0;
      for (int k = 0; k <= this->KMaximum2; k++)
        {
        for (int j = 0; j <= this->JMaximum2; j++)
          {
          for (int i = 0; i <= this->IMaximum2; i++)
            {
            this->Points->InsertNextPoint( radialX, radialY, radialZ );
            if ( i == this->IMaximum2 )
              {
              pointX = pointX + this->Dx->GetValue(i-1);
              }
            else if ( i == 0 )
              {
              pointX = -this->Dx->GetValue(0);
              }
            else
              {
              pointX = pointX + this->Dx->GetValue(i);
              }
            radialX = pointX * cos(pointZ);
            radialZ = pointX * sin(pointZ) * -1;
            }
          pointX = -this->Dx->GetValue(0);
          radialX = 0.0;
          radialZ = 0.0;
          if ( j == this->JMaximum2)
            {
            pointY = pointY + this->Dy->GetValue(j-1);
            }
          else
            {
            pointY = pointY + this->Dy->GetValue(j);
            }
          radialY = pointY;
          }
        pointY = -this->Dy->GetValue(0);
        radialY = 0.0;
        if ( k == this->KMaximum2)
          {
          pointZ = pointZ + this->Dz->GetValue(k-1);
          }
        else
          {
          pointZ = pointZ + this->Dz->GetValue(k);
          }
        }
      }

    //
    //  Let's put the points in a mesh
    //
    this->Mesh->SetPoints(this->Points);
    int point0 = 0;
    int count = 0;
    if (!strcmp(this->CoordinateSystem,"CYLINDRICAL") &&
               (this->KMaximum2 == 1))
      {
      for (int j = 0; j < this->JMaximum2; j++)
        {
        for (int i = 0; i < this->IMaximum2; i++)
          {
          if ( this->Flag->GetValue(count) < 10 )
            {
            this->AQuad->GetPointIds()->SetId( 0, point0);
            this->AQuad->GetPointIds()->SetId( 1, point0+1);
            this->AQuad->GetPointIds()->SetId( 2, point0+2+this->IMaximum2);
            this->AQuad->GetPointIds()->SetId( 3, point0+1+this->IMaximum2);
            this->Mesh->InsertNextCell(this->AQuad->GetCellType(),
              this->AQuad->GetPointIds());
            }
          point0++;
          count++;
          }
        point0++;
        }
      point0 = point0 + this->IMaximum2+1;
      }
    else if (!strcmp(this->CoordinateSystem,"CYLINDRICAL") &&
                    (this->KMaximum2 != 1))
      {
      for (int k = 0; k < this->KMaximum2; k++)
        {
        for (int j = 0; j < this->JMaximum2; j++)
          {
          for (int i = 0; i < this->IMaximum2; i++)
            {
            if ( this->Flag->GetValue(count) < 10 )
              {
              if (( k == (this->KMaximum2-2)) && (i != 1))
                {
                this->AHexahedron->GetPointIds()->SetId( 0, point0);
                this->AHexahedron->GetPointIds()->SetId( 1, point0+1);
                this->AHexahedron->GetPointIds()->SetId( 2,
                  (point0+1+((this->IMaximum2+1)*(this->JMaximum2+1)))-
                  ((this->IMaximum2+1)*(this->JMaximum2+1)
                  *(this->KMaximum2-2)));
                this->AHexahedron->GetPointIds()->SetId( 3,
                  (point0+((this->IMaximum2+1)*(this->JMaximum2+1)))-
                  ((this->IMaximum2+1)*(this->JMaximum2+1)
                  *(this->KMaximum2-2)));
                this->AHexahedron->GetPointIds()->
                  SetId( 4, point0+1+this->IMaximum2);
                this->AHexahedron->GetPointIds()->
                  SetId( 5, point0+2+this->IMaximum2);
                this->AHexahedron->GetPointIds()->
                  SetId( 6, (point0+2+this->IMaximum2 +
                  ((this->IMaximum2+1)*(this->JMaximum2+1))) -
                  ((this->IMaximum2+1)*(this->JMaximum2+1)
                  * (this->KMaximum2-2)));
                this->AHexahedron->GetPointIds()->
                  SetId( 7, (point0+1+this->IMaximum2 +
                  ((this->IMaximum2+1)*(this->JMaximum2+1)))-
                  ((this->IMaximum2+1)*(this->JMaximum2+1)
                  *(this->KMaximum2-2)));
                this->Mesh->InsertNextCell(this->AHexahedron->GetCellType(),
                  this->AHexahedron->GetPointIds());
                }
              else if ((k != (this->KMaximum2-2)) && (i != 1))
                {
                this->AHexahedron->GetPointIds()->SetId( 0, point0);
                this->AHexahedron->GetPointIds()->SetId( 1, point0+1);
                this->AHexahedron->GetPointIds()->SetId( 2,
                  point0+1+((this->IMaximum2+1)*(this->JMaximum2+1)));
                this->AHexahedron->GetPointIds()->SetId( 3,
                  point0+((this->IMaximum2+1)*(this->JMaximum2+1)));
                this->AHexahedron->GetPointIds()->
                  SetId( 4, point0+1+this->IMaximum2);
                this->AHexahedron->GetPointIds()->
                  SetId( 5, point0+2+this->IMaximum2);
                this->AHexahedron->GetPointIds()->
                  SetId( 6, point0+2+this->IMaximum2+
                  ((this->IMaximum2+1)*(this->JMaximum2+1)));
                this->AHexahedron->GetPointIds()->
                  SetId( 7, point0+1+this->IMaximum2+
                  ((this->IMaximum2+1)*(this->JMaximum2+1)));
                this->Mesh->InsertNextCell(this->AHexahedron->GetCellType(),
                  this->AHexahedron->GetPointIds());
                }
              else if ( (k != (this->KMaximum2-2)) && (i == 1))
                {
                this->AWedge->GetPointIds()->SetId( 0, j*(this->IMaximum2+1));
                this->AWedge->GetPointIds()->SetId( 1, point0+1);
                this->AWedge->GetPointIds()->
                  SetId( 2, point0+1+((this->IMaximum2+1)
                  *(this->JMaximum2+1)));
                this->AWedge->GetPointIds()->
                  SetId( 3, (j+1)*(this->IMaximum2+1));
                this->AWedge->GetPointIds()->
                  SetId( 4, point0+2+this->IMaximum2);
                this->AWedge->GetPointIds()->
                  SetId( 5, point0+2+this->IMaximum2+
                  ((this->IMaximum2+1)*(this->JMaximum2+1)));
                this->Mesh->InsertNextCell(this->AWedge->GetCellType(),
                  this->AWedge->GetPointIds());
                }
              else if (( k == (this->KMaximum2-2)) && (i == 1))
                {
                this->AWedge->GetPointIds()->SetId( 0, j*(this->IMaximum2+1));
                this->AWedge->GetPointIds()->SetId( 1, point0+1);
                this->AWedge->GetPointIds()->SetId( 2,
                 (point0+1+((this->IMaximum2+1)
                 *(this->JMaximum2+1)))-((this->IMaximum2+1)
                 *(this->JMaximum2+1)*(this->KMaximum2-2)));
                this->AWedge->GetPointIds()->
                  SetId( 3, (j+1)*(this->IMaximum2+1));
                this->AWedge->GetPointIds()->
                  SetId( 4, point0+2+this->IMaximum2);
                this->AWedge->GetPointIds()->
                  SetId( 5, (point0+2+this->IMaximum2 +
                  ((this->IMaximum2+1)*(this->JMaximum2+1)))
                  -((this->IMaximum2+1)
                  *(this->JMaximum2+1)*(this->KMaximum2-2)));
                this->Mesh->InsertNextCell(this->AWedge->GetCellType(),
                  this->AWedge->GetPointIds());
                }
              }
            point0++;
            count++;
            }
          point0++;
          }
        point0 = point0 + this->IMaximum2+1;
        }
      }
    else if (!strcmp(this->CoordinateSystem,"CARTESIAN") &&
                    (this->KMaximum2 == 1))
      {
      for (int j = 0; j < this->JMaximum2; j++)
        {
        for (int i = 0; i < this->IMaximum2; i++)
          {
          if ( this->Flag->GetValue(count) < 10 )
            {
            this->AQuad->GetPointIds()->SetId( 0, point0);
            this->AQuad->GetPointIds()->SetId( 1, point0+1);
            this->AQuad->GetPointIds()->SetId( 2, point0+2+this->IMaximum2);
            this->AQuad->GetPointIds()->SetId( 3, point0+1+this->IMaximum2);
            this->Mesh->InsertNextCell(this->AQuad->GetCellType(),
              this->AQuad->GetPointIds());
            }
          point0++;
          count++;
          }
        point0++;
        }
      }
    else
      {
      for (int k = 0; k < this->KMaximum2; k++)
        {
        for (int j = 0; j < this->JMaximum2; j++)
          {
          for (int i = 0; i < this->IMaximum2; i++)
            {
            if ( this->Flag->GetValue(count) < 10 )
              {
              this->AHexahedron->GetPointIds()->SetId( 0, point0);
              this->AHexahedron->GetPointIds()->SetId( 1, point0+1);
              this->AHexahedron->GetPointIds()->
                SetId( 2, point0+1+((this->IMaximum2+1)
                *(this->JMaximum2+1)));
              this->AHexahedron->GetPointIds()->
                SetId( 3, point0+((this->IMaximum2+1)
                *(this->JMaximum2+1)));
              this->AHexahedron->GetPointIds()->
                SetId( 4, point0+1+this->IMaximum2);
              this->AHexahedron->GetPointIds()->
                SetId( 5, point0+2+this->IMaximum2);
              this->AHexahedron->GetPointIds()->
                SetId( 6, point0+2+this->IMaximum2 +
                ((this->IMaximum2+1)*(this->JMaximum2+1)));
              this->AHexahedron->GetPointIds()->
                SetId( 7, point0+1+this->IMaximum2 +
                ((this->IMaximum2+1)*(this->JMaximum2+1)));
              this->Mesh->InsertNextCell(this->AHexahedron->GetCellType(),
                this->AHexahedron->GetPointIds());
              }
            point0++;
            count++;
            }
          point0++;
          }
        point0 = point0 + this->IMaximum2+1;
        }
      }


    this->CellDataArray = new vtkFloatArray
      * [this->VariableNames->GetMaxId()+2];
    for (int j = 0; j <= this->VariableNames->GetMaxId(); j++)
      {
      this->CellDataArray[ j ] = vtkFloatArray::New();
      this->CellDataArray[ j ]->SetName(this->VariableNames->GetValue(j));
      this->CellDataArray[ j ]->
        SetNumberOfComponents(this->VariableComponents->GetValue(j));
      }

    this->MakeMeshFlag = 1;
    }

  output->DeepCopy(this->Mesh);  // If mesh has already
                                 // been made copy it to output
  int first = 0;
  for (int j = 0; j <= this->VariableNames->GetMaxId(); j++)
    {
    if ( this->CellDataArraySelection->GetArraySetting(j) == 1 )
      {
      if (this->VariableComponents->GetValue(j) == 1)
        {
        this->GetVariableAtTimestep( j, this->CurrentTimeStep, CellDataArray[j]);
        }
      else
        {
        if ( !strcmp(CoordinateSystem,"CYLINDRICAL" ))
          {
          this->ConvertVectorFromCylindricalToCartesian( j-3, j-1);
          }
        this->FillVectorVariable( j-3, j-2, j-1, this->CellDataArray[j]);
        }
      if (first == 0)
        {
        output->GetCellData()->SetScalars(this->CellDataArray[j]);
        }
      else
        {
        output->GetCellData()->AddArray(this->CellDataArray[j]);
        }
      double tempRange[2];
      this->CellDataArray[j]->GetRange(tempRange, -1);
      this->Minimum->InsertValue( j, (float)tempRange[0]);
      this->Maximum->InsertValue( j, (float)tempRange[1]);
      this->VectorLength->InsertValue( j, 1);
      first = 1;
      }
    }
}


//----------------------------------------------------------------------------
int vtkMFIXReader::RequestInformation(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **vtkNotUsed(inputVector),
  vtkInformationVector *outputVector)
{
  if ( this->RequestInformationFlag == 0)
    {
    if ( !this->FileName )
      {
      this->NumberOfPoints = 0;
      this->NumberOfCells = 0;
      vtkErrorMacro("No filename specified");
      return 0;
      }

    this->SetProjectName(this->FileName);
    this->ReadRestartFile();
    this->CreateVariableNames();
    this->GetTimeSteps();
    this->CalculateMaxTimeStep();
    this->MakeTimeStepTable(VariableNames->GetMaxId()+1);
    this->GetNumberOfVariablesInSPXFiles();
    this->MakeSPXTimeStepIndexTable(VariableNames->GetMaxId()+1);

    for (int j = 0; j <= this->VariableNames->GetMaxId(); j++)
      {
      this->CellDataArraySelection->AddArray(
        this->VariableNames->GetValue(j));
      }

    this->NumberOfPoints = (this->IMaximum2+1)
      *(this->JMaximum2+1)*(this->KMaximum2+1);
    this->NumberOfCells = this->IJKMaximum2;
    this->NumberOfCellFields = this->VariableNames->GetMaxId()+1;
    this->NumberOfTimeSteps = this->MaximumTimestep;
    this->TimeStepRange[0] = 0;
    this->TimeStepRange[1] = this->NumberOfTimeSteps-1;
    this->RequestInformationFlag = 1;
    this->GetAllTimes(outputVector);
    }
  return 1;
}

//----------------------------------------------------------------------------
int vtkMFIXReader::GetNumberOfCellArrays()
{
  return this->CellDataArraySelection->GetNumberOfArrays();
}

//----------------------------------------------------------------------------
const char* vtkMFIXReader::GetCellArrayName(int index)
{
  return this->CellDataArraySelection->GetArrayName(index);
}

//----------------------------------------------------------------------------
int vtkMFIXReader::GetCellArrayStatus(const char* name)
{
  return this->CellDataArraySelection->ArrayIsEnabled(name);
}

//----------------------------------------------------------------------------
void vtkMFIXReader::SetCellArrayStatus(const char* name, int status)
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
void vtkMFIXReader::DisableAllCellArrays()
{
  this->CellDataArraySelection->DisableAllArrays();
}

//----------------------------------------------------------------------------
void vtkMFIXReader::EnableAllCellArrays()
{
  this->CellDataArraySelection->EnableAllArrays();
}

#if !defined(VTK_LEGACY_REMOVE)
//----------------------------------------------------------------------------
void vtkMFIXReader::GetCellDataRange(int cellComp, int /* index */,
     float *min, float *max)
{
#if !defined(VTK_LEGACY_SILENT)
  vtkGenericWarningMacro("vtkMFIXReader::GetCellDataRange with \"index\" was deprecated in VTK 6.0");
#endif

  *min = this->Minimum->GetValue(cellComp);
  *max = this->Maximum->GetValue(cellComp);
}
#endif

//----------------------------------------------------------------------------
void vtkMFIXReader::GetCellDataRange(int cellComp, float *min, float *max)
{
  *min = this->Minimum->GetValue(cellComp);
  *max = this->Maximum->GetValue(cellComp);
}

//----------------------------------------------------------------------------
void vtkMFIXReader::SetProjectName (const char *infile)
{
  int len = static_cast<int>(strlen(infile));
  strncpy(this->RunName, infile, len-4);
}

//----------------------------------------------------------------------------
void vtkMFIXReader::RestartVersionNumber(const char* buffer)
{
  char s1[512];
  char s2[512];
  sscanf(buffer,"%s %s %f", s1, s2, &this->VersionNumber);
  strncpy(this->Version, buffer, 100);
}

//----------------------------------------------------------------------------
void vtkMFIXReader::GetInt(istream& in, int &val)
{
  in.read( (char*)&val,sizeof(int));
  this->SwapInt(val);
}

//----------------------------------------------------------------------------
void vtkMFIXReader::SwapInt(int &value)
{
  int result = ((value & 0x00FF) << 24) |
               ((value & 0xFF00) << 8) |
               ((value >> 8) & 0xFF00) |
               ((value >> 24) & 0x00FF);
  value = result;
}

//----------------------------------------------------------------------------
void vtkMFIXReader::SwapDouble(double &value)
{
  union Swap
    {
    double valDouble;
    unsigned char valByte[8];
    };

  Swap source;
  source.valDouble = value;

  Swap result;
  result.valByte[0] = source.valByte[7];
  result.valByte[1] = source.valByte[6];
  result.valByte[2] = source.valByte[5];
  result.valByte[3] = source.valByte[4];
  result.valByte[4] = source.valByte[3];
  result.valByte[5] = source.valByte[2];
  result.valByte[6] = source.valByte[1];
  result.valByte[7] = source.valByte[0];

  value = result.valDouble;
}

//----------------------------------------------------------------------------
void vtkMFIXReader::SwapFloat(float &value)
{
  union Swap
    {
    float valFloat;
    unsigned char valByte[4];
    };

  Swap source;
  source.valFloat = value;

  Swap result;
  result.valByte[0] = source.valByte[3];
  result.valByte[1] = source.valByte[2];
  result.valByte[2] = source.valByte[1];
  result.valByte[3] = source.valByte[0];

  value = result.valFloat;
}

//----------------------------------------------------------------------------
void vtkMFIXReader::GetDouble(istream& in, double& val)
{
  in.read( (char*)&val,sizeof(double));
  this->SwapDouble(val);
}

//----------------------------------------------------------------------------
void vtkMFIXReader::SkipBytes(istream& in, int n)
{
  in.read(this->DataBuffer,n); // maybe seekg instead
}

//----------------------------------------------------------------------------
void vtkMFIXReader::GetBlockOfDoubles(istream& in, vtkDoubleArray *v, int n)
{
  const int numberOfDoublesInBlock = 512/sizeof(double);
  double tempArray[numberOfDoublesInBlock];
  int numberOfRecords;

  if ( n%numberOfDoublesInBlock == 0)
    {
    numberOfRecords = n/numberOfDoublesInBlock;
    }
  else
    {
    numberOfRecords = 1 + n/numberOfDoublesInBlock;
    }

  int c = 0;
  for (int i=0; i<numberOfRecords; ++i)
    {
    in.read( (char*)&tempArray , 512 );
    for (int j=0; j<numberOfDoublesInBlock; ++j)
      {
      if (c < n)
        {
        double temp = tempArray[j];
        this->SwapDouble(temp);
        v->InsertValue( c, temp);
        ++c;
        }
      }
    }
}

//----------------------------------------------------------------------------
void vtkMFIXReader::GetBlockOfInts(istream& in, vtkIntArray *v, int n)
{
  const int numberOfIntsInBlock = 512/sizeof(int);
  int tempArray[numberOfIntsInBlock];
  int numberOfRecords;

  if ( n%numberOfIntsInBlock == 0)
    {
    numberOfRecords = n/numberOfIntsInBlock;
    }
  else
    {
    numberOfRecords = 1 + n/numberOfIntsInBlock;
    }

  int c = 0;
  for (int i = 0; i < numberOfRecords; ++i)
    {
    in.read( (char*)&tempArray , 512 );
    for (int j=0; j<numberOfIntsInBlock; ++j)
      {
      if (c < n)
        {
        int temp = tempArray[j];
        this->SwapInt(temp);
        v->InsertValue( c, temp);
        ++c;
        }
      }
    }
}

//----------------------------------------------------------------------------
void vtkMFIXReader::GetBlockOfFloats(istream& in, vtkFloatArray *v, int n)
{
  const int numberOfFloatsInBlock = 512/sizeof(float);
  float tempArray[numberOfFloatsInBlock];
  int numberOfRecords;

  if ( n%numberOfFloatsInBlock == 0)
    {
    numberOfRecords = n/numberOfFloatsInBlock;
    }
  else
    {
    numberOfRecords = 1 + n/numberOfFloatsInBlock;
    }

  bool modified = false;
  int c = 0;
  int cnt = 0;
  for (int i=0; i<numberOfRecords; ++i)
    {
    in.read( (char*)&tempArray , 512 );
    for (int j=0; j<numberOfFloatsInBlock; ++j)
      {
      if (c < n)
        {
        float temp = tempArray[j];
        this->SwapFloat(temp);
        if ( this->Flag->GetValue(c) < 10)
          {
          v->InsertValue(cnt, temp);
          cnt++;
          modified = true;
          }
        ++c;
        }
      }
    }

  if (modified)
    {
    v->Modified();
    }
}

//----------------------------------------------------------------------------
void vtkMFIXReader::ReadRestartFile()
{
  int dimensionUsr = 5;

#ifdef _WIN32
  ifstream in(this->FileName,ios::binary);
#else
  ifstream in(this->FileName);
#endif

  if (!in)
    {
    //cout << "could not open file" << endl;
    return;
    }

  this->DataBuffer[512] = '\0';

  // version : record 1
  memset(this->DataBuffer,0,513);
  in.read(this->DataBuffer,512);
  this->RestartVersionNumber(this->DataBuffer);

  // skip 2 linesline : records 2 and 3
  in.read(this->DataBuffer,512);
  in.read(this->DataBuffer,512);

  // IMinimum1 etc : record 4
  memset(this->DataBuffer,0,513);

  if (strcmp(this->Version, "RES = 01.00") == 0)
    {
    this->GetInt(in,this->IMinimum1);
    this->GetInt(in,this->JMinimum1);
    this->GetInt(in,this->KMinimum1);
    this->GetInt(in,this->IMaximum);
    this->GetInt(in,this->JMaximum);
    this->GetInt(in,this->KMaximum);
    this->GetInt(in,this->IMaximum1);
    this->GetInt(in,this->JMaximum1);
    this->GetInt(in,this->KMaximum1);
    this->GetInt(in,this->IMaximum2);
    this->GetInt(in,this->JMaximum2);
    this->GetInt(in,this->KMaximum2);
    this->GetInt(in,this->IJMaximum2);
    this->GetInt(in,this->IJKMaximum2);
    this->GetInt(in,this->MMAX);
    this->GetDouble(in,this->DeltaTime);
    this->GetDouble(in,this->XLength);
    this->GetDouble(in,this->YLength);
    this->GetDouble(in,this->ZLength);

    // 15 ints ... 4 doubles = 92 bytes
    this->SkipBytes(in,420);
    }
  else if (strcmp(this->Version, "RES = 01.01") == 0 ||
           strcmp(this->Version, "RES = 01.02") == 0)
    {
    this->GetInt(in,this->IMinimum1);
    this->GetInt(in,this->JMinimum1);
    this->GetInt(in,this->KMinimum1);
    this->GetInt(in,this->IMaximum);
    this->GetInt(in,this->JMaximum);
    this->GetInt(in,this->KMaximum);
    this->GetInt(in,this->IMaximum1);
    this->GetInt(in,this->JMaximum1);
    this->GetInt(in,this->KMaximum1);
    this->GetInt(in,this->IMaximum2);
    this->GetInt(in,this->JMaximum2);
    this->GetInt(in,this->KMaximum2);
    this->GetInt(in,this->IJMaximum2);
    this->GetInt(in,this->IJKMaximum2);
    this->GetInt(in,this->MMAX);
    this->GetInt(in,this->DimensionIc);
    this->GetInt(in,this->DimensionBc);
    this->GetDouble(in,this->DeltaTime);
    this->GetDouble(in,this->XLength);
    this->GetDouble(in,this->YLength);
    this->GetDouble(in,this->ZLength);

    // 17 ints ... 4 doubles = 100 bytes
    this->SkipBytes(in,412);
    }
  else if(strcmp(this->Version, "RES = 01.03") == 0)
    {
    this->GetInt(in,this->IMinimum1);
    this->GetInt(in,this->JMinimum1);
    this->GetInt(in,this->KMinimum1);
    this->GetInt(in,this->IMaximum);
    this->GetInt(in,this->JMaximum);
    this->GetInt(in,this->KMaximum);
    this->GetInt(in,this->IMaximum1);
    this->GetInt(in,this->JMaximum1);
    this->GetInt(in,this->KMaximum1);
    this->GetInt(in,this->IMaximum2);
    this->GetInt(in,this->JMaximum2);
    this->GetInt(in,this->KMaximum2);
    this->GetInt(in,this->IJMaximum2);
    this->GetInt(in,this->IJKMaximum2);
    this->GetInt(in,this->MMAX);
    this->GetInt(in,this->DimensionIc);
    this->GetInt(in,this->DimensionBc);
    this->GetDouble(in,this->DeltaTime);
    this->GetDouble(in,this->XMinimum);
    this->GetDouble(in,this->XLength);
    this->GetDouble(in,this->YLength);
    this->GetDouble(in,this->ZLength);

    // 17 ints ... 5 doubles = 108 bytes
    this->SkipBytes(in,404);
    }
  else if(strcmp(this->Version, "RES = 01.04") == 0)
    {
    this->GetInt(in,this->IMinimum1);
    this->GetInt(in,this->JMinimum1);
    this->GetInt(in,this->KMinimum1);
    this->GetInt(in,this->IMaximum);
    this->GetInt(in,this->JMaximum);
    this->GetInt(in,this->KMaximum);
    this->GetInt(in,this->IMaximum1);
    this->GetInt(in,this->JMaximum1);
    this->GetInt(in,this->KMaximum1);
    this->GetInt(in,this->IMaximum2);
    this->GetInt(in,this->JMaximum2);
    this->GetInt(in,this->KMaximum2);
    this->GetInt(in,this->IJMaximum2);
    this->GetInt(in,this->IJKMaximum2);
    this->GetInt(in,this->MMAX);
    this->GetInt(in,this->DimensionIc);
    this->GetInt(in,this->DimensionBc);
    this->GetInt(in,this->DimensionC);
    this->GetDouble(in,this->DeltaTime);
    this->GetDouble(in,this->XMinimum);
    this->GetDouble(in,this->XLength);
    this->GetDouble(in,this->YLength);
    this->GetDouble(in,this->ZLength);

    // 18 ints ... 5 doubles = 112 bytes
    this->SkipBytes(in,400);
    }
  else if(strcmp(this->Version, "RES = 01.05") == 0)
    {
    this->GetInt(in,this->IMinimum1);
    this->GetInt(in,this->JMinimum1);
    this->GetInt(in,this->KMinimum1);
    this->GetInt(in,this->IMaximum);
    this->GetInt(in,this->JMaximum);
    this->GetInt(in,this->KMaximum);
    this->GetInt(in,this->IMaximum1);
    this->GetInt(in,this->JMaximum1);
    this->GetInt(in,this->KMaximum1);
    this->GetInt(in,this->IMaximum2);
    this->GetInt(in,this->JMaximum2);
    this->GetInt(in,this->KMaximum2);
    this->GetInt(in,this->IJMaximum2);
    this->GetInt(in,this->IJKMaximum2);
    this->GetInt(in,this->MMAX);
    this->GetInt(in,this->DimensionIc);
    this->GetInt(in,this->DimensionBc);
    this->GetInt(in,this->DimensionC);
    this->GetInt(in,this->DimensionIs);
    this->GetDouble(in,this->DeltaTime);
    this->GetDouble(in,this->XMinimum);
    this->GetDouble(in,this->XLength);
    this->GetDouble(in,this->YLength);
    this->GetDouble(in,this->ZLength);

    // 19 ints ... 5 doubles = 116 bytes
    this->SkipBytes(in,396);
    }
  else
    {
    this->GetInt(in,this->IMinimum1);
    this->GetInt(in,this->JMinimum1);
    this->GetInt(in,this->KMinimum1);
    this->GetInt(in,this->IMaximum);
    this->GetInt(in,this->JMaximum);
    this->GetInt(in,this->KMaximum);
    this->GetInt(in,this->IMaximum1);
    this->GetInt(in,this->JMaximum1);
    this->GetInt(in,this->KMaximum1);
    this->GetInt(in,this->IMaximum2);
    this->GetInt(in,this->JMaximum2);
    this->GetInt(in,this->KMaximum2);
    this->GetInt(in,this->IJMaximum2);
    this->GetInt(in,this->IJKMaximum2);
    this->GetInt(in,this->MMAX);
    this->GetInt(in,this->DimensionIc);
    this->GetInt(in,this->DimensionBc);
    this->GetInt(in,this->DimensionC);
    this->GetInt(in,this->DimensionIs);
    this->GetDouble(in,this->DeltaTime);
    this->GetDouble(in,this->XMinimum);
    this->GetDouble(in,this->XLength);
    this->GetDouble(in,this->YLength);
    this->GetDouble(in,this->ZLength);
    this->GetDouble(in,this->Ce);
    this->GetDouble(in,this->Cf);
    this->GetDouble(in,this->Phi);
    this->GetDouble(in,this->PhiW);

    // 19 ints ... 9 doubles = 148 bytes
    this->SkipBytes(in,364);
    }

  const int numberOfFloatsInBlock = 512/sizeof(float);

  if ( this->IJKMaximum2%numberOfFloatsInBlock == 0)
    {
    this->SPXRecordsPerTimestep = this->IJKMaximum2/numberOfFloatsInBlock;
    }
  else
    {
    this->SPXRecordsPerTimestep = 1 + this->IJKMaximum2/numberOfFloatsInBlock;
    }

  // C , C_name and nmax

  this->NMax->Resize(this->MMAX+1);
  for (int lc=0; lc<this->MMAX+1; ++lc)
    {
    this->NMax->InsertValue(lc, 1);
    }

  this->C->Resize(this->DimensionC);

  if (this->VersionNumber > 1.04)
    {
    this->GetBlockOfDoubles (in, this->C, this->DimensionC);

    for (int lc=0; lc<DimensionC; ++lc)
      {
      in.read(this->DataBuffer,512);  // c_name[]
      }

    if (this->VersionNumber < 1.12)
      {
      this->GetBlockOfInts(in, this->NMax,this->MMAX+1);
      }
    else
      {
      // what is the diff between this and above ???
      for (int lc=0; lc<this->MMAX+1; ++lc)
        {
        int temp;
        this->GetInt(in,temp);
        this->NMax->InsertValue(lc, temp);
        }
      this->SkipBytes(in,512-(this->MMAX+1)*sizeof(int));
      }
    }

  this->Dx->Resize(this->IMaximum2);
  this->Dy->Resize(this->JMaximum2);
  this->Dz->Resize(this->KMaximum2);

  this->GetBlockOfDoubles(in, this->Dx,this->IMaximum2);
  this->GetBlockOfDoubles(in, this->Dy,this->JMaximum2);
  this->GetBlockOfDoubles(in, this->Dz,this->KMaximum2);

  // RunName etc.

  memset(this->Units,0,17);
  memset(this->CoordinateSystem,0,17);

  in.read(this->DataBuffer,120);      // run_name , description
  in.read(this->Units,16);        // Units
  in.read(this->DataBuffer,16);       // run_type
  in.read(this->CoordinateSystem,16);  // CoordinateSystem

  this->SkipBytes(in,512-168);

  char tempCharArray[17];

  memset(tempCharArray,0,17);

  int ic = 0;
  for (int i=0; i<17; ++i)
    {
    if (this->Units[i] != ' ')
      {
      tempCharArray[ic++] = this->Units[i];
      }
    }

  memset(tempCharArray,0,17);

  ic = 0;
  for (int i=0; i<17; ++i)
    {
    if (this->CoordinateSystem[i] != ' ')
      {
      tempCharArray[ic++] = this->CoordinateSystem[i];
      }
    }
  strcpy(this->CoordinateSystem,tempCharArray);

  if (this->VersionNumber >= 1.04)
    {
    this->TempD->Resize(this->NMax->GetValue(0));
    this->GetBlockOfDoubles(in, this->TempD, this->NMax->GetValue(0)); // MW_g
    for (int i=0; i<this->MMAX; ++i)
      {
      in.read(this->DataBuffer,512);  // MW_s
      }
    }

  in.read(this->DataBuffer,512);  // D_p etc.

  // read in the "DimensionIc" variables (and ignore ... not used by ani_mfix)
  this->TempI->Resize(this->DimensionIc);
  this->TempD->Resize(this->DimensionIc);

  this->GetBlockOfDoubles(in, this->TempD,this->DimensionIc);  // ic_x_w
  this->GetBlockOfDoubles(in, this->TempD,this->DimensionIc);  // ic_x_e
  this->GetBlockOfDoubles(in, this->TempD,this->DimensionIc);  // ic_y_s
  this->GetBlockOfDoubles(in, this->TempD,this->DimensionIc);  // ic_y_n
  this->GetBlockOfDoubles(in, this->TempD,this->DimensionIc);  // ic_z_b
  this->GetBlockOfDoubles(in, this->TempD,this->DimensionIc);  // ic_z_t

  this->GetBlockOfInts(in, this->TempI,this->DimensionIc);  // ic_i_w
  this->GetBlockOfInts(in, this->TempI,this->DimensionIc);  // ic_i_e
  this->GetBlockOfInts(in, this->TempI,this->DimensionIc);  // ic_j_s
  this->GetBlockOfInts(in, this->TempI,this->DimensionIc);  // ic_j_n
  this->GetBlockOfInts(in, this->TempI,this->DimensionIc);  // ic_k_b
  this->GetBlockOfInts(in, this->TempI,this->DimensionIc);  // ic_k_t

  this->GetBlockOfDoubles(in, this->TempD,this->DimensionIc);  // ic_ep_g
  this->GetBlockOfDoubles(in, this->TempD,this->DimensionIc);  // ic_p_g
  this->GetBlockOfDoubles(in, this->TempD,this->DimensionIc);  // ic_t_g

  if (this->VersionNumber < 1.15)
    {
    this->GetBlockOfDoubles(in,this->TempD,this->DimensionIc);  // ic_t_s(1,1)
    this->GetBlockOfDoubles(in,this->TempD,this->DimensionIc);  // ic_t_s(1,2)
                                                                // or ic_tmp
    }

  if (this->VersionNumber >= 1.04)
    {
    for (int i=0; i<this->NMax->GetValue(0); ++i)
      {
      this->GetBlockOfDoubles(in,this->TempD,this->DimensionIc); // ic_x_g
      }
    }

  this->GetBlockOfDoubles(in,this->TempD,this->DimensionIc); // ic_u_g
  this->GetBlockOfDoubles(in,this->TempD,this->DimensionIc); // ic_v_g
  this->GetBlockOfDoubles(in,this->TempD,this->DimensionIc); // ic_w_g

  for (int lc=0; lc<this->MMAX; ++lc)
    {
    this->GetBlockOfDoubles(in,this->TempD,this->DimensionIc); // ic_rop_s
    this->GetBlockOfDoubles(in,this->TempD,this->DimensionIc); // ic_u_s
    this->GetBlockOfDoubles(in,this->TempD,this->DimensionIc); // ic_v_s
    this->GetBlockOfDoubles(in,this->TempD,this->DimensionIc); // ic_w_s

    if (this->VersionNumber >= 1.15)
      {
      this->GetBlockOfDoubles(in,this->TempD,this->DimensionIc); // ic_t_s
      }

    if (this->VersionNumber >= 1.04)
      {
      for (int n=0; n<this->NMax->GetValue(lc+1); ++n)
        {
        this->GetBlockOfDoubles(in,this->TempD,this->DimensionIc); // ic_x_s
        }
      }
    }

  // read in the "DimensionBc" variables (and ignore ... not used by ani_mfix)
  this->TempI->Resize(this->DimensionBc);
  this->TempD->Resize(this->DimensionBc);

  this->GetBlockOfDoubles(in,this->TempD,this->DimensionBc); // bc_x_w
  this->GetBlockOfDoubles(in,this->TempD,this->DimensionBc); // bc_x_e
  this->GetBlockOfDoubles(in,this->TempD,this->DimensionBc); // bc y s
  this->GetBlockOfDoubles(in,this->TempD,this->DimensionBc); // bc y n
  this->GetBlockOfDoubles(in,this->TempD,this->DimensionBc); // bc z b
  this->GetBlockOfDoubles(in,this->TempD,this->DimensionBc);  // bc z t
  this->GetBlockOfInts(in,this->TempI,this->DimensionBc);  // bc i w
  this->GetBlockOfInts(in,this->TempI,this->DimensionBc); // bc i e
  this->GetBlockOfInts(in,this->TempI,this->DimensionBc); // bc j s
  this->GetBlockOfInts(in,this->TempI,this->DimensionBc); // bc j n
  this->GetBlockOfInts(in,this->TempI,this->DimensionBc); // bc k b
  this->GetBlockOfInts(in,this->TempI,this->DimensionBc); // bc k t
  this->GetBlockOfDoubles(in,this->TempD,this->DimensionBc); // bc ep g
  this->GetBlockOfDoubles(in,this->TempD,this->DimensionBc); // bc p g
  this->GetBlockOfDoubles(in,this->TempD,this->DimensionBc); // bc t g

  if (VersionNumber < 1.15)
    {
    this->GetBlockOfDoubles(in,this->TempD,this->DimensionBc); // bc_t_s(1,1)
    this->GetBlockOfDoubles(in,this->TempD,this->DimensionBc); // bc_t_s(1,1)
                                                               // or bc_tmp
    }

  if (VersionNumber >= 1.04)
    {
    for (int i=0; i<this->NMax->GetValue(0); ++i)
      {
      this->GetBlockOfDoubles(in,this->TempD,this->DimensionBc); // bc_x_g
      }
    }

  this->GetBlockOfDoubles(in,this->TempD,this->DimensionBc); // bc u g
  this->GetBlockOfDoubles(in,this->TempD,this->DimensionBc); // bc v g
  this->GetBlockOfDoubles(in,this->TempD,this->DimensionBc); // bc w g
  this->GetBlockOfDoubles(in,this->TempD,this->DimensionBc); // bc ro g
  this->GetBlockOfDoubles(in,this->TempD,this->DimensionBc); // bc_rop_g
  this->GetBlockOfDoubles(in,this->TempD,this->DimensionBc); // bc volflow g
  this->GetBlockOfDoubles(in,this->TempD,this->DimensionBc); // bc massflow g

  for (int lc=0; lc<this->MMAX; ++lc)
    {
    this->GetBlockOfDoubles(in,this->TempD,this->DimensionBc); // bc rop s
    this->GetBlockOfDoubles(in,this->TempD,this->DimensionBc); // bc u s
    this->GetBlockOfDoubles(in,this->TempD,this->DimensionBc); // bc v s

    if (this->VersionNumber >= 1.04)
      {
      this->GetBlockOfDoubles(in,this->TempD,this->DimensionBc); // bc w s

      if (this->VersionNumber >= 1.15)
        {
        this->GetBlockOfDoubles(in,this->TempD,this->DimensionBc); // bc t s
        }
      for (int n=0; n<this->NMax->GetValue(lc+1); ++n)
        {
        this->GetBlockOfDoubles(in,this->TempD,this->DimensionBc); // bc x s
        }
      }
    this->GetBlockOfDoubles(in,this->TempD,this->DimensionBc); // bc volflow s
    this->GetBlockOfDoubles(in,this->TempD,this->DimensionBc); // bc massflow s
    }

  if (strcmp(this->Version,"RES = 01.00") == 0)
    {
    for (int lc=0; lc<10; ++lc)
      {
      in.read(this->DataBuffer,512); // BC TYPE
      }
    }
  else
    {
    for (int lc=0; lc<this->DimensionBc; ++lc)
      {
      in.read(this->DataBuffer,512); // BC TYPE
      }
    }

  this->Flag->Resize(this->IJKMaximum2);
  this->GetBlockOfInts(in, this->Flag,this->IJKMaximum2);

  // DimensionIs varibles (not needed by ani_mfix)
  this->TempI->Resize(this->DimensionIs);
  this->TempD->Resize(this->DimensionIs);

  if (this->VersionNumber >= 1.04)
    {
    this->GetBlockOfDoubles(in,this->TempD,this->DimensionIs); // is x w
    this->GetBlockOfDoubles(in,this->TempD,this->DimensionIs); // is x e
    this->GetBlockOfDoubles(in,this->TempD,this->DimensionIs); // is y s
    this->GetBlockOfDoubles(in,this->TempD,this->DimensionIs); // is y n
    this->GetBlockOfDoubles(in,this->TempD,this->DimensionIs); // is z b
    this->GetBlockOfDoubles(in,this->TempD,this->DimensionIs); // is z t
    this->GetBlockOfInts(in,this->TempI,this->DimensionIs); // is i w
    this->GetBlockOfInts(in,this->TempI,this->DimensionIs); // is i e
    this->GetBlockOfInts(in,this->TempI,this->DimensionIs); // is j s
    this->GetBlockOfInts(in,this->TempI,this->DimensionIs); // is j n
    this->GetBlockOfInts(in,this->TempI,this->DimensionIs); // is k b
    this->GetBlockOfInts(in,this->TempI,this->DimensionIs); // is k t
    this->GetBlockOfDoubles(in,this->TempD,this->DimensionIs);  // is_pc(1,1)
    this->GetBlockOfDoubles(in,this->TempD,this->DimensionIs);  // is_pc(1,2)

    if (this->VersionNumber >= 1.07)
      {
      for (int i=0; i<this->MMAX; ++i)
        {
        this->GetBlockOfDoubles(in,this->TempD,this->DimensionIs);//is_vel_s
        }
      }

    for (int lc=0; lc<this->DimensionIs; ++lc)
      {
      in.read(this->DataBuffer,512); // is_type
      }
    }

  if (this->VersionNumber >= 1.08)
    {
    in.read(this->DataBuffer,512);
    }

  if (this->VersionNumber >= 1.09)
    {
    in.read(this->DataBuffer,512);

    if (this->VersionNumber >= 1.5)
      {
      this->GetInt(in,this->NumberOfSPXFilesUsed);
      this->SkipBytes(in,508);
      }

    for (int lc=0; lc< this->NumberOfSPXFilesUsed; ++lc)
      {
      in.read(this->DataBuffer,512); // spx_dt
      }

    for (int lc=0; lc<this->MMAX+1; ++lc)
      {
      in.read(this->DataBuffer,512);    // species_eq
      }

    this->TempD->Resize(dimensionUsr);

    this->GetBlockOfDoubles(in,this->TempD,dimensionUsr); // usr_dt
    this->GetBlockOfDoubles(in,this->TempD,dimensionUsr); // usr x w
    this->GetBlockOfDoubles(in,this->TempD,dimensionUsr); // usr x e
    this->GetBlockOfDoubles(in,this->TempD,dimensionUsr); // usr y s
    this->GetBlockOfDoubles(in,this->TempD,dimensionUsr); // usr y n
    this->GetBlockOfDoubles(in,this->TempD,dimensionUsr); // usr z b
    this->GetBlockOfDoubles(in,this->TempD,dimensionUsr); // usr z t

    for (int lc=0; lc<dimensionUsr; ++lc)
      {
      in.read(this->DataBuffer,512);    // usr_ext etc.
      }

    this->TempD->Resize(this->DimensionIc);
    this->GetBlockOfDoubles(in,this->TempD,this->DimensionIc); // ic_p_star
    this->GetBlockOfDoubles(in,this->TempD,this->DimensionIc); // ic_l_scale
    for (int lc=0; lc<this->DimensionIc; ++lc)
      {
      in.read(this->DataBuffer,512);    // ic_type
      }

    this->TempD->Resize(DimensionBc);
    this->GetBlockOfDoubles(in,this->TempD,this->DimensionBc); // bc_dt_0
    this->GetBlockOfDoubles(in,this->TempD,this->DimensionBc); // bc_jet_g0
    this->GetBlockOfDoubles(in,this->TempD,this->DimensionBc); // bc_dt_h
    this->GetBlockOfDoubles(in,this->TempD,this->DimensionBc); // bc_jet_gh
    this->GetBlockOfDoubles(in,this->TempD,this->DimensionBc); // bc_dt_l
    this->GetBlockOfDoubles(in,this->TempD,this->DimensionBc); // bc_jet_gl
    }

  if (this->VersionNumber >= 1.1)
    {
    in.read(this->DataBuffer,512);  // mu_gmax
    }

  if (this->VersionNumber >= 1.11)
    {
    in.read(this->DataBuffer,512);  // x_ex , model_b
    }

  if (this->VersionNumber >= 1.12)
    {
    in.read(this->DataBuffer,512);   // p_ref , etc.
    in.read(this->DataBuffer,512);   // leq_it , leq_method

    this->GetBlockOfDoubles(in,this->TempD,this->DimensionBc); // bc_hw_g
    this->GetBlockOfDoubles(in,this->TempD,this->DimensionBc); // bc_uw_g
    this->GetBlockOfDoubles(in,this->TempD,this->DimensionBc); // bc_vw_g
    this->GetBlockOfDoubles(in,this->TempD,this->DimensionBc); // bc_ww_g

    for (int lc=0; lc<this->MMAX; ++lc)
      {
      this->GetBlockOfDoubles(in,this->TempD,this->DimensionBc); // bc_hw_s
      this->GetBlockOfDoubles(in,this->TempD,this->DimensionBc); // bc_uw_s
      this->GetBlockOfDoubles(in,this->TempD,this->DimensionBc); // bc_vw_s
      this->GetBlockOfDoubles(in,this->TempD,this->DimensionBc); // bc_ww_s
      }
    }

  if (this->VersionNumber >= 1.13)
    {
    in.read(this->DataBuffer,512);    // momentum_x_eq , etc.
    }

  if (this->VersionNumber >= 1.14)
    {
    in.read(this->DataBuffer,512);    // detect_small
    }

  if (this->VersionNumber >= 1.15)
    {
    in.read(this->DataBuffer,512);    // k_g0 , etc.

    this->TempD->Resize(this->DimensionIc);

    this->GetBlockOfDoubles(in,this->TempD,this->DimensionIc); // ic_gama_rg
    this->GetBlockOfDoubles(in,this->TempD,this->DimensionIc); // ic_t_rg

    for (int lc=0; lc<this->MMAX; ++lc)
      {
      this->GetBlockOfDoubles(in,this->TempD,this->DimensionIc); // ic_gama_rs
      this->GetBlockOfDoubles(in,this->TempD,this->DimensionIc); // ic_t_rs
      }
    }

  if (this->VersionNumber >= 1.2)
    {
    in.read(this->DataBuffer,512); // norm_g , norm_s
    }

  if (this->VersionNumber >= 1.3)
    {
    this->GetInt(in,this->NumberOfScalars);
    this->SkipBytes(in,sizeof(double)); // tol_resid_scalar

    int DIM_tmp;
    this->GetInt(in,DIM_tmp);
    this->SkipBytes(in,512-sizeof(double)-2*sizeof(int));

    this->TempI->Resize(DIM_tmp);
    this->GetBlockOfInts(in,this->TempI,DIM_tmp);  // Phase4Scalar;
    }

  if (this->VersionNumber >= 1.5)
    {
    this->GetInt(in,this->NumberOfReactionRates);
    this->SkipBytes(in,508);
    }

  if (this->VersionNumber >= 1.5999)
    {
    int tmp;
    this->GetInt(in,tmp);
    this->SkipBytes(in,508);

    if (tmp != 0)
      {
      this->BkEpsilon = true;
      }
    }
  if (this->VersionNumber >= 1.7999)
    {
    for( int i = 0; i < this->MMAX; ++i)
      {
      this->SkipBytes(in,512);
      }
    }
  in.close();
}

//----------------------------------------------------------------------------
void vtkMFIXReader::CreateVariableNames()
{
  char fileName[256];
  int cnt = 0;
  char uString[120];
  char vString[120];
  char wString[120];
  char svString[120];
  char tempString[120];
  char ropString[120];
  char temperatureString[120];
  char variableString[120];

  for (int i=0; i<this->NumberOfSPXFilesUsed; ++i)
    {
    for(int k = 0; k < (int)sizeof(fileName); k++)
      {
      fileName[k]=0;
      }
    strncpy(fileName, this->FileName, strlen(this->FileName)-4);

    if (i==0)
      {
      strcat(fileName, ".SP1");
      }
    else if (i==1)
      {
      strcat(fileName, ".SP2");
      }
    else if (i==2)
      {
      strcat(fileName, ".SP3");
      }
    else if (i==3)
      {
      strcat(fileName, ".SP4");
      }
    else if (i==4)
      {
      strcat(fileName, ".SP5");
      }
    else if (i==5)
      {
      strcat(fileName, ".SP6");
      }
    else if (i==6)
      {
      strcat(fileName, ".SP7");
      }
    else if (i==7)
      {
      strcat(fileName, ".SP8");
      }
    else if (i==8)
      {
      strcat(fileName, ".SP9");
      }
    else if (i==9)
      {
      strcat(fileName, ".SPA");
      }
    else
      {
      strcat(fileName, ".SPB");
      }

#ifdef _WIN32
    ifstream in(fileName,ios::binary);
#else
    ifstream in(fileName);
#endif
    if (in) // file exists
      {
      this->SpxFileExists->InsertValue(i, 1);

      switch (i+1)
        {

        case 1:
          this->VariableNames->InsertValue(cnt++,"EP_g");
          this->VariableIndexToSPX->InsertValue(cnt-1, 1);
          this->VariableComponents->InsertValue(cnt-1, 1);
          break;

        case 2:
          this->VariableNames->InsertValue(cnt++,"P_g");
          this->VariableIndexToSPX->InsertValue(cnt-1, 2);
          this->VariableComponents->InsertValue(cnt-1, 1);
          this->VariableNames->InsertValue(cnt++,"P_star");
          this->VariableIndexToSPX->InsertValue(cnt-1, 2);
          this->VariableComponents->InsertValue(cnt-1, 1);
          break;

        case 3:
          this->VariableNames->InsertValue(cnt++,"U_g");
          this->VariableIndexToSPX->InsertValue(cnt-1, 3);
          this->VariableComponents->InsertValue(cnt-1, 1);
          this->VariableNames->InsertValue(cnt++,"V_g");
          this->VariableIndexToSPX->InsertValue(cnt-1, 3);
          this->VariableComponents->InsertValue(cnt-1, 1);
          this->VariableNames->InsertValue(cnt++,"W_g");
          this->VariableIndexToSPX->InsertValue(cnt-1, 3);
          this->VariableComponents->InsertValue(cnt-1, 1);
          this->VariableNames->InsertValue(cnt++,"Gas Velocity");
          this->VariableIndexToSPX->InsertValue(cnt-1, 3);
          this->VariableComponents->InsertValue(cnt-1, 3);
          break;

        case 4:
          for (int j=0; j<this->MMAX; ++j)
            {
            for(int k=0;k<(int)sizeof(uString);k++)
              {
              uString[k]=0;
              }
            for(int k=0;k<(int)sizeof(vString);k++)
              {
              vString[k]=0;
              }
            for(int k=0;k<(int)sizeof(wString);k++)
              {
              wString[k]=0;
              }
            for(int k=0;k<(int)sizeof(svString);k++)
              {
              svString[k]=0;
              }
            strcpy(uString, "U_s_");
            strcpy(vString, "V_s_");
            strcpy(wString, "W_s_");
            strcpy(svString, "Solids_Velocity_");
            sprintf(tempString, "%d", j+1);
            strcat(uString, tempString);
            strcat(vString, tempString);
            strcat(wString, tempString);
            strcat(svString, tempString);
            this->VariableNames->InsertValue(cnt++, uString);
            this->VariableIndexToSPX->InsertValue(cnt-1, 4);
            this->VariableComponents->InsertValue(cnt-1, 1);

            this->VariableNames->InsertValue(cnt++, vString);
            this->VariableIndexToSPX->InsertValue(cnt-1, 4);
            this->VariableComponents->InsertValue(cnt-1, 1);

            this->VariableNames->InsertValue(cnt++, wString);
            this->VariableIndexToSPX->InsertValue(cnt-1, 4);
            this->VariableComponents->InsertValue(cnt-1, 1);

            this->VariableNames->InsertValue(cnt++, svString);
            this->VariableIndexToSPX->InsertValue(cnt-1, 4);
            this->VariableComponents->InsertValue(cnt-1, 3);
            }
          break;

        case 5:
          for (int j=0; j<this->MMAX; ++j)
            {
            for(int k=0;k<(int)sizeof(ropString);k++)
              {
              ropString[k]=0;
              }
            strcpy(ropString, "ROP_s_");
            sprintf(tempString, "%d", j+1);
            strcat(ropString, tempString);
            this->VariableNames->InsertValue(cnt++, ropString);
            this->VariableIndexToSPX->InsertValue(cnt-1, 5);
            this->VariableComponents->InsertValue(cnt-1, 1);
            }
          break;

        case 6:
          this->VariableNames->InsertValue(cnt++, "T_g");
          this->VariableIndexToSPX->InsertValue(cnt-1, 6);
          this->VariableComponents->InsertValue(cnt-1, 1);

          if (this->VersionNumber <= 1.15)
            {
            this->VariableNames->InsertValue(cnt++, "T_s_1");
            this->VariableIndexToSPX->InsertValue(cnt-1, 6);
            this->VariableComponents->InsertValue(cnt-1, 1);

            if (this->MMAX > 1)
              {
              this->VariableNames->InsertValue(cnt++, "T_s_2");
              this->VariableIndexToSPX->InsertValue(cnt-1, 6);
              this->VariableComponents->InsertValue(cnt-1, 1);
              }
            else
              {
              this->VariableNames->InsertValue(cnt++, "T_s_2_not_used");
              this->VariableIndexToSPX->InsertValue(cnt-1, 6);
              this->VariableComponents->InsertValue(cnt-1, 1);
              }
            }
          else
            {
            for (int j=0; j<this->MMAX; ++j)
              {
              for(int k=0;k<(int)sizeof(temperatureString);k++)
                {
                temperatureString[k]=0;
                }
              strcpy(temperatureString, "T_s_");
              sprintf(tempString, "%d", j+1);
              strcat(temperatureString, tempString);
              this->VariableNames->InsertValue(cnt++, temperatureString);
              this->VariableIndexToSPX->InsertValue(cnt-1, 6);
              this->VariableComponents->InsertValue(cnt-1, 1);
              }
            }
          break;

        case 7:
          for (int j=0; j<this->NMax->GetValue(0); ++j)
            {
            for (int k=0;k<(int)sizeof(variableString);k++)
              {
              variableString[k]=0;
              }
            strcpy(variableString, "X_g_");
            sprintf(tempString, "%d", j+1);
            strcat(variableString, tempString);
            this->VariableNames->InsertValue(cnt++, variableString);
            this->VariableIndexToSPX->InsertValue(cnt-1, 7);
            this->VariableComponents->InsertValue(cnt-1, 1);
            }

          for (int m=1; m<=this->MMAX; ++m)
            {
            for (int j=0; j<this->NMax->GetValue(m); ++j)
              {
              char tempString1[120];
              char tempString2[120];
              for (int k=0;k<(int)sizeof(variableString);k++)
                {
                variableString[k]=0;
                }
              strcpy(variableString, "X_s_");
              sprintf(tempString1, "%d", m);
              sprintf(tempString2, "%d", j+1);
              strcat(variableString, tempString1);
              strcat(variableString, "_");
              strcat(variableString, tempString2);
              this->VariableNames->InsertValue(cnt++, variableString);
              this->VariableIndexToSPX->InsertValue(cnt-1, 7);
              this->VariableComponents->InsertValue(cnt-1, 1);
              }
            }
          break;

        case 8:
          for (int j=0; j<MMAX; ++j)
            {
            for (int k=0;k<(int)sizeof(variableString);k++)
              {
              variableString[k]=0;
              }
            strcpy(variableString, "Theta_m_");
            sprintf(tempString, "%d", j+1);
            strcat(variableString, tempString);
            this->VariableNames->InsertValue(cnt++, variableString);
            this->VariableIndexToSPX->InsertValue(cnt-1, 8);
            this->VariableComponents->InsertValue(cnt-1, 1);
            }
          break;

        case 9:
          for (int j=0; j<this->NumberOfScalars; ++j)
            {
            for(int k=0;k<(int)sizeof(variableString);k++)
              {
              variableString[k]=0;
              }
            strcpy(variableString, "Scalar_");
            sprintf(tempString, "%d", j+1);
            strcat(variableString, tempString);
            this->VariableNames->InsertValue(cnt++, variableString);
            this->VariableIndexToSPX->InsertValue(cnt-1, 9);
            this->VariableComponents->InsertValue(cnt-1, 1);
            }
          break;

        case 10:
          for (int j=0; j<this->NumberOfReactionRates; ++j)
            {
            for (int k=0;k<(int)sizeof(variableString);k++)
              {
              variableString[k]=0;
              }
            strcpy(variableString, "RRates_");
            sprintf(tempString, "%d", j+1);
            strcat(variableString, tempString);
            this->VariableNames->InsertValue(cnt++, variableString);
            this->VariableIndexToSPX->InsertValue(cnt-1, 10);
            this->VariableComponents->InsertValue(cnt-1, 1);
            }
          break;

        case 11:
          if (BkEpsilon)
            {
            this->VariableNames->InsertValue(cnt++, "k_turb_g");
            this->VariableIndexToSPX->InsertValue(cnt-1, 11);
            this->VariableComponents->InsertValue(cnt-1, 1);
            this->VariableNames->InsertValue(cnt++, "e_turb_g");
            this->VariableIndexToSPX->InsertValue(cnt-1, 11);
            this->VariableComponents->InsertValue(cnt-1, 1);
            }
          break;
        default:
          vtkWarningMacro(<< "unknown SPx file : " << i << "\n");
          break;
        }
      }
    else
      {
      this->SpxFileExists->InsertValue(i, 0);
      }
    in.close();
    }
}

//----------------------------------------------------------------------------
void vtkMFIXReader::GetTimeSteps()
{
  int nextRecord, numberOfRecords;
  char fileName[256];
  int cnt = 0;

  for (int i=0; i<this->NumberOfSPXFilesUsed; ++i)
    {
    for (int k=0;k<(int)sizeof(fileName);k++)
      {
      fileName[k]=0;
      }
    strncpy(fileName, this->FileName, strlen(this->FileName)-4);
    if (i==0)
      {
      strcat(fileName, ".SP1");
      }
    else if (i==1)
      {
      strcat(fileName, ".SP2");
      }
    else if (i==2)
      {
      strcat(fileName, ".SP3");
      }
    else if (i==3)
      {
      strcat(fileName, ".SP4");
      }
    else if (i==4)
      {
      strcat(fileName, ".SP5");
      }
    else if (i==5)
      {
      strcat(fileName, ".SP6");
      }
    else if (i==6)
      {
      strcat(fileName, ".SP7");
      }
    else if (i==7)
      {
      strcat(fileName, ".SP8");
      }
    else if (i==8)
      {
      strcat(fileName, ".SP9");
      }
    else if (i==9)
      {
      strcat(fileName, ".SPA");
      }
    else
      {
      strcat(fileName, ".SPB");
      }
#ifdef _WIN32
    ifstream in(fileName , ios::binary);
#else
    ifstream in(fileName);
#endif

    int numberOfVariables=0;
    if (in) // file exists
      {
      in.clear();
      in.seekg( 1024, ios::beg );
      in.read( (char*)&nextRecord,sizeof(int) );
      this->SwapInt(nextRecord);
      in.read( (char*)&numberOfRecords,sizeof(int) );
      this->SwapInt(numberOfRecords);

      switch (i+1)
        {
        case 1:
          {
          numberOfVariables = 1;
          break;
          }
        case 2:
          {
          numberOfVariables = 2;
          break;
          }
        case 3:
          {
          numberOfVariables = 4;
          break;
          }
        case 4:
          {
          numberOfVariables = 4*this->MMAX;
          break;
          }
        case 5:
          {
          numberOfVariables = this->MMAX;
          break;
          }
        case 6:
          {
          if (this->VersionNumber <= 1.15)
            {
            numberOfVariables = 3;
            }
          else
            {
            numberOfVariables = this->MMAX + 1;
            }
          break;
          }
        case 7:
          {
          numberOfVariables = this->NMax->GetValue(0);
          for (int m=1; m<=this->MMAX; ++m)
            {
            numberOfVariables += this->NMax->GetValue(m);
            }
          break;
          }
        case 8:
          {
          numberOfVariables = this->MMAX;
          break;
          }
        case 9:
          {
          numberOfVariables = this->NumberOfScalars;
          break;
          }
        case 10:
          {
          numberOfVariables = this->NumberOfReactionRates;
          break;
          }
        case 11:
          {
          if (this->BkEpsilon)
            {
            numberOfVariables = 2;
            }
          break;
          }
        }

      for(int j=0; j<numberOfVariables; j++)
        {
        this->VariableTimesteps->InsertValue(cnt,
          (nextRecord-4)/numberOfRecords);
        cnt++;
        }
      }
    in.close();
    }
}

//----------------------------------------------------------------------------
void vtkMFIXReader::MakeTimeStepTable(int numberOfVariables)
{
  this->VariableTimestepTable->SetNumberOfComponents(numberOfVariables);

  for(int i=0; i<numberOfVariables; i++)
    {
    int timestepIncrement = (int)((float)this->MaximumTimestep/
                            (float)this->VariableTimesteps->GetValue(i) + 0.5);
    int timestep = 1;
    for (int j=0; j<this->MaximumTimestep; j++)
      {
      this->VariableTimestepTable->InsertComponent(j, i, timestep);
      timestepIncrement--;
      if (timestepIncrement <= 0)
        {
        timestepIncrement = (int)((float)this->MaximumTimestep/
                            (float)this->VariableTimesteps->GetValue(i) + 0.5);
        timestep++;
        }
      if (timestep > this->VariableTimesteps->GetValue(i))
        {
        timestep = this->VariableTimesteps->GetValue(i);
        }
      }
    }
}

//----------------------------------------------------------------------------
void vtkMFIXReader::GetVariableAtTimestep(int vari , int tstep,
  vtkFloatArray *v)
{
  // This routine opens and closes the file for each request.
  // Maybe keep all SPX files open, and just perform relative
  // moves to get to the correct location in the file
  // get filename that vaiable # vari is located in
  // assumptions : there are <10 solid phases,
  // <10 scalars and <10 ReactionRates (need to change this)

  char variableName[256];
  strcpy(variableName, this->VariableNames->GetValue(vari));
  int spx = this->VariableIndexToSPX->GetValue(vari);
  char fileName[256];

  for(int k=0;k<(int)sizeof(fileName);k++)
    {
    fileName[k]=0;
    }

  strncpy(fileName, this->FileName, strlen(this->FileName)-4);

  if (spx==1)
    {
    strcat(fileName, ".SP1");
    }
  else if (spx==2)
    {
    strcat(fileName, ".SP2");
    }
  else if (spx==3)
    {
    strcat(fileName, ".SP3");
    }
  else if (spx==4)
    {
    strcat(fileName, ".SP4");
    }
  else if (spx==5)
    {
    strcat(fileName, ".SP5");
    }
  else if (spx==6)
    {
    strcat(fileName, ".SP6");
    }
  else if (spx==7)
    {
    strcat(fileName, ".SP7");
    }
  else if (spx==8)
    {
    strcat(fileName, ".SP8");
    }
  else if (spx==9)
    {
    strcat(fileName, ".SP9");
    }
  else if (spx==10)
    {
    strcat(fileName, ".SPA");
    }
  else
    {
    strcat(fileName, ".SPB");
    }

  int index = (vari*this->MaximumTimestep) + tstep;
  int nBytesSkip = this->SPXTimestepIndexTable->GetValue(index);
#ifdef _WIN32
  ifstream in(fileName,ios::binary);
#else
  ifstream in(fileName);
#endif
  in.seekg(nBytesSkip,ios::beg);
  this->GetBlockOfFloats (in, v, this->IJKMaximum2);
  in.close();
}

//----------------------------------------------------------------------------
void vtkMFIXReader::MakeSPXTimeStepIndexTable(int nvars)
{

  int timestep;
  int spx;
  int NumberOfVariablesInSPX;

  for (int i=0; i<nvars; i++)
    {
    for (int j=0; j<this->MaximumTimestep; j++)
      {
      timestep = (int) this->VariableTimestepTable->GetComponent(j, i);
      spx = this->VariableIndexToSPX->GetValue(i);
      NumberOfVariablesInSPX = this->SPXToNVarTable->GetValue(spx);
      int skip = this->VariableToSkipTable->GetValue(i);
      int index = (3*512) + (timestep-1) *
        ((NumberOfVariablesInSPX*this->SPXRecordsPerTimestep*512)+512) +
        512 + (skip*this->SPXRecordsPerTimestep*512);
      int ind = (i*this->MaximumTimestep) + j;
      SPXTimestepIndexTable->InsertValue(ind, index);
      }
    }
}

//----------------------------------------------------------------------------
void vtkMFIXReader::CalculateMaxTimeStep()
{
  this->MaximumTimestep = 0;
  for ( int i=0; i <= this->VariableNames->GetMaxId(); i++ )
    {
    if (this->VariableTimesteps->GetValue(i) > this->MaximumTimestep)
      {
      this->MaximumTimestep = this->VariableTimesteps->GetValue(i);
      }
    }
}

//----------------------------------------------------------------------------
void vtkMFIXReader::GetNumberOfVariablesInSPXFiles()
{
  int NumberOfVariablesInSPX = 0;
  int skip = 0;
  for (int j=1; j<this->NumberOfSPXFilesUsed; j++)
    {
    for(int i=0;i<=this->VariableNames->GetMaxId();i++)
      {
      if ((this->VariableIndexToSPX->GetValue(i) == j)
        && (this->VariableComponents->GetValue(i) == 1))
        {
        NumberOfVariablesInSPX++;
        this->VariableToSkipTable->InsertValue(i,skip);
        skip++;
        }
      }
    this->SPXToNVarTable->InsertValue(j, NumberOfVariablesInSPX);
    NumberOfVariablesInSPX = 0;
    skip = 0;
    }
}

//----------------------------------------------------------------------------
void vtkMFIXReader::FillVectorVariable( int xindex, int yindex,
  int zindex, vtkFloatArray *v)
{
  for(int i=0;i<=this->CellDataArray[xindex]->GetMaxId();i++)
    {
    v->InsertComponent(i, 0, this->CellDataArray[xindex]->GetValue(i));
    v->InsertComponent(i, 1, this->CellDataArray[yindex]->GetValue(i));
    v->InsertComponent(i, 2, this->CellDataArray[zindex]->GetValue(i));
    }
  v->Modified();
}

//----------------------------------------------------------------------------
void vtkMFIXReader::ConvertVectorFromCylindricalToCartesian( int xindex,
  int zindex)
{
  int count = 0;
  double radius = 0.0;
  double y = 0.0;
  double theta = 0.0;
  int cnt=0;

  for (int k=0; k< this->KMaximum2; k++)
    {
    for (int j=0; j< this->JMaximum2; j++)
      {
      for (int i=0; i< this->IMaximum2; i++)
        {
        if ( this->Flag->GetValue(cnt) < 10 )
          {
          double ucart = (this->CellDataArray[xindex]->
            GetValue(count)*cos(theta)) -
            (this->CellDataArray[zindex]->GetValue(count)*sin(theta));
          double wcart = (this->CellDataArray[xindex]->
            GetValue(count)*sin(theta)) +
            (this->CellDataArray[zindex]->GetValue(count)*cos(theta));
          this->CellDataArray[xindex]->InsertValue(count, (float)ucart);
          this->CellDataArray[zindex]->InsertValue(count, (float)wcart);
          count++;
          }
        cnt++;
        radius = radius + this->Dx->GetValue(i);
        }
      radius = 0.0;
      y = y + this->Dy->GetValue(j);
      }
    y = 0.0;
    theta = theta + this->Dz->GetValue(k);
    }
}

//----------------------------------------------------------------------------
void vtkMFIXReader::GetAllTimes(vtkInformationVector *outputVector)
{
  int max = 0;
  int maxVar = 0;

  for(int j=0; j<=this->VariableNames->GetMaxId(); j++)
    {
    int n = this->VariableTimesteps->GetValue(j);
    if (n > max)
      {
      max = n;
      maxVar = j;
      }
    }

  char fileName[256];

  for(int k=0;k<(int)sizeof(fileName);k++)
    {
    fileName[k]=0;
    }
  strncpy(fileName, this->FileName, strlen(this->FileName)-4);

  if (maxVar==0)
    {
    strcat(fileName, ".SP1");
    }
  else if (maxVar==1)
    {
    strcat(fileName, ".SP2");
    }
  else if (maxVar==2)
    {
    strcat(fileName, ".SP3");
    }
  else if (maxVar==3)
    {
    strcat(fileName, ".SP4");
    }
  else if (maxVar==4)
    {
    strcat(fileName, ".SP5");
    }
  else if (maxVar==5)
    {
    strcat(fileName, ".SP6");
    }
  else if (maxVar==6)
    {
    strcat(fileName, ".SP7");
    }
  else if (maxVar==7)
    {
    strcat(fileName, ".SP8");
    }
  else if (maxVar==8)
    {
    strcat(fileName, ".SP9");
    }
  else if (maxVar==9)
    {
    strcat(fileName, ".SPA");
    }
  else
    {
    strcat(fileName, ".SPB");
    }

#ifdef _WIN32
  ifstream tfile(fileName, ios::binary);
#else
  ifstream tfile(fileName);
#endif

  int numberOfVariablesInSPX =
    this->SPXToNVarTable->GetValue(this->VariableIndexToSPX->GetValue(maxVar));
  int offset = 512-(int)sizeof(float) +
    512*(numberOfVariablesInSPX*SPXRecordsPerTimestep);
  tfile.clear();
  tfile.seekg( 3*512, ios::beg ); // first time
  float time;
  double* steps = new double[this->NumberOfTimeSteps];

  for (int i = 0; i < this->NumberOfTimeSteps; i++)
    {
    tfile.read( (char*)&time,sizeof(float) );
    SwapFloat(time);
    steps[i] = (double)time;
    tfile.seekg(offset,ios::cur);
    }

  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_STEPS(),
    steps, this->NumberOfTimeSteps);
  double timeRange[2];
  timeRange[0] = steps[0];
  timeRange[1] = steps[this->NumberOfTimeSteps - 1];
  outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_RANGE(), timeRange, 2);

  tfile.close();
  delete [] steps;
}
