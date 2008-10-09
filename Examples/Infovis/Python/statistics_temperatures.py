from vtk import *

data_dir = "../../../../../VTKData/Data/Infovis/SQLite/"
sqlite_file = data_dir + "temperatures.db"
database = vtkSQLDatabase.CreateFromURL("sqlite://" + sqlite_file)
database.Open("")

query = database.GetQueryInstance()
query.SetQuery("select * from main_tbl")

data = vtkRowQueryToTable()
data.SetQuery(query)

ds = vtkDescriptiveStatistics()
ds.AddInputConnection(data.GetOutputPort())
ds.AddColumn("Temp1")
ds.AddColumn("Temp2")
ds.SetAssess(1)
ds.Update()

dStats = ds.GetOutput(1)
dStats.Dump( 9 )

os = vtkOrderStatistics()
os.AddInputConnection(data.GetOutputPort())
os.AddColumn("Temp1")
os.AddColumn("Temp2")
os.SetAssess(1)
os.Update()

oStats = os.GetOutput(1)
oStats.Dump( 9 )
