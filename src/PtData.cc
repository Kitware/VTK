//
// PointData methods
//
#include "PtData.hh"

vlPointData::vlPointData (const vlPointData& pd)
{
  this->Scalars = pd.Scalars;
  if (this->Scalars) this->Scalars->Register((void *)this);

  this->Vectors = pd.Vectors;
  if (this->Vectors) this->Vectors->Register((void *)this);

  this->Normals = pd.Normals;
  if (this->Normals) this->Normals->Register((void *)this);

  this->TCoords = pd.TCoords;
  if(this->TCoords) this->TCoords->Register((void *)this);
}

vlPointData::~vlPointData()
{
  vlPointData::Initialize();
}

vlPointData& vlPointData::operator=(vlPointData& pd)
{
  vlScalars *s;
  vlVectors *v;
  vlNormals *n;
  vlTCoords *t;

  if ( (s = pd.GetScalars()) ) 
    {
    this->SetScalars(s);
    }

  if ( (v = pd.GetVectors()) ) 
    {
    this->SetVectors(v);
    }

  if ( (n = pd.GetNormals()) ) 
    {
    this->SetNormals(n);
    }

  if ( (t = pd.GetTCoords()) ) 
    {
    this->SetTCoords(t);
    }

  return *this;
}

//
// Copy the point data from one point to another
//
void vlPointData::CopyData(vlPointData* from_pd, int from_id, int to_id)
{
  if ( from_pd->Scalars && this->Scalars )
    {
    this->Scalars->InsertScalar(to_id,from_pd->Scalars->GetScalar(from_id));
    }

  if ( from_pd->Vectors && this->Vectors )
    {
    this->Vectors->InsertVector(to_id,from_pd->Vectors->GetVector(from_id));
    }

  if ( from_pd->Normals && this->Normals )
    {
    this->Normals->InsertNormal(to_id,from_pd->Normals->GetNormal(from_id));
    }

  if ( from_pd->TCoords && this->TCoords )
    {
    this->TCoords->InsertTCoord(to_id,from_pd->TCoords->GetTCoord(from_id));
    }
}


void vlPointData::Initialize()
{
//
// Modify ourselves
//
  this->Modified();
//
// First free up any memory
//
  if ( this->Scalars )
    {
    this->Scalars->UnRegister((void *)this);
    this->Scalars = 0;
    }

  if ( this->Vectors )
    {
    this->Vectors->UnRegister((void *)this);
    this->Vectors = 0;
    }

  if ( this->Normals )
    {
    this->Normals->UnRegister((void *)this);
    this->Normals = 0;
    }

  if ( this->TCoords )
    {
    this->TCoords->UnRegister((void *)this);
    this->TCoords = 0;
    }
};

//
// Initializes point data for point-by-point copy operation.  If sze=0, then use 
// the input PointData to create (i.e., find initial size of) new objects; otherwise
// use the sze variable.
//
void vlPointData::CopyInitialize(vlPointData* pd, int sze, int ext)
{
  vlScalars *s, *newScalars;
  vlVectors *v, *newVectors;
  vlNormals *n, *newNormals;
  vlTCoords *t, *newTCoords;

  vlPointData::Initialize();
//
// Now create various point data depending upon input
//
  if ( !pd ) return;

  if ( (s = pd->GetScalars()) ) 
    {
    if ( sze > 0 ) newScalars = s->MakeObject(sze,ext);
    else newScalars = s->MakeObject(s->NumScalars());
    this->SetScalars(newScalars);
    }

  if ( (v = pd->GetVectors()) ) 
    {
    if ( sze > 0 ) newVectors = v->MakeObject(sze,ext);
    else newVectors = v->MakeObject(v->NumVectors());
    this->SetVectors(newVectors);
    }

  if ( (n = pd->GetNormals()) ) 
    {
    if ( sze > 0 ) newNormals = n->MakeObject(sze,ext);
    else newNormals = n->MakeObject(n->NumNormals());
    this->SetNormals(newNormals);
    }

  if ( (t = pd->GetTCoords()) ) 
    {
    if ( sze > 0 ) newTCoords = t->MakeObject(sze,t->GetDimension(),ext);
    else newTCoords = t->MakeObject(t->NumTCoords(),t->GetDimension());
    this->SetTCoords(newTCoords);
    }
};
