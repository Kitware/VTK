/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRandomAttributeGenerator.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkRandomAttributeGenerator.h"

#include "vtkDataSet.h"
#include "vtkPointData.h"
#include "vtkCellData.h"
#include "vtkFieldData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"

#include "vtkBitArray.h"
#include "vtkCharArray.h"
#include "vtkUnsignedCharArray.h"
#include "vtkShortArray.h"
#include "vtkUnsignedShortArray.h"
#include "vtkIntArray.h"
#include "vtkUnsignedIntArray.h"
#include "vtkLongArray.h"
#include "vtkUnsignedLongArray.h"
#include "vtkFloatArray.h"
#include "vtkDoubleArray.h"
#include "vtkIdTypeArray.h"

vtkStandardNewMacro(vtkRandomAttributeGenerator);

// ----------------------------------------------------------------------------
vtkRandomAttributeGenerator::vtkRandomAttributeGenerator()
{
  this->DataType = VTK_FLOAT;
  this->NumberOfComponents = 1;
  this->NumberOfTuples = 0;
  this->MinimumComponentValue = 0.0;
  this->MaximumComponentValue = 1.0;

  this->GeneratePointScalars = 0;
  this->GeneratePointVectors = 0;
  this->GeneratePointNormals = 0;
  this->GeneratePointTCoords = 0;
  this->GeneratePointTensors = 0;
  this->GeneratePointArray = 0;

  this->GenerateCellScalars = 0;
  this->GenerateCellVectors = 0;
  this->GenerateCellNormals = 0;
  this->GenerateCellTCoords = 0;
  this->GenerateCellTensors = 0;
  this->GenerateCellArray = 0;

  this->GenerateFieldArray = 0;
}

// ----------------------------------------------------------------------------
// This function template creates random attributes within a given range. It is
// assumed that the input data array may have a variable number of components.
template <class T>
void vtkRandomAttributeGeneratorExecute(vtkRandomAttributeGenerator *self,
                                        T *data,
                                        vtkIdType numTuples,
                                        int numComp,
                                        int minComp,
                                        int maxComp,
                                        double min,
                                        double max)
{
  vtkIdType total = numComp * numTuples;
  vtkIdType tenth = total/10 + 1;
  for ( vtkIdType i=0; i < numTuples; i++ )
    {
    for ( int comp=minComp; comp <= maxComp; comp++ )
      {
      // update progess and check for aborts
      if ( ! (i % tenth) )
        {
        self->UpdateProgress(static_cast<double>(i)/total);
        if ( self->GetAbortExecute() )
          {
          break;
          }
        }

      // Now generate a random component value
      data[i*numComp + comp] = static_cast<T>(vtkMath::Random(min,max));
      }//for each component
    }//for each tuple
}

