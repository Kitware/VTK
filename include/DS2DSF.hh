//
// DataSetToDataSetFilter takes any dataset as input and copies it through, 
// changing the point attributes along the way.
//
#ifndef __vlDataSetToDataSetFilter_h
#define __vlDataSetToDataSetFilter_h

#include "DataSetF.hh"
#include "DataSet.hh"

class vlDataSetToDataSetFilter : public vlDataSetFilter,  public vlDataSet 
{
public:
  vlDataSetToDataSetFilter();
  ~vlDataSetToDataSetFilter();
  char *GetClassName() {return "vlDataSetToDataSetFilter";};
  vlDataSet *CopySelf() {return this->DataSet->CopySelf();};
  int NumCells() {return this->DataSet->NumCells();}
  int NumPoints() {return this->DataSet->NumPoints();}
  int CellDimension(int cellId) {return this->DataSet->CellDimension(cellId);}
  void CellPoints(int cellId, vlIdList& ptId) 
    {this->DataSet->CellPoints(cellId, ptId);}
  void Initialize();
  vlFloatTriple& PointCoord(int i)
    {return this->DataSet->PointCoord(i);}
  void PointCoords(vlIdList& ptId, vlFloatPoints& fp)
    {this->DataSet->PointCoords(ptId,fp);}
  void ComputeBounds() {this->DataSet->ComputeBounds();};
  vlMapper *MakeMapper(vlDataSet *ds) 
    {return this->DataSet->MakeMapper(this->DataSet);};

  void Update();

private:
  vlDataSet *DataSet;

};

#endif


