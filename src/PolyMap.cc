//
// Methods for polygon mapper
//
#include "PolyMap.h"

PolyMapper::PolyMapper()
{
  input = 0;
  verts = 0;
  lines = 0;
  polys = 0;
  strips = 0;
}

PolyMapper::~PolyMapper()
{
  if ( input != 0 )
  {
    input->UnRegister((void *)this);
  }
}

void PolyMapper::setInput(PolyData *in)
{
  if (in != input )
  {
    input = in;
    input->Register((void *)this);
    modified();
  }
}
PolyData* PolyMapper::getInput()
{
    return input;
}

//
// Receives from Actor -> maps data to primitives
//
void PolyMapper::Render(Renderer *ren)
{
  PointData *pd;
  RGBArray *colors;
  FloatScalars *scalars;
  int i;
  char forceBuild = 0;
//
// make sure that we've been properly initialized
//
  if ( ! input ) 
    return;
  else
    input->update();

  if ( ! lut )
  {
    lut = new LookupTable;
    lut->build();
    forceBuild = 1;
  }

  if ( ! polys )
  {
    forceBuild = 1;
//    verts = ren->GetPrimitive("points");
//    lines = ren->GetPrimitive("lines");
    polys = ren->GetPrimitive("polygons");
//    strips = ren->GetPrimitive("triangle_strips");
  }
//
// create colors
//
  if ( scalarsVisible && (pd=input->getPointData()) && 
  (scalars=pd->getScalars()) )
  {
    colors = new RGBArray;
    colors->Initialize (input->numPoints());

    for (i=0; i<input->numPoints(); i++)
    {
      (*colors)[i] = lut->mapValue((*scalars)[i]);
    }
  }
  else
  {
    colors = 0;
  }
//
// Now send data down to primitives and draw it
//
  if ( forceBuild || input->getMtime() > buildTime.getMtime() || 
  lut->getMtime() > buildTime.getMtime() )
  {
//      verts->Build(input,colors);
//      lines->Build(input,colors);
      polys->Build(input,colors);
//      strips->Build(input,colors);
      buildTime.modified();
  }

//  verts->Draw(ren);
//  lines->Draw(ren);
  polys->Draw(ren);
//  strips->Draw(ren);

}

