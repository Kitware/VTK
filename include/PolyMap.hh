//
// PolyMapper takes PolyData as input
//
#ifndef __vlPolyMapper_h
#define __vlPolyMapper_h

#include "Mapper.hh"
#include "PolyData.hh"
#include "Renderer.hh"

class vlPolyMapper : public vlMapper 
{
public:
  vlPolyMapper();
  ~vlPolyMapper();
  char *GetClassName() {return "vlPolyMapper";};
  void Render(vlRenderer *ren);
  virtual void SetInput(vlPolyData *in);
  virtual vlPolyData* GetInput();

protected:
  vlPolyData *Input;
  vlGeometryPrimitive *Verts;
  vlGeometryPrimitive *Lines;
  vlGeometryPrimitive *Polys;
  vlGeometryPrimitive *Strips;

};

#endif


