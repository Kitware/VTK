//
// PolyMapper takes PolyData as input
//
#ifndef PolyMapper_h
#define PolyMapper_h

#include "Params.h"
#include "Mapper.h"
#include "PolyData.h"
#include "Renderer.h"

class PolyMapper : public Mapper {
public:
  PolyMapper();
  ~PolyMapper();
  virtual void setInput(PolyData *in);
  virtual PolyData* getInput();
  virtual void Render(Renderer *ren);

protected:
  PolyData *input;
  GeometryPrimitive *verts;
  GeometryPrimitive *lines;
  GeometryPrimitive *polys;
  GeometryPrimitive *strips;

};

#endif


