/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPointData.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1995 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.


=========================================================================*/
#include "vtkPointData.hh"
#include "vtkCell.hh"
#include "vtkFloatScalars.hh"
#include "vtkFloatVectors.hh"
#include "vtkFloatNormals.hh"
#include "vtkFloatTCoords.hh"
#include "vtkFloatTensors.hh"
#include "vtkPixmap.hh"

// Description:
// Construct object with copying turned on for all data.
vtkPointData::vtkPointData()
{
  this->Scalars = NULL;
  this->Vectors = NULL;
  this->Normals = NULL;
  this->TCoords = NULL;
  this->Tensors = NULL;
  this->UserDefined = NULL;

  this->CopyScalars = 1;
  this->CopyVectors = 1;
  this->CopyNormals = 1;
  this->CopyTCoords = 1;
  this->CopyTensors = 1;
  this->CopyUserDefined = 1;
}

vtkPointData::vtkPointData (const vtkPointData& pd)
{
  this->Scalars = pd.Scalars;
  if (this->Scalars) this->Scalars->Register(this);

  this->Vectors = pd.Vectors;
  if (this->Vectors) this->Vectors->Register(this);

  this->Normals = pd.Normals;
  if (this->Normals) this->Normals->Register(this);

  this->TCoords = pd.TCoords;
  if(this->TCoords) this->TCoords->Register(this);

  this->Tensors = pd.Tensors;
  if(this->Tensors) this->Tensors->Register(this);

  this->UserDefined = pd.UserDefined;
  if(this->UserDefined) this->UserDefined->Register(this);

  this->CopyScalars = 1;
  this->CopyVectors = 1;
  this->CopyNormals = 1;
  this->CopyTCoords = 1;
  this->CopyTensors = 1;
  this->CopyUserDefined = 1;
}

vtkPointData::~vtkPointData()
{
  vtkPointData::Initialize();
}

// Description:
// Shallow copy of data.
vtkPointData& vtkPointData::operator=(vtkPointData& pd)
{
  this->SetScalars(pd.GetScalars());
  this->SetVectors(pd.GetVectors());
  this->SetNormals(pd.GetNormals());
  this->SetTCoords(pd.GetTCoords());
  this->SetTensors(pd.GetTensors());
  this->SetUserDefined(pd.GetUserDefined());

  this->CopyScalars = pd.CopyScalars;
  this->CopyVectors = pd.CopyVectors;
  this->CopyNormals = pd.CopyNormals;
  this->CopyTCoords = pd.CopyTCoords;
  this->CopyTensors = pd.CopyTensors;
  this->CopyUserDefined = pd.CopyUserDefined;

  return *this;
}

// Description:
// Copy the point data from one point to another.
void vtkPointData::CopyData(vtkPointData* fromPd, int fromId, int toId)
{
  if ( fromPd->Scalars && this->Scalars && this->CopyScalars )
    {
    if (strcmp(this->Scalars->GetScalarType(),"ColorScalar"))
      {
      this->Scalars->InsertScalar(toId,fromPd->Scalars->GetScalar(fromId));
      }
    else //color scalar
      {
      vtkColorScalars *to=(vtkColorScalars *)this->Scalars;
      vtkColorScalars *from=(vtkColorScalars *)fromPd->Scalars;
      to->InsertColor(toId,from->GetColor(fromId));
      }
    }

  if ( fromPd->Vectors && this->Vectors && this->CopyVectors )
    {
    this->Vectors->InsertVector(toId,fromPd->Vectors->GetVector(fromId));
    }

  if ( fromPd->Normals && this->Normals && this->CopyNormals )
    {
    this->Normals->InsertNormal(toId,fromPd->Normals->GetNormal(fromId));
    }

  if ( fromPd->TCoords && this->TCoords && this->CopyTCoords )
    {
    this->TCoords->InsertTCoord(toId,fromPd->TCoords->GetTCoord(fromId));
    }

  if ( fromPd->Tensors && this->Tensors && this->CopyTensors )
    {
    this->Tensors->InsertTensor(toId,fromPd->Tensors->GetTensor(fromId));
    }

  if ( fromPd->UserDefined && this->UserDefined && this->CopyUserDefined )
    {
    this->UserDefined->InsertUserDefined(toId,fromPd->UserDefined->GetUserDefined(fromId));
    }
}


