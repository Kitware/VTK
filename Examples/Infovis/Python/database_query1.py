#!/usr/bin/env python
from __future__ import print_function
from vtk import *
import os.path
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

data_dir = VTK_DATA_ROOT + "/Data/Infovis/SQLite/"
if not os.path.exists(data_dir):
  data_dir = VTK_DATA_ROOT + "/Data/Infovis/SQLite/"
sqlite_file = data_dir + "SmallEmailTest.db"

database = vtkSQLDatabase.CreateFromURL("sqlite://" + sqlite_file)
database.Open("")

query = database.GetQueryInstance()
query.SetQuery("select Name, Job, Age from employee")

queryToTable = vtkRowQueryToTable()
queryToTable.SetQuery(query)
queryToTable.Update()

T = queryToTable.GetOutput()

print("Query Results:")
T.Dump(12)

database.FastDelete()

