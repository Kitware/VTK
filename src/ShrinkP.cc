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
  vlFloatPoints *inPts;
  vlPointData *pd;
  vlCellArray *inVerts,*inLines,*inPolys,*inStrips;
  int numNewPts, numNewLines, numNewPolys, poly_alloc_size;
  int npts, *pts;
  vlFloatPoints *newPoints;
  vlCellArray *newVerts, *newLines, *newPolys;
  int newIds[MAX_CELL_SIZE];
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

  inPts = this->Input->GetPoints();
  pd = this->Input->GetPointData();

  inVerts = this->Input->GetVerts();
  inLines = this->Input->GetLines();
  inPolys = this->Input->GetPolys();
  inStrips = this->Input->GetStrips();
//
// Count the number of new points and other primitives that 
// need to be created.
//
  numNewPts = this->Input->NumVerts();
  numNewLines = 0;
  numNewPolys = 0;
  poly_alloc_size = 0;

  for (inLines->InitTraversal(); inLines->GetNextCell(npts,pts); )
    {
    numNewPts += (npts-1) * 2;
    numNewLines += npts - 1;
    }
  for (inPolys->InitTraversal(); inPolys->GetNextCell(npts,pts); )
    {
    numNewPts += npts;
    numNewPolys++;
    poly_alloc_size += npts + 1;
    }
  for (inStrips->InitTraversal(); inStrips->GetNextCell(npts,pts); )
    {
    numNewPts += (npts-2) * 3;
    poly_alloc_size += (npts - 2) * 4;
    }
//
// Allocate
//
  newPoints = new vlFloatPoints;
  newPoints->Initialize(numNewPts);

  newVerts = new vlCellArray;
  newVerts->Initialize(this->Input->NumVerts());

  newLines = new vlCellArray;
  newLines->Initialize(numNewLines*3);
 
  newPolys = new vlCellArray;
  newPolys->Initialize(poly_alloc_size);

//
// Copy vertices (no shrinking necessary)
//
  for (inVerts->InitTraversal(); inVerts->GetNextCell(npts,pts); )
    {
    for (j=0; j<npts; j++)
      {
      newIds[j] = newPoints->InsertNextPoint((*inPts)[pts[j]]);
      this->PointData.CopyData(pd,pts[j],newIds[j]);
      }    
    newVerts->InsertNextCell(npts,newIds);
    }
//
// Lines need to be shrunk, and if polyline, split into separate pieces
//
  for (inLines->InitTraversal(); inLines->GetNextCell(npts,pts); )
    {
    for (j=0; j<(npts-1); j++)
      {
      p1 = (*inPts)[pts[j]];
      p2 = (*inPts)[pts[j+1]];
      for (k=0; k<3; k++) center[k] = (p1[k] + p2[k]) / 2.0;

      for (k=0; k<3; k++)
        pt[k] = center[k] + this->ShrinkFactor*(p1[k] - center[k]);
      newIds[0] = newPoints->InsertNextPoint(pt);
      this->PointData.CopyData(pd,pts[j],newIds[0]);

      for (k=0; k<3; k++)
        pt[k] = center[k] + this->ShrinkFactor*(p2[k] - center[k]);
      newIds[1] = newPoints->InsertNextPoint(pt);
      this->PointData.CopyData(pd,pts[j+1],newIds[1]);

      newLines->InsertNextCell(2,newIds);
      }
    }
//
// Polygons need to be shrunk
//
  for (inPolys->InitTraversal(); inPolys->GetNextCell(npts,pts); )
    {
    for (center[0]=center[1]=center[2]=0.0, j=0; j<npts; j++)
      {
      p1 = (*inPts)[pts[j]];
      for (k=0; k<3; k++) center[k] += p1[k];
      }

    for (k=0; k<3; k++) center[k] /= npts;

    for (j=0; j<npts; j++)
      {
      p1 = (*inPts)[pts[j]];
      for (k=0; k<3; k++)
        pt[k] = center[k] + this->ShrinkFactor*(p1[k] - center[k]);
      newIds[j] = newPoints->InsertNextPoint(pt);
      this->PointData.CopyData(pd,pts[j],newIds[j]);
      }
    newPolys->InsertNextCell(npts,newIds);
    }
//
// Triangle strips need to be shrunk and split into separate pieces.
//
  for (inStrips->InitTraversal(); inStrips->GetNextCell(npts,pts); )
    {
    for (j=0; j<(npts-2); j++)
      {
      p1 = (*inPts)[pts[j]];
      p2 = (*inPts)[pts[j+1]];
      p3 = (*inPts)[pts[j+1]];
      for (k=0; k<3; k++) center[k] = (p1[k] + p2[k] + p3[k]) / 3.0;

      for (k=0; k<3; k++)
        pt[k] = center[k] + this->ShrinkFactor*(p1[k] - center[k]);
      newIds[0] = newPoints->InsertNextPoint(pt);
      this->PointData.CopyData(pd,pts[j],newIds[0]);

      for (k=0; k<3; k++)
        pt[k] = center[k] + this->ShrinkFactor*(p2[k] - center[k]);
      newIds[1] = newPoints->InsertNextPoint(pt);
      this->PointData.CopyData(pd,pts[j+1],newIds[1]);

      for (k=0; k<3; k++)
        pt[k] = center[k] + this->ShrinkFactor*(p3[k] - center[k]);
      newIds[2] = newPoints->InsertNextPoint(pt);
      this->PointData.CopyData(pd,pts[j+2],newIds[2]);

      newPolys->InsertNextCell(3,newIds);
      }
    }
//
// Update self
//
  this->SetPoints(newPoints);

  this->SetVerts(newVerts);
  this->SetLines(newLines);
  this->SetPolys(newPolys);
}

