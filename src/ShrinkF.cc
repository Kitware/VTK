//
// Methods for shrink filter
//
#include "ShrinkF.h"

//
// Shrink cells towards their centroid
//
void vlShrinkFilter::Execute()
{
  vlIdList ptId(25);
  vlFloatPoints pt(25);
  int i, j, k;
  float center[3], *p;


//
// Traverse all cells, obtaining node coordinates.  Compute "center" of cell,
// then create new vertices shrunk towards center.
//
  for (i=0; i<this->Input->NumCells(); i++)
    {
    // get the center of the cell
    this->CellPoints(i,ptId);
    if ( ptId.NumIds() > 0 )
      {
      this->PointCoords(ptId, pt);
      for (center[0]=center[1]=center[2]=0.0, j=0; j<pt.NumPoints(); j++)
        {
        p = pt[j];
        for (k=0; k<3; k++) center[k] += p[k];
        }
      for (k=0; k<3; k++) center[k] /= pt.NumPoints();

      // Create new points and cells
      for (j=0; j<pt.NumPoints(); j++)
        {
        

        }
      }
    }
}

