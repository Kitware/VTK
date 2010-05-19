/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAbstractTransform.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkAbstractTransform.h"

#include "vtkCriticalSection.h"
#include "vtkDataArray.h"
#include "vtkDebugLeaks.h"
#include "vtkHomogeneousTransform.h"
#include "vtkMath.h"
#include "vtkMatrix4x4.h"
#include "vtkPoints.h"


//----------------------------------------------------------------------------
vtkAbstractTransform::vtkAbstractTransform()
{
  this->MyInverse = NULL;
  this->DependsOnInverse = 0;
  this->InUnRegister = 0;
  this->UpdateMutex = vtkSimpleCriticalSection::New();
  this->InverseMutex = vtkSimpleCriticalSection::New();
}

//----------------------------------------------------------------------------
vtkAbstractTransform::~vtkAbstractTransform()
{
  if (this->MyInverse) 
    { 
    this->MyInverse->Delete(); 
    } 
  if (this->UpdateMutex)
    {
    this->UpdateMutex->Delete();
    }
  if (this->InverseMutex)
    {
    this->InverseMutex->Delete();
    }
}

//----------------------------------------------------------------------------
void vtkAbstractTransform::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Inverse: (" << this->MyInverse << ")\n";
}

//----------------------------------------------------------------------------
void vtkAbstractTransform::TransformNormalAtPoint(const double point[3],
                                                  const double in[3],
                                                  double out[3])
{
  this->Update();

  double matrix[3][3];
  double coord[3];

  this->InternalTransformDerivative(point,coord,matrix);
  vtkMath::Transpose3x3(matrix,matrix);
  vtkMath::LinearSolve3x3(matrix,in,out);
  vtkMath::Normalize(out);
}

void vtkAbstractTransform::TransformNormalAtPoint(const float point[3],
                                                  const float in[3],
                                                  float out[3])
{
  double coord[3];
  double normal[3];

  coord[0] = point[0];
  coord[1] = point[1];
  coord[2] = point[2];

  normal[0] = in[0];
  normal[1] = in[1];
  normal[2] = in[2];

  this->TransformNormalAtPoint(coord,normal,normal);

  out[0] = static_cast<float>(normal[0]);
  out[1] = static_cast<float>(normal[1]);
  out[2] = static_cast<float>(normal[2]);
}

//----------------------------------------------------------------------------
void vtkAbstractTransform::TransformVectorAtPoint(const double point[3],
                                                  const double in[3],
                                                  double out[3])
{
  this->Update();

  double matrix[3][3];
  double coord[3];

  this->InternalTransformDerivative(point,coord,matrix);
  vtkMath::Multiply3x3(matrix,in,out);
}

void vtkAbstractTransform::TransformVectorAtPoint(const float point[3],
                                                  const float in[3],
                                                  float out[3])
{
  double coord[3];
  double vector[3];

  coord[0] = point[0];
  coord[1] = point[1];
  coord[2] = point[2];

  vector[0] = in[0];
  vector[1] = in[1];
  vector[2] = in[2];

  this->TransformVectorAtPoint(coord,vector,vector);

  out[0] = static_cast<float>(vector[0]);
  out[1] = static_cast<float>(vector[1]);
  out[2] = static_cast<float>(vector[2]);
}

//----------------------------------------------------------------------------
// Transform a series of points.
void vtkAbstractTransform::TransformPoints(vtkPoints *in, vtkPoints *out)
{
  this->Update();

  double point[3];
  vtkIdType i;
  vtkIdType n = in->GetNumberOfPoints();

  for (i = 0; i < n; i++)
    {
    in->GetPoint(i,point);
    this->InternalTransformPoint(point,point);
    out->InsertNextPoint(point);
    }
}

//----------------------------------------------------------------------------
// Transform the normals and vectors using the derivative of the 
// transformation.  Either inNms or inVrs can be set to NULL.
// Normals are multiplied by the inverse transpose of the transform
// derivative, while vectors are simply multiplied by the derivative.
// Note that the derivative of the inverse transform is simply the
// inverse of the derivative of the forward transform. 

