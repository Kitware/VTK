//
// DataSet methods
//
#include "PolyData.hh"
#include "PolyMap.hh"
//
// Initialize static member.  This member is used to simplify traversal of lists 
// of verts, lines, polygons, and triangle strips.  It basically "marks" empty lists
// so that the traveral method "GetNextCell" works properly.
//
vlCellArray *vlPolyData::Dummy = 0;

vlPolyData::vlPolyData ()
{
  this->Points = 0;

  this->Verts = 0;
  this->Lines = 0;
  this->Polys = 0;
  this->Strips = 0;

  if (!this->Dummy) this->Dummy = new vlCellArray;
}

vlPolyData::vlPolyData(const vlPolyData& pd)
{

  this->Points = pd.Points;
  if (this->Points) this->Points->Register((void *)this);

  this->Verts = pd.Verts;
  if (this->Verts) this->Verts->Register((void *)this);

  this->Lines = pd.Lines;
  if (this->Lines) this->Lines->Register((void *)this);

  this->Polys = pd.Polys;
  if (this->Polys) this->Polys->Register((void *)this);

  this->Strips = pd.Strips;
  if (this->Strips) this->Strips->Register((void *)this);
}

vlPolyData::~vlPolyData()
{
  vlPolyData::Initialize();
}

vlDataSet* vlPolyData::MakeObject()
{
  return new vlPolyData(*this);
}
int vlPolyData::CellDimension (int cellId)
{
  return 2;
}

void vlPolyData::CellPoints (int cellId, vlIdList& ptId)
{

}

void vlPolyData::GetPoints (vlIdList& ptId, vlFloatPoints& fp)
{

}

void vlPolyData::SetVerts (vlCellArray* v) 
{
  if ( v != this->Verts && v != this->Dummy )
    {
    if (this->Verts) this->Verts->UnRegister((void *)this);
    this->Verts = v;
    if (this->Verts) this->Verts->Register((void *)this);
    this->Modified();
    }
}
vlCellArray* vlPolyData::GetVerts()
{
  if ( !this->Verts ) return this->Dummy;
  else return this->Verts;
}

void vlPolyData::SetLines (vlCellArray* l) 
{
  if ( l != this->Lines && l != this->Dummy )
    {
    if (this->Lines) this->Lines->UnRegister((void *)this);
    this->Lines = l;
    if (this->Lines) this->Lines->Register((void *)this);
    this->Modified();
    }
}
vlCellArray* vlPolyData::GetLines()
{
  if ( !this->Lines ) return this->Dummy;
  else return this->Lines;
}

void vlPolyData::SetPolys (vlCellArray* p) 
{
  if ( p != this->Polys && p != this->Dummy )
    {
    if (this->Polys) this->Polys->UnRegister((void *)this);
    this->Polys = p;
    if (this->Polys) this->Polys->Register((void *)this);
    this->Modified();
    }
}
vlCellArray* vlPolyData::GetPolys()
{
  if ( !this->Polys ) return this->Dummy;
  else return this->Polys;
}

void vlPolyData::SetStrips (vlCellArray* s) 
{
  if ( s != this->Strips && s != this->Dummy )
    {
    if (this->Strips) this->Strips->UnRegister((void *)this);
    this->Strips = s;
    if (this->Strips) this->Strips->Register((void *)this);
    this->Modified();
    }
}
vlCellArray* vlPolyData::GetStrips()
{
  if ( !this->Strips ) return this->Dummy;
  else return this->Strips;
}

void vlPolyData::Initialize()
{
  vlDataSet::Initialize();

  if ( this->Points != 0 ) 
  {
    this->Points->UnRegister((void *)this);
    this->Points = 0;
  }

  if ( this->Verts != 0 ) 
  {
    this->Verts->UnRegister((void *)this);
    this->Verts = 0;
  }

  if ( this->Lines != 0 ) 
  {
    this->Lines->UnRegister((void *)this);
    this->Lines = 0;
  }

  if ( this->Polys != 0 ) 
  {
    this->Polys->UnRegister((void *)this);
    this->Polys = 0;
  }

  if ( this->Strips != 0 ) 
  {
    this->Strips->UnRegister((void *)this);
    this->Strips = 0;
  }
};

int vlPolyData::NumCells() 
{
  return NumVerts() + NumLines() + NumPolys() + NumStrips();
}

int vlPolyData::NumPoints() 
{
  return (this->Points ? this->Points->NumPoints() : 0);
}

int vlPolyData::NumVerts() 
{
  return (this->Verts ? this->Verts->GetNumCells() : 0);
}

int vlPolyData::NumLines() 
{
  return (this->Lines ? this->Lines->GetNumCells() : 0);
}

int vlPolyData::NumPolys() 
{
  return (this->Polys ? this->Polys->GetNumCells() : 0);
}

int vlPolyData::NumStrips() 
{
  return (this->Strips ? this->Strips->GetNumCells() : 0);
}

vlMapper *vlPolyData::MakeMapper()
{
  vlPolyMapper *mapper;

  if ( ! this->Mapper )
    {
    this->Mapper = mapper = new vlPolyMapper;
    this->Mapper->Register((void *)this);
    mapper->SetInput(this);
    }
  return this->Mapper;
}
void vlPolyData::ComputeBounds()
{
  int i;
  float *bounds;

  if ( this->Points )
    {
    bounds = this->Points->GetBounds();
    for (i=0; i<6; i++) this->Bounds[i] = bounds[i];
    this->ComputeTime.Modified();
    }
}
void vlPolyData::PrintSelf(ostream& os)
{
  os << "    Number Points: " << this->NumPoints() << "\n";
  os << "    Number Verts: " << this->NumVerts() << "\n";
  os << "    Number Lines: " << this->NumLines() << "\n";
  os << "    Number Polygons: " << this->NumPolys() << "\n";
  os << "    Number Triangle Strips: " << this->NumStrips() << "\n";

  vlDataSet::PrintSelf(os);
}
