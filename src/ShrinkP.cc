//
// Methods for shrink filter
//
#include "ShrinkP.hh"

//
// Shrink verts, lines, polygons, triangle strips towards their centroid.
// Polylines and triangle strips are broken into pieces.
//
void vlShrinkPolyData::Execute()
{
  int i, j, k;
  float center[3], *p;
  vlFloatPoints *in_pts;
  vlPointData *pd;
  vlCellArray *in_verts,*in_lines,*in_polys,*in_strips;
  int num_new_points, num_new_lines, num_new_polys, poly_alloc_size;
  int npts, *pts;
  vlFloatPoints *new_points;
  vlPointData *new_pointData;
  vlCellArray *new_verts, *new_lines, *new_polys;
  int new_ids[MAX_CELL_SIZE];
  float *p1, *p2, *p3, pt[3];
//
// Initialize
//
  this->Initialize();
  if ( !this->Input )
    {
    cerr << "No input available for ShrinkFilter\n";
    return;
    }

  in_pts = this->Input->GetPoints();
  pd = this->Input->GetPointData();

  in_verts = this->Input->GetVerts();
  in_lines = this->Input->GetLines();
  in_polys = this->Input->GetPolys();
  in_strips = this->Input->GetStrips();
//
// Count the number of new points and other primitives that 
// need to be created.
//
  num_new_points = this->Input->NumVerts();
  num_new_lines = 0;
  num_new_polys = 0;
  poly_alloc_size = 0;

  for (in_lines->InitTraversal(); in_lines->GetNextCell(npts,pts); )
    {
    num_new_points += (npts-1) * 2;
    num_new_lines += npts - 1;
    }
  for (in_polys->InitTraversal(); in_polys->GetNextCell(npts,pts); )
    {
    num_new_points += npts;
    num_new_polys++;
    poly_alloc_size += npts + 1;
    }
  for (in_strips->InitTraversal(); in_strips->GetNextCell(npts,pts); )
    {
    num_new_points += (npts-2) * 3;
    poly_alloc_size += (npts - 2) * 4;
    }
//
// Allocate
//
  new_points = new vlFloatPoints;
  new_points->Initialize(num_new_points);

  new_pointData = new vlPointData;
  new_pointData->Initialize(pd,num_new_points);

  new_verts = new vlCellArray;
  new_verts->Initialize(this->Input->NumVerts());

  new_lines = new vlCellArray;
  new_lines->Initialize(num_new_lines*3);
 
  new_polys = new vlCellArray;
  new_polys->Initialize(poly_alloc_size);

//
// Copy vertices (no shrinking necessary)
//
  for (in_verts->InitTraversal(); in_verts->GetNextCell(npts,pts); )
    {
    for (j=0; j<npts; j++)
      {
      new_ids[j] = new_points->InsertNextPoint((*in_pts)[pts[j]]);
      new_pointData->CopyData(pd,pts[j],new_pointData,new_ids[j]);
      }    
    new_verts->InsertNextCell(npts,new_ids);
    }
//
// Lines need to be shrunk, and if polyline, split into separate pieces
//
  for (in_lines->InitTraversal(); in_lines->GetNextCell(npts,pts); )
    {
    for (j=0; j<(npts-1); j++)
      {
      p1 = (*in_pts)[pts[j]];
      p2 = (*in_pts)[pts[j+1]];
      for (k=0; k<3; k++) center[k] = (p1[k] + p2[k]) / 2.0;

      for (k=0; k<3; k++)
        pt[k] = center[k] + this->ShrinkFactor*(p1[k] - center[k]);
      new_ids[0] = new_points->InsertNextPoint(pt);
      new_pointData->CopyData(pd,pts[j],new_pointData,new_ids[0]);

      for (k=0; k<3; k++)
        pt[k] = center[k] + this->ShrinkFactor*(p2[k] - center[k]);
      new_ids[1] = new_points->InsertNextPoint(pt);
      new_pointData->CopyData(pd,pts[j+1],new_pointData,new_ids[1]);

      new_lines->InsertNextCell(2,new_ids);
      }
    }
//
// Polygons need to be shrunk
//
  for (in_polys->InitTraversal(); in_polys->GetNextCell(npts,pts); )
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
        pt[k] = center[k] + this->ShrinkFactor*(p1[k] - center[k]);
      new_ids[j] = new_points->InsertNextPoint(pt);
      new_pointData->CopyData(pd,pts[j],new_pointData,new_ids[j]);
      }
    new_polys->InsertNextCell(npts,new_ids);
    }
//
// Triangle strips need to be shrunk and split into separate pieces.
//
  for (in_strips->InitTraversal(); in_strips->GetNextCell(npts,pts); )
    {
    for (j=0; j<(npts-2); j++)
      {
      p1 = (*in_pts)[pts[j]];
      p2 = (*in_pts)[pts[j+1]];
      p3 = (*in_pts)[pts[j+1]];
      for (k=0; k<3; k++) center[k] = (p1[k] + p2[k] + p3[k]) / 3.0;

      for (k=0; k<3; k++)
        pt[k] = center[k] + this->ShrinkFactor*(p1[k] - center[k]);
      new_ids[0] = new_points->InsertNextPoint(pt);
      new_pointData->CopyData(pd,pts[j],new_pointData,new_ids[0]);

      for (k=0; k<3; k++)
        pt[k] = center[k] + this->ShrinkFactor*(p2[k] - center[k]);
      new_ids[1] = new_points->InsertNextPoint(pt);
      new_pointData->CopyData(pd,pts[j+1],new_pointData,new_ids[1]);

      for (k=0; k<3; k++)
        pt[k] = center[k] + this->ShrinkFactor*(p3[k] - center[k]);
      new_ids[2] = new_points->InsertNextPoint(pt);
      new_pointData->CopyData(pd,pts[j+2],new_pointData,new_ids[2]);

      new_polys->InsertNextCell(3,new_ids);
      }
    }
//
// Update self
//
  this->SetPoints(new_points);
  this->SetPointData(new_pointData);

  this->SetVerts(new_verts);
  this->SetLines(new_lines);
  this->SetPolys(new_polys);
}

