/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkCellData.h"
#include "vtkCompositeDataDisplayAttributes.h"
#include "vtkCompositeDataSet.h"
#include "vtkCullerCollection.h"
#include "vtkDataObjectTreeIterator.h"
#include "vtkGlyph3DMapper.h"
#include "vtkHardwareSelector.h"
#include "vtkInformation.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkNew.h"
#include "vtkPlaneSource.h"
#include "vtkProperty.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSelection.h"
#include "vtkSelectionNode.h"
#include "vtkSmartPointer.h"
#include "vtkSphereSource.h"
#include "vtkUnsignedIntArray.h"

#include "vtkRegressionTestImage.h"
#include "vtkTestUtilities.h"

#include <functional>
#include <set>

template <typename T, typename U, typename V>
void prepareDisplayAttribute(
  T& expected, U attr, V mbds, std::function<std::pair<bool, bool>(int)> config)
{
  expected.clear();
  auto bit = mbds->NewTreeIterator();
  for (bit->InitTraversal(); !bit->IsDoneWithTraversal(); bit->GoToNextItem())
  {
    int ii = bit->GetCurrentFlatIndex();
    auto cfg = config(ii);
    bool visible = cfg.first;
    bool pickable = cfg.second;
    auto dataObj = bit->GetCurrentDataObject();
    if (visible && pickable)
    {
      auto pd = vtkPolyData::SafeDownCast(dataObj);
      if (pd)
      {
        auto cid = pd->GetCellData()->GetArray("vtkCompositeIndex");
        int idx = cid ? cid->GetTuple1(0) : ii;
        expected.insert(idx);
      }
    }
    attr->SetBlockVisibility(dataObj, visible);
    attr->SetBlockPickability(dataObj, pickable);
  }
  bit->Delete();
}

template <typename T>
void addCompositeIndex(T mbds, int& nextIndex)
{
  int nblk = static_cast<int>(mbds->GetNumberOfBlocks());
  for (int i = 0; i < nblk; ++i)
  {
    auto blk = mbds->GetBlock(i);
    if (blk->IsA("vtkCompositeDataSet"))
    {
      addCompositeIndex(vtkMultiBlockDataSet::SafeDownCast(blk), nextIndex);
    }
    else if (blk->IsA("vtkPolyData"))
    {
      auto pdata = vtkPolyData::SafeDownCast(blk);
      vtkIdType nc = pdata->GetNumberOfCells();
      auto cid = vtkSmartPointer<vtkUnsignedIntArray>::New();
      cid->SetName("vtkCompositeIndex");
      cid->SetNumberOfTuples(nc);
      cid->FillComponent(0, nextIndex);
      pdata->GetCellData()->AddArray(cid);
      ++nextIndex;
    }
  }
}

template <typename T, typename U>
int checkSelection(T seln, const U& expected, int& tt)
{
  std::cout << "Test " << tt << "\n";
  ++tt;
  int numNodes = seln->GetNumberOfNodes();
  U actual;
  for (int nn = 0; nn < numNodes; ++nn)
  {
    auto sn = seln->GetNode(nn);
    auto actor = vtkActor::SafeDownCast(sn->GetProperties()->Get(vtkSelectionNode::PROP()));
    if (actor)
    {
      auto ci = sn->GetProperties()->Get(vtkSelectionNode::COMPOSITE_INDEX());
      actual.insert(ci);
    }
  }

  std::cout << "  Expected:";
  for (auto ee : expected)
  {
    std::cout << " " << ee;
  }
  std::cout << "\n  Actual:";
  for (auto aa : actual)
  {
    std::cout << " " << aa;
  }
  std::cout << "\n";
  int result = (expected == actual ? 1 : 0);
  if (!result)
  {
    vtkGenericWarningMacro("Mismatch between expected selection and actual selection.");
  }
  return result;
}