void vtkPointData::Initialize()
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
    this->Scalars->UnRegister(this);
    this->Scalars = NULL;
    }

  if ( this->Vectors != NULL )
    {
    this->Vectors->UnRegister(this);
    this->Vectors = NULL;
    }

  if ( this->Normals != NULL )
    {
    this->Normals->UnRegister(this);
    this->Normals = NULL;
    }

  if ( this->TCoords != NULL )
    {
    this->TCoords->UnRegister(this);
    this->TCoords = NULL;
    }

  if ( this->Tensors != NULL )
    {
    this->Tensors->UnRegister(this);
    this->Tensors = NULL;
    }

  if ( this->UserDefined != NULL )
    {
    this->UserDefined->UnRegister(this);
    this->UserDefined = NULL;
    }
};

// Description:
// Pass entire arrays of input data through to output. Obey the "copy"
// flags.
void vtkPointData::PassData(vtkPointData* pd)
{
  if ( this->CopyScalars ) this->SetScalars(pd->GetScalars());
  if ( this->CopyVectors ) this->SetVectors(pd->GetVectors());
  if ( this->CopyNormals ) this->SetNormals(pd->GetNormals());
  if ( this->CopyTCoords ) this->SetTCoords(pd->GetTCoords());
  if ( this->CopyTensors ) this->SetTensors(pd->GetTensors());
  if ( this->CopyUserDefined ) this->SetUserDefined(pd->GetUserDefined());
}

// Description:
// Allocates point data for point-by-point copy operation.  If sze=0, then 
// use the input PointData to create (i.e., find initial size of) new 
// objects; otherwise use the sze variable.
void vtkPointData::CopyAllocate(vtkPointData* pd, int sze, int ext)
{
  vtkScalars *s, *newScalars;
  vtkVectors *v, *newVectors;
  vtkNormals *n, *newNormals;
  vtkTCoords *t, *newTCoords;
  vtkTensors *tens, *newTensors;
  vtkUserDefined *ud, *newUserDefined;

  vtkPointData::Initialize();
//
// Now create various point data depending upon input
//
  if ( !pd ) return;

  if ( this->CopyScalars && (s = pd->GetScalars()) ) 
    {
    if ( sze > 0 ) newScalars = s->MakeObject(sze,ext);
    else newScalars = s->MakeObject(s->GetNumberOfScalars());
    newScalars->SetLookupTable(s->GetLookupTable());
    this->SetScalars(newScalars);
    newScalars->Delete();
    }

  if ( this->CopyVectors && (v = pd->GetVectors()) ) 
    {
    if ( sze > 0 ) newVectors = v->MakeObject(sze,ext);
    else newVectors = v->MakeObject(v->GetNumberOfVectors());
    this->SetVectors(newVectors);
    newVectors->Delete();
    }

  if ( this->CopyNormals && (n = pd->GetNormals()) ) 
    {
    if ( sze > 0 ) newNormals = n->MakeObject(sze,ext);
    else newNormals = n->MakeObject(n->GetNumberOfNormals());
    this->SetNormals(newNormals);
    newNormals->Delete();
    }

  if ( this->CopyTCoords && (t = pd->GetTCoords()) ) 
    {
    if ( sze > 0 ) newTCoords = t->MakeObject(sze,t->GetDimension(),ext);
    else newTCoords = t->MakeObject(t->GetNumberOfTCoords(),t->GetDimension());
    this->SetTCoords(newTCoords);
    newTCoords->Delete();
    }

  if ( this->CopyTensors && (tens = pd->GetTensors()) ) 
    {
    if ( sze > 0 ) newTensors = tens->MakeObject(sze,tens->GetDimension(),ext);
    else newTensors = tens->MakeObject(tens->GetNumberOfTensors(),tens->GetDimension());
    this->SetTensors(newTensors);
    newTensors->Delete();
    }

  if ( this->CopyUserDefined && (ud = pd->GetUserDefined()) ) 
    {
    if ( sze > 0 ) newUserDefined = ud->MakeObject(sze,ext);
    else newUserDefined = ud->MakeObject(ud->GetNumberOfUserDefined());
    this->SetUserDefined(newUserDefined);
    newUserDefined->Delete();
    }
};


// do it this way because some compilers don't initialize file scope statics
static vtkFloatScalars *cellScalars;
static vtkFloatVectors *cellVectors;
static vtkFloatNormals *cellNormals;
static vtkFloatTCoords *cellTCoords;
static vtkFloatTensors *cellTensors;
static vtkUserDefined *cellUserDefined;
static vtkAPixmap *cellColors;

