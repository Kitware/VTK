//
// PolyMapper takes PolyData as input
//
#ifndef __vlPolyMapper_h
#define __vlPolyMapper_h

#include "Mapper.h"
#include "PolyData.h"
#include "Renderer.h"

class vlPolyMapper : public vlMapper 
{
public:
  vlPolyMapper();
  ~vlPolyMapper();
  void Render(Renderer *ren);
  virtual void SetInput(vlPolyData *in);
  virtual vlPolyData* GetInput();

protected:
  vlPolyData *Input;
  GeometryPrimitive *Verts;
  GeometryPrimitive *Lines;
  GeometryPrimitive *Polys;
  GeometryPrimitive *Strips;

};

#endif


