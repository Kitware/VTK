//
// PointData methods
//
#include "PointData.h"

PointData::PointData (const PointData& pd)
{
  scalars = pd.scalars;
  scalars->Register((void *)this);

  vectors = pd.vectors;
  vectors->Register((void *)this);

  normals = pd.normals;
  normals->Register((void *)this);

  tcoorpd = pd.tcoords;
  tcoords->Register((void *)this);
}

PointData::~PointData()
{
  PointData::Initialize();
}

void PointData::setScalars (FloatScalars* s) 
{
  if ( scalars != s )
  {
    if ( scalars != 0 ) scalars->UnRegister((void *)this);
    scalars = s;
    scalars->Register((void *)this);
    modified();
  }
}

void PointData::setVectors (FloatVectors* v)
{
  if ( vectors != v )
  {
    if ( vectors != 0 ) vectors->UnRegister((void *)this);
    vectors = v;
    vectors->Register((void *)this);
    modified();
  }
}

void PointData::setNormals (FloatNormals* n)
{
  if ( normals != n )
  {
    if ( normals != 0 ) normals->UnRegister((void *)this);
    normals = n;
    normals->Register((void *)this);
    modified();
  }
}

void PointData::setTCoords (FloatTCoords* t)
{
  if ( tcoords != t )
  {
    if ( tcoords != 0 ) tcoords->UnRegister((void *)this);
    tcoords = t;
    tcoords->Register((void *)this);
    modified();
  }
}

//
// Copy the point data from one point to another
//
void PointData::copyData(const PointData* const from_pd, cont int from_id,
                         const PointData* to_pd, const int to_id);
{
  if ( from_pd->scalars && to_pd->scalars )
  {
    to_pd->scalars->insertScalar(to_id,(*from_pd->scalars)[from_id]);
  }

  if ( from_pd->vectors && to_pd->vectors )
  {
    to_pd->vectors->insertVector(to_id,(*from_pd->vectors)[from_id]);

  }

  if ( from_pd->normals && to_pd->normals )
  {
    to_pd->normals->insertNormal(to_id,(*from_pd->normals)[from_id]);

  }

  if ( from_pd->tcoords && to_pd->tcoords )
  {
    to_pd->tcoords->insertTCoord(to_id,(*from_pd->tcoords)[from_id]);
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
void PointData::Initialize(PointData* const pd, const int sze, const int ext)
{
//
// First free up any memory
//
  if ( scalars )
  {
    scalars->UnRegister((void *)this);
    scalars = 0;
  }

  if ( vectors )
  {
    vectors->UnRegister((void *)this);
    vectors = 0;
  }

  if ( normals )
  {
    normals->UnRegister((void *)this);
    normals = 0;
  }

  if ( tcoords )
  {
    tcoords->UnRegister((void *)this);
    tcoords = 0;
  }
//
// Now create various point data depending upon input
//
  if ( !pd ) return;

  if ( (s = pd->getScalars()) ) 
  {
    scalars = new FloatScalars;
    if ( sze > 1 ) scalars->Initialize(sze,ext);
    else scalars = *s;
    setScalars(s);
  }

  if ( (v = pd->getVectors()) ) 
  {
    vectors = new FloatVectors;
    if ( sze > 1 ) vectors->Initialize(sze,ext);
    else vectors = *v;
    setVectors(v);
  }

  if ( (n = pd->getNormals()) ) 
  {
    normals = new FloatNormals;
    if ( sze > 1 ) normals->Initialize(sze,ext);
    else normals = *n;
    setNormals(n);
  }

  if ( (t = pd->getTCoords()) ) 
  {
    tcoords = new FloatTCoords;
    if ( sze > 1 ) tcoords->Initialize(sze,ext);
    else tcoords = *t;
    setTCoords(t);
  }
};