// Description:
// Initialize point interpolation.
void vtkPointData::InterpolateAllocate(vtkPointData* pd, int sze, int ext)
{
  // statics avoid constructor/destructor calls
  static vtkFloatScalars cellScalars_s(VTK_CELL_SIZE);
  static vtkFloatVectors cellVectors_s(VTK_CELL_SIZE);
  static vtkFloatNormals cellNormals_s(VTK_CELL_SIZE);
  static vtkFloatTCoords cellTCoords_s(VTK_CELL_SIZE,3);
  static vtkFloatTensors cellTensors_s(VTK_CELL_SIZE,3);
  static vtkUserDefined cellUserDefined_s(VTK_CELL_SIZE);
  static vtkAPixmap cellColors_s(VTK_CELL_SIZE);
  static int initialized = 0;
  
  if (!initialized)
    {
    cellScalars_s.ReferenceCountingOff();
    cellVectors_s.ReferenceCountingOff();
    cellNormals_s.ReferenceCountingOff();
    cellTCoords_s.ReferenceCountingOff();
    cellTensors_s.ReferenceCountingOff();
    cellUserDefined_s.ReferenceCountingOff();
    cellColors_s.ReferenceCountingOff();
    }
  
  cellScalars = &cellScalars_s;
  cellVectors = &cellVectors_s;
  cellNormals = &cellNormals_s;
  cellTCoords = &cellTCoords_s;
  cellTensors = &cellTensors_s;
  cellUserDefined = &cellUserDefined_s;
  cellColors = &cellColors_s;

  this->CopyAllocate(pd, sze, ext);

  if ( pd->TCoords )
    {
    cellTCoords->SetDimension(pd->TCoords->GetDimension());
    }

  if ( pd->Tensors )
    {
    cellTensors->SetDimension(pd->Tensors->GetDimension());
    }
}

// Description:
// Interpolate data from points and interpolation weights.
void vtkPointData::InterpolatePoint(vtkPointData *fromPd, int toId, vtkIdList *ptIds, float *weights)
{
  int i, j;
  float s, *pv, v[3], *pn, n[3], *ptc, tc[3];
  static vtkTensor tensor(3), *pt;
  void *ud;

  if ( fromPd->Scalars && this->Scalars && this->CopyScalars )
    {
    if ( this->Scalars->GetNumberOfValuesPerScalar() == 1 ) //single-valued scalar
      {
      fromPd->Scalars->GetScalars(*ptIds, *cellScalars);
      for (s=0.0, i=0; i < ptIds->GetNumberOfIds(); i++)
        {
        s += cellScalars->GetScalar(i) * weights[i];
        }
      this->Scalars->InsertScalar(toId,s);
      }
    else //color scalar
      {
      unsigned char rgb[4], *prgb;
      vtkColorScalars *to=(vtkColorScalars *)this->Scalars;
      vtkColorScalars *from=(vtkColorScalars *)fromPd->Scalars;

      from->GetColors(*ptIds, *cellColors);
      for (rgb[0]=rgb[1]=rgb[2]=rgb[3]=0, i=0; 
	   i < ptIds->GetNumberOfIds(); i++)
        {
        prgb = cellColors->GetColor(i);
        rgb[0] += (unsigned char) ((float)prgb[0]*weights[i]);
        rgb[1] += (unsigned char) ((float)prgb[1]*weights[i]);
        rgb[2] += (unsigned char) ((float)prgb[2]*weights[i]);
        rgb[3] += (unsigned char) ((float)prgb[3]*weights[i]);
        }
      to->InsertColor(toId,rgb);
      }
    }

  if ( fromPd->Vectors && this->Vectors && this->CopyVectors )
    {
    fromPd->Vectors->GetVectors(*ptIds, *cellVectors);
    for (v[0]=v[1]=v[2]=0.0, i=0; i < ptIds->GetNumberOfIds(); i++)
      {
      pv = cellVectors->GetVector(i);
      v[0] += pv[0]*weights[i];
      v[1] += pv[1]*weights[i];
      v[2] += pv[2]*weights[i];
      }
    this->Vectors->InsertVector(toId,v);
    }

  if ( fromPd->Normals && this->Normals && this->CopyNormals )
    {
    fromPd->Normals->GetNormals(*ptIds, *cellNormals);
    for (n[0]=n[1]=n[2]=0.0, i=0; i < ptIds->GetNumberOfIds(); i++)
      {
      pn = cellNormals->GetNormal(i);
      n[0] += pn[0]*weights[i];
      n[1] += pn[1]*weights[i];
      n[2] += pn[2]*weights[i];
      }
    this->Normals->InsertNormal(toId,n);
    }

  if ( fromPd->TCoords && this->TCoords && this->CopyTCoords )
    {
    fromPd->TCoords->GetTCoords(*ptIds, *cellTCoords);
    for (tc[0]=tc[1]=tc[2]=0.0, i=0; i < ptIds->GetNumberOfIds(); i++)
      {
      ptc = cellTCoords->GetTCoord(i);
      for (j=0; j<cellTCoords->GetDimension(); j++) tc[j] += ptc[0]*weights[i];
      }
    this->TCoords->InsertTCoord(toId,tc);
    }

  if ( fromPd->Tensors && this->Tensors && this->CopyTensors )
    {
    fromPd->Tensors->GetTensors(*ptIds, *cellTensors);
    tensor.Initialize();
    for (i=0; i < ptIds->GetNumberOfIds(); i++)
      {
      pt = cellTensors->GetTensor(i);
      for (j=0; j<cellTensors->GetDimension(); j++) 
        for (int k=0; k<cellTensors->GetDimension(); k++) 
          tensor.AddComponent(j,k,pt->GetComponent(j,k)*weights[i]);
      }
    this->Tensors->InsertTensor(toId,&tensor);
    }

  if ( fromPd->UserDefined && this->UserDefined && this->CopyUserDefined )
    {
    fromPd->UserDefined->GetUserDefined(*ptIds, *cellUserDefined);
    ud = cellUserDefined->Interpolate(weights);
    this->UserDefined->InsertUserDefined(toId,ud);
    }
}