void vtkAbstractTransform::TransformPointsNormalsVectors(vtkPoints *inPts,
                                                         vtkPoints *outPts,
                                                         vtkDataArray *inNms, 
                                                         vtkDataArray *outNms,
                                                         vtkDataArray *inVrs,
                                                         vtkDataArray *outVrs)
{
  this->Update();

  double matrix[3][3];
  double coord[3];

  vtkIdType i;
  vtkIdType n = inPts->GetNumberOfPoints();

  for (i = 0; i < n; i++)
    {
    inPts->GetPoint(i,coord);
    this->InternalTransformDerivative(coord,coord,matrix);
    outPts->InsertNextPoint(coord);
    
    if (inVrs)
      {
      inVrs->GetTuple(i,coord);
      vtkMath::Multiply3x3(matrix,coord,coord);
      outVrs->InsertNextTuple(coord);
      }
    
    if (inNms)
      {
      inNms->GetTuple(i,coord);
      vtkMath::Transpose3x3(matrix,matrix);
      vtkMath::LinearSolve3x3(matrix,coord,coord);
      vtkMath::Normalize(coord);
      outNms->InsertNextTuple(coord);
      }
    }
}

//----------------------------------------------------------------------------
vtkAbstractTransform *vtkAbstractTransform::GetInverse()
{
  this->InverseMutex->Lock();
  if (this->MyInverse == NULL)
    {
    // we create a circular reference here, it is dealt with in UnRegister
    this->MyInverse = this->MakeTransform();
    this->MyInverse->SetInverse(this);
    }
  this->InverseMutex->Unlock();
  return this->MyInverse;
}

//----------------------------------------------------------------------------
void vtkAbstractTransform::SetInverse(vtkAbstractTransform *transform)
{
  if (this->MyInverse == transform)
    {
    return;
    }

  // check type first
  if (!transform->IsA(this->GetClassName()))
    {
    vtkErrorMacro("SetInverse: requires a " << this->GetClassName() << ", a "
                  << transform->GetClassName() << " is not compatible.");
    return;
    }

  if (transform->CircuitCheck(this))
    {
    vtkErrorMacro("SetInverse: this would create a circular reference.");
    return;
    }

  if (this->MyInverse)
    {
    this->MyInverse->Delete();
    }

  transform->Register(this);
  this->MyInverse = transform;

  // we are now a special 'inverse transform'
  this->DependsOnInverse = (transform != 0);

  this->Modified();
}

//----------------------------------------------------------------------------
void vtkAbstractTransform::DeepCopy(vtkAbstractTransform *transform)
{
  // check whether we're trying to copy a transform to itself
  if (transform == this)
    {
    return;
    }

  // check to see if the transform is the same type as this one
  if (!transform->IsA(this->GetClassName()))
    {
    vtkErrorMacro("DeepCopy: can't copy a " << transform->GetClassName()
                  << " into a " << this->GetClassName() << ".");
    return;
    }

  if (transform->CircuitCheck(this))
    {
    vtkErrorMacro("DeepCopy: this would create a circular reference.");
    return;
    }

  // call InternalDeepCopy for subtype
  this->InternalDeepCopy(transform);

  this->Modified();
}    

//----------------------------------------------------------------------------
void vtkAbstractTransform::Update()
{
  // locking is require to ensure that the class is thread-safe
  this->UpdateMutex->Lock();

  // check to see if we are a special 'inverse' transform
  if (this->DependsOnInverse && 
      this->MyInverse->GetMTime() >= this->UpdateTime.GetMTime())
    {
    vtkDebugMacro("Updating transformation from its inverse");
    this->InternalDeepCopy(this->MyInverse);
    this->Inverse();
    vtkDebugMacro("Calling InternalUpdate on the transformation");
    this->InternalUpdate();
    }
  // otherwise just check our MTime against our last update
  else if (this->GetMTime() >= this->UpdateTime.GetMTime())
    {
    // do internal update for subclass
    vtkDebugMacro("Calling InternalUpdate on the transformation");
    this->InternalUpdate();
    }

  this->UpdateTime.Modified();
  this->UpdateMutex->Unlock();
}

//----------------------------------------------------------------------------
int vtkAbstractTransform::CircuitCheck(vtkAbstractTransform *transform)
{
  return (transform == this || (this->DependsOnInverse && 
                                this->MyInverse->CircuitCheck(transform)));
}