// ----------------------------------------------------------------------------
// This method does the data type allocation and switching for various types.
vtkDataArray *vtkRandomAttributeGenerator::GenerateData(int dataType,
                                                        vtkIdType numTuples,
                                                        int numComp,
                                                        int minComp,
                                                        int maxComp,
                                                        double min,
                                                        double max)
{
  vtkDataArray *dataArray=NULL;

  switch(dataType)
    {
    case VTK_CHAR:
      {
      dataArray = vtkCharArray::New();
      dataArray->SetNumberOfComponents(numComp);
      dataArray->SetNumberOfTuples(numTuples);
      char *data = static_cast<vtkCharArray*>(dataArray)->GetPointer(0);
      vtkRandomAttributeGeneratorExecute(this,data,numTuples,numComp,
                                         minComp,maxComp,min,max);
      }
      break;
    case VTK_UNSIGNED_CHAR:
      {
      dataArray = vtkUnsignedCharArray::New();
      dataArray->SetNumberOfComponents(numComp);
      dataArray->SetNumberOfTuples(numTuples);
      unsigned char *data=
        static_cast<vtkUnsignedCharArray*>(dataArray)->GetPointer(0);
      vtkRandomAttributeGeneratorExecute(this, data,numTuples,numComp,
                                         minComp,maxComp,min,max);
      }
      break;
    case VTK_SHORT:
      {
      dataArray = vtkShortArray::New();
      dataArray->SetNumberOfComponents(numComp);
      dataArray->SetNumberOfTuples(numTuples);
      short *data = static_cast<vtkShortArray*>(dataArray)->GetPointer(0);
      vtkRandomAttributeGeneratorExecute(this, data,numTuples,numComp,
                                         minComp,maxComp,min,max);
      }
      break;
    case VTK_UNSIGNED_SHORT:
      {
      dataArray = vtkUnsignedShortArray::New();
      dataArray->SetNumberOfComponents(numComp);
      dataArray->SetNumberOfTuples(numTuples);
      unsigned short *data=
        static_cast<vtkUnsignedShortArray*>(dataArray)->GetPointer(0);
      vtkRandomAttributeGeneratorExecute(this, data,numTuples,numComp,
                                         minComp,maxComp,min,max);
      }
      break;
    case VTK_INT:
      {
      dataArray = vtkIntArray::New();
      dataArray->SetNumberOfComponents(numComp);
      dataArray->SetNumberOfTuples(numTuples);
      int *data = static_cast<vtkIntArray*>(dataArray)->GetPointer(0);
      vtkRandomAttributeGeneratorExecute(this, data,numTuples,numComp,
                                         minComp,maxComp,min,max);
      }
      break;
    case VTK_UNSIGNED_INT:
      {
      dataArray = vtkUnsignedIntArray::New();
      dataArray->SetNumberOfComponents(numComp);
      dataArray->SetNumberOfTuples(numTuples);
      unsigned int *data =
        static_cast<vtkUnsignedIntArray*>(dataArray)->GetPointer(0);
      vtkRandomAttributeGeneratorExecute(this, data,numTuples,numComp,
                                         minComp,maxComp,min,max);
      }
      break;
    case VTK_LONG:
      {
      dataArray = vtkLongArray::New();
      dataArray->SetNumberOfComponents(numComp);
      dataArray->SetNumberOfTuples(numTuples);
      long *data = static_cast<vtkLongArray*>(dataArray)->GetPointer(0);
      vtkRandomAttributeGeneratorExecute(this, data,numTuples,numComp,
                                         minComp,maxComp,min,max);
      }
      break;
    case VTK_UNSIGNED_LONG:
      {
      dataArray = vtkUnsignedLongArray::New();
      dataArray->SetNumberOfComponents(numComp);
      dataArray->SetNumberOfTuples(numTuples);
      unsigned long *data =
        static_cast<vtkUnsignedLongArray*>(dataArray)->GetPointer(0);
      vtkRandomAttributeGeneratorExecute(this, data,numTuples,numComp,
                                         minComp,maxComp,min,max);
      }
      break;
    case VTK_FLOAT:
      {
      dataArray = vtkFloatArray::New();
      dataArray->SetNumberOfComponents(numComp);
      dataArray->SetNumberOfTuples(numTuples);
      float *data = static_cast<vtkFloatArray*>(dataArray)->GetPointer(0);
      vtkRandomAttributeGeneratorExecute(this, data,numTuples,numComp,
                                         minComp,maxComp,min,max);
      }
      break;
    case VTK_DOUBLE:
      {
      dataArray = vtkDoubleArray::New();
      dataArray->SetNumberOfComponents(numComp);
      dataArray->SetNumberOfTuples(numTuples);
      double *data = static_cast<vtkDoubleArray*>(dataArray)->GetPointer(0);
      vtkRandomAttributeGeneratorExecute(this, data,numTuples,numComp,
                                         minComp,maxComp,min,max);
      }
      break;
    case VTK_ID_TYPE:
      {
      dataArray = vtkIdTypeArray::New();
      dataArray->SetNumberOfComponents(numComp);
      dataArray->SetNumberOfTuples(numTuples);
      vtkIdType *data = static_cast<vtkIdTypeArray*>(dataArray)->GetPointer(0);
      vtkRandomAttributeGeneratorExecute(this, data,numTuples,numComp,
                                         minComp,maxComp,min,max);
      }
      break;
    case VTK_BIT: //we'll do something special for bit arrays
      {
      vtkIdType total = numComp * numTuples;
      vtkIdType tenth = total/10 + 1;
      dataArray = vtkBitArray::New();
      dataArray->SetNumberOfComponents(numComp);
      dataArray->SetNumberOfTuples(numTuples);
      for ( vtkIdType i=0; i < numTuples; i++ )
        {
        for ( int comp=minComp; comp <= maxComp; comp++ )
          {
          // update progess and check for aborts
          if ( ! (i % tenth) )
            {
            this->UpdateProgress(static_cast<double>(i)/total);
            if ( this->GetAbortExecute() )
              {
              break;
              }
            }

          // Now generate a random component value
          dataArray->SetComponent(i,comp,
                                  vtkMath::Random(0.0,1.0)<0.5?0:1);
          }//for each component
        }//for each tuple
      }
      break;

    default:
      vtkGenericWarningMacro("Cannot create random data array\n");
    }

  return dataArray;
}

