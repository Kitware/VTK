#!/usr/bin/env python
from vtk import *
import os.path

data_dir = "../../../../VTKData/Data/Infovis/SQLite/"
if not os.path.exists(data_dir):
  data_dir = "../../../../../VTKData/Data/Infovis/SQLite/"
sqlite_file = data_dir + "SmallEmailTest.db"

database = vtkSQLDatabase.CreateFromURL("sqlite://" + sqlite_file)
database.Open("")

query = database.GetQueryInstance()
query.SetQuery("select Name, Job, Age from employee")

queryToTable = vtkRowQueryToTable()
queryToTable.SetQuery(query)
queryToTable.Update()

T = queryToTable.GetOutput()

print "Query Results:"
T.Dump(12)

database.FastDelete()

