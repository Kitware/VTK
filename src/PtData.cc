//
// PointData methods
//
#include "PtData.hh"

vlPointData::vlPointData (const vlPointData& pd)
{
  this->Scalars = pd.Scalars;
  this->Scalars->Register((void *)this);

  this->Vectors = pd.Vectors;
  this->Vectors->Register((void *)this);

  this->Normals = pd.Normals;
  this->Normals->Register((void *)this);

  this->TCoords = pd.TCoords;
  this->TCoords->Register((void *)this);
}

vlPointData::~vlPointData()
{
  vlPointData::Initialize();
}

void vlPointData::SetScalars (vlFloatScalars* s) 
{
  if ( this->Scalars != s )
    {
    if ( this->Scalars != 0 ) this->Scalars->UnRegister((void *)this);
    this->Scalars = s;
    this->Scalars->Register((void *)this);
    this->Modified();
    }
}

void vlPointData::SetVectors (vlFloatVectors* v)
{
  if ( this->Vectors != v )
    {
    if ( this->Vectors != 0 ) this->Vectors->UnRegister((void *)this);
    this->Vectors = v;
    this->Vectors->Register((void *)this);
    this->Modified();
    }
}

void vlPointData::SetNormals (vlFloatNormals* n)
{
  if ( this->Normals != n )
    {
    if ( this->Normals != 0 ) this->Normals->UnRegister((void *)this);
    this->Normals = n;
    this->Normals->Register((void *)this);
    this->Modified();
    }
}

void vlPointData::SetTCoords (vlFloatTCoords* t)
{
  if ( this->TCoords != t )
    {
    if ( this->TCoords != 0 ) this->TCoords->UnRegister((void *)this);
    this->TCoords = t;
    this->TCoords->Register((void *)this);
    this->Modified();
    }
}

//
// Copy the point data from one point to another
//
void vlPointData::CopyData(const vlPointData* const from_pd, const int from_id,
                         const vlPointData* to_pd, const int to_id)
{
  if ( from_pd->Scalars && to_pd->Scalars )
    {
    to_pd->Scalars->InsertScalar(to_id,(*from_pd->Scalars)[from_id]);
    }

  if ( from_pd->Vectors && to_pd->Vectors )
    {
    to_pd->Vectors->InsertVector(to_id,(*from_pd->Vectors)[from_id]);
    }

  if ( from_pd->Normals && to_pd->Normals )
    {
    to_pd->Normals->InsertNormal(to_id,(*from_pd->Normals)[from_id]);
    }

  if ( from_pd->TCoords && to_pd->TCoords )
    {
    to_pd->TCoords->InsertTCoord(to_id,(*from_pd->TCoords)[from_id]);
    }
}


//
// Initializes data depending upon input.  If pd=0 (default value), then data
// is deleted and pointers set to null values; if pd!=0 but sze=0 (default),
// then the point data is copied from the input pd.  If pd!=0 and sze!=0
// then the point data is sized according to sze. Note: only those components
// e.g. scalars, vectors, ... that are non-NULL in the input are created on
// output. are created in point data. 
//
void vlPointData::Initialize(vlPointData* const pd,const int sze,const int ext)
{
  vlFloatScalars *s;
  vlFloatVectors *v;
  vlFloatNormals *n;
  vlFloatTCoords *t;
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
//
// Now create various point data depending upon input
//
  if ( !pd ) return;

  if ( (s = pd->GetScalars()) ) 
    {
    this->Scalars = new vlFloatScalars;
    if ( sze > 1 ) this->Scalars->Initialize(sze,ext);
    else this->Scalars = s;
    this->SetScalars(s);
    }

  if ( (v = pd->GetVectors()) ) 
    {
    this->Vectors = new vlFloatVectors;
    if ( sze > 1 ) this->Vectors->Initialize(sze,ext);
    else this->Vectors = v;
    this->SetVectors(v);
    }

  if ( (n = pd->GetNormals()) ) 
    {
    this->Normals = new vlFloatNormals;
    if ( sze > 1 ) this->Normals->Initialize(sze,ext);
    else this->Normals = n;
    this->SetNormals(n);
    }

  if ( (t = pd->GetTCoords()) ) 
    {
    this->TCoords = new vlFloatTCoords;
    if ( sze > 1 ) this->TCoords->Initialize(sze,ext);
    else this->TCoords = t;
    this->SetTCoords(t);
    }
};