// ----------------------------------------------------------------------------
int vtkRandomAttributeGenerator::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkDataSet *input = vtkDataSet::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkDataSet *output = vtkDataSet::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  // First, copy the input to the output as a starting point
  output->CopyStructure( input );

  vtkDebugMacro(<< "Producing random attributes");
  vtkIdType numPts = input->GetNumberOfPoints();
  vtkIdType numCells = input->GetNumberOfCells();

  if ( numPts < 1 )
    {
    vtkDebugMacro(<< "No input!");
    return 1;
    }

  // Configure the output
  output->CopyStructure(input);
  output->CopyAttributes(input);

  // Produce the appropriate output
  // First the point data
  if ( this->GeneratePointScalars)
    {
    vtkDataArray *ptScalars = this->GenerateData(this->DataType,numPts,
                                                 this->NumberOfComponents,0,
                                                 this->NumberOfComponents-1,
                                                 this->MinimumComponentValue,
                                                 this->MaximumComponentValue);
    output->GetPointData()->SetScalars(ptScalars);
    ptScalars->Delete();
    }
  if ( this->GeneratePointVectors)
    {
    vtkDataArray *ptVectors = this->GenerateData(this->DataType,numPts,3,0,2,
                                                 this->MinimumComponentValue,
                                                 this->MaximumComponentValue);
    output->GetPointData()->SetVectors(ptVectors);
    ptVectors->Delete();
    }
  if ( this->GeneratePointNormals)
    {
    vtkDataArray *ptNormals = this->GenerateData(this->DataType,numPts,3,0,2,
                                                 this->MinimumComponentValue,
                                                 this->MaximumComponentValue);
    double v[3];
    for ( vtkIdType id=0; id < numPts; id++ )
      {
      ptNormals->GetTuple(id,v);
      vtkMath::Normalize(v);
      ptNormals->SetTuple(id,v);
      }
    output->GetPointData()->SetNormals(ptNormals);
    ptNormals->Delete();
    }
  if ( this->GeneratePointTensors)
    {
    // fill in 6 components, and then shift them around to make them symmetric
    vtkDataArray *ptTensors = this->GenerateData(this->DataType,numPts,9,0,5,
                                                 this->MinimumComponentValue,
                                                 this->MaximumComponentValue);
    double t[9];
    for ( vtkIdType id=0; id < numPts; id++ )
      {
      ptTensors->GetTuple(id,t);
      t[8] = t[3];//make sure the tensor is symmetric
      t[3] = t[1];
      t[6] = t[2];
      t[7] = t[5];
      ptTensors->SetTuple(id,t);
      }
    output->GetPointData()->SetTensors(ptTensors);
    ptTensors->Delete();
    }
  if ( this->GeneratePointTCoords)
    {
    int numComp = this->NumberOfComponents < 1 ? 1
      :(this->NumberOfComponents > 3 ? 3 : this->NumberOfComponents);
    vtkDataArray *ptTCoords = this->GenerateData(this->DataType,numPts,numComp,
                                                 0,this->NumberOfComponents-1,
                                                 this->MinimumComponentValue,
                                                 this->MaximumComponentValue);
    output->GetPointData()->SetTCoords(ptTCoords);
    ptTCoords->Delete();
    }
  if ( this->GeneratePointArray)
    {
    vtkDataArray *ptScalars = this->GenerateData(this->DataType,numPts,
                                                 this->NumberOfComponents,0,
                                                 this->NumberOfComponents-1,
                                                 this->MinimumComponentValue,
                                                 this->MaximumComponentValue);
    output->GetPointData()->SetScalars(ptScalars);
    ptScalars->Delete();
    }

  // Now the cell data
  if ( this->GenerateCellScalars)
    {
    vtkDataArray *ptScalars = this->GenerateData(this->DataType,numCells,
                                                 this->NumberOfComponents,0,
                                                 this->NumberOfComponents-1,
                                                 this->MinimumComponentValue,
                                                 this->MaximumComponentValue);
    output->GetCellData()->SetScalars(ptScalars);
    ptScalars->Delete();
    }
  if ( this->GenerateCellVectors)
    {
    vtkDataArray *ptVectors = this->GenerateData(this->DataType,numCells,3,0,2,
                                                 this->MinimumComponentValue,
                                                 this->MaximumComponentValue);
    output->GetCellData()->SetVectors(ptVectors);
    ptVectors->Delete();
    }
  if ( this->GenerateCellNormals)
    {
    vtkDataArray *ptNormals = this->GenerateData(this->DataType,numCells,3,0,2,
                                                 this->MinimumComponentValue,
                                                 this->MaximumComponentValue);
    double v[3];
    for ( vtkIdType id=0; id < numCells; id++ )
      {
      ptNormals->GetTuple(id,v);
      vtkMath::Normalize(v);
      ptNormals->SetTuple(id,v);
      }
    output->GetCellData()->SetNormals(ptNormals);
    ptNormals->Delete();
    }
  if ( this->GenerateCellTensors)
    {
    vtkDataArray *ptTensors = this->GenerateData(this->DataType,numCells,9,0,5,
                                                 this->MinimumComponentValue,
                                                 this->MaximumComponentValue);
    double t[9];
    for ( vtkIdType id=0; id < numCells; id++ )
      {
      ptTensors->GetTuple(id,t);
      t[6] = t[1];//make sure the tensor is symmetric
      t[7] = t[2];
      t[8] = t[4];
      ptTensors->SetTuple(id,t);
      }
    output->GetCellData()->SetTensors(ptTensors);
    ptTensors->Delete();
    }
  if ( this->GenerateCellTCoords)
    {
    int numComp = this->NumberOfComponents < 1 ? 1
      : (this->NumberOfComponents > 3 ? 3 : this->NumberOfComponents);
    vtkDataArray *ptTCoords = this->GenerateData(this->DataType,numCells,
                                                 numComp,0,
                                                 this->NumberOfComponents-1,
                                                 this->MinimumComponentValue,
                                                 this->MaximumComponentValue);
    output->GetCellData()->SetTCoords(ptTCoords);
    ptTCoords->Delete();
    }
  if ( this->GenerateCellArray)
    {
    vtkDataArray *ptScalars = this->GenerateData(this->DataType,numCells,
                                                 this->NumberOfComponents,0,
                                                 this->NumberOfComponents-1,
                                                 this->MinimumComponentValue,
                                                 this->MaximumComponentValue);
    output->GetCellData()->SetScalars(ptScalars);
    ptScalars->Delete();
    }

  // Finally any field data
  if ( this->GenerateFieldArray)
    {
    vtkDataArray *data = this->GenerateData(this->DataType,
                                            this->NumberOfTuples,
                                            this->NumberOfComponents,0,
                                            this->NumberOfComponents-1,
                                            this->MinimumComponentValue,
                                            this->MaximumComponentValue);
    output->GetFieldData()->AddArray(data);
    data->Delete();
    }

  return 1;
}

