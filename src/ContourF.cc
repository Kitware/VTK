#include "ContourF.hh"
#include "FScalars.hh"

//
// General contouring filter.  Handles arbitrary input.
//
void vlContourFilter::Execute()
{
  static int CASE_MASK[8] = {1,2,4,8,16,32,64,128};
  int cellId, i;
  int index;
  vlIdList cellPts(8);
  vlScalars *inScalars;
  vlFloatScalars cellScalars(8);

//
// Check that value is within scalar range
//

//
// Loop over all cells
//
  for (cellId=0; cellId<Input->NumCells(); cellId++)
    {
    Input->CellPoints(cellId,cellPts);
    inScalars->GetScalars(cellPts,cellScalars);

    // Build the case table
    for ( i=0, index = 0; i < cellPts.NumIds(); i++)
	if (cellScalars.GetScalar(i) >= this->Value)
            index |= CASE_MASK[i];

    // Use different primitives to generate


    } // for all cells

}
