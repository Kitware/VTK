//
// DataSet methods
//
#include "PolyData.h"

// Initialize static member
CellArray PolyData::dummy;

PolyData::PolyData ()
{
  points = 0;

  verts = 0;
  lines = 0;
  polys = 0;
  strips = 0;
}

PolyData::PolyData(const PolyData& pd)
{

  points = pd.points;
  points->Register((void *)this);

  verts = pd.verts;
  verts->Register((void *)this);

  lines = pd.lines;
  lines->Register((void *)this);

  polys = pd.polys;
  polys->Register((void *)this);

  strips = pd.strips;
  strips->Register((void *)this);
}

PolyData::~PolyData()
{
  PolyData::Initialize();
}

int PolyData::cellDimension (int cellId)
{
  return 2;
}

void PolyData::cellPoints (int cellId, IdList& ptId)
{

}

void PolyData::pointCoords (IdList& ptId, FloatPoints& fp)
{

}

void PolyData::setPoints (FloatPoints* pts) 
{
  if ( points != pts )
  {
    if ( points != 0 ) points->UnRegister((void *)this);
    points = pts;
    points->Register((void *)this);
    modified();
  }
}
FloatPoints* PolyData::getPoints()
{
  return points;
}

void PolyData::setVerts (CellArray* v) 
{
  if ( verts != v && verts != &dummy )
  {
    if ( verts != 0 ) verts->UnRegister((void *)this);
    verts = v;
    verts->Register((void *)this);
    modified();
  }
}
CellArray* PolyData::getVerts()
{
  if ( !verts ) return &dummy;
  else return verts;
}

void PolyData::setLines (CellArray* l) 
{
  if ( lines != l && lines != &dummy )
  {
    if ( lines != 0 ) lines->UnRegister((void *)this);
    lines = l;
    lines->Register((void *)this);
    modified();
  }
}
CellArray* PolyData::getLines()
{
  if ( !lines ) return &dummy;
  else return lines;
}

void PolyData::setPolys (CellArray* p) 
{
  if ( polys != p && polys != &dummy )
  {
    if ( polys != 0 ) polys->UnRegister((void *)this);
    polys = p;
    polys->Register((void *)this);
    modified();
  }
}
CellArray* PolyData::getPolys()
{
  if ( !polys ) return &dummy;
  else return polys;
}

void PolyData::setStrips (CellArray* s) 
{
  if ( strips != s && strips != &dummy )
  {
    if ( strips != 0 ) strips->UnRegister((void *)this);
    strips = s;
    strips->Register((void *)this);
    modified();
  }
}
CellArray* PolyData::getStrips()
{
  if ( !strips ) return &dummy;
  else return strips;
}

void PolyData::Initialize()
{
  if ( points != 0 ) 
  {
    points->UnRegister((void *)this);
    points = 0;
  }

  if ( verts != 0 ) 
  {
    verts->UnRegister((void *)this);
    verts = 0;
  }

  if ( lines != 0 ) 
  {
    lines->UnRegister((void *)this);
    lines = 0;
  }

  if ( polys != 0 ) 
  {
    polys->UnRegister((void *)this);
    polys = 0;
  }

  if ( strips != 0 ) 
  {
    strips->UnRegister((void *)this);
    strips = 0;
  }

  DataSet::Initialize();
};
