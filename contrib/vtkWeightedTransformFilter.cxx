/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWeightedTransformFilter.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Michael Halle, Brigham and Women's Hospital


Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include "vtkWeightedTransformFilter.h"
#include "vtkObjectFactory.h"
#include "vtkLinearTransform.h"
#include "vtkMath.h"

//----------------------------------------------------------------------------

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

vtkWeightedTransformFilter* vtkWeightedTransformFilter::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = 
    vtkObjectFactory::CreateInstance("vtkWeightedTransformFilter");
  if(ret)
  {
    return (vtkWeightedTransformFilter*)ret;
  }
  // If the factory was unable to create the object, then create it here.
  return new vtkWeightedTransformFilter;
}

//----------------------------------------------------------------------------

vtkWeightedTransformFilter::vtkWeightedTransformFilter()
{
  this->AddInputValues = 0;
  this->Transforms = NULL;
  this->NumberOfTransforms = 0;
  this->CellDataWeightArray = NULL;
  this->WeightArray = NULL;

  this->SetCellDataWeightArray("");
  this->SetWeightArray("");
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
  delete [] this->CellDataWeightArray;
  delete [] this->WeightArray;
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

void vtkWeightedTransformFilter::Execute()
{
  vtkPoints *inPts;
  vtkPoints *newPts;
  vtkVectors *inVectors, *inCellVectors;;
  vtkNormals *inNormals, *inCellNormals;
  vtkVectors *newVectors=NULL, *newCellVectors=NULL;
  vtkNormals *newNormals=NULL, *newCellNormals=NULL;
  vtkIdType numPts, numCells, p;
  int activeTransforms, allLinear;
  int i, c;
  int pdComponents, cdComponents;
  double **linearPtMtx;
  double **linearNormMtx;
  double inNorm[3], xformNorm[3], cumNorm[3];
  double inPt[3], xformPt[3], cumPt[3];
  double inVec[3], xformVec[3], cumVec[3];
  double derivMatrix[3][3];
  float *weights = NULL;
  float thisWeight;
  vtkDataArray *pdArray, *cdArray;
  vtkFieldData *fd;
  vtkPointSet *input = this->GetInput();
  vtkPointSet *output = this->GetOutput();
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
    return;
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
    return;
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
    fd = pd->GetFieldData();
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
      return;
    }

    pdComponents = pdArray->GetNumberOfComponents();
    if(pdComponents > this->NumberOfTransforms) 
    {
      pdComponents =  this->NumberOfTransforms;
    }
  }

  cdArray = NULL;
  cdComponents = 0;
  if(this->CellDataWeightArray != NULL &&
     this->CellDataWeightArray[0] != '\0') 
  {
    fd = cd->GetFieldData();
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
      return;
    }
    cdComponents = cdArray->GetNumberOfComponents();
    if(cdComponents > this->NumberOfTransforms) 
    {
      cdComponents = this->NumberOfTransforms;
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
    return;
  }

  numPts = inPts->GetNumberOfPoints();
  numCells = input->GetNumberOfCells();

  newPts = vtkPoints::New();
  newPts->Allocate(numPts);
  if ( inVectors ) 
  {
    newVectors = vtkVectors::New();
    newVectors->Allocate(numPts);
  }
  if ( inNormals ) 
  {
    newNormals = vtkNormals::New();
    newNormals->Allocate(numPts);
  }

  this->UpdateProgress (.2);
  // Loop over all points, updating position
  //

  // since we may be doing multiple transforms, we must duplicate
  // work done in vtkTransform

  weights = new float[pdArray->GetNumberOfComponents()];

  // -------------------------- POINT DATA -------------------------------
  if(pdArray != NULL) 
  {
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
	inVectors->GetVector(p, inVec);
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
	inNormals->GetNormal(p, inNorm);
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

      pdArray->GetTuple(p, weights);

      // for each transform...
      for(c = 0; c < pdComponents; c++)
      {
	thisWeight = weights[c];
	if(this->Transforms[c] == NULL || thisWeight == 0.0)
	{
	  continue;
	}

	if(linearPtMtx[c] != NULL)
	{ 
	  // -------------------- linear fast path ------------------------
	  LinearTransformPoint((double (*)[4])linearPtMtx[c], 
			       inPt, xformPt);

	  if(inVectors)
	  {
	    LinearTransformVector((double (*)[4])linearPtMtx[c], 
				  inVec, xformVec);
	  }

	  if(inNormals)
	  {
	    LinearTransformVector((double (*)[4])linearNormMtx[c], 
				  inNorm, xformNorm);
	    // normalize below
	  }
	}
	else
	{  
	  // -------------------- general, slow path ------------------------
	  this->Transforms[c]->InternalTransformDerivative(inPt, xformPt,
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
	newVectors->InsertNextVector(cumVec);
      }

      if (inNormals)
      {
	// normalize normal again
	vtkMath::Normalize(cumNorm);
	newNormals->InsertNextNormal(cumNorm);
      }
	
    }
    delete [] weights;
    weights = NULL;
  }

  this->UpdateProgress (.6);

  // -------------------------- CELL DATA -------------------------------

  // can only work on cell data if the transforms are all linear
  if(cdArray != NULL && allLinear)
  {
    if( inCellVectors )
    {
      newCellVectors = vtkVectors::New();
      newCellVectors->Allocate(numCells);
    }
    if( inCellNormals )
    {
      newCellNormals = vtkNormals::New();
      newCellNormals->Allocate(numCells);
    }

    weights = new float[cdArray->GetNumberOfComponents()];

    for(p = 0; p < numCells; p++)
    {
      // -------- normals init ---------------
      if(inCellNormals)
      {
	inCellNormals->GetNormal(p, inNorm);
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
	inVectors->GetVector(p, inVec);
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

      cdArray->GetTuple(p, weights);

      // for each transform...
      for(c = 0; c < cdComponents; c++)
      {
	thisWeight = weights[c];
	if(linearPtMtx[c] == NULL || thisWeight == 0.0)
	{
	  continue;
	}

	if(inCellNormals) 
	{
	  LinearTransformVector((double (*)[4])linearNormMtx[c], 
				     inNorm, xformNorm);

	  vtkMath::Normalize(xformNorm);
	  cumNorm[0] += xformNorm[0]*thisWeight;
	  cumNorm[1] += xformNorm[1]*thisWeight;
	  cumNorm[2] += xformNorm[2]*thisWeight;
	}

	if(inVectors)
	{
	  LinearTransformVector((double (*)[4])linearPtMtx[c], 
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
	newCellNormals->InsertNextNormal(cumNorm);
      }
	
      if (inCellVectors)
      {
	newCellVectors->InsertNextVector(cumVec);
      }
    }
    delete [] weights;
  }

  // ----- cleanup ------
  for(c = 0; c < this->NumberOfTransforms; c++)
  {
    if(linearNormMtx[c])
    {
      delete [] linearNormMtx[c];
    }
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
}

//----------------------------------------------------------------------------

unsigned long vtkWeightedTransformFilter::GetMTime()
{
  int i;
  unsigned long mTime=this->MTime.GetMTime();
  unsigned long transMTime;

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
  vtkPointSetToPointSetFilter::PrintSelf(os,indent);

  os << indent << "NumberOfTransforms: " << this->NumberOfTransforms << "\n";
  for(i = 0; i < this->NumberOfTransforms; i++)
  {
    os << indent << "Transform " << i << ": " << this->Transforms[i] << "\n";

  }
  os << indent << "AddInputValues: " << (this->AddInputValues ? "On" : "Off") << "\n";
  os << indent << "WeightArray: " << this->WeightArray << "\n";
  os << indent << "CellDataWeightArray: " << this->CellDataWeightArray << "\n";
}

