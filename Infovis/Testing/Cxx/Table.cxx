#include "vtkTable.h"
#include "vtkIntArray.h"
#include "vtkStringArray.h"
#include "vtkDoubleArray.h"
#include "vtkVariantArray.h"
#include "vtkFieldData.h"
#include "vtkMath.h"

#include <time.h>
#include <vtkstd/vector>
using vtkstd::vector;

void CheckEqual(vtkTable* table, vector<vector<double> > & stdTable)
{
  // Check sizes
  if (table->GetNumberOfRows() != static_cast<vtkIdType>(stdTable[0].size()))
    {
    cout << "Number of rows is incorrect (" 
         << table->GetNumberOfRows() << " != " << stdTable.size() << ")" << endl;
    exit(1);
    }
  if (table->GetNumberOfColumns() != static_cast<vtkIdType>(stdTable.size()))
    {
    cout << "Number of columns is incorrect (" 
         << table->GetNumberOfColumns() << " != " << stdTable.size() << ")" << endl;
    exit(1);
    }

  // Use GetValue() and GetValueByName() to check
  for (int i = 0; i < table->GetNumberOfRows(); i++)
    {
    for (int j = 0; j < table->GetNumberOfColumns(); j++)
      {
      double tableVal = table->GetValue(i, j).ToDouble();
      double stdTableVal = stdTable[j][i];
      if (stdTableVal && tableVal != stdTableVal)
        {
        cout << "Values not equal at row " << i << " column " << j << ": ";
        cout << "(" << tableVal << " != " << stdTableVal << ")" << endl;
        exit(1);
        }
      }
    }

  // Use GetColumn() and GetColumnByName() to check
  for (int j = 0; j < table->GetNumberOfColumns(); j++)
    {
    vtkAbstractArray* arr;
    if (vtkMath::Random() < 0.5)
      {
      arr = table->GetColumn(j);
      }
    else
      {
      arr = table->GetColumnByName(table->GetColumnName(j));
      }
    for (int i = 0; i < table->GetNumberOfRows(); i++)
      {
      double val;
      if (arr->IsA("vtkVariantArray"))
        {
        val = vtkVariantArray::SafeDownCast(arr)->GetValue(i).ToDouble();
        }
      else if (arr->IsA("vtkStringArray"))
        {
        vtkVariant v(vtkStringArray::SafeDownCast(arr)->GetValue(i));
        val = v.ToDouble();
        }
      else if (arr->IsA("vtkDataArray"))
        {
        val = vtkDataArray::SafeDownCast(arr)->GetTuple1(i);
        }
      else
        {
        cout << "Unknown array type" << endl;
        exit(1);
        }
      double stdTableVal = stdTable[j][i];
      if (stdTableVal && val != stdTableVal)
        {
        cout << "Values not equal at row " << i << " column " << j << ": ";
        cout << "(" << val << " != " << stdTableVal << ")" << endl;
        exit(1);
        }
      }
    }

  // Use GetRow() to check
  for (int i = 0; i < table->GetNumberOfRows(); i++)
    {
    vtkVariantArray* arr = table->GetRow(i);
    for (int j = 0; j < table->GetNumberOfColumns(); j++)
      {
      double val = arr->GetValue(j).ToDouble();
      double stdTableVal = stdTable[j][i];
      if (stdTableVal && val != stdTableVal)
        {
        cout << "Values not equal at row " << i << " column " << j << ": ";
        cout << "(" << val << " != " << stdTableVal << ")" << endl;
        exit(1);
        }
      }
    arr->Delete();
    }
}