void vtkPointData::NullPoint (int ptId)
{
  static float null[3] = {0.0, 0.0, 0.0};
  static unsigned char cnull[4] = {0, 0, 0, 1};
  static vtkTensor nullTensor;

  if ( this->Scalars )
    {
    if ( this->Scalars->GetNumberOfValuesPerScalar() == 1 ) //single-valued scalar
      {
      this->Scalars->InsertScalar(ptId, 0.0);
      }
    else //color scalar
      {
      vtkColorScalars *to=(vtkColorScalars *)this->Scalars;
      to->InsertColor(ptId,cnull);
      }
    }

  if ( this->Vectors )
    {
    this->Vectors->InsertVector(ptId,null);
    }

  if ( this->Normals )
    {
    this->Normals->InsertNormal(ptId,null);
    }

  if ( this->TCoords )
    {
    this->TCoords->InsertTCoord(ptId,null);
    }

  if ( this->Tensors )
    {
    this->Tensors->InsertTensor(ptId,&nullTensor);
    }

  if ( this->UserDefined )
    {
    this->UserDefined->InsertUserDefined(ptId,NULL);
    }

}

void vtkPointData::Squeeze()
{
  if ( this->Scalars ) this->Scalars->Squeeze();
  if ( this->Vectors ) this->Vectors->Squeeze();
  if ( this->Normals ) this->Normals->Squeeze();
  if ( this->TCoords ) this->TCoords->Squeeze();
  if ( this->Tensors ) this->Tensors->Squeeze();
  if ( this->UserDefined ) this->UserDefined->Squeeze();
}

// Description:
// Turn on copying of all data.
void vtkPointData::CopyAllOn()
{
  this->CopyScalarsOn();
  this->CopyVectorsOn();
  this->CopyNormalsOn();
  this->CopyTCoordsOn();
  this->CopyTensorsOn();
  this->CopyUserDefinedOn();
}

// Description:
// Turn off copying of all data.
void vtkPointData::CopyAllOff()
{
  this->CopyScalarsOff();
  this->CopyVectorsOff();
  this->CopyNormalsOff();
  this->CopyTCoordsOff();
  this->CopyTensorsOff();
  this->CopyUserDefinedOff();
}

void vtkPointData::PrintSelf(ostream& os, vtkIndent indent)
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

  if ( this->UserDefined )
    {
    os << indent << "User Defined:\n";
    this->UserDefined->PrintSelf(os,indent.GetNextIndent());
    }
  else
    {
    os << indent << "User Defined: (none)\n";
    }

  os << indent << "Copy Scalars: " << (this->CopyScalars ? "On\n" : "Off\n");
  os << indent << "Copy Vectors: " << (this->CopyVectors ? "On\n" : "Off\n");
  os << indent << "Copy Normals: " << (this->CopyNormals ? "On\n" : "Off\n");
  os << indent << "Copy Texture Coordinates: " << (this->CopyTCoords ? "On\n" : "Off\n");
  os << indent << "Copy Tensors: " << (this->CopyTensors ? "On\n" : "Off\n");
  os << indent << "Copy User Defined: " << (this->CopyUserDefined ? "On\n" : "Off\n");

}
