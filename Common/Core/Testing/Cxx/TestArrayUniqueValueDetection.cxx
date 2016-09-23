#include "vtkAbstractArray.h"
#include "vtkDoubleArray.h"
#include "vtkIntArray.h"
#include "vtkNew.h"
#include "vtkStringArray.h"
#include "vtkVariantArray.h"

static double testTuplesDouble[] = {
  // A simple test where every component *and* the
  // tuples as a whole behave discretely.
  0., 0., 1.,
  1., 0., 3.,
  0., 2., 7.,
  0., 0., 9.,
  1., 0., 1.,
  0., 2., 8.,
};
// Bounds on the number of unique values that should be identified.
// For larger data, the number may not be precise
// as we sample subsets of the data.
static int numUniqueDouble[] = {
  2, 2,
  2, 2,
  5, 5,
  6, 6,
};

static int testTuplesInt[] = {
  /*
   These next tuples repeat 16 coordinate values but are frequently not
   identical in both components to other tuples. Thus the components
   should have 16 discrete values reported, but the vectors as
   a whole should not since there are 43 distinct tuples.
   Furthermore, the array is sampled as opposed to exhastively enumerated
   and so only 15 unique values are detected for component 1.
   */
  16,  1,
   8, 14,
  10,  3,
  11,  4,
   2, 13,
   7, 12,
   6,  5,
  15,  9,
  15,  6,
   9,  7,
  11, 16,
   1,  5,
   2,  3,
  13, 12,
   4,  8,
  14, 10,
   4, 14,
  11,  9,
   7,  3,
   8,  2,
  12, 13,
   1,  6,
  15, 10,
  16,  5,
   4, 10,
  12,  3,
   5,  8,
  13,  1,
  14, 11,
   2,  6,
  15,  9,
   7, 16,
   1,  2,
   5,  3,
  16, 13,
  15,  9,
  11, 12,
   7, 14,
   8, 10,
   4,  6,
   8, 13,
  16, 14,
  15,  2,
  11,  1,
   3, 10,
   4,  6,
   7, 12,
   5,  9,
};
static int numUniqueInt[] = {
  1, 16,
  1, 16,
  0, 0,
};

static int testTuplesInt2[] = {
  // There are no repeats in this set.
   1,  2,  3,  4,  5,  6,  7,  8,  9, 10,
  11, 12, 13, 14, 15, 16, 17, 18, 19, 20,
  21, 22, 23, 24, 25, 26, 27, 28, 29, 30,
  31, 32, 33,
};
static int numUniqueInt2[] = {
  0, 0,
};