// ----------------------------------------------------------------------------
void vtkRandomAttributeGenerator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Data Type: " << this->DataType << endl;
  os << indent << "Number of Components: " << this->NumberOfComponents << endl;
  os << indent << "Number of Tuples: " << this->NumberOfTuples << endl;
  os << indent << "Minimum Component Value: " << this->MinimumComponentValue
     << endl;
  os << indent << "Maximum Component Value: " << this->MaximumComponentValue
     << endl;

  os << indent << "Generate Point Scalars: "
     << (this->GeneratePointScalars ? "On\n" : "Off\n");
  os << indent << "Generate Point Vectors: "
     << (this->GeneratePointVectors ? "On\n" : "Off\n");
  os << indent << "Generate Point Normals: "
     << (this->GeneratePointNormals ? "On\n" : "Off\n");
  os << indent << "Generate Point TCoords: "
     << (this->GeneratePointTCoords ? "On\n" : "Off\n");
  os << indent << "Generate Point Tensors: "
     << (this->GeneratePointTensors ? "On\n" : "Off\n");
  os << indent << "Generate Point Array: "
     << (this->GeneratePointArray ? "On\n" : "Off\n");

  os << indent << "Generate Cell Scalars: "
     << (this->GenerateCellScalars ? "On\n" : "Off\n");
  os << indent << "Generate Cell Vectors: "
     << (this->GenerateCellVectors ? "On\n" : "Off\n");
  os << indent << "Generate Cell Normals: "
     << (this->GenerateCellNormals ? "On\n" : "Off\n");
  os << indent << "Generate Cell TCoords: "
     << (this->GenerateCellTCoords ? "On\n" : "Off\n");
  os << indent << "Generate Cell Tensors: "
     << (this->GenerateCellTensors ? "On\n" : "Off\n");
  os << indent << "Generate Cell Array: "
     << (this->GenerateCellArray ? "On\n" : "Off\n");

  os << indent << "Generate Field Array: "
     << (this->GenerateFieldArray ? "On\n" : "Off\n");
}