int Table(int, char*[])
{
  long seed = time(NULL);
  cout << "Seed: " << seed << endl;
  vtkMath::RandomSeed(seed);

  // Make a table and a parallel vector of vectors
  vtkTable* table = vtkTable::New();
  vector<vector< double > > stdTable;

  int size = 100;
  double prob = 1.0 - 1.0 / size;
  double highProb = 1.0 - 1.0 / (size*size);

  cout << "Creating columns." << endl;
  vtkIdType columnId = 0;
  bool noColumns = true;
  while (noColumns || vtkMath::Random() < prob)
    {
    noColumns = false;

    stdTable.push_back(vector<double>());

    double r = vtkMath::Random();
    vtkVariant name(columnId);
    vtkAbstractArray* arr;
    if (r < 0.25)
      {
      arr = vtkIntArray::New();
      arr->SetName((name.ToString() + " (vtkIntArray)").c_str());
      }
    else if (r < 0.5)
      {
      arr = vtkDoubleArray::New();
      arr->SetName((name.ToString() + " (vtkDoubleArray)").c_str());
      }
    else if (r < 0.75)
      {
      arr = vtkStringArray::New();
      arr->SetName((name.ToString() + " (vtkStringArray)").c_str());
      }
    else
      {
      arr = vtkVariantArray::New();
      arr->SetName((name.ToString() + " (vtkVariantArray)").c_str());
      }
    table->AddColumn(arr);
    arr->Delete();
    columnId++;
    }

  cout << "Inserting empty rows." << endl;
  bool noRows = true;
  while (noRows || vtkMath::Random() < prob)
    {
    noRows = false;
    table->InsertNextBlankRow();
    for (unsigned int i = 0; i < stdTable.size(); i++)
      {
      stdTable[i].push_back(0.0);
      }
    }

  cout << "Inserting full rows." << endl;
  while (vtkMath::Random() < prob)
    {
    vtkVariantArray* rowArray = vtkVariantArray::New();
    for (vtkIdType j = 0; j < table->GetNumberOfColumns(); j++)
      {
      rowArray->InsertNextValue(vtkVariant(j));
      stdTable[j].push_back(j);
      }
    table->InsertNextRow(rowArray);
    rowArray->Delete();
    }

  cout << "Performing all kinds of inserts." << endl;
  int id = 0;
  while (vtkMath::Random() < highProb)
    {
    vtkIdType row = static_cast<vtkIdType>(vtkMath::Random(0, table->GetNumberOfRows()));
    vtkIdType col = static_cast<vtkIdType>(vtkMath::Random(0, table->GetNumberOfColumns()));
    vtkVariant v;
    if (vtkMath::Random() < 0.25)
      {
      vtkVariant temp(id);
      v = vtkVariant(temp.ToString());
      }
    else if (vtkMath::Random() < 0.5)
      {
      v = vtkVariant(id);
      }
    else
      {
      v = vtkVariant(static_cast<double>(id));
      }

    if (vtkMath::Random() < 0.5)
      {
      table->SetValue(row, col, v);
      }
    else
      {
      table->SetValueByName(row, table->GetColumnName(col), v);
      }
    stdTable[col][row] = id;

    id++;
    }

  cout << "Removing half of the rows." << endl;
  int numRowsToRemove = table->GetNumberOfRows() / 2;
  for (int i = 0; i < numRowsToRemove; i++)
    {
    vtkIdType row = static_cast<vtkIdType>(vtkMath::Random(0, table->GetNumberOfRows()));
    table->RemoveRow(row);

    for (unsigned int j = 0; j < stdTable.size(); j++)
      {
      vector<double>::iterator rowIt = stdTable[j].begin() + row;
      stdTable[j].erase(rowIt);
      }
    }

  cout << "Removing half of the columns." << endl;
  int numColsToRemove = table->GetNumberOfColumns() / 2;
  for (int i = 0; i < numColsToRemove; i++)
    {
    vtkIdType col = static_cast<vtkIdType>(vtkMath::Random(0, table->GetNumberOfColumns()));
    if (vtkMath::Random() < 0.5)
      {
      table->RemoveColumn(col);
      }
    else
      {
      table->RemoveColumnByName(table->GetColumnName(col));
      }

    vector<vector<double> >::iterator colIt = stdTable.begin() + col;
    stdTable.erase(colIt);
    }

  cout << "vtkTable size: " << table->GetNumberOfRows() << "," << table->GetNumberOfColumns() << endl;
  cout << "vector<vector<double> > size: " << stdTable[0].size() << "," << stdTable.size() << endl;

  cout << "Checking that table matches expected table." << endl;
  CheckEqual(table, stdTable);

  table->Delete();
  return 0;
}