static vtkStdString testTuplesString[] = {
  /*
   To test the log(N) sampling strategy, we must
   have a large number of entries in the array.

   The 4 words marked "not detected" below are placed to leave them
   intentionally undetected on at least one platform.
   The vtkObject ModificationTime counter is used as a
   seed for the random number generator and its count
   will vary across platforms and compile-time options,
   so we accept anywhere from 4 to 8 values being detected
   when subsampling. This data is used twice: once forcing all
   tuples to be sampled and once using subsampling.
   */
  "Eeny", "Meeny", "Miny", "Mo", "Miny",
  "Eeny", "Meeny", "Miny", "Mo", "Miny",
  "Eeny", "Meeny", "Miny", "Mo", "Miny",
  "Eeny", "Meeny", "Miny", "Mo", "Miny",
  "Skeenie", "Beeny", "Piny", "Po", "Po", // not detected.
  "Eeny", "Meeny", "Miny", "Mo", "Miny",
  "Eeny", "Meeny", "Miny", "Mo", "Miny",
  "Eeny", "Meeny", "Miny", "Mo", "Miny",
  "Eeny", "Meeny", "Miny", "Mo", "Miny",
  "Eeny", "Meeny", "Miny", "Mo", "Miny",
  "Eeny", "Meeny", "Miny", "Mo", "Miny",
  "Eeny", "Meeny", "Miny", "Mo", "Miny",
  "Eeny", "Meeny", "Miny", "Mo", "Miny",
  "Eeny", "Meeny", "Miny", "Mo", "Miny",
  "Eeny", "Meeny", "Miny", "Mo", "Miny",
  "Eeny", "Meeny", "Miny", "Mo", "Miny",
  "Eeny", "Meeny", "Miny", "Mo", "Miny",
  "Eeny", "Meeny", "Miny", "Mo", "Miny",
  "Eeny", "Meeny", "Miny", "Mo", "Miny",
  "Eeny", "Meeny", "Miny", "Mo", "Miny",
  "Eeny", "Meeny", "Miny", "Mo", "Miny",
  "Eeny", "Meeny", "Miny", "Mo", "Miny",
  "Eeny", "Meeny", "Miny", "Mo", "Miny",
  "Eeny", "Meeny", "Miny", "Mo", "Miny",
  "Eeny", "Meeny", "Miny", "Mo", "Miny",
  "Eeny", "Meeny", "Miny", "Mo", "Miny",
  "Eeny", "Meeny", "Miny", "Mo", "Miny",
  "Eeny", "Meeny", "Miny", "Mo", "Miny",
  "Eeny", "Meeny", "Miny", "Mo", "Miny",
  "Eeny", "Meeny", "Miny", "Mo", "Miny",
  "Eeny", "Meeny", "Miny", "Mo", "Miny",
  "Eeny", "Meeny", "Miny", "Mo", "Miny",
  "Eeny", "Meeny", "Miny", "Mo", "Miny",
  "Eeny", "Meeny", "Miny", "Mo", "Miny",
  "Eeny", "Meeny", "Miny", "Mo", "Miny",
  "Eeny", "Meeny", "Miny", "Mo", "Miny",
  "Eeny", "Meeny", "Miny", "Mo", "Miny",
  "Eeny", "Meeny", "Miny", "Mo", "Miny",
  "Eeny", "Meeny", "Miny", "Mo", "Miny",
  "Eeny", "Meeny", "Miny", "Mo", "Miny",
  "Eeny", "Meeny", "Miny", "Mo", "Miny",
  "Eeny", "Meeny", "Miny", "Mo", "Miny",
  "Eeny", "Meeny", "Miny", "Mo", "Miny",
  "Eeny", "Meeny", "Miny", "Mo", "Miny",
  "Eeny", "Meeny", "Miny", "Mo", "Miny",
  "Eeny", "Meeny", "Miny", "Mo", "Miny",
  "Eeny", "Meeny", "Miny", "Mo", "Miny",
  "Eeny", "Meeny", "Miny", "Mo", "Miny",
  "Eeny", "Meeny", "Miny", "Mo", "Miny",
  "Eeny", "Meeny", "Miny", "Mo", "Miny",
  "Eeny", "Meeny", "Miny", "Mo", "Miny",
  "Eeny", "Meeny", "Miny", "Mo", "Miny",
  "Eeny", "Meeny", "Miny", "Mo", "Miny",
  "Eeny", "Meeny", "Miny", "Mo", "Miny",
  "Eeny", "Meeny", "Miny", "Mo", "Miny",
  "Eeny", "Meeny", "Miny", "Mo", "Miny",
  "Eeny", "Meeny", "Miny", "Mo", "Miny",
  "Eeny", "Meeny", "Miny", "Mo", "Miny",
  "Eeny", "Meeny", "Miny", "Mo", "Miny",
  "Eeny", "Meeny", "Miny", "Mo", "Miny",
  "Eeny", "Meeny", "Miny", "Mo", "Miny",
  "Eeny", "Meeny", "Miny", "Mo", "Miny",
  "Eeny", "Meeny", "Miny", "Mo", "Miny",
  "Eeny", "Meeny", "Miny", "Mo", "Miny",
  "Eeny", "Meeny", "Miny", "Mo", "Miny",
  "Eeny", "Meeny", "Miny", "Mo", "Miny",
  "Eeny", "Meeny", "Miny", "Mo", "Miny",
  "Eeny", "Meeny", "Miny", "Mo", "Miny",
  "Eeny", "Meeny", "Miny", "Mo", "Miny",
  "Eeny", "Meeny", "Miny", "Mo", "Miny",
  "Eeny", "Meeny", "Miny", "Mo", "Miny",
  "Eeny", "Meeny", "Miny", "Mo", "Miny",
  "Eeny", "Meeny", "Miny", "Mo", "Miny",
  "Eeny", "Meeny", "Miny", "Mo", "Miny",
  "Eeny", "Meeny", "Miny", "Mo", "Miny",
  "Eeny", "Meeny", "Miny", "Mo", "Miny",
  "Eeny", "Meeny", "Miny", "Mo", "Miny",
  "Eeny", "Meeny", "Miny", "Mo", "Miny",
  "Eeny", "Meeny", "Miny", "Mo", "Miny",
  "Eeny", "Meeny", "Miny", "Mo", "Miny",
  "Eeny", "Meeny", "Miny", "Mo", "Miny",
  "Eeny", "Meeny", "Miny", "Mo", "Miny",
  "Eeny", "Meeny", "Miny", "Mo", "Miny",
  "Eeny", "Meeny", "Miny", "Mo", "Miny",
  "Eeny", "Meeny", "Miny", "Mo", "Miny",
  "Eeny", "Meeny", "Miny", "Mo", "Miny",
  "Eeny", "Meeny", "Miny", "Mo", "Miny",
  "Eeny", "Meeny", "Miny", "Mo", "Miny",
  "Eeny", "Meeny", "Miny", "Mo", "Miny",
  "Eeny", "Meeny", "Miny", "Mo", "Miny",
  "Eeny", "Meeny", "Miny", "Mo", "Miny",
  "Eeny", "Meeny", "Miny", "Mo", "Miny",
  "Eeny", "Meeny", "Miny", "Mo", "Miny",
  "Eeny", "Meeny", "Miny", "Mo", "Miny",
  "Eeny", "Meeny", "Miny", "Mo", "Miny",
  "Eeny", "Meeny", "Miny", "Mo", "Miny",
  "Eeny", "Meeny", "Miny", "Mo", "Miny",
  "Eeny", "Meeny", "Miny", "Mo", "Miny",
  "Eeny", "Meeny", "Miny", "Mo", "Miny",
  "Eeny", "Meeny", "Miny", "Mo", "Miny",
  "Eeny", "Meeny", "Miny", "Mo", "Miny",
  "Eeny", "Meeny", "Miny", "Mo", "Miny",
  "Eeny", "Meeny", "Miny", "Mo", "Miny",
  "Eeny", "Meeny", "Miny", "Mo", "Miny",
  "Eeny", "Meeny", "Miny", "Mo", "Miny",
  "Eeny", "Meeny", "Miny", "Mo", "Miny",
  "Eeny", "Meeny", "Miny", "Mo", "Miny",
  "Eeny", "Meeny", "Miny", "Mo", "Miny",
  "Eeny", "Meeny", "Miny", "Mo", "Miny",
  "Eeny", "Meeny", "Miny", "Mo", "Miny",
  "Eeny", "Meeny", "Miny", "Mo", "Miny",
  "Eeny", "Meeny", "Miny", "Mo", "Miny",
  "Eeny", "Meeny", "Miny", "Mo", "Miny",
  "Eeny", "Meeny", "Miny", "Mo", "Miny",
  "Eeny", "Meeny", "Miny", "Mo", "Miny",
  "Eeny", "Meeny", "Miny", "Mo", "Miny",
  "Eeny", "Meeny", "Miny", "Mo", "Miny",
  "Eeny", "Meeny", "Miny", "Mo", "Miny",
  "Eeny", "Meeny", "Miny", "Mo", "Miny",
  "Eeny", "Meeny", "Miny", "Mo", "Miny",
  "Eeny", "Meeny", "Miny", "Mo", "Miny",
  "Eeny", "Meeny", "Miny", "Mo", "Miny",
  "Eeny", "Meeny", "Miny", "Mo", "Miny",
  "Eeny", "Meeny", "Miny", "Mo", "Miny",
  "Eeny", "Meeny", "Miny", "Mo", "Miny",
  "Eeny", "Meeny", "Miny", "Mo", "Miny",
  "Eeny", "Meeny", "Miny", "Mo", "Miny",
  "Eeny", "Meeny", "Miny", "Mo", "Miny",
  "Eeny", "Meeny", "Miny", "Mo", "Miny",
};
static int numUniqueStringComplete[2] = {
  8, 8,
};
static int numUniqueStringSubset[2] = {
  4, 8,
};

