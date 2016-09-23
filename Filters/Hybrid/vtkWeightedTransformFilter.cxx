/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWeightedTransformFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkWeightedTransformFilter.h"

#include "vtkCellData.h"
#include "vtkFieldData.h"
#include "vtkFloatArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkLinearTransform.h"
#include "vtkMath.h"
#include "vtkMatrix4x4.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPointSet.h"
#include "vtkUnsignedShortArray.h"

vtkStandardNewMacro(vtkWeightedTransformFilter);

// helper functions.  Can't easily get to these in Matrix4x4 as written.

static inline void LinearTransformVector(double matrix[4][4],
                                         double in[3], double out[3])
{
  out[0] = matrix[0][0]*in[0] + matrix[0][1]*in[1] + matrix[0][2]*in[2];
  out[1] = matrix[1][0]*in[0] + matrix[1][1]*in[1] + matrix[1][2]*in[2];
  out[2] = matrix[2][0]*in[0] + matrix[2][1]*in[1] + matrix[2][2]*in[2];
}

static inline void LinearTransformPoint(double mtx[4][4],
                                        double in[3], double out[3])
{
  out[0] = mtx[0][0]*in[0]+mtx[0][1]*in[1]+mtx[0][2]*in[2]+mtx[0][3];
  out[1] = mtx[1][0]*in[0]+mtx[1][1]*in[1]+mtx[1][2]*in[2]+mtx[1][3];
  out[2] = mtx[2][0]*in[0]+mtx[2][1]*in[1]+mtx[2][2]*in[2]+mtx[2][3];
}

//----------------------------------------------------------------------------
vtkWeightedTransformFilter::vtkWeightedTransformFilter()
{
  this->AddInputValues = 0;
  this->Transforms = NULL;
  this->NumberOfTransforms = 0;

  this->CellDataWeightArray = NULL;
  this->WeightArray = NULL;
  this->CellDataTransformIndexArray = NULL;
  this->TransformIndexArray = NULL;
}

//----------------------------------------------------------------------------
vtkWeightedTransformFilter::~vtkWeightedTransformFilter()
{
  int i;

  if(this->Transforms != NULL)
  {
    for(i = 0; i < this->NumberOfTransforms; i++)
    {
      if(this->Transforms[i] != NULL)
      {
        this->Transforms[i]->UnRegister(this);
      }
    }
    delete [] this->Transforms;
  }

  // Setting these strings to NULL has the side-effect of deleting them.
  this->SetCellDataWeightArray(NULL);
  this->SetWeightArray(NULL);
  this->SetCellDataTransformIndexArray(NULL);
  this->SetTransformIndexArray(NULL);
}