int TestGlyph3DMapperPickability(int argc, char* argv[])
{
  auto rw = vtkSmartPointer<vtkRenderWindow>::New();
  auto ri = vtkSmartPointer<vtkRenderWindowInteractor>::New();
  auto rr = vtkSmartPointer<vtkRenderer>::New();
  auto ss = vtkSmartPointer<vtkSphereSource>::New();
  auto mp = vtkSmartPointer<vtkGlyph3DMapper>::New();
  auto ac = vtkSmartPointer<vtkActor>::New();
  auto mb = vtkSmartPointer<vtkMultiBlockDataSet>::New();
  auto da = vtkSmartPointer<vtkCompositeDataDisplayAttributes>::New();
  rw->AddRenderer(rr);
  rw->SetMultiSamples(0);
  rw->SetInteractor(ri);
  mp->SetBlockAttributes(da);

  vtkNew<vtkPlaneSource> plane;
  mb->SetNumberOfBlocks(4);
  for (int ii = 0; ii < static_cast<int>(mb->GetNumberOfBlocks()); ++ii)
  {
    double ll[2];
    ll[0] = -0.5 + 1.0 * (ii % 2);
    ll[1] = -0.5 + 1.0 * (ii / 2);
    plane->SetOrigin(ll[0], ll[1], ii);
    plane->SetPoint1(ll[0] + 1.0, ll[1], ii);
    plane->SetPoint2(ll[0], ll[1] + 1.0, ii);
    plane->Update();
    vtkNew<vtkPolyData> pblk;
    pblk->DeepCopy(plane->GetOutputDataObject(0));
    mb->SetBlock(ii, pblk.GetPointer());
  }

  mp->SetInputDataObject(0, mb.GetPointer());
  mp->SetSourceConnection(ss->GetOutputPort());
  ac->SetMapper(mp);
  rr->AddActor(ac);
  rw->SetSize(400, 400);
  rr->RemoveCuller(rr->GetCullers()->GetLastItem());
  rr->ResetCamera();
  rw->Render(); // get the window up

  double rgb[4][3] = { { .5, .5, .5 }, { 0., 1., 1. }, { 1., 1., 0. }, { 1., 0., 1. } };

  auto it = mb->NewIterator();
  int ii = 0;
  for (it->InitTraversal(); !it->IsDoneWithTraversal(); it->GoToNextItem())
  {
    auto dataObj = it->GetCurrentDataObject();
    da->SetBlockColor(dataObj, rgb[ii++]);
  }
  it->Delete();

  vtkNew<vtkHardwareSelector> hw;
  hw->SetArea(0, 0, 400, 400);
  hw->SetFieldAssociation(vtkDataObject::FIELD_ASSOCIATION_CELLS);
  hw->SetRenderer(rr);
  hw->SetProcessID(0);

  int testNum = 0;
  std::set<int> expected;

  // Nothing visible, but everything pickable.
  prepareDisplayAttribute(expected, da, mb, [](int) { return std::pair<bool, bool>(false, true); });
  mp->Modified();
  auto sel = hw->Select();
  int retVal = checkSelection(sel, expected, testNum);
  sel->Delete();

  // Everything visible, but nothing pickable.
  prepareDisplayAttribute(expected, da, mb, [](int) { return std::pair<bool, bool>(true, false); });
  mp->Modified();
  sel = hw->Select();
  retVal &= checkSelection(sel, expected, testNum);
  sel->Delete();

  // One block in every possible state.
  prepareDisplayAttribute(expected, da, mb, [](int nn) {
    --nn;
    return std::pair<bool, bool>(!!(nn / 2), !!(nn % 2));
  });
  mp->Modified();
  sel = hw->Select();
  retVal &= checkSelection(sel, expected, testNum);
  sel->Delete();

  // One block in every possible state (but different).
  prepareDisplayAttribute(expected, da, mb, [](int nn) {
    --nn;
    return std::pair<bool, bool>(!(nn / 2), !(nn % 2));
  });
  mp->Modified();
  sel = hw->Select();
  retVal &= checkSelection(sel, expected, testNum);
  sel->Delete();

  // Everything visible and pickable..
  prepareDisplayAttribute(expected, da, mb, [](int) { return std::pair<bool, bool>(true, true); });
  mp->Modified();
  rw->Render();
  sel = hw->Select();
  retVal &= checkSelection(sel, expected, testNum);
  sel->Delete();

  int retTestImage = vtkRegressionTestImage(rw);
  retVal &= retTestImage;
  if (retTestImage == vtkRegressionTester::DO_INTERACTOR)
  {
    ri->Start();
  }

  return !retVal;
}