static bool CheckUniques(
  vtkAbstractArray* arr, int* uniqueSizeBds,
  double uncertainty, double minProminence)
{
  bool ok = true;
  if (!arr)
  {
    return ok;
  }

  cout << arr->GetName() << endl;

  int nc = arr->GetNumberOfComponents();
  vtkNew<vtkVariantArray> uniques;
  for (int c = 0; c < nc; ++c)
  {
    arr->GetProminentComponentValues(
      c, uniques.GetPointer(), uncertainty, minProminence);
    int numUniques = static_cast<int>(uniques->GetNumberOfTuples());
    cout << "  comp " << c << " (" << numUniques << "): ";
    for (int n = 0; n <= uniques->GetMaxId(); ++n)
    {
      cout
        << (n > 0 ? ", " : " ")
        << uniques->GetVariantValue(n).ToString().c_str();
    }
    cout << endl;
    if (
      uniqueSizeBds[2 * c] > numUniques ||
      uniqueSizeBds[2 * c + 1] < numUniques)
    {
      cout
        << "    ** ERROR: Expected between " << uniqueSizeBds[2 * c]
        << " and " << uniqueSizeBds[2 * c + 1] << " values\n";
      ok = false;
    }
  }
  if (nc > 1)
  {
    arr->GetProminentComponentValues(
      -1, uniques.GetPointer(), uncertainty, minProminence);
    int numUniques = static_cast<int>(uniques->GetNumberOfTuples());
    cout << "  tuples (" << numUniques << "): ";
    for (int n = 0; n <= uniques->GetMaxId(); ++n)
    {
      cout
        << (n > 0 && n % nc == 0 ? ", " : " ")
        << uniques->GetVariantValue(n).ToString().c_str();
    }
    cout << endl;
    if (
      uniqueSizeBds[2 * nc] > numUniques ||
      uniqueSizeBds[2 * nc + 1] < numUniques)
    {
      cout
        << "    ** ERROR: Expected between " << uniqueSizeBds[2 * nc]
        << " and " << uniqueSizeBds[2 * nc + 1] << " values\n";
      ok = false;
    }
  }
  return ok;
}