//----------------------------------------------------------------------------
void vtkWeightedTransformFilter::SetNumberOfTransforms(int num)
{
  int i;
  vtkAbstractTransform **newTransforms;

  if(num < 0)
  {
    vtkErrorMacro(<< "Cannot set transform count below zero");
    return;
  }

  if(this->Transforms == NULL)
  {
    // first time
    this->Transforms = new vtkAbstractTransform*[num];
    for(i = 0; i < num; i++)
    {
      this->Transforms[i] = NULL;
    }
    this->NumberOfTransforms = num;
    return;
  }

  if(num == this->NumberOfTransforms)
  {
    return;
  }

  if(num < this->NumberOfTransforms)
  {
    // create a smaller array, free up references to cut-off elements,
    // and copy other elements
    for(i = num; i < this->NumberOfTransforms; i++)
    {
      if(this->Transforms[i] != NULL)
      {
        this->Transforms[i]->UnRegister(this);
        this->Transforms[i] = NULL;
      }
    }
    newTransforms = new vtkAbstractTransform*[num];
    for(i = 0; i < num; i++)
    {
      newTransforms[i] = this->Transforms[i];
    }
    delete [] this->Transforms;
    this->Transforms = newTransforms;
  }
  else
  {
    // create a new array and copy elements, no unregistering needed.
    newTransforms = new vtkAbstractTransform*[num];
    for(i = 0; i < this->NumberOfTransforms; i++) {
    newTransforms[i] = this->Transforms[i];
    }

    for(i = this->NumberOfTransforms; i < num; i++)
    {
      newTransforms[i] = NULL;
    }
    delete [] this->Transforms;
    this->Transforms = newTransforms;
  }

  this->NumberOfTransforms = num;
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkWeightedTransformFilter::SetTransform(vtkAbstractTransform *trans,
                                              int num)
{
  if(num < 0) {
  vtkErrorMacro(<<"Transform number must be greater than 0");
  return;
  }

  if(num >= this->NumberOfTransforms)
  {
    vtkErrorMacro(<<"Transform number exceeds maximum of " <<
    this->NumberOfTransforms);
    return;
  }
  if(this->Transforms[num] != NULL)
  {
    this->Transforms[num]->UnRegister(this);
  }
  this->Transforms[num] = trans;
  if(trans != NULL)
  {
    trans->Register(this);
  }
  this->Modified();
}

//----------------------------------------------------------------------------
vtkAbstractTransform *vtkWeightedTransformFilter::GetTransform(int num)
{
  if(num < 0)
  {
    vtkErrorMacro(<<"Transform number must be greater than 0");
    return NULL;
  }

  if(num >= this->NumberOfTransforms)
  {
    vtkErrorMacro(<<"Transform number exceeds maximum of " <<
    this->NumberOfTransforms);
    return NULL;
  }

  return this->Transforms[num];
}

//----------------------------------------------------------------------------
int vtkWeightedTransformFilter::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkPointSet *input = vtkPointSet::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkPointSet *output = vtkPointSet::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkPoints *inPts;
  vtkPoints *newPts;
  vtkDataArray *inVectors, *inCellVectors;
  vtkDataArray *inNormals, *inCellNormals;
  vtkFloatArray *newVectors=NULL, *newCellVectors=NULL;
  vtkFloatArray *newNormals=NULL, *newCellNormals=NULL;
  vtkIdType numPts, numCells, p;
  int activeTransforms, allLinear;
  int i, c, tidx;
  int pdComponents, cdComponents;
  double **linearPtMtx;
  double **linearNormMtx;
  double inVec[3], inPt[3], inNorm[3];
  double xformNorm[3], cumNorm[3];
  double xformPt[3], cumPt[3];
  double xformVec[3], cumVec[3];
  double derivMatrix[3][3];
  float *weights = NULL;
  unsigned short *transformIndices = NULL;
  double thisWeight;
  vtkDataArray *pdArray, *cdArray;
  vtkUnsignedShortArray *tiArray, *cdtiArray;
  vtkFieldData *fd;
  vtkPointData *pd=input->GetPointData(), *outPD=output->GetPointData();
  vtkCellData *cd=input->GetCellData(), *outCD=output->GetCellData();
  vtkLinearTransform *linearTransform;

  vtkDebugMacro(<<"Executing weighted transform filter");

  // First, copy the input to the output as a starting point
  output->CopyStructure( input );

  // Check input
  //
  if ( this->Transforms == NULL || this->NumberOfTransforms == 0)
  {
    vtkErrorMacro(<<"No transform defined!");
    return 1;
  }

  activeTransforms = 0;
  for(c = 0; c < this->NumberOfTransforms; c++)
  {
    if(this->Transforms[c] != NULL)
    {
      activeTransforms++;
    }
  }

  if(activeTransforms == 0)
  {
    vtkErrorMacro(<<"No transform defined!");
    return 1;
  }

  linearPtMtx   = new double*[this->NumberOfTransforms];
  linearNormMtx = new double*[this->NumberOfTransforms];
  allLinear = 1;
  for(c = 0; c < this->NumberOfTransforms; c++)
  {
    if(this->Transforms[c] == NULL)
    {
      linearPtMtx[c] = NULL;
      linearNormMtx[c] = NULL;
      continue;
    }

    this->Transforms[c]->Update();

    if(! this->Transforms[c]->IsA("vtkLinearTransform"))
    {
      linearPtMtx[c] = NULL;
      linearNormMtx[c] = NULL;
      allLinear = 0;
      continue;
    }
    linearTransform = vtkLinearTransform::SafeDownCast(this->Transforms[c]);
    linearPtMtx[c] = (double *)linearTransform->GetMatrix()->Element;
    linearNormMtx[c] = new double[16];
    vtkMatrix4x4::DeepCopy(linearNormMtx[c], linearTransform->GetMatrix());
    vtkMatrix4x4::Invert(linearNormMtx[c], linearNormMtx[c]);
    vtkMatrix4x4::Transpose(linearNormMtx[c], linearNormMtx[c]);
  }

  pdArray = NULL;
  pdComponents = 0;
  if(this->WeightArray != NULL && this->WeightArray[0] != '\0')
  {
    fd = pd;
    if(fd != NULL) {
    pdArray = fd->GetArray(this->WeightArray);
    }
    if(pdArray == NULL)
    {
      fd = input->GetFieldData();
      if(fd != NULL) {
      pdArray = fd->GetArray(this->WeightArray);
      }
    }
    if(pdArray == NULL)
    {
      vtkErrorMacro(<<"WeightArray " << this->WeightArray <<
      " " << "doesn't exist");
      delete [] linearNormMtx;
      delete [] linearPtMtx;
      return 1;
    }

    pdComponents = pdArray->GetNumberOfComponents();
    if(pdComponents > this->NumberOfTransforms)
    {
      pdComponents =  this->NumberOfTransforms;
    }
  }

  tiArray = NULL;
  if(this->TransformIndexArray!=NULL && this->TransformIndexArray[0]!='\0')
  {
    fd = pd;
    if(fd != NULL) {
    tiArray = reinterpret_cast<vtkUnsignedShortArray *>
      (fd->GetArray(this->TransformIndexArray));
    }
    if(tiArray == NULL)
    {
      fd = input->GetFieldData();
      if(fd != NULL) {
      tiArray = reinterpret_cast<vtkUnsignedShortArray *>
        (fd->GetArray(this->TransformIndexArray));
      }
    }
    if(tiArray == NULL)
    {
      vtkErrorMacro(<<"TransformIndexArray " << this->TransformIndexArray <<
      " " << "doesn't exist");
      delete [] linearNormMtx;
      delete [] linearPtMtx;
      return 1;
    }

    if (pdComponents != tiArray->GetNumberOfComponents())
    {
      vtkWarningMacro(<<"TransformIndexArray " << this->TransformIndexArray <<
      " " << "does not have the same number of components as WeightArray " <<
                    this->WeightArray);
      tiArray = NULL;
    }
    if (tiArray->GetDataType() != VTK_UNSIGNED_SHORT)
    {
      vtkWarningMacro(<<"TransformIndexArray " << this->TransformIndexArray <<
                      " " << " is not of type unsigned short, ignoring.");
      tiArray = NULL;
    }
  }

  cdArray = NULL;
  cdComponents = 0;
  if(this->CellDataWeightArray != NULL &&
     this->CellDataWeightArray[0] != '\0')
  {
    fd = cd;
    if(fd != NULL)
    {
      cdArray = fd->GetArray(this->CellDataWeightArray);
    }
    if(cdArray == NULL)
    {
      fd = input->GetFieldData();
      if(fd != NULL) {
      cdArray = fd->GetArray(this->CellDataWeightArray);
      }
    }
    if(cdArray == NULL)
    {
      vtkErrorMacro(<<"CellDataWeightArray " << this->CellDataWeightArray <<
      " " << "doesn't exist");
      delete [] linearNormMtx;
      delete [] linearPtMtx;
      return 1;
    }
    cdComponents = cdArray->GetNumberOfComponents();
    if(cdComponents > this->NumberOfTransforms)
    {
      cdComponents = this->NumberOfTransforms;
    }
  }

  cdtiArray = NULL;
  if(this->CellDataTransformIndexArray != NULL &&
     this->CellDataTransformIndexArray[0] != '\0')
  {
    fd = pd;
    if(fd != NULL)
    {
      cdtiArray = reinterpret_cast<vtkUnsignedShortArray *>
        (fd->GetArray(this->CellDataTransformIndexArray));
    }
    if(cdtiArray == NULL)
    {
      fd = input->GetFieldData();
      if(fd != NULL)
      {
        cdtiArray = reinterpret_cast<vtkUnsignedShortArray *>
          (fd->GetArray(this->CellDataTransformIndexArray));
      }
    }
    if(cdtiArray == NULL)
    {
      vtkErrorMacro(<<"CellDataTransformIndexArray " <<
                    this->CellDataTransformIndexArray <<
                    " " << "doesn't exist");
      delete [] linearNormMtx;
      delete [] linearPtMtx;
      return 1;
    }

    if (cdComponents != cdtiArray->GetNumberOfComponents())
    {
      vtkWarningMacro(<<"CellDataTransformIndexArray " <<
                    this->CellDataTransformIndexArray <<
                    " " <<
                    "does not have the same number of components as " <<
                    "CellDataWeightArray " << this->WeightArray);
      cdtiArray = NULL;
    }
    if (cdtiArray && (cdtiArray->GetDataType() != VTK_UNSIGNED_SHORT))
    {
      vtkWarningMacro(<<"CellDataTransformIndexArray " <<
                      this->CellDataTransformIndexArray <<
                      " " << " is not of type unsigned short, ignoring.");
      cdtiArray = NULL;
    }
  }

  inPts = input->GetPoints();
  inVectors = pd->GetVectors();
  inNormals = pd->GetNormals();
  inCellVectors = cd->GetVectors();
  inCellNormals = cd->GetNormals();

  if ( !inPts )
  {
    vtkErrorMacro(<<"No input data");
    delete [] linearNormMtx;
    delete [] linearPtMtx;
    return 1;
  }

  numPts = inPts->GetNumberOfPoints();
  numCells = input->GetNumberOfCells();

  newPts = vtkPoints::New();
  newPts->Allocate(numPts);
  if ( inVectors )
  {
    newVectors = vtkFloatArray::New();
    newVectors->SetNumberOfComponents(3);
    newVectors->Allocate(3*numPts);
  }
  if ( inNormals )
  {
    newNormals = vtkFloatArray::New();
    newNormals->SetNumberOfComponents(3);
    newNormals->Allocate(3*numPts);
  }

  this->UpdateProgress (.2);
  // Loop over all points, updating position
  //

  // since we may be doing multiple transforms, we must duplicate
  // work done in vtkTransform

  // -------------------------- POINT DATA -------------------------------
  if(pdArray != NULL)
  {
    transformIndices = NULL;
    // do points
    for(p = 0; p < numPts; p++)
    {
      // -------- points init ---------------
      inPts->GetPoint(p, inPt);
      if(this->AddInputValues)
      {
        cumPt[0] = inPt[0];
        cumPt[1] = inPt[1];
        cumPt[2] = inPt[2];
      }
      else
      {
        cumPt[0] = 0.0;
        cumPt[1] = 0.0;
        cumPt[2] = 0.0;
      }
      // -------- vectors init ---------------
      if(inVectors)
      {
        inVectors->GetTuple(p, inVec);
        if(this->AddInputValues)
        {
          cumVec[0] = inVec[0];
          cumVec[1] = inVec[1];
          cumVec[2] = inVec[2];
        }
        else
        {
          cumVec[0] = 0.0;
          cumVec[1] = 0.0;
          cumVec[2] = 0.0;
        }
      }
      // -------- normals init ---------------
      if(inNormals)
      {
        inNormals->GetTuple(p, inNorm);
        if(this->AddInputValues)
        {
          cumNorm[0] = inNorm[0];
          cumNorm[1] = inNorm[1];
          cumNorm[2] = inNorm[2];
        }
        else
        {
          cumNorm[0] = 0.0;
          cumNorm[1] = 0.0;
          cumNorm[2] = 0.0;
        }
      }

      weights = reinterpret_cast<vtkFloatArray *>
        (pdArray)->GetPointer(p*pdComponents);

      if(tiArray != NULL)
      {
        transformIndices = reinterpret_cast<vtkUnsignedShortArray *>
          (tiArray)->GetPointer(p*pdComponents);
      }

      // for each transform...
      for(c = 0; c < pdComponents; c++)
      {
        if (transformIndices != NULL)
        {
          tidx = transformIndices[c];
        }
        else
        {
          tidx = c;
        }
        if(tidx >= this->NumberOfTransforms || tidx < 0)
        {
          vtkWarningMacro(<< "transform index " << tidx <<
                          " outside valid range, ignoring");
          continue;
        }
        thisWeight = weights[c];
        if(this->Transforms[tidx] == NULL || thisWeight == 0.0)
        {
          continue;
        }

        if(linearPtMtx[tidx] != NULL)
        {
          // -------------------- linear fast path ------------------------
          LinearTransformPoint((double (*)[4])linearPtMtx[tidx],
                               inPt, xformPt);

          if(inVectors)
          {
            LinearTransformVector((double (*)[4])linearPtMtx[tidx],
                                  inVec, xformVec);
          }

          if(inNormals)
          {
            LinearTransformVector((double (*)[4])linearNormMtx[tidx],
                                  inNorm, xformNorm);
            // normalize below
          }
        }
        else
        {
          // -------------------- general, slow path ------------------------
          this->Transforms[tidx]->InternalTransformDerivative(inPt, xformPt,
                                                           derivMatrix);
          if(inVectors)
          {
            vtkMath::Multiply3x3(derivMatrix, inVec, xformVec);
          }

          if(inNormals)
          {
            vtkMath::Transpose3x3(derivMatrix, derivMatrix);
            vtkMath::LinearSolve3x3(derivMatrix, inNorm, xformNorm);
            // normalize below
          }
        }

        // ------ accumulate the results into respective tuples -------
        cumPt[0] += xformPt[0]*thisWeight;
        cumPt[1] += xformPt[1]*thisWeight;
        cumPt[2] += xformPt[2]*thisWeight;

        if(inVectors)
        {
          cumVec[0] += xformVec[0]*thisWeight;
          cumVec[1] += xformVec[1]*thisWeight;
          cumVec[2] += xformVec[2]*thisWeight;
        }

        if(inNormals)
        {
          vtkMath::Normalize(xformNorm);
          cumNorm[0] += xformNorm[0]*thisWeight;
          cumNorm[1] += xformNorm[1]*thisWeight;
          cumNorm[2] += xformNorm[2]*thisWeight;
        }
      }

      // assign components
      newPts->InsertNextPoint(cumPt);

      if (inVectors)
      {
        newVectors->InsertNextTuple(cumVec);
      }

      if (inNormals)
      {
        // normalize normal again
        vtkMath::Normalize(cumNorm);
        newNormals->InsertNextTuple(cumNorm);
      }

    }
  }

  this->UpdateProgress (.6);

  // -------------------------- CELL DATA -------------------------------

  // can only work on cell data if the transforms are all linear
  if(cdArray != NULL && allLinear)
  {
    if( inCellVectors )
    {
      newCellVectors = vtkFloatArray::New();
      newCellVectors->SetNumberOfComponents(3);
      newCellVectors->Allocate(3*numCells);
    }
    if( inCellNormals )
    {
      newCellNormals = vtkFloatArray::New();
      newCellNormals->SetNumberOfComponents(3);
      newCellNormals->Allocate(3*numCells);
    }
    transformIndices = NULL;
    for(p = 0; p < numCells; p++)
    {
      // -------- normals init ---------------
      if(inCellNormals)
      {
        inCellNormals->GetTuple(p, inNorm);
        if(this->AddInputValues)
        {
          for(i = 0; i < 3; i++)
          {
            cumNorm[i] = inNorm[i];
          }
        }
        else
        {
          for(i = 0; i < 3; i++)
          {
            cumNorm[i] = 0.0;
          }
        }
      }
      // -------- vectors init ---------------
      if(inVectors)
      {
        inVectors->GetTuple(p, inVec);
        if(this->AddInputValues)
        {
          for(i = 0; i < 3; i++)
          {
            cumVec[i] = inVec[i];
          }
        }
        else
        {
          for(i = 0; i < 3; i++)
          {
            cumVec[i] = 0.0;
          }
        }
      }

      weights = reinterpret_cast<vtkFloatArray *>
        (cdArray)->GetPointer(p*cdComponents);
      if(cdtiArray != NULL)
      {
        transformIndices = reinterpret_cast<vtkUnsignedShortArray *>
          (cdtiArray)->GetPointer(p*cdComponents);
      }

      // for each transform...
      for(c = 0; c < cdComponents; c++)
      {
        if (transformIndices != NULL)
        {
          tidx = transformIndices[c];
        }
        else
        {
          tidx = c;
        }
        if(tidx >= this->NumberOfTransforms || tidx < 0)
        {
          vtkWarningMacro(<< "transform index " << tidx <<
                          " outside valid range, ignoring");
          continue;
        }
        thisWeight = weights[c];
        if(linearPtMtx[tidx] == NULL || thisWeight == 0.0)
        {
          continue;
        }

        if(inCellNormals)
        {
          LinearTransformVector((double (*)[4])linearNormMtx[tidx],
                                inNorm, xformNorm);

          vtkMath::Normalize(xformNorm);
          cumNorm[0] += xformNorm[0]*thisWeight;
          cumNorm[1] += xformNorm[1]*thisWeight;
          cumNorm[2] += xformNorm[2]*thisWeight;
        }

        if(inVectors)
        {
          LinearTransformVector((double (*)[4])linearPtMtx[tidx],
                                inVec, xformVec);
          cumVec[0] += xformVec[0]*thisWeight;
          cumVec[1] += xformVec[1]*thisWeight;
          cumVec[2] += xformVec[2]*thisWeight;
        }
      }

      if (inCellNormals)
      {
        // normalize normal again
        vtkMath::Normalize(cumNorm);
        newCellNormals->InsertNextTuple(cumNorm);
      }

      if (inCellVectors)
      {
        newCellVectors->InsertNextTuple(cumVec);
      }
    }
  }

  // ----- cleanup ------
  for(c = 0; c < this->NumberOfTransforms; c++)
  {
    delete [] linearNormMtx[c];
  }
  delete [] linearNormMtx;
  delete [] linearPtMtx;

  // ---------------------

  this->UpdateProgress (0.8);

  // Update ourselves and release memory
  //
  output->SetPoints(newPts);
  newPts->Delete();

  if (newNormals)
  {
    outPD->SetNormals(newNormals);
    outPD->CopyNormalsOff();
    newNormals->Delete();
  }

  if (newVectors)
  {
    outPD->SetVectors(newVectors);
    outPD->CopyVectorsOff();
    newVectors->Delete();
  }

  if (newCellNormals)
  {
    outCD->SetNormals(newCellNormals);
    outCD->CopyNormalsOff();
    newCellNormals->Delete();
  }

  if (newCellVectors)
  {
    outCD->SetVectors(newCellVectors);
    outCD->CopyVectorsOff();
    newCellVectors->Delete();
  }

  outPD->PassData(pd);
  outCD->PassData(cd);

  return 1;
}

