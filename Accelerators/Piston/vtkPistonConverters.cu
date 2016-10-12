#include <thrust/copy.h>
#include <piston/piston_math.h>
#include <piston/choose_container.h>
#include <piston/image3d.h>
#include <piston/vtk_image3d.h>
#include "vtkCellArray.h"
#include "vtkFloatArray.h"
#include "vtkIdTypeArray.h"
#include "vtkImageData.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkPistonDataObject.h"
#include "vtkPistonDataWrangling.h"
#include "vtkPistonReference.h"

#include "vtkgl.h"

#include <iostream>

using namespace std;
using namespace piston;

namespace vtkpiston {

//-----------------------------------------------------------------------------
void DeleteData(vtkPistonReference *tr)
{
  if (tr == NULL || tr->data == NULL)
    {
    return;
    }
  switch(tr->type)
  {
  case VTK_IMAGE_DATA:
    {
    vtk_image3d<SPACE>*oldD =
      (vtk_image3d<SPACE>*)tr->data;
    delete oldD;
    }
    break;
  case VTK_POLY_DATA:
    {
    vtk_polydata *oldD = (vtk_polydata *)tr->data;
    if (oldD->points)
      {
      oldD->points->clear();
      }
    delete oldD->points;
    if (oldD->scalars)
      {
      oldD->scalars->clear();
      }
    delete oldD->scalars;
    if (oldD->normals)
      {
      oldD->normals->clear();
      }
    delete oldD->normals;
    delete oldD;
    }
    break;
  default:
    cerr << "I don't have a deallocator for " << tr->type << " yet." << endl;
  }
  tr->data = NULL;
  tr->type = -1;
}

//-----------------------------------------------------------------------------
void DeepCopy(vtkPistonReference *tr, vtkPistonReference *other)
{
  if (tr == NULL)
    {
    return;
    }
  DeleteData(tr);
  if (other == NULL)
     {
     return;
     }

  switch(other->type)
  {
  case VTK_IMAGE_DATA:
    {
    vtk_image3d<SPACE>*oldD =
      (vtk_image3d<SPACE>*)other->data;
    thrust::device_vector<float>*scalars = new thrust::device_vector<float>(oldD->NPoints);
    thrust::copy(oldD->point_data_begin(), oldD->point_data_end(), scalars->begin());
    int dims[3];
    dims[0] = oldD->dim0;
    dims[1] = oldD->dim1;
    dims[2] = oldD->dim2;
    vtk_image3d<SPACE> *newD = new vtk_image3d<SPACE>(
      dims, oldD->origin, oldD->spacing, oldD->extents, *scalars);
    tr->data = (void*)newD;
    }
    break;
  case VTK_POLY_DATA:
    {
    vtk_polydata *oldD = (vtk_polydata *)other->data;
    vtk_polydata *newD = new vtk_polydata;
    newD->nPoints = oldD->nPoints;
    newD->vertsPer = oldD->vertsPer;
    newD->points = new thrust::device_vector<float>(oldD->points->size());
    thrust::copy(oldD->points->begin(), oldD->points->end(), newD->points->begin());
    newD->scalars = new thrust::device_vector<float>(oldD->scalars->size());
    thrust::copy(oldD->scalars->begin(), oldD->scalars->end(), newD->scalars->begin());
    newD->normals = new thrust::device_vector<float>(oldD->normals->size());
    thrust::copy(oldD->normals->begin(), oldD->normals->end(), newD->normals->begin());
    tr->data = (void*)newD;
    }
    break;
  default:
    cerr << "I don't have a copy method for " << tr->type << " yet." << endl;
  }
  tr->type = other->type;
}

//-----------------------------------------------------------------------------
bool CheckDirty(vtkDataSet *ds, vtkPistonReference *tr)
{
  vtkMTimeType dstime = ds->GetMTime();
  if (dstime != tr->mtime)
    {
    tr->mtime = dstime;
    return true;
    }
  return false;
}

//-----------------------------------------------------------------------------
vtkFloatArray *makeScalars(thrust::host_vector<float> *D)
{
  //copy from thrust to C
  int nPoints = D->size();
  float *raw_ptr = thrust::raw_pointer_cast(&*(D->begin()));
  float *toArray = new float[nPoints];
  memcpy(toArray, raw_ptr, nPoints*sizeof(float));

  //wrap result in vtkArray container
  vtkFloatArray *outfloats = vtkFloatArray::New();
  outfloats->SetNumberOfComponents(1);
  outfloats->SetArray(toArray, nPoints, 0); //0 let vtkArray delete[] toArray
  return outfloats;
}

//-----------------------------------------------------------------------------
vtkFloatArray *makeNormals(thrust::host_vector<float> *D)
{
  //copy from thrust to C
  int nPoints = D->size()/3;
  float *raw_ptr = thrust::raw_pointer_cast(&*(D->begin()));
  float *toArray = new float[nPoints*3];
  memcpy(toArray, raw_ptr, 3*nPoints*sizeof(float));

  //wrap result in vtkArray container
  vtkFloatArray *outfloats = vtkFloatArray::New();
  outfloats->SetNumberOfComponents(3);
  outfloats->SetName("Normals");
  outfloats->SetArray(toArray, nPoints*3, 0);//0 lets vtkArray delete[] toArray
  return outfloats;
}

//-----------------------------------------------------------------------------
int QueryNumVerts(vtkPistonDataObject *id)
{
  vtkPistonReference *tr = id->GetReference();
  if (tr->type != VTK_POLY_DATA || tr->data == NULL)
    {
    //type mismatch, don't bother trying
    return 0;
    }
  vtk_polydata *pD = (vtk_polydata *)tr->data;
  return pD->nPoints;
}

//-----------------------------------------------------------------------------
int QueryVertsPer(vtkPistonDataObject *id)
{
  vtkPistonReference *tr = id->GetReference();
  if (tr->type != VTK_POLY_DATA || tr->data == NULL)
    {
    //type mismatch, don't bother trying
    return 0;
    }
  vtk_polydata *pD = (vtk_polydata *)tr->data;
  return pD->vertsPer;
}

//-----------------------------------------------------------------------------
void CopyToGPU(vtkImageData *id, vtkPistonDataObject *od)
{
  vtkPistonReference *tr = od->GetReference();
  if (CheckDirty(id, tr))
    {
    DeleteData(tr);
    vtk_image3d<SPACE> *newD =
      new vtk_image3d<SPACE>(id);
    tr->data = (void*)newD;
    if(id->GetPointData() && id->GetPointData()->GetScalars())
      {
      od->SetScalarsArrayName(id->GetPointData()->GetScalars()->GetName());
      }
    }
  tr->type = VTK_IMAGE_DATA;
}

//-----------------------------------------------------------------------------
void CopyToGPU(vtkPolyData *id, vtkPistonDataObject *od)
{
  vtkPistonReference *tr = od->GetReference();
  if (CheckDirty(id, tr))
    {
    DeleteData(tr);

    vtk_polydata *newD = new vtk_polydata;
    tr->data = (void*)newD;

    int nPoints = id->GetNumberOfPoints();
    newD->nPoints = nPoints;
    //cerr << nPoints << endl;

    thrust::host_vector<float> hG(nPoints*3);
    for (vtkIdType i = 0; i < nPoints; i++)
      {
      double *next = id->GetPoint(i);
      hG[i*3+0] = (float)next[0];
      hG[i*3+1] = (float)next[1];
      hG[i*3+2] = (float)next[2];
      }
    thrust::device_vector<float> *dG =
      new thrust::device_vector<float>(nPoints*3);
    *dG = hG;
    newD->points = dG;

    newD->vertsPer = 3;

    vtkFloatArray *inscalars = vtkFloatArray::SafeDownCast(
      id->GetPointData()->GetScalars()
      );
    if (inscalars)
      {
      thrust::host_vector<float> hA(nPoints);
      for (vtkIdType i = 0; i < nPoints; i++)
        {
        double *next = inscalars->GetTuple(i);
        hA[i] = next[0];
        }
      thrust::device_vector<float> *dA =
        new thrust::device_vector<float>(nPoints);
      *dA = hA;
      newD->scalars = dA;
      od->SetScalarsArrayName(inscalars->GetName());
      }
    else
      {
      newD->scalars = NULL;
      }

    vtkFloatArray *innormals = vtkFloatArray::SafeDownCast(
      id->GetPointData()->GetNormals()
      );
    if (!innormals)
    {
      innormals = vtkFloatArray::SafeDownCast(
        id->GetPointData()->GetArray("Normals"));
    }
    if (innormals && innormals->GetNumberOfTuples())
      {
      thrust::host_vector<float> hA(nPoints*3);
      for (vtkIdType i = 0; i < nPoints; i++)
        {
        double *next = innormals->GetTuple(i);
        hA[i*3+0] = (float)next[0];
        hA[i*3+1] = (float)next[1];
        hA[i*3+2] = (float)next[2];
        }
      thrust::device_vector<float> *dA =
        new thrust::device_vector<float>(nPoints*3);
      *dA = hA;
      newD->normals = dA;
      }
    else
      {
      newD->normals = NULL;
      }
    }
  tr->type = VTK_POLY_DATA;

}

//-----------------------------------------------------------------------------
void CopyFromGPU(vtkPistonDataObject *id, vtkImageData *od)
{
  vtkPistonReference *tr = id->GetReference();
  if (tr->type != VTK_IMAGE_DATA || tr->data == NULL)
    {
    //type mismatch, don't bother trying
    return;
    }
  if (!CheckDirty(od, tr))
    {
    //it hasn't changed, don't recompute
    return;
    }
  vtk_image3d<SPACE>*oldD =
    (vtk_image3d<SPACE>*)tr->data;

  //geometry/topology
  od->SetExtent(0, oldD->dim0-1, 0, oldD->dim1-1, 0, oldD->dim2-1);
  od->SetOrigin(id->GetOrigin());
  od->SetSpacing(id->GetSpacing());

  //attributes
  int nPoints = oldD->NPoints;
  thrust::host_vector<float> D(nPoints);
  thrust::copy(oldD->point_data_begin(), oldD->point_data_end(), D.begin());
  //assign that to the output dataset
  vtkFloatArray *outFloats = makeScalars(&D);
  outFloats->SetName(id->GetScalarsArrayName());
  od->GetPointData()->SetScalars(outFloats);
  outFloats->Delete();
}

//-----------------------------------------------------------------------------
void CopyFromGPU(vtkPistonDataObject *id, vtkPolyData *od)
{
  vtkPistonReference *tr = id->GetReference();
  if (tr->type != VTK_POLY_DATA || tr->data == NULL)
    {
    //type mismatch, don't bother trying
    return;
    }
  if (!CheckDirty(od,tr))
    {
    //it hasn't changed, don't recompute
    return;
    }

  vtk_polydata *pD = (vtk_polydata *)tr->data;
  int nPoints = pD->nPoints;

  //geometry
  vtkPoints *points = vtkPoints::New();
  od->SetPoints(points);
  points->Delete();
  points->SetDataTypeToFloat();
  points->SetNumberOfPoints(nPoints);
  thrust::host_vector<float> G(nPoints*3);
  thrust::copy(pD->points->begin(), pD->points->end(), G.begin());
  float *raw_ptr = thrust::raw_pointer_cast(&G[0]);
  float *toPoints = (float*)points->GetVoidPointer(0);
  memcpy(toPoints, raw_ptr, nPoints*3*sizeof(float));

  //topology
  int vertsPer = pD->vertsPer;
  int nCells = nPoints/vertsPer;
  od->Allocate(nCells);
  vtkCellArray *cells = od->GetPolys();
  vtkIdTypeArray *cl = vtkIdTypeArray::New();
  cells->SetCells(nCells, cl);
  cl->Delete();
  cl->SetNumberOfValues(nCells*(vertsPer+1));
  for (int i = 0; i < nCells; i++)
    {
    cl->SetValue(i*(vertsPer+1)+0,vertsPer);
    for (int j = 0; j < vertsPer; j++)
      {
      cl->SetValue(i*(vertsPer+1)+j+1,i*vertsPer+j);
      }
    }

  //attributes
  //scalars
  if (pD->scalars)
    {
    thrust::host_vector<float> V(nPoints);
    thrust::copy(pD->scalars->begin(), pD->scalars->end(), V.begin());
    //assign that to the output dataset
    vtkFloatArray *outScalars = makeScalars(&V);
    outScalars->SetName(id->GetScalarsArrayName());
    od->GetPointData()->SetScalars(outScalars);
    outScalars->Delete();
    }
  //normals
  if (pD->normals)
    {
    thrust::host_vector<float> N(nPoints*3);
    thrust::copy(pD->normals->begin(), pD->normals->end(), N.begin());
    //assign that to the output dataset
    vtkFloatArray *outNormals = makeNormals(&N);
    od->GetPointData()->SetNormals(outNormals);
    outNormals->Delete();
    }
}

} //namespace