//----------------------------------------------------------------------------
// Need to check inverse's MTime if we are an inverse transform
unsigned long vtkAbstractTransform::GetMTime()
{
  unsigned long mtime = this->vtkObject::GetMTime();
  if (this->DependsOnInverse)
    {
    unsigned long inverseMTime = this->MyInverse->GetMTime();
    if (inverseMTime > mtime)
      {
      return inverseMTime;
      }
    }

  return mtime;
}

//----------------------------------------------------------------------------
// We need to handle the circular reference between a transform and its
// inverse.
void vtkAbstractTransform::UnRegister(vtkObjectBase *o)
{
  if (this->InUnRegister)
    { // we don't want to go into infinite recursion...
    vtkDebugMacro(<<"UnRegister: circular reference eliminated"); 
    this->ReferenceCount--;
    return;
    }

  // check to see if the only reason our reference count is not 1
  // is the circular reference from MyInverse
  if (this->MyInverse && this->ReferenceCount == 2 &&
      this->MyInverse->ReferenceCount == 1)
    { // break the cycle
    vtkDebugMacro(<<"UnRegister: eliminating circular reference"); 
    this->InUnRegister = 1;
    this->MyInverse->UnRegister(this);
    this->MyInverse = NULL;
    this->InUnRegister = 0;
    }

  this->vtkObject::UnRegister(o);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// All of the following methods are for vtkTransformConcatenation
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// A very, very minimal transformation
class vtkSimpleTransform : public vtkHomogeneousTransform
{
public:
  vtkTypeMacro(vtkSimpleTransform,vtkHomogeneousTransform);
  static vtkSimpleTransform *New() {
#ifdef VTK_DEBUG_LEAKS
    vtkDebugLeaks::ConstructClass("vtkSimpleTransform");
#endif    
    return new vtkSimpleTransform; };
  vtkAbstractTransform *MakeTransform() { return vtkSimpleTransform::New(); };
  void Inverse() { this->Matrix->Invert(); this->Modified(); };
protected:
  vtkSimpleTransform() {};
  vtkSimpleTransform(const vtkSimpleTransform&);
  void operator=(const vtkSimpleTransform&);
};


//----------------------------------------------------------------------------
vtkTransformConcatenation::vtkTransformConcatenation()
{
  this->PreMatrix = NULL;
  this->PostMatrix = NULL;
  this->PreMatrixTransform = NULL;
  this->PostMatrixTransform = NULL;

  this->PreMultiplyFlag = 1;
  this->InverseFlag = 0;

  this->NumberOfTransforms = 0;
  this->NumberOfPreTransforms = 0;
  this->MaxNumberOfTransforms = 0;

  // The transform list is the list of the transforms to be concatenated.
  this->TransformList = NULL;
}

//----------------------------------------------------------------------------
vtkTransformConcatenation::~vtkTransformConcatenation()
{
  if (this->NumberOfTransforms > 0)
    {
    for (int i = 0; i < this->NumberOfTransforms; i++)
      {
      vtkTransformPair *tuple = &this->TransformList[i];
      if (tuple->ForwardTransform)
        {
        tuple->ForwardTransform->Delete();
        }
      if (tuple->InverseTransform)
        {
        tuple->InverseTransform->Delete();
        }
      }
    }
  if (this->TransformList)
    {
    delete [] this->TransformList;
    }
}

//----------------------------------------------------------------------------
void vtkTransformConcatenation::Concatenate(vtkAbstractTransform *trans)
{
  // in case either PreMatrix or PostMatrix is going to be pushed
  // into the concatenation from their position at the end
  if (this->PreMultiplyFlag && this->PreMatrix)
    {
    this->PreMatrix = NULL;
    this->PreMatrixTransform = NULL;
    }
  else if (!this->PreMultiplyFlag && this->PostMatrix)
    {
    this->PostMatrix = NULL;
    this->PostMatrixTransform = NULL;
    }

  vtkTransformPair *transList = this->TransformList;
  int n = this->NumberOfTransforms;
  this->NumberOfTransforms++;
  
  // check to see if we need to allocate more space
  if (this->NumberOfTransforms > this->MaxNumberOfTransforms)
    {
    int nMax = this->MaxNumberOfTransforms + 5;
    transList = new vtkTransformPair[nMax];
    for (int i = 0; i < n; i++)
      {
      transList[i].ForwardTransform = this->TransformList[i].ForwardTransform;
      transList[i].InverseTransform = this->TransformList[i].InverseTransform;
      }
    if (this->TransformList)
      {
      delete [] this->TransformList;
      }
    this->TransformList = transList;
    this->MaxNumberOfTransforms = nMax;
    }

  // add the transform either the beginning or end of the list,
  // according to flags
  if (this->PreMultiplyFlag ^ this->InverseFlag)
    {
    for (int i = n; i > 0; i--)
      {
      transList[i].ForwardTransform = transList[i-1].ForwardTransform;
      transList[i].InverseTransform = transList[i-1].InverseTransform;
      }
    n = 0;
    this->NumberOfPreTransforms++;
    }

  trans->Register(NULL);
  
  if (this->InverseFlag)
    {
    transList[n].ForwardTransform = NULL;
    transList[n].InverseTransform = trans;
    }
  else
    {
    transList[n].ForwardTransform = trans;
    transList[n].InverseTransform = NULL;
    }
}

//----------------------------------------------------------------------------
void vtkTransformConcatenation::Concatenate(const double elements[16])
{
  // concatenate the matrix with either the Pre- or PostMatrix
  if (this->PreMultiplyFlag) 
    {
    if (this->PreMatrix == NULL)
      {
      // add the matrix to the concatenation
      vtkSimpleTransform *mtrans = vtkSimpleTransform::New();
      this->Concatenate(mtrans);
      mtrans->Delete();
      this->PreMatrixTransform = mtrans;
      this->PreMatrix = mtrans->GetMatrix();
      }
    vtkMatrix4x4::Multiply4x4(*this->PreMatrix->Element, elements, 
                              *this->PreMatrix->Element);
    this->PreMatrix->Modified();
    this->PreMatrixTransform->Modified();
    }
  else 
    {
    if (this->PostMatrix == NULL)
      {
      // add the matrix to the concatenation
      vtkSimpleTransform *mtrans = vtkSimpleTransform::New();
      this->Concatenate(mtrans);
      mtrans->Delete();
      this->PostMatrixTransform = mtrans;
      this->PostMatrix = mtrans->GetMatrix();
      }
    vtkMatrix4x4::Multiply4x4(elements, *this->PostMatrix->Element, 
                              *this->PostMatrix->Element);
    this->PostMatrix->Modified();
    this->PostMatrixTransform->Modified();
    }
}  

//----------------------------------------------------------------------------
void vtkTransformConcatenation::Translate(double x, double y, double z)
{
  if (x == 0.0 && y == 0.0 && z == 0.0) 
    {
    return;
    }

  double matrix[4][4];
  vtkMatrix4x4::Identity(*matrix);

  matrix[0][3] = x;
  matrix[1][3] = y;
  matrix[2][3] = z;

  this->Concatenate(*matrix);
}

//----------------------------------------------------------------------------
void vtkTransformConcatenation::Rotate(double angle, 
                                       double x, double y, double z)
{
  if (angle == 0.0 || (x == 0.0 && y == 0.0 && z == 0.0)) 
    {
    return;
    }

  // convert to radians
  angle = vtkMath::RadiansFromDegrees( angle );

  // make a normalized quaternion
  double w = cos(0.5*angle);
  double f = sin(0.5*angle)/sqrt(x*x+y*y+z*z);
  x *= f;
  y *= f;
  z *= f;

  // convert the quaternion to a matrix
  double matrix[4][4];
  vtkMatrix4x4::Identity(*matrix);

  double ww = w*w;
  double wx = w*x;
  double wy = w*y;
  double wz = w*z;

  double xx = x*x;
  double yy = y*y;
  double zz = z*z;

  double xy = x*y;
  double xz = x*z;
  double yz = y*z;

  double s = ww - xx - yy - zz;

  matrix[0][0] = xx*2 + s;
  matrix[1][0] = (xy + wz)*2;
  matrix[2][0] = (xz - wy)*2;

  matrix[0][1] = (xy - wz)*2;
  matrix[1][1] = yy*2 + s;
  matrix[2][1] = (yz + wx)*2;

  matrix[0][2] = (xz + wy)*2;
  matrix[1][2] = (yz - wx)*2;
  matrix[2][2] = zz*2 + s;

  this->Concatenate(*matrix);
}

//----------------------------------------------------------------------------
void vtkTransformConcatenation::Scale(double x, double y, double z)
{
  if (x == 1.0 && y == 1.0 && z == 1.0) 
    {
    return;
    }

  double matrix[4][4];
  vtkMatrix4x4::Identity(*matrix);

  matrix[0][0] = x;
  matrix[1][1] = y;
  matrix[2][2] = z;

  this->Concatenate(*matrix);
}

//----------------------------------------------------------------------------
void vtkTransformConcatenation::Inverse()
{
  // invert the matrices
  if (this->PreMatrix)
    {
    this->PreMatrix->Invert();
    this->PreMatrixTransform->Modified();
    int i = (this->InverseFlag ? this->NumberOfTransforms-1 : 0);
    this->TransformList[i].SwapForwardInverse();
    }

  if (this->PostMatrix)
    {
    this->PostMatrix->Invert();
    this->PostMatrixTransform->Modified();
    int i = (this->InverseFlag ? 0 : this->NumberOfTransforms-1);
    this->TransformList[i].SwapForwardInverse();
    }

  // swap the pre- and post-matrices
  vtkMatrix4x4 *tmp = this->PreMatrix;
  vtkAbstractTransform *tmp2 = this->PreMatrixTransform;
  this->PreMatrix = this->PostMatrix;
  this->PreMatrixTransform = this->PostMatrixTransform;
  this->PostMatrix = tmp;
  this->PostMatrixTransform = tmp2;

  // what used to be pre-transforms are now post-transforms
  this->NumberOfPreTransforms = 
    this->NumberOfTransforms - this->NumberOfPreTransforms;

  this->InverseFlag = !this->InverseFlag;
}

//----------------------------------------------------------------------------
void vtkTransformConcatenation::Identity()
{
  // forget the Pre- and PostMatrix
  this->PreMatrix = NULL;
  this->PostMatrix = NULL;
  this->PreMatrixTransform = NULL;
  this->PostMatrixTransform = NULL;

  // delete all the transforms
  if (this->NumberOfTransforms > 0)
    {
    for (int i = 0; i < this->NumberOfTransforms; i++)
      {
      vtkTransformPair *tuple = &this->TransformList[i];
      if (tuple->ForwardTransform)
        {
        tuple->ForwardTransform->Delete();
        }
      if (tuple->InverseTransform)
        {
        tuple->InverseTransform->Delete();
        }
      }
    }
  this->NumberOfTransforms = 0;
  this->NumberOfPreTransforms = 0;
}

//----------------------------------------------------------------------------
vtkAbstractTransform *vtkTransformConcatenation::GetTransform(int i)
{
  // we walk through the list in reverse order if InverseFlag is set
  if (this->InverseFlag)
    {
    int j = this->NumberOfTransforms-i-1;    
    vtkTransformPair *tuple = &this->TransformList[j];
    // if inverse is NULL, then get it from the forward transform
    if (tuple->InverseTransform == NULL)
      {
      tuple->InverseTransform = tuple->ForwardTransform->GetInverse();
      tuple->InverseTransform->Register(NULL);
      }
    return tuple->InverseTransform;
    }
  else
    {
    vtkTransformPair *tuple = &this->TransformList[i];
    // if transform is NULL, then get it from its inverse
    if (tuple->ForwardTransform == NULL)
      {
      tuple->ForwardTransform = tuple->InverseTransform->GetInverse();
      tuple->ForwardTransform->Register(NULL);
      }
    return tuple->ForwardTransform;
    }
}

//----------------------------------------------------------------------------
unsigned long vtkTransformConcatenation::GetMaxMTime()
{
  unsigned long result = 0;
  unsigned long mtime;

  for (int i = 0; i < this->NumberOfTransforms; i++)
    {
    vtkTransformPair *tuple = &this->TransformList[i];
    if (tuple->ForwardTransform)
      {
      mtime = tuple->ForwardTransform->GetMTime();
      }
    else
      {
      mtime = tuple->InverseTransform->GetMTime();
      }

    if (mtime > result)
      {
      result = mtime;
      }
    }

  return result;
}

//----------------------------------------------------------------------------
void vtkTransformConcatenation::DeepCopy(vtkTransformConcatenation *concat)
{
  // allocate a larger list if necessary
  if (this->MaxNumberOfTransforms < concat->NumberOfTransforms)
    {
    int newMax = concat->NumberOfTransforms;
    vtkTransformPair *newList = new vtkTransformPair[newMax];
    // copy items onto new list
    int i = 0;
    for (; i < this->NumberOfTransforms; i++)
      {
      newList[i].ForwardTransform = this->TransformList[i].ForwardTransform;
      newList[i].InverseTransform = this->TransformList[i].InverseTransform;
      }
    for (; i < concat->NumberOfTransforms; i++)
      {
      newList[i].ForwardTransform = NULL;
      newList[i].InverseTransform = NULL;
      }
    if (this->TransformList)
      {
      delete [] this->TransformList;
      }
    this->MaxNumberOfTransforms = newMax;
    this->TransformList = newList;
    }

  // save the PreMatrix and PostMatrix in case they can be re-used
  vtkSimpleTransform *oldPreMatrixTransform = NULL;
  vtkSimpleTransform *oldPostMatrixTransform = NULL;

  if (this->PreMatrix)
    {
    vtkTransformPair *tuple;
    if (this->InverseFlag)
      {
      tuple = &this->TransformList[this->NumberOfTransforms-1];
      tuple->SwapForwardInverse();
      }
    else
      {
      tuple = &this->TransformList[0];
      }
    tuple->ForwardTransform = NULL;
    if (tuple->InverseTransform)
      {
      tuple->InverseTransform->Delete();
      tuple->InverseTransform = NULL;
      }
    oldPreMatrixTransform =
      static_cast<vtkSimpleTransform *>(this->PreMatrixTransform);
    this->PreMatrixTransform = NULL;
    this->PreMatrix = NULL;
    }    

  if (this->PostMatrix)
    {
    vtkTransformPair *tuple;
    if (this->InverseFlag)
      {
      tuple = &this->TransformList[0];
      tuple->SwapForwardInverse();
      }
    else
      {
      tuple = &this->TransformList[this->NumberOfTransforms-1];
      }
    tuple->ForwardTransform = NULL;
    if (tuple->InverseTransform)
      {
      tuple->InverseTransform->Delete();
      tuple->InverseTransform = NULL;
      }
    oldPostMatrixTransform =
      static_cast<vtkSimpleTransform *>(this->PostMatrixTransform);
    this->PostMatrixTransform = NULL;
    this->PostMatrix = NULL;
    }    

  // the PreMatrix and PostMatrix transforms must be DeepCopied,
  // not copied by reference, so adjust the copy loop accordingly
  int i = 0;
  int n = concat->NumberOfTransforms;
  if (concat->PreMatrix)
    {
    if (concat->InverseFlag) { n--; } else { i++; }
    }
  if (concat->PostMatrix)
    {
    if (concat->InverseFlag) { i++; } else { n--; }
    }

  // copy the transforms by reference
  for (; i < n; i++)
    {
    vtkTransformPair *pair = &this->TransformList[i];
    vtkTransformPair *pair2 = &concat->TransformList[i];

    if (pair->ForwardTransform != pair2->ForwardTransform)
      {
      if (pair->ForwardTransform && i < this->NumberOfTransforms)
        {
        pair->ForwardTransform->Delete();
        }
      pair->ForwardTransform = pair2->ForwardTransform;
      if (pair->ForwardTransform)
        {
        pair->ForwardTransform->Register(NULL);
        }
      }
    if (pair->InverseTransform != pair2->InverseTransform)
      {
      if (pair->InverseTransform && i < this->NumberOfTransforms)
        {
        pair->InverseTransform->Delete();
        }
      pair->InverseTransform = pair2->InverseTransform;
      if (pair->InverseTransform)
        {
        pair->InverseTransform->Register(NULL);
        }
      }
    }

  // delete surplus items from the list
  for (i = concat->NumberOfTransforms; i < this->NumberOfTransforms; i++)
    {
    if (this->TransformList[i].ForwardTransform)
      {
      this->TransformList[i].ForwardTransform->Delete();
      }
    if (this->TransformList[i].InverseTransform)
      {
      this->TransformList[i].InverseTransform->Delete();
      }
    }

  // make a DeepCopy of the PreMatrix transform
  if (concat->PreMatrix)
    {
    i = (concat->InverseFlag ? concat->NumberOfTransforms-1 : 0);
    vtkTransformPair *pair = &this->TransformList[i];
    vtkSimpleTransform *mtrans;

    if (concat->InverseFlag == this->InverseFlag)
      {
      mtrans = (oldPreMatrixTransform ? oldPreMatrixTransform :
                vtkSimpleTransform::New());
      oldPreMatrixTransform = NULL;
      }
    else 
      {
      mtrans = (oldPostMatrixTransform ? oldPostMatrixTransform :
                vtkSimpleTransform::New());      
      oldPostMatrixTransform = NULL;
      }

    this->PreMatrix = mtrans->GetMatrix();
    this->PreMatrix->DeepCopy(concat->PreMatrix);
    this->PreMatrixTransform = mtrans;
    this->PreMatrixTransform->Modified();

    if (pair->ForwardTransform)
      {
      pair->ForwardTransform->Delete();
      pair->ForwardTransform = NULL;
      }
    if (pair->InverseTransform)
      {
      pair->InverseTransform->Delete();
      pair->InverseTransform = NULL;
      }

    if (concat->InverseFlag)
      {
      pair->ForwardTransform = NULL;
      pair->InverseTransform = this->PreMatrixTransform;
      }
    else
      {
      pair->ForwardTransform = this->PreMatrixTransform;
      pair->InverseTransform = NULL;
      }
    }

  // make a DeepCopy of the PostMatrix transform
  if (concat->PostMatrix)
    {
    i = (concat->InverseFlag ? 0 : concat->NumberOfTransforms-1);
    vtkTransformPair *pair = &this->TransformList[i];
    vtkSimpleTransform *mtrans;

    if (concat->InverseFlag == this->InverseFlag)
      {
      mtrans = (oldPostMatrixTransform ? oldPostMatrixTransform :
                vtkSimpleTransform::New());
      oldPostMatrixTransform = NULL;
      }
    else 
      {
      mtrans = (oldPreMatrixTransform ? oldPreMatrixTransform :
                vtkSimpleTransform::New());      
      oldPreMatrixTransform = NULL;
      }

    this->PostMatrix = mtrans->GetMatrix();
    this->PostMatrix->DeepCopy(concat->PostMatrix);
    this->PostMatrixTransform = mtrans;
    this->PostMatrixTransform->Modified();

    if (pair->ForwardTransform)
      {
      pair->ForwardTransform->Delete();
      pair->ForwardTransform = NULL;
      }
    if (pair->InverseTransform)
      {
      pair->InverseTransform->Delete();
      pair->InverseTransform = NULL;
      }
    if (concat->InverseFlag)
      {
      pair->ForwardTransform = NULL;
      pair->InverseTransform = this->PostMatrixTransform;
      }
    else
      {
      pair->ForwardTransform = this->PostMatrixTransform;
      pair->InverseTransform = NULL;
      }
    }

  // delete the old PreMatrix and PostMatrix transforms if not re-used
  if (oldPreMatrixTransform)
    {
    oldPreMatrixTransform->Delete();
    }
  if (oldPostMatrixTransform)
    {
    oldPostMatrixTransform->Delete();
    }

  // copy misc. ivars
  this->InverseFlag = concat->InverseFlag;
  this->PreMultiplyFlag = concat->PreMultiplyFlag;

  this->NumberOfTransforms = concat->NumberOfTransforms;
  this->NumberOfPreTransforms = concat->NumberOfPreTransforms;
}

//----------------------------------------------------------------------------
void vtkTransformConcatenation::PrintSelf(ostream& os, vtkIndent indent)
{
  os << indent << "InverseFlag: " << this->InverseFlag << "\n";
  os << indent << (this->PreMultiplyFlag ? "PreMultiply\n" : "PostMultiply\n");
  os << indent << "NumberOfPreTransforms: " << 
    this->GetNumberOfPreTransforms() << "\n";
  os << indent << "NumberOfPostTransforms: " << 
    this->GetNumberOfPostTransforms() << "\n"; 
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// All of the following methods are for vtkTransformConcatenationStack
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
vtkTransformConcatenationStack::vtkTransformConcatenationStack()
{
  this->StackSize = 0;
  this->StackBottom = NULL;
  this->Stack = NULL;
}

//----------------------------------------------------------------------------
vtkTransformConcatenationStack::~vtkTransformConcatenationStack()
{
  int n = static_cast<int>(this->Stack-this->StackBottom);
  for (int i = 0; i < n; i++)
    {
    this->StackBottom[i]->Delete();
    }

  if (this->StackBottom)
    {
    delete [] this->StackBottom;
    }
}  

//----------------------------------------------------------------------------
void vtkTransformConcatenationStack::Pop(vtkTransformConcatenation **concat)
{
  // if we're at the bottom of the stack, don't pop
  if (this->Stack == this->StackBottom)
    {
    return;
    }

  // get the previous PreMultiplyFlag
  int preMultiplyFlag = (*concat)->GetPreMultiplyFlag();

  // delete the previous item
  (*concat)->Delete();

  // pop new item off the stack
  *concat = *--this->Stack;
 
  // re-set the PreMultiplyFlag
  (*concat)->SetPreMultiplyFlag(preMultiplyFlag);
}

//----------------------------------------------------------------------------
void vtkTransformConcatenationStack::Push(vtkTransformConcatenation **concat)
{
  // check stack size and grow if necessary
  if ((this->Stack - this->StackBottom) == this->StackSize) 
    {
    int newStackSize = this->StackSize + 10;
    vtkTransformConcatenation **newStackBottom =
      new vtkTransformConcatenation *[newStackSize];
    for (int i = 0; i < this->StackSize; i++)
      {
      newStackBottom[i] = this->StackBottom[i];
      }
    if (this->StackBottom)
      {
      delete [] this->StackBottom;
      }
    this->StackBottom = newStackBottom;
    this->Stack = this->StackBottom+this->StackSize;
    this->StackSize = newStackSize;
    }

  // add item to the stack
  *this->Stack++ = *concat;

  // make a copy of that item the current item
  *concat = vtkTransformConcatenation::New();
  (*concat)->DeepCopy(*(this->Stack-1));
}

//----------------------------------------------------------------------------
void vtkTransformConcatenationStack::DeepCopy(
                                      vtkTransformConcatenationStack *stack)
{
  int n = static_cast<int>(stack->Stack - stack->StackBottom);
  int m = static_cast<int>(this->Stack - this->StackBottom);

  // check to see if we have to grow the stack
  if (n > this->StackSize)
    {
    int newStackSize = n + n%10;
    vtkTransformConcatenation **newStackBottom =
      new vtkTransformConcatenation *[newStackSize];
    for (int j = 0; j < m; j++)
      {
      newStackBottom[j] = this->StackBottom[j];
      }
    if (this->StackBottom)
      {
      delete [] this->StackBottom;
      }
    this->StackBottom = newStackBottom;
    this->Stack = this->StackBottom+this->StackSize;
    this->StackSize = newStackSize;
    }
  
  // delete surplus items
  for (int l = n; l < m; l++)
    {
    (*--this->Stack)->Delete();
    }

  // allocate new items
  for (int i = m; i < n; i++)
    {
    *this->Stack++ = vtkTransformConcatenation::New();
    }

  // deep copy the items
  for (int k = 0; k < n; k++)
    {
    this->StackBottom[k]->DeepCopy(stack->StackBottom[k]);
    }
}

#ifndef VTK_LEGACY_REMOVE
void vtkAbstractTransform::Identity()
{
  vtkWarningMacro("vtkAbstractTransform::Identity() is deprecated");
}
#endif