int TestArrayUniqueValueDetection(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
{
  bool ok = true;
  vtkNew<vtkDoubleArray> darr;
  vtkNew<vtkIntArray> iarr;
  vtkNew<vtkStringArray> sarr;

  darr->SetNumberOfComponents(3);
  darr->SetArray(testTuplesDouble,
    sizeof(testTuplesDouble)/sizeof(testTuplesDouble[0]), 1);
  darr->SetName("Some3DPoints - Show off distinct tuple detection");
  ok &= CheckUniques(darr.GetPointer(), numUniqueDouble, 0., 0.);

  iarr->SetNumberOfComponents(2);
  iarr->SetArray(testTuplesInt,
    sizeof(testTuplesInt)/sizeof(testTuplesInt[0]), 1);
  iarr->SetName("Some2DPoints - Show off tuple non-detection");
  ok &= CheckUniques(iarr.GetPointer(), numUniqueInt, 0., 0.);

  iarr->SetNumberOfComponents(1);
  iarr->SetArray(testTuplesInt2,
    sizeof(testTuplesInt2)/sizeof(testTuplesInt2[0]), 1);
  //iarr->Modified(); // required since we have changed the tuples?
  iarr->SetName("Some1DPoints - Show off component non-detection");
  ok &= CheckUniques(iarr.GetPointer(), numUniqueInt2, 0., 0.);

  sarr->SetNumberOfComponents(1);
  sarr->SetArray(testTuplesString,
    sizeof(testTuplesString)/sizeof(testTuplesString[0]), 1);
  sarr->SetName("SomeNonWords - Show off string value detection");
  ok &= CheckUniques(sarr.GetPointer(), numUniqueStringComplete, 0.0, 0.0);

  sarr->Modified(); // Verify that modifying the array causes a re-compute
  sarr->SetName("SomeNonWords - Show off non-exhaustive search");
  ok &= CheckUniques(sarr.GetPointer(), numUniqueStringSubset, 0.5, 0.1);

  return ok ? 0 : 1;
}
