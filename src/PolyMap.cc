//
// Methods for polygon mapper
//
#include "PolyMap.hh"

vlPolyMapper::vlPolyMapper()
{
  this->Input = 0;
  this->Verts = 0;
  this->Lines = 0;
  this->Polys = 0;
  this->Strips = 0;
  this->VertsVisibility = 1;
  this->LinesVisibility = 1;
  this->PolysVisibility = 1;
  this->StripsVisibility = 1;
}

vlPolyMapper::~vlPolyMapper()
{
  if ( this->Input != 0 )
    {
    this->Input->UnRegister((void *)this);
    }
}

void vlPolyMapper::SetInput(vlPolyData *in)
{
  if (in != this->Input )
    {
    this->Input = in;
    this->Input->Register((void *)this);
    this->Modified();
    }
}
vlPolyData* vlPolyMapper::GetInput()
{
  return this->Input;
}

//
// Receives from Actor -> maps data to primitives
//
void vlPolyMapper::Render(vlRenderer *ren)
{
  vlPointData *pd;
  vlRGBArray *colors;
  vlFloatScalars *scalars;
  int i;
  char forceBuild = 0;
//
// make sure that we've been properly initialized
//
  if ( ! this->Input ) 
    return;
  else
    this->Input->Update();

  if ( ! this->Lut )
    {
    this->Lut = new vlLookupTable;
    this->Lut->Build();
    forceBuild = 1;
    }

  if ( ! this->Polys )
    {
    forceBuild = 1;
//    this->Verts = ren->GetPrimitive("points");
//    this->Lines = ren->GetPrimitive("lines");
    this->Polys = ren->GetPrimitive("polygons");
//    this->Strips = ren->GetPrimitive("triangle_strips");
    }
//
// create colors
//
  if ( this->ScalarsVisible && (pd=this->Input->GetPointData()) && 
  (scalars=pd->GetScalars()) )
    {
    colors = new vlRGBArray;
    colors->Initialize (this->Input->NumPoints());

    for (i=0; i<this->Input->NumPoints(); i++)
      {
      (*colors)[i] = this->Lut->MapValue((*scalars)[i]);
      }
    }
  else
    {
    colors = 0;
    }
//
// Now send data down to primitives and draw it
//
  if ( forceBuild || this->Input->Mtime > this->BuildTime || 
  this->Lut->Mtime > this->BuildTime )
    {
//      this->Verts->Build(this->Input,colors);
//      this->Lines->Build(this->Input,colors);
      this->Polys->Build(this->Input,colors);
//      this->Strips->Build(this->Input,colors);
      this->BuildTime.Modified();
    }

//  if ( this->VertsVisibility ) this->Verts->Draw(ren);
//  if ( this->LinesVisibility ) this->Lines->Draw(ren);
  if ( this->PolysVisibility ) this->Polys->Draw(ren);
//  if ( this->StripsVisibility ) this->Strips->Draw(ren);

}