//----------------------------------------------------------------------------
vtkMTimeType vtkWeightedTransformFilter::GetMTime()
{
  int i;
  vtkMTimeType mTime=this->MTime.GetMTime();
  vtkMTimeType transMTime;

  if ( this->Transforms )
  {
    for(i = 0; i < this->NumberOfTransforms; i++)
    {
      if(this->Transforms[i])
      {
        transMTime = this->Transforms[i]->GetMTime();
        mTime = ( transMTime > mTime ? transMTime : mTime );
      }
    }
  }

  return mTime;
}

//----------------------------------------------------------------------------
void vtkWeightedTransformFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  int i;
  this->Superclass::PrintSelf(os,indent);

  os << indent << "NumberOfTransforms: " << this->NumberOfTransforms << "\n";
  for(i = 0; i < this->NumberOfTransforms; i++)
  {
    os << indent << "Transform " << i << ": " << this->Transforms[i] << "\n";

  }
  os << indent << "AddInputValues: " << (this->AddInputValues ? "On" : "Off") << "\n";
  os << indent << "WeightArray: "
     << (this->WeightArray ? this->WeightArray : "(none)") << "\n";
  os << indent << "CellDataWeightArray: "
     << (this->CellDataWeightArray ? this->CellDataWeightArray : "(none)")
     << "\n";
  os << indent << "TransformIndexArray: "
     << (this->TransformIndexArray ? this->TransformIndexArray : "(none)")
     << "\n";
  os << indent << "CellDataTransformIndexArray: "
     << (this->CellDataTransformIndexArray ? this->CellDataTransformIndexArray : "(none)")
     << "\n";
}
