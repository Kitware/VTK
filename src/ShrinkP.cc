//
// Methods for shrink filter
//
#include "ShrinkP.h"

void ShrinkPolyData::setShrinkFactor(float sf)
{
  if ( shrinkFactor != sf )
  {
    shrinkFactor = sf;
    modified();
  }
}

float ShrinkPolyData::getShrinkFactor()
{
  return shrinkFactor;
}

//
// Shrink verts, lines, polygons, triangle strips towards their centroid.
// Polylines and triangle strips are broken into pieces.
//
void ShrinkPolyData::execute()
{
  int i, j, k;
  float center[3], *p;
  FloatPoints *in_pts;
  PointData *pd;
  CellArray *in_verts,*in_lines,*in_polys,*in_strips;
  int num_new_points, num_new_lines, num_new_polys, poly_alloc_size;
  int npts, *pts;
  FloatPoints *new_points;
  PointData *new_pointData;
  CellArray *new_verts, *new_lines, *new_polys;
  int new_ids[MAX_CELL_SIZE];
  float *p1, *p2, *p3, pt[3];
//
// Initialize
//
  Initialize();
  if ( !input )
  {
    cout << "No input available for ShrinkFilter\n";
    return;
  }

  in_pts = input->getPoints();
  pd = input->getPointData();

  in_verts = input->getVerts();
  in_lines = input->getLines();
  in_polys = input->getPolys();
  in_strips = input->getStrips();
//
// Count the number of new points and other primitives that 
// need to be created.
//
  num_new_points = input->numVerts();
  num_new_lines = 0;
  num_new_polys = 0;
  poly_alloc_size = 0;

  for (in_lines->initTraversal(); in_lines->getNextCell(npts,pts); )
  {
    num_new_points += (npts-1) * 2;
    num_new_lines += npts - 1;
  }
  for (in_polys->initTraversal(); in_polys->getNextCell(npts,pts); )
  {
    num_new_points += npts;
    num_new_polys++;
    poly_alloc_size += npts + 1;
  }
  for (in_strips->initTraversal(); in_strips->getNextCell(npts,pts); )
  {
    num_new_points += (npts-2) * 3;
    poly_alloc_size += (npts - 2) * 4;
  }
//
// Allocate
//
  new_points = new FloatPoints;
  new_points->Initialize(num_new_points);

  new_pointData = new PointData;
  new_pointData->Initialize(pd,num_new_points);

  new_verts = new CellArray;
  new_verts->Initialize(input->numVerts());

  new_lines = new CellArray;
  new_lines->Initialize(num_new_lines*3);
 
  new_polys = new CellArray;
  new_polys->Initialize(poly_alloc_size);

//
// Copy vertices (no shrinking necessary)
//
  for (in_verts->initTraversal(); in_verts->getNextCell(npts,pts); )
  {
    for (j=0; j<npts; j++)
    {
      new_ids[j] = new_points->insertNextPoint((*in_pts)[pts[j]]);
      new_pointData->copyData(pd,pts[j],new_pointData,new_ids[j]);
    }    
    new_verts->insertNextCell(npts,new_ids);
  }
//
// Lines need to be shrunk, and if polyline, split into separate pieces
//
  for (in_lines->initTraversal(); in_lines->getNextCell(npts,pts); )
  {
    for (j=0; j<(npts-1); j++)
    {
      p1 = (*in_pts)[pts[j]];
      p2 = (*in_pts)[pts[j+1]];
      for (k=0; k<3; k++) center[k] = (p1[k] + p2[k]) / 2.0;

      for (k=0; k<3; k++)
        pt[k] = center[k] + shrinkFactor*(p1[k] - center[k]);
      new_ids[0] = new_points->insertNextPoint(pt);
      new_pointData->copyData(pd,pts[j],new_pointData,new_ids[0]);

      for (k=0; k<3; k++)
        pt[k] = center[k] + shrinkFactor*(p2[k] - center[k]);
      new_ids[1] = new_points->insertNextPoint(pt);
      new_pointData->copyData(pd,pts[j+1],new_pointData,new_ids[1]);

      new_lines->insertNextCell(2,new_ids);
    }
  }
//
// Polygons need to be shrunk
//
  for (in_polys->initTraversal(); in_polys->getNextCell(npts,pts); )
  {
    for (center[0]=center[1]=center[2]=0.0, j=0; j<npts; j++)
    {
      p1 = (*in_pts)[pts[j]];
      for (k=0; k<3; k++) center[k] += p1[k];
    }

    for (k=0; k<3; k++) center[k] /= npts;

    for (j=0; j<npts; j++)
    {
      p1 = (*in_pts)[pts[j]];
      for (k=0; k<3; k++)
        pt[k] = center[k] + shrinkFactor*(p1[k] - center[k]);
      new_ids[j] = new_points->insertNextPoint(pt);
      new_pointData->copyData(pd,pts[j],new_pointData,new_ids[j]);
    }
    new_polys->insertNextCell(npts,new_ids);
  }
//
// Triangle strips need to be shrunk and split into separate pieces.
//
  for (in_strips->initTraversal(); in_strips->getNextCell(npts,pts); )
  {
    for (j=0; j<(npts-2); j++)
    {
      p1 = (*in_pts)[pts[j]];
      p2 = (*in_pts)[pts[j+1]];
      p3 = (*in_pts)[pts[j+1]];
      for (k=0; k<3; k++) center[k] = (p1[k] + p2[k] + p3[k]) / 3.0;

      for (k=0; k<3; k++)
        pt[k] = center[k] + shrinkFactor*(p1[k] - center[k]);
      new_ids[0] = new_points->insertNextPoint(pt);
      new_pointData->copyData(pd,pts[j],new_pointData,new_ids[0]);

      for (k=0; k<3; k++)
        pt[k] = center[k] + shrinkFactor*(p2[k] - center[k]);
      new_ids[1] = new_points->insertNextPoint(pt);
      new_pointData->copyData(pd,pts[j+1],new_pointData,new_ids[1]);

      for (k=0; k<3; k++)
        pt[k] = center[k] + shrinkFactor*(p3[k] - center[k]);
      new_ids[2] = new_points->insertNextPoint(pt);
      new_pointData->copyData(pd,pts[j+2],new_pointData,new_ids[2]);

      new_polys->insertNextCell(3,new_ids);
    }
  }
//
// Update self
//
  setPoints(new_points);
  setPointData(new_pointData);

  setVerts(new_verts);
  setLines(new_lines);
  setPolys(new_polys);
}

