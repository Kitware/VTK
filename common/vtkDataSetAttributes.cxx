/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDataSetAttributes.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


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
#include "vtkDataSetAttributes.h"
#include "vtkCell.h"
#include "vtkMath.h"
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
#include "vtkObjectFactory.h"



//------------------------------------------------------------------------------
vtkDataSetAttributes* vtkDataSetAttributes::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkDataSetAttributes");
  if(ret)
    {
    return (vtkDataSetAttributes*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkDataSetAttributes;
}




// Construct object with copying turned on for all data.
vtkDataSetAttributes::vtkDataSetAttributes()
{
  this->Scalars = NULL;
  this->Vectors = NULL;
  this->Normals = NULL;
  this->TCoords = NULL;
  this->Tensors = NULL;
  this->FieldData = NULL;
  this->GhostLevels = NULL;

  this->CopyScalars = 1;
  this->CopyVectors = 1;
  this->CopyNormals = 1;
  this->CopyTCoords = 1;
  this->CopyTensors = 1;
  this->CopyFieldData = 1;
  this->CopyGhostLevels = 1;
  
  this->Null3Tuple[0] = this->Null3Tuple[1] = this->Null3Tuple[2] = 0.0;
  this->Null4Tuple[0] = this->Null4Tuple[1] = this->Null4Tuple[2] = this->Null4Tuple[3] = 0;

  this->NullTuple = new float[3];
  this->Tuple = new float[3];
  this->TupleSize = 3;

  this->AnyEnabled = 0;
  this->CopyScalarsEnabled = 0;
  this->CopyVectorsEnabled = 0;
  this->CopyNormalsEnabled = 0;
  this->CopyTCoordsEnabled = 0;
  this->CopyTensorsEnabled = 0;
  this->CopyFieldDataEnabled = 0;
  this->CopyGhostLevelsEnabled = 0;

  this->NullTensor = vtkTensor::New();
}

// Destructor for the vtkDataSetAttributes objects.
vtkDataSetAttributes::~vtkDataSetAttributes()
{
  vtkDataSetAttributes::Initialize();

  if ( this->NullTuple )
    {
    delete [] this->NullTuple;
    }
  if ( this->Tuple )
    {
    delete [] this->Tuple;
    }
  this->NullTensor->Delete();
  this->NullTensor = NULL;
}

// Deep copy of data (i.e., create new data arrays and
// copy from input data).
void vtkDataSetAttributes::DeepCopy(vtkDataSetAttributes *pd)
{
  vtkAttributeData *data, *newData;
  vtkFieldData *f, *newF;

  this->Initialize(); //free up memory
  
  if ( (data=pd->GetScalars()) != NULL )
    {
    newData = data->MakeObject();
    newData->DeepCopy(data);
    this->SetScalars((vtkScalars *) newData);
    newData->Delete ();
    }
  
  if ( (data=pd->GetVectors()) != NULL )
    {
    newData = data->MakeObject();
    newData->DeepCopy(data);
    this->SetVectors((vtkVectors *) newData);
    newData->Delete ();
    }
  
  if ( (data=pd->GetNormals()) != NULL )
    {
    newData = data->MakeObject();
    newData->DeepCopy(data);
    this->SetNormals((vtkNormals *) newData);
    newData->Delete ();
    }
  
  if ( (data=pd->GetTCoords()) != NULL )
    {
    newData = data->MakeObject();
    newData->DeepCopy(data);
    this->SetTCoords((vtkTCoords *) newData);
    newData->Delete ();
    }
  
  if ( (data=pd->GetTensors()) != NULL )
    {
    newData = data->MakeObject();
    newData->DeepCopy(data);
    this->SetTensors((vtkTensors *) newData);
    newData->Delete ();
    }
  
  if ( (f=pd->GetFieldData()) != NULL )
    {
    newF = (vtkFieldData *)f->MakeObject();
    newF->DeepCopy(f);
    this->SetFieldData((vtkFieldData *) newF);
    newF->Delete ();
    }
  
  if ( (data=pd->GetGhostLevels()) != NULL )
    {
    newData = data->MakeObject();
    newData->DeepCopy(data);
    this->SetGhostLevels((vtkGhostLevels *) newData);
    newData->Delete();
    }

  this->CopyScalars = pd->CopyScalars;
  this->CopyVectors = pd->CopyVectors;
  this->CopyNormals = pd->CopyNormals;
  this->CopyTCoords = pd->CopyTCoords;
  this->CopyTensors = pd->CopyTensors;
  this->CopyFieldData = pd->CopyFieldData;
  this->CopyGhostLevels = pd->CopyGhostLevels;
}

// Shallow copy of data (i.e., use reference counting).
void vtkDataSetAttributes::ShallowCopy(vtkDataSetAttributes *pd)
{
  this->SetScalars(pd->GetScalars());
  this->SetVectors(pd->GetVectors());
  this->SetNormals(pd->GetNormals());
  this->SetTCoords(pd->GetTCoords());
  this->SetTensors(pd->GetTensors());
  this->SetFieldData(pd->GetFieldData());
  this->SetGhostLevels(pd->GetGhostLevels());

  this->CopyScalars = pd->CopyScalars;
  this->CopyVectors = pd->CopyVectors;
  this->CopyNormals = pd->CopyNormals;
  this->CopyTCoords = pd->CopyTCoords;
  this->CopyTensors = pd->CopyTensors;
  this->CopyFieldData = pd->CopyFieldData;
  this->CopyGhostLevels = pd->CopyGhostLevels;
}

// Check object's components for modified times.
unsigned long int vtkDataSetAttributes::GetMTime()
{
  unsigned long int mtime = this->MTime;
  unsigned long int otherMTime;

  if (this->Scalars)
    {
    otherMTime = this->Scalars->GetMTime();
    if ( otherMTime > mtime )
      {
      mtime = otherMTime;
      }
    }

  if (this->Vectors)
    {
    otherMTime = this->Vectors->GetMTime();
    if ( otherMTime > mtime )
      {
      mtime = otherMTime;
      }
    }

  if (this->Normals)
    {
    otherMTime = this->Normals->GetMTime();
    if ( otherMTime > mtime )
      {
      mtime = otherMTime;
      }
    }

  if(this->TCoords) 
    {
    otherMTime = this->TCoords->GetMTime();
    if ( otherMTime > mtime )
      {
      mtime = otherMTime;
      }
    }

  if(this->Tensors) 
    {
    otherMTime = this->Tensors->GetMTime();
    if ( otherMTime > mtime )
      {
      mtime = otherMTime;
      }
    }

  if(this->FieldData) 
    {
    otherMTime = this->FieldData->GetMTime();
    if ( otherMTime > mtime )
      {
      mtime = otherMTime;
      }
    }

  if (this->GhostLevels)
    {
    otherMTime = this->GhostLevels->GetMTime();
    if ( otherMTime > mtime)
      {
      mtime = otherMTime;
      }
    }

  return mtime;
}

// Initialize all of the object's data to NULL
void vtkDataSetAttributes::Initialize()
{
//
// We don't modify ourselves because the "ReleaseData" methods depend upon
// no modification when initialized.
//

//
// First free up any memory
//
  if ( this->Scalars != NULL )
    {
    this->Scalars->Delete();
    this->Scalars = NULL;
    }

  if ( this->Vectors != NULL )
    {
    this->Vectors->Delete();
    this->Vectors = NULL;
    }

  if ( this->Normals != NULL )
    {
    this->Normals->Delete();
    this->Normals = NULL;
    }

  if ( this->TCoords != NULL )
    {
    this->TCoords->Delete();
    this->TCoords = NULL;
    }

  if ( this->Tensors != NULL )
    {
    this->Tensors->Delete();
    this->Tensors = NULL;
    }

  if ( this->FieldData != NULL )
    {
    this->FieldData->Delete();
    this->FieldData = NULL;
    }

  if ( this->GhostLevels != NULL)
    {
    this->GhostLevels->Delete();
    this->GhostLevels = NULL;
    }

}

// Pass entire arrays of input data through to output. Obey the "copy"
// flags.
void vtkDataSetAttributes::PassData(vtkDataSetAttributes* pd)
{
  if ( this->CopyScalars )
    {
    this->SetScalars(pd->GetScalars());
    }
  if ( this->CopyVectors )
    {
    this->SetVectors(pd->GetVectors());
    }
  if ( this->CopyNormals )
    {
    this->SetNormals(pd->GetNormals());
    }
  if ( this->CopyTCoords )
    {
    this->SetTCoords(pd->GetTCoords());
    }
  if ( this->CopyTensors )
    {
    this->SetTensors(pd->GetTensors());
    }
  if ( this->CopyFieldData )
    {
    this->SetFieldData(pd->GetFieldData());
    }
  if ( this->CopyGhostLevels )
    {
    this->SetGhostLevels(pd->GetGhostLevels());
    }
}

// Pass entire arrays of input data through to output. Obey the "copy"
// flags. Only passes the data if the output attribute is NULL (i.e., not set).
void vtkDataSetAttributes::PassNoReplaceData(vtkDataSetAttributes* pd)
{
  if ( this->CopyScalars && !this->Scalars )
    {
    this->SetScalars(pd->GetScalars());
    }
  if ( this->CopyVectors && !this->Vectors )
    {
    this->SetVectors(pd->GetVectors());
    }
  if ( this->CopyNormals && !this->Normals )
    {
    this->SetNormals(pd->GetNormals());
    }
  if ( this->CopyTCoords && !this->TCoords )
    {
    this->SetTCoords(pd->GetTCoords());
    }
  if ( this->CopyTensors && !this->Tensors )
    {
    this->SetTensors(pd->GetTensors());
    }
  if ( this->CopyFieldData && !this->FieldData )
    {
    this->SetFieldData(pd->GetFieldData());
    }
  if ( this->CopyGhostLevels && !this->GhostLevels )
    {
    this->SetGhostLevels(pd->GetGhostLevels());
    }
}

// Allocates point data for point-by-point (or cell-by-cell) copy operation.  
// If sze=0, then use the input DataSetAttributes to create (i.e., find 
// initial size of) new objects; otherwise use the sze variable.
void vtkDataSetAttributes::CopyAllocate(vtkDataSetAttributes* pd, int sze, int ext)
{
  vtkScalars *s, *newScalars;
  vtkVectors *v, *newVectors;
  vtkNormals *n, *newNormals;
  vtkTCoords *t, *newTCoords;
  vtkTensors *tens, *newTensors;
  vtkFieldData *f, *newFieldData;
  vtkGhostLevels *g, *newGhostLevels;

  vtkDataSetAttributes::Initialize();
//
// Create various point data depending upon input
//
  if ( !pd )
    {
    return;
    }

  if ( this->CopyScalars && (s = pd->GetScalars()) ) 
    {
    newScalars = (vtkScalars *)s->MakeObject();
    if ( sze > 0 )
      {
      newScalars->Allocate(sze,ext);
      }
    else
      {
      newScalars->Allocate(s->GetNumberOfScalars());
      }
    newScalars->SetLookupTable(s->GetLookupTable());
    this->SetScalars(newScalars);
    newScalars->Delete();
    this->CopyScalarsEnabled = 1;
    }
  else
    {
    this->CopyScalarsEnabled = 0;
    }
  
  if ( this->CopyVectors && (v = pd->GetVectors()) ) 
    {
    newVectors = (vtkVectors *)v->MakeObject();
    if ( sze > 0 )
      {
      newVectors->Allocate(sze,ext);
      }
    else
      {
      newVectors->Allocate(v->GetNumberOfVectors());
      }
    this->SetVectors(newVectors);
    newVectors->Delete();
    this->CopyVectorsEnabled = 1;
    }
  else
    {
    this->CopyVectorsEnabled = 0;
    }

  if ( this->CopyNormals && (n = pd->GetNormals()) ) 
    {
    newNormals = (vtkNormals *)n->MakeObject();
    if ( sze > 0 )
      {
      newNormals->Allocate(sze,ext);
      }
    else
      {
      newNormals->Allocate(n->GetNumberOfNormals());
      }
    this->SetNormals(newNormals);
    newNormals->Delete();
    this->CopyNormalsEnabled = 1;
    }
  else
    {
    this->CopyNormalsEnabled = 0;
    }

  if ( this->CopyGhostLevels && (g = pd->GetGhostLevels()) )
    {
    newGhostLevels = (vtkGhostLevels *)g->MakeObject();
    if (sze > 0)
      {
      newGhostLevels->Allocate(sze, ext);
      }
    else
      {
      newGhostLevels->Allocate(0, ext);
      }
    this->SetGhostLevels(newGhostLevels);
    newGhostLevels->Delete();
    this->CopyGhostLevelsEnabled = 1;
    }
  else
    {
    this->CopyGhostLevelsEnabled = 0;
    }

  if ( this->CopyTCoords && (t = pd->GetTCoords()) ) 
    {
    newTCoords = (vtkTCoords *)t->MakeObject();
    if ( sze > 0 )
      {
      newTCoords->Allocate(sze,ext);
      }
    else
      {
      newTCoords->Allocate(t->GetNumberOfTCoords());
      }
    this->SetTCoords(newTCoords);
    newTCoords->Delete();
    this->CopyTCoordsEnabled = 1;
    }
  else
    {
    this->CopyTCoordsEnabled = 0;
    }

  if ( this->CopyTensors && (tens = pd->GetTensors()) ) 
    {
    newTensors = (vtkTensors *)tens->MakeObject();
    if ( sze > 0 )
      {
      newTensors->Allocate(sze,ext);
      }
    else
      {
      newTensors->Allocate(tens->GetNumberOfTensors());
      }
    this->SetTensors(newTensors);
    newTensors->Delete();
    this->CopyTensorsEnabled = 1;
    }
  else
    {
    this->CopyTensorsEnabled = 0;
    }
  
  if ( this->CopyFieldData && (f = pd->GetFieldData()) ) 
    {
    int i, numComp=f->GetNumberOfComponents();
    if ( this->TupleSize != numComp )
      {
      if ( this->Tuple )
        {
        delete [] this->Tuple;
        }
      if ( this->NullTuple )
        {
        delete [] this->NullTuple;
        }
      this->NullTuple = new float[numComp];
      for (i=0; i<numComp; i++)
        {
        this->NullTuple[i] = 0.0;
        }
      this->Tuple = new float[numComp];
      this->TupleSize = numComp;
      }
    
    newFieldData = (vtkFieldData *)f->MakeObject();
    if ( sze > 0 )
      {
      newFieldData->Allocate(sze,ext);
      }
    else
      {
      newFieldData->Allocate(f->GetNumberOfTuples());
      }
    this->SetFieldData(newFieldData);
    newFieldData->Delete();
    this->CopyFieldDataEnabled = 1;
    }
  else
    {
    this->CopyFieldDataEnabled = 0;
    }

  this->AnyEnabled = (this->CopyScalarsEnabled || this->CopyVectorsEnabled || 
                      this->CopyNormalsEnabled || this->CopyTCoordsEnabled ||
                      this->CopyTensorsEnabled || this->CopyFieldDataEnabled ||
                      this->CopyGhostLevelsEnabled);
}


// Copy the attribute data from one id to another. Make sure CopyAllocate() has
// been invoked before using this method.
void vtkDataSetAttributes::CopyData(vtkDataSetAttributes* fromPd, int fromId, int toId)
{
  if ( !this->AnyEnabled )
    {
    return;
    }

  if ( this->CopyScalarsEnabled )
    {
    this->CopyTuple(fromPd->Scalars->GetData(), this->Scalars->GetData(), fromId, toId);
    }

  if ( this->CopyVectorsEnabled )
    {
    this->CopyTuple(fromPd->Vectors->GetData(), this->Vectors->GetData(), fromId, toId);
    }

  if ( this->CopyNormalsEnabled )
    {
    this->CopyTuple(fromPd->Normals->GetData(), this->Normals->GetData(), fromId, toId);
    }

  if ( this->CopyTCoordsEnabled )
    {
    this->CopyTuple(fromPd->TCoords->GetData(), this->TCoords->GetData(), fromId, toId);
    }

  if ( this->CopyTensorsEnabled )
    {
    this->CopyTuple(fromPd->Tensors->GetData(), this->Tensors->GetData(), fromId, toId);
    }

  if ( this->CopyGhostLevelsEnabled )
    {
    this->CopyTuple(fromPd->GhostLevels->GetData(), this->GhostLevels->GetData(), fromId, toId);
    }

  if ( this->CopyFieldDataEnabled )
    {
    int numArrays=this->FieldData->GetNumberOfArrays();
    for (int i=0; i<numArrays; i++)
      {
      vtkDataArray *to = this->FieldData->GetArray(i);
      vtkDataArray *from = fromPd->FieldData->GetArray(i);
      if ( to != NULL )
        {
        this->CopyTuple(from, to, fromId, toId);
        }
      }
    }
}

// Initialize point interpolation method.
void vtkDataSetAttributes::InterpolateAllocate(vtkDataSetAttributes* pd, int sze, int ext)
{
  this->CopyAllocate(pd, sze, ext);
}

// Interpolate data from points and interpolation weights. Make sure that the 
// method InterpolateAllocate() has been invoked before using this method.
void vtkDataSetAttributes::InterpolatePoint(vtkDataSetAttributes *fromPd, int toId, 
                                            vtkIdList *ptIds, float *weights)
{
  if ( !this->AnyEnabled )
    {
    return;
    }

  if ( this->CopyScalarsEnabled )
    {
    this->InterpolateTuple(fromPd->Scalars->GetData(), this->Scalars->GetData(), toId,
                           ptIds, weights);
    }

  if ( this->CopyVectorsEnabled )
    {
    this->InterpolateTuple(fromPd->Vectors->GetData(), this->Vectors->GetData(), toId,
                           ptIds, weights);
    }

  if ( this->CopyNormalsEnabled )
    {
    this->InterpolateTuple(fromPd->Normals->GetData(), this->Normals->GetData(), toId,
                           ptIds, weights);
    }

  if ( this->CopyTCoordsEnabled )
    {
    this->InterpolateTuple(fromPd->TCoords->GetData(), this->TCoords->GetData(), toId,
                           ptIds, weights);
    }

  if ( this->CopyTensorsEnabled )
    {
    this->InterpolateTuple(fromPd->Tensors->GetData(), this->Tensors->GetData(), toId,
                           ptIds, weights);
    }

  if ( this->CopyGhostLevelsEnabled )
    {
    this->InterpolateTuple(fromPd->GhostLevels->GetData(), this->GhostLevels->GetData(), toId,
                           ptIds, weights);
    }

  if ( this->CopyFieldDataEnabled )
    {
    int numArrays=this->FieldData->GetNumberOfArrays();
    for (int i=0; i<numArrays; i++)
      {
      vtkDataArray *to = this->FieldData->GetArray(i);
      if ( to != NULL )
        {
        this->InterpolateTuple(fromPd->FieldData->GetArray(i), this->FieldData->GetArray(i),  
                               toId, ptIds, weights);
        }
      }
    }
}

// Interpolate data from the two points p1,p2 (forming an edge) and an 
// interpolation factor, t, along the edge. The weight ranges from (0,1), 
// with t=0 located at p1. Make sure that the method InterpolateAllocate() 
// has been invoked before using this method.
void vtkDataSetAttributes::InterpolateEdge(vtkDataSetAttributes *fromPd, int toId,
                                   int p1, int p2, float t)
{
  if ( !this->AnyEnabled )
    {
    return;
    }

  if ( this->CopyScalarsEnabled )
    {
    this->InterpolateTuple(fromPd->Scalars->GetData(), this->Scalars->GetData(), 
                           toId, p1, p2, t);
    }

  if ( this->CopyVectorsEnabled )
    {
    this->InterpolateTuple(fromPd->Vectors->GetData(), this->Vectors->GetData(), 
                           toId, p1, p2, t);
    }

  if ( this->CopyNormalsEnabled )
    {
    this->InterpolateTuple(fromPd->Normals->GetData(), this->Normals->GetData(), 
                           toId, p1, p2, t);
    }

  if ( this->CopyTCoordsEnabled )
    {
    this->InterpolateTuple(fromPd->TCoords->GetData(), this->TCoords->GetData(), 
                           toId, p1, p2, t);
    }

  if ( this->CopyTensorsEnabled )
    {
    this->InterpolateTuple(fromPd->Tensors->GetData(), this->Tensors->GetData(), 
                           toId, p1, p2, t);
    }

  if ( this->CopyGhostLevelsEnabled )
    {
    this->InterpolateTuple(fromPd->GhostLevels->GetData(), this->GhostLevels->GetData(),
                           toId, p1, p2, t);
    }

  if ( this->CopyFieldDataEnabled )
    {
    int numArrays=this->FieldData->GetNumberOfArrays();
    for (int i=0; i<numArrays; i++)
      {
      vtkDataArray *to = this->FieldData->GetArray(i);
      if ( to != NULL )
        {
        this->InterpolateTuple(fromPd->FieldData->GetArray(i), this->FieldData->GetArray(i), 
                               toId, p1, p2, t);
        }
      }
    }
}

// Interpolate data from the two points p1,p2 (forming an edge) and an 
// interpolation factor, t, along the edge. The weight ranges from (0,1), 
// with t=0 located at p1. Make sure that the method InterpolateAllocate() 
// has been invoked before using this method.
void vtkDataSetAttributes::InterpolateTime(vtkDataSetAttributes *from1,
                                           vtkDataSetAttributes *from2,
                                           int id, float t)
{
  if ( !this->AnyEnabled )
    {
    return;
    }

  if ( this->CopyScalarsEnabled )
    {
    this->InterpolateTuple(from1->Scalars->GetData(), from2->Scalars->GetData(),
                           this->Scalars->GetData(), id, t);
    }

  if ( this->CopyVectorsEnabled )
    {
    this->InterpolateTuple(from1->Vectors->GetData(), from2->Vectors->GetData(),
                           this->Vectors->GetData(), id, t);
    }

  if ( this->CopyNormalsEnabled )
    {
    this->InterpolateTuple(from1->Normals->GetData(), from2->Normals->GetData(),
                           this->Normals->GetData(), id, t);
    }

  if ( this->CopyTCoordsEnabled )
    {
    this->InterpolateTuple(from1->TCoords->GetData(), from2->TCoords->GetData(),
                           this->TCoords->GetData(), id, t);
    }

  if ( this->CopyTensorsEnabled )
    {
    this->InterpolateTuple(from1->Tensors->GetData(), from2->Tensors->GetData(),
                           this->Tensors->GetData(), id, t);
    }
  
  if ( this->CopyGhostLevelsEnabled )
    {
    this->InterpolateTuple(from1->GhostLevels->GetData(), from2->GhostLevels->GetData(),
			   this->GhostLevels->GetData(), id, t);
    }

  if ( this->CopyFieldDataEnabled )
    {
    int numArrays=this->FieldData->GetNumberOfArrays();
    for (int i=0; i<numArrays; i++)
      {
      vtkDataArray *to = this->FieldData->GetArray(i);
      if ( to != NULL )
        {
        this->InterpolateTuple(from1->FieldData->GetArray(i),
                               from2->FieldData->GetArray(i),
                               this->FieldData->GetArray(i), id, t);
        }
      }
    }
}

// Resize object to just fit data requirements. Reclaims extra memory.
void vtkDataSetAttributes::Squeeze()
{
  if ( this->Scalars )
    {
    this->Scalars->Squeeze();
    }
  if ( this->Vectors )
    {
    this->Vectors->Squeeze();
    }
  if ( this->Normals )
    {
    this->Normals->Squeeze();
    }
  if ( this->TCoords )
    {
    this->TCoords->Squeeze();
    }
  if ( this->Tensors )
    {
    this->Tensors->Squeeze();
    }
  if ( this->FieldData )
    {
    this->FieldData->Squeeze();
    }
  if ( this->GhostLevels )
    {
    this->GhostLevels->Squeeze();
    }
}

// Turn on copying of all data.
void vtkDataSetAttributes::CopyAllOn()
{
  this->CopyScalarsOn();
  this->CopyVectorsOn();
  this->CopyNormalsOn();
  this->CopyTCoordsOn();
  this->CopyTensorsOn();
  this->CopyFieldDataOn();
  this->CopyGhostLevelsOn();
}

// Turn off copying of all data.
void vtkDataSetAttributes::CopyAllOff()
{
  this->CopyScalarsOff();
  this->CopyVectorsOff();
  this->CopyNormalsOff();
  this->CopyTCoordsOff();
  this->CopyTensorsOff();
  this->CopyFieldDataOff();
  this->CopyGhostLevelsOff();
}

// Copy a tuple of data from one data array to another. This method (and
// following ones) assume that the fromData and toData objects are of the
// same type, and have the same number of components. This is true if you
// invoke CopyAllocate() or InterpolateAllocate().
void vtkDataSetAttributes::CopyTuple(vtkDataArray *fromData, vtkDataArray *toData, 
                                     int fromId, int toId)
{
  int i;
  int numComp=fromData->GetNumberOfComponents();

  switch (fromData->GetDataType())
    {
    case VTK_BIT:
      {
      vtkBitArray *from=(vtkBitArray *)fromData;
      vtkBitArray *to=(vtkBitArray *)toData;
      for (i=0; i<numComp; i++)
        {
        to->InsertValue(toId+i, from->GetValue(fromId+i));
        }
      }
      break;

    case VTK_CHAR:
      {
      char *from=((vtkCharArray *)fromData)->GetPointer(fromId*numComp);
      char *to=((vtkCharArray *)toData)->WritePointer(toId*numComp,numComp);
      for (i=0; i<numComp; i++)
        {
        *to++ = *from++;
        }
      }
      break;

    case VTK_UNSIGNED_CHAR:
      {
      unsigned char *from=((vtkUnsignedCharArray *)fromData)->GetPointer(fromId*numComp);
      unsigned char *to=((vtkUnsignedCharArray *)toData)->WritePointer(toId*numComp,numComp);
      for (i=0; i<numComp; i++)
        {
        *to++ = *from++;
        }
      }
      break;

    case VTK_SHORT:
      {
      short *from=((vtkShortArray *)fromData)->GetPointer(fromId*numComp);
      short *to=((vtkShortArray *)toData)->WritePointer(toId*numComp,numComp);
      for (i=0; i<numComp; i++)
        {
        *to++ = *from++;
        }
      }
      break;

    case VTK_UNSIGNED_SHORT:
      {
      unsigned short *from=((vtkUnsignedShortArray *)fromData)->GetPointer(fromId*numComp);
      unsigned short *to=((vtkUnsignedShortArray *)toData)->WritePointer(toId*numComp,numComp);
      for (i=0; i<numComp; i++)
        {
        *to++ = *from++;
        }
      }
      break;

    case VTK_INT:
      {
      int *from=((vtkIntArray *)fromData)->GetPointer(fromId*numComp);
      int *to=((vtkIntArray *)toData)->WritePointer(toId*numComp,numComp);
      for (i=0; i<numComp; i++)
        {
        *to++ = *from++;
        }
      }
      break;

    case VTK_UNSIGNED_INT:
      {
      unsigned int *from=((vtkUnsignedIntArray *)fromData)->GetPointer(fromId*numComp);
      unsigned int *to=((vtkUnsignedIntArray *)toData)->WritePointer(toId*numComp,numComp);
      for (i=0; i<numComp; i++)
        {
        *to++ = *from++;
        }
      }
      break;

    case VTK_LONG:
      {
      long *from=((vtkLongArray *)fromData)->GetPointer(fromId*numComp);
      long *to=((vtkLongArray *)toData)->WritePointer(toId*numComp,numComp);
      for (i=0; i<numComp; i++)
        {
        *to++ = *from++;
        }
      }
      break;

    case VTK_UNSIGNED_LONG:
      {
      unsigned long *from=((vtkUnsignedLongArray *)fromData)->GetPointer(fromId*numComp);
      unsigned long *to=((vtkUnsignedLongArray *)toData)->WritePointer(toId*numComp,numComp);
      for (i=0; i<numComp; i++)
        {
        *to++ = *from++;
        }
      }
      break;

    case VTK_FLOAT:
      {
      float *from=((vtkFloatArray *)fromData)->GetPointer(fromId*numComp);
      float *to=((vtkFloatArray *)toData)->WritePointer(toId*numComp,numComp);
      for (i=0; i<numComp; i++)
        {
        *to++ = *from++;
        }
      }
      break;

    case VTK_DOUBLE:
      {
      double *from=((vtkDoubleArray *)fromData)->GetPointer(fromId*numComp);
      double *to=((vtkDoubleArray *)toData)->WritePointer(toId*numComp,numComp);
      for (i=0; i<numComp; i++)
        {
        *to++ = *from++;
        }
      }
      break;

    default:
      vtkErrorMacro(<<"Unsupported data type during copy!");
    }
}

void vtkDataSetAttributes::InterpolateTuple(vtkDataArray *fromData, 
                                            vtkDataArray *toData, 
                                            int toId, vtkIdList *ptIds, 
                                            float *weights)
{
  int numComp=fromData->GetNumberOfComponents();
  int i, j, numIds=ptIds->GetNumberOfIds();
  int *ids=ptIds->GetPointer(0), idx=toId*numComp;
  double c;
  
  switch (fromData->GetDataType())
    {
    case VTK_BIT:
      {
      vtkBitArray *from=(vtkBitArray *)fromData;
      vtkBitArray *to=(vtkBitArray *)toData;
      for (i=0; i<numComp; i++)
        {
        for (c=0, j=0; j<numIds; j++)
          {
          c += weights[j]*from->GetValue(ids[j]*numComp+i);
          }
        to->InsertValue(idx+i, (int)c);
        }
      }
      break;

    case VTK_CHAR:
      {
      char *from=((vtkCharArray *)fromData)->GetPointer(0);
      char *to=((vtkCharArray *)toData)->WritePointer(idx,numComp);
      for (i=0; i<numComp; i++)
        {
        for (c=0, j=0; j<numIds; j++)
          {
          c += weights[j]*from[ids[j]*numComp+i];
          }
        *to++ = (char) c;
        }
      }
      break;

    case VTK_UNSIGNED_CHAR:
      {
      unsigned char *from=((vtkUnsignedCharArray *)fromData)->GetPointer(0);
      unsigned char *to=((vtkUnsignedCharArray *)toData)->WritePointer(idx,numComp);
      for (i=0; i<numComp; i++)
        {
        for (c=0, j=0; j<numIds; j++)
          {
          c += weights[j]*from[ids[j]*numComp+i];
          }
        *to++ = (unsigned char) c;
        }
      }
      break;

    case VTK_SHORT:
      {
      short *from=((vtkShortArray *)fromData)->GetPointer(0);
      short *to=((vtkShortArray *)toData)->WritePointer(idx,numComp);
      for (i=0; i<numComp; i++)
        {
        for (c=0, j=0; j<numIds; j++)
          {
          c += weights[j]*from[ids[j]*numComp+i];
          }
        *to++ = (short) c;
        }
      }
      break;

    case VTK_UNSIGNED_SHORT:
      {
      unsigned short *from=((vtkUnsignedShortArray *)fromData)->GetPointer(0);
      unsigned short *to=((vtkUnsignedShortArray *)toData)->WritePointer(idx,numComp);
      for (i=0; i<numComp; i++)
        {
        for (c=0, j=0; j<numIds; j++)
          {
          c += weights[j]*from[ids[j]*numComp+i];
          }
        *to++ = (unsigned short) c;
        }
      }
      break;

    case VTK_INT:
      {
      int *from=((vtkIntArray *)fromData)->GetPointer(0);
      int *to=((vtkIntArray *)toData)->WritePointer(idx,numComp);
      for (i=0; i<numComp; i++)
        {
        for (c=0, j=0; j<numIds; j++)
          {
          c += weights[j]*from[ids[j]*numComp+i];
          }
        *to++ = (int) c;
        }
      }
      break;

    case VTK_UNSIGNED_INT:
      {
      unsigned int *from=((vtkUnsignedIntArray *)fromData)->GetPointer(0);
      unsigned int *to=((vtkUnsignedIntArray *)toData)->WritePointer(idx,numComp);
      for (i=0; i<numComp; i++)
        {
        for (c=0, j=0; j<numIds; j++)
          {
          c += weights[j]*from[ids[j]*numComp+i];
          }
        *to++ = (unsigned int) c;
        }
      }
      break;

    case VTK_LONG:
      {
      long *from=((vtkLongArray *)fromData)->GetPointer(0);
      long *to=((vtkLongArray *)toData)->WritePointer(idx,numComp);
      for (i=0; i<numComp; i++)
        {
        for (c=0, j=0; j<numIds; j++)
          {
          c += weights[j]*(float)from[ids[j]*numComp+i];
          }
        *to++ = (long) c;
        }
      }
      break;

    case VTK_UNSIGNED_LONG:
      {
      unsigned long *from=((vtkUnsignedLongArray *)fromData)->GetPointer(0);
      unsigned long *to=((vtkUnsignedLongArray *)toData)->WritePointer(idx,numComp);
      for (i=0; i<numComp; i++)
        {
        for (c=0, j=0; j<numIds; j++)
          {
          c += weights[j]*(float)from[ids[j]*numComp+i];
          }
        *to++ = (unsigned long) c;
        }
      }
      break;

    case VTK_FLOAT:
      {
      float *from=((vtkFloatArray *)fromData)->GetPointer(0);
      float *to=((vtkFloatArray *)toData)->WritePointer(idx,numComp);
      for (i=0; i<numComp; i++)
        {
        for (c=0.0, j=0; j<numIds; j++)
          {
          c += weights[j]*from[ids[j]*numComp+i];
          }
        *to++ = c;
        }
      }
      break;

    case VTK_DOUBLE:
      {
      double *from=((vtkDoubleArray *)fromData)->GetPointer(0);
      double *to=((vtkDoubleArray *)toData)->WritePointer(idx,numComp);
      for (i=0; i<numComp; i++)
        {
        for (c=0.0, j=0; j<numIds; j++)
          {
          c += weights[j]*from[ids[j]*numComp+i];
          }
        *to++ = (double) c;
        }
      }
      break;

    default:
      vtkErrorMacro(<<"Unsupported data type during copy!");
    }
}

void vtkDataSetAttributes::InterpolateTuple(vtkDataArray *fromData, 
                                            vtkDataArray *toData, 
                                            int toId, int id1, int id2, float t)
{
  int numComp=fromData->GetNumberOfComponents();
  int i, idx=toId*numComp;
  int idx1=id1*numComp, idx2=id2*numComp;
  float c;
  
  switch (fromData->GetDataType())
    {
    case VTK_BIT:
      {
      vtkBitArray *from=(vtkBitArray *)fromData;
      vtkBitArray *to=(vtkBitArray *)toData;
      for (i=0; i<numComp; i++)
        {
        c = from->GetValue(idx1+i)+ t * (from->GetValue(idx2+i) - from->GetValue(idx1+i));
        to->InsertValue(idx+i, (int)c);
        }
      }
      break;

    case VTK_CHAR:
      {
      char *from=((vtkCharArray *)fromData)->GetPointer(0);
      char *to=((vtkCharArray *)toData)->WritePointer(idx,numComp);
      for (i=0; i<numComp; i++)
        {
        c = from[idx1+i] + t * (from[idx2+i] - from[idx1+i]);
        *to++ = (char) c;
        }
      }
      break;

    case VTK_UNSIGNED_CHAR:
      {
      unsigned char *from=((vtkUnsignedCharArray *)fromData)->GetPointer(0);
      unsigned char *to=((vtkUnsignedCharArray *)toData)->WritePointer(idx,numComp);
      for (i=0; i<numComp; i++)
        {
        c = from[idx1+i] + t * (from[idx2+i] - from[idx1+i]);
        *to++ = (unsigned char) c;
        }
      }
      break;

    case VTK_SHORT:
      {
      short *from=((vtkShortArray *)fromData)->GetPointer(0);
      short *to=((vtkShortArray *)toData)->WritePointer(idx,numComp);
      for (i=0; i<numComp; i++)
        {
        c = from[idx1+i] + t * (from[idx2+i] - from[idx1+i]);
        *to++ = (short) c;
        }
      }
      break;

    case VTK_UNSIGNED_SHORT:
      {
      unsigned short *from=((vtkUnsignedShortArray *)fromData)->GetPointer(0);
      unsigned short *to=((vtkUnsignedShortArray *)toData)->WritePointer(idx,numComp);
      for (i=0; i<numComp; i++)
        {
        c = from[idx1+i] + t * (from[idx2+i] - from[idx1+i]);
        *to++ = (unsigned short) c;
        }
      }
      break;

    case VTK_INT:
      {
      int *from=((vtkIntArray *)fromData)->GetPointer(0);
      int *to=((vtkIntArray *)toData)->WritePointer(idx,numComp);
      for (i=0; i<numComp; i++)
        {
        c = from[idx1+i] + t * (from[idx2+i] - from[idx1+i]);
        *to++ = (int) c;
        }
      }
      break;

    case VTK_UNSIGNED_INT:
      {
      unsigned int *from=((vtkUnsignedIntArray *)fromData)->GetPointer(0);
      unsigned int *to=((vtkUnsignedIntArray *)toData)->WritePointer(idx,numComp);
      for (i=0; i<numComp; i++)
        {
        c = from[idx1+i] + t * (from[idx2+i] - from[idx1+i]);
        *to++ = (unsigned int) c;
        }
      }
      break;

    case VTK_LONG:
      {
      long *from=((vtkLongArray *)fromData)->GetPointer(0);
      long *to=((vtkLongArray *)toData)->WritePointer(idx,numComp);
      for (i=0; i<numComp; i++)
        {
        c = (float)from[idx1+i] + t * (float)(from[idx2+i] - from[idx1+i]);
        *to++ = (long) c;
        }
      }
      break;

    case VTK_UNSIGNED_LONG:
      {
      unsigned long *from=((vtkUnsignedLongArray *)fromData)->GetPointer(0);
      unsigned long *to=((vtkUnsignedLongArray *)toData)->WritePointer(idx,numComp);
      for (i=0; i<numComp; i++)
        {
        c = (float)from[idx1+i] + t * (float)(from[idx2+i] - from[idx1+i]);
        *to++ = (unsigned long) c;
        }
      }
      break;

    case VTK_FLOAT:
      {
      float *from=((vtkFloatArray *)fromData)->GetPointer(0);
      float *to=((vtkFloatArray *)toData)->WritePointer(idx,numComp);
      for (i=0; i<numComp; i++)
        {
        c = (1.0 - t) * from[idx1+i] + t * from[idx2+i];
        *to++ = c;
        }
      }
      break;

    case VTK_DOUBLE:
      {
      double *from=((vtkDoubleArray *)fromData)->GetPointer(0);
      double *to=((vtkDoubleArray *)toData)->WritePointer(idx,numComp);
      for (i=0; i<numComp; i++)
        {
        c = (1.0 - t) * from[idx1+i] + t * from[idx2+i];
        *to++ = (double) c;
        }
      }
      break;

    default:
      vtkErrorMacro(<<"Unsupported data type during copy!");
    }
}

void vtkDataSetAttributes::InterpolateTuple(vtkDataArray *fromData1,
                                            vtkDataArray *fromData2, 
                                            vtkDataArray *toData, int id,
                                            float t)
{
  int numComp=fromData1->GetNumberOfComponents();
  int i, idx=id*numComp, ii;
  float c;
  
  switch (fromData1->GetDataType())
    {
    case VTK_BIT:
      {
      vtkBitArray *from1=(vtkBitArray *)fromData1;
      vtkBitArray *from2=(vtkBitArray *)fromData2;
      vtkBitArray *to=(vtkBitArray *)toData;
      for (i=0; i<numComp; i++)
        {
        ii = idx + i;
        c = from1->GetValue(ii)+ t * (from2->GetValue(ii) - from1->GetValue(ii));
        to->InsertValue(ii, (int)c);
        }
      }
      break;

    case VTK_CHAR:
      {
      char *from1=((vtkCharArray *)fromData1)->GetPointer(0);
      char *from2=((vtkCharArray *)fromData2)->GetPointer(0);
      char *to=((vtkCharArray *)toData)->WritePointer(idx,numComp);
      for (i=0; i<numComp; i++)
        {
        ii = idx + i;
        c = from1[ii] + t * (from2[ii] - from1[ii]);
        *to++ = (char) c;
        }
      }
      break;

    case VTK_UNSIGNED_CHAR:
      {
      unsigned char *from1=((vtkUnsignedCharArray *)fromData1)->GetPointer(0);
      unsigned char *from2=((vtkUnsignedCharArray *)fromData2)->GetPointer(0);
      unsigned char *to=((vtkUnsignedCharArray *)toData)->WritePointer(idx,numComp);
      for (i=0; i<numComp; i++)
        {
        ii = idx + i;
        c = from1[ii] + t * (from2[ii] - from1[ii]);
        *to++ = (unsigned char) c;
        }
      }
      break;

    case VTK_SHORT:
      {
      short *from1=((vtkShortArray *)fromData1)->GetPointer(0);
      short *from2=((vtkShortArray *)fromData2)->GetPointer(0);
      short *to=((vtkShortArray *)toData)->WritePointer(idx,numComp);
      for (i=0; i<numComp; i++)
        {
        ii = idx + i;
        c = from1[ii] + t * (from2[ii] - from1[ii]);
        *to++ = (short) c;
        }
      }
      break;

    case VTK_UNSIGNED_SHORT:
      {
      unsigned short *from1=((vtkUnsignedShortArray *)fromData1)->GetPointer(0);
      unsigned short *from2=((vtkUnsignedShortArray *)fromData2)->GetPointer(0);
      unsigned short *to=((vtkUnsignedShortArray *)toData)->WritePointer(idx,numComp);
      for (i=0; i<numComp; i++)
        {
        ii = idx + i;
        c = from1[ii] + t * (from2[ii] - from1[ii]);
        *to++ = (unsigned short) c;
        }
      }
      break;

    case VTK_INT:
      {
      int *from1=((vtkIntArray *)fromData1)->GetPointer(0);
      int *from2=((vtkIntArray *)fromData2)->GetPointer(0);
      int *to=((vtkIntArray *)toData)->WritePointer(idx,numComp);
      for (i=0; i<numComp; i++)
        {
        ii = idx + i;
        c = from1[ii] + t * (from2[ii] - from1[ii]);
        *to++ = (int) c;
        }
      }
      break;

    case VTK_UNSIGNED_INT:
      {
      unsigned int *from1=((vtkUnsignedIntArray *)fromData1)->GetPointer(0);
      unsigned int *from2=((vtkUnsignedIntArray *)fromData2)->GetPointer(0);
      unsigned int *to=((vtkUnsignedIntArray *)toData)->WritePointer(idx,numComp);
      for (i=0; i<numComp; i++)
        {
        ii = idx + i;
        c = from1[ii] + t * (from2[ii] - from1[ii]);
        *to++ = (unsigned int) c;
        }
      }
      break;

    case VTK_LONG:
      {
      long *from1=((vtkLongArray *)fromData1)->GetPointer(0);
      long *from2=((vtkLongArray *)fromData2)->GetPointer(0);
      long *to=((vtkLongArray *)toData)->WritePointer(idx,numComp);
      for (i=0; i<numComp; i++)
        {
        ii = idx + i;
        c = (float)from1[ii] + t * (float)(from2[ii] - from1[ii]);
        *to++ = (long) c;
        }
      }
      break;

    case VTK_UNSIGNED_LONG:
      {
      unsigned long *from1=((vtkUnsignedLongArray *)fromData1)->GetPointer(0);
      unsigned long *from2=((vtkUnsignedLongArray *)fromData2)->GetPointer(0);
      unsigned long *to=((vtkUnsignedLongArray *)toData)->WritePointer(idx,numComp);
      for (i=0; i<numComp; i++)
        {
        ii = idx + i;
        c = (float)from1[ii] + t * (float)(from2[ii] - from1[ii]);
        *to++ = (unsigned long) c;
        }
      }
      break;

    case VTK_FLOAT:
      {
      float *from1=((vtkFloatArray *)fromData1)->GetPointer(0);
      float *from2=((vtkFloatArray *)fromData2)->GetPointer(0);
      float *to=((vtkFloatArray *)toData)->WritePointer(idx,numComp);
      for (i=0; i<numComp; i++)
        {
        ii = idx + i;
        c = (1.0 - t) * from1[ii] + t * from2[ii];
        *to++ = c;
        }
      }
      break;

    case VTK_DOUBLE:
      {
      double *from1=((vtkDoubleArray *)fromData1)->GetPointer(0);
      double *from2=((vtkDoubleArray *)fromData2)->GetPointer(0);
      double *to=((vtkDoubleArray *)toData)->WritePointer(idx,numComp);
      for (i=0; i<numComp; i++)
        {
        ii = idx + i;
        c = (1.0 - t) * from1[ii] + t * from2[ii];
        *to++ = (double) c;
        }
      }
      break;

    default:
      vtkErrorMacro(<<"Unsupported data type during interpolation!");
    }
}

unsigned long vtkDataSetAttributes::GetActualMemorySize()
{
  unsigned long size=0;

  if ( this->Scalars )
    {
    size += this->Scalars->GetActualMemorySize();
    }
  if ( this->Vectors )
    {
    size += this->Vectors->GetActualMemorySize();
    }
  if ( this->Normals )
    {
    size += this->Normals->GetActualMemorySize();
    }
  if ( this->TCoords )
    {
    size += this->TCoords->GetActualMemorySize();
    }
  if ( this->Tensors )
    {
    size += this->Tensors->GetActualMemorySize();
    }
  if ( this->FieldData )
    {
    size += this->FieldData->GetActualMemorySize();
    }
  if ( this->GhostLevels )
    {
    size += this->GhostLevels->GetActualMemorySize();
    }
  
  return size;
}

void vtkDataSetAttributes::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkObject::PrintSelf(os,indent);

  if ( this->Scalars )
    {
    os << indent << "Scalars:\n";
    this->Scalars->PrintSelf(os,indent.GetNextIndent());
    }
  else
    {
    os << indent << "Scalars: (none)\n";
    }

  if ( this->Vectors )
    {
    os << indent << "Vectors:\n";
    this->Vectors->PrintSelf(os,indent.GetNextIndent());
    }
  else
    {
    os << indent << "Vectors: (none)\n";
    }

  if ( this->Normals )
    {
    os << indent << "Normals:\n";
    this->Normals->PrintSelf(os,indent.GetNextIndent());
    }
  else
    {
    os << indent << "Normals: (none)\n";
    }

  if ( this->TCoords )
    {
    os << indent << "Texture Coordinates:\n";
    this->TCoords->PrintSelf(os,indent.GetNextIndent());
    }
  else
    {
    os << indent << "Texture Coordinates: (none)\n";
    }

  if ( this->Tensors )
    {
    os << indent << "Tensors:\n";
    this->Tensors->PrintSelf(os,indent.GetNextIndent());
    }
  else
    {
    os << indent << "Tensors: (none)\n";
    }

  if ( this->FieldData )
    {
    os << indent << "FieldData:\n";
    this->FieldData->PrintSelf(os,indent.GetNextIndent());
    }
  else
    {
    os << indent << "FieldData: (none)\n";
    }
  
  if ( this->GhostLevels )
    {
    os << indent << "GhostLevels:\n";
    this->GhostLevels->PrintSelf(os, indent.GetNextIndent());
    }
  else
    {
    os << indent << "GhostLevels: (none)\n";
    }

  os << indent << "Copy Scalars: " << (this->CopyScalars ? "On\n" : "Off\n");
  os << indent << "Copy Vectors: " << (this->CopyVectors ? "On\n" : "Off\n");
  os << indent << "Copy Normals: " << (this->CopyNormals ? "On\n" : "Off\n");
  os << indent << "Copy Texture Coordinates: " << (this->CopyTCoords ? "On\n" : "Off\n");
  os << indent << "Copy Tensors: " << (this->CopyTensors ? "On\n" : "Off\n");
  os << indent << "Copy FieldData: " << (this->CopyFieldData ? "On\n" : "Off\n");
  os << indent << "Copy GhostLevels: " << (this->CopyGhostLevels ? "On\n" : "Off\n");
}
