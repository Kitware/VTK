//
// PolyMapper takes PolyData as input
//
#ifndef __vlPolyMapper_h
#define __vlPolyMapper_h

#include "Mapper.h"
#include "PolyData.h"
#include "Renderer.h"

class vlPolyMapper : public vlMapper {
public:
  vlPolyMapper();
  virtual ~vlPolyMapper();
  virtual void SetInput(vlPolyData *in);
  virtual vlPolyData* GetInput();
  virtual void Render(Renderer *ren);

protected:
  vlPolyData *Input;
  GeometryPrimitive *Verts;
  GeometryPrimitive *Lines;
  GeometryPrimitive *Polys;
  GeometryPrimitive *Strips;

};

#endif


