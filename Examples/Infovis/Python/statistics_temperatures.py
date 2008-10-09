from vtk import *

data_dir = "../../../../../VTKData/Data/Infovis/SQLite/"
sqlite_file = data_dir + "temperatures.db"
database = vtkSQLDatabase.CreateFromURL("sqlite://" + sqlite_file)
database.Open("")

query = database.GetQueryInstance()
query.SetQuery("select * from main_tbl")

table = vtkRowQueryToTable()
table.SetQuery(query)

ds = vtkDescriptiveStatistics()
ds.AddInputConnection(table.GetOutputPort())
ds.AddColumn("Temp1")
ds.AddColumn("Temp2")
ds.SetAssess(1)
ds.Update()

print ds.GetOutput(1)

