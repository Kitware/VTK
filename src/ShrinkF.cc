//
// Methods for shrink filter
//
#include "ShrinkF.h"

//
// Shrink cells towards their centroid
//
void ShrinkFilter::execute()
{
  IdList ptId(25);
  FloatPoints pt(25);
  int i, j, k;
  float center[3], *p;
//
// Initialize, check input
//
  if ( !input || input->numCells() < 1 )
  {
    cout << "No input for shrink filter\n";
    return;
  }


//
// Traverse all cells, obtaining node coordinates.  Compute "center" of cell,
// then create new vertices shrunk towards center.
//
  for (i=0; i<input->numCells(); i++)
  {
    // get the center of the cell
    cellPoints(i,ptId);
    if ( ptId.numIds() > 0 )
    {
      pointCoords(ptId, pt);
      for (center[0]=center[1]=center[2]=0.0, j=0; j<pt.numPoints(); j++)
      {
        p = pt[j];
        for (k=0; k<3; k++) center[k] += p[k];
      }
      for (k=0; k<3; k++) center[k] /= pt.numPoints();

      // Create new points and cells
      for (j=0; j<pt.numPoints(); j++)
      {
        

      }
      

    }
  }






}

