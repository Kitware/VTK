from vtk import *
import os.path

data_dir = "../../../../VTKData/Data/Infovis/SQLite/"
if not os.path.exists(data_dir):
  data_dir = "../../../../../VTKData/Data/Infovis/SQLite/"
if not os.path.exists(data_dir):
  data_dir = "../../../../../../VTKData/Data/Infovis/SQLite/"
sqlite_file = data_dir + "temperatures.db"

# Pull the table from the database
databaseToTable = vtkSQLDatabaseTableSource()
databaseToTable.SetURL("sqlite://" + sqlite_file)
databaseToTable.SetQuery("select * from main_tbl")

# Calculate descriptive statistics
print "# Calculate descriptive statistics:"
ds = vtkDescriptiveStatistics()
ds.AddInputConnection(databaseToTable.GetOutputPort())
ds.AddColumn("Temp1")
ds.AddColumn("Temp2")
ds.Update()

dStats = ds.GetOutputDataObject( 1 )
dPrimary = dStats.GetBlock( 0 )
dDerived = dStats.GetBlock( 1 )
dPrimary.Dump( 15 )
dDerived.Dump( 15 )
print

print "# Calculate 5-point statistics:"
# Calculate 5-point statistics
os = vtkOrderStatistics()
os.AddInputConnection(databaseToTable.GetOutputPort())
os.AddColumn("Temp1")
os.AddColumn("Temp2")
os.Update()

oStats = os.GetOutputDataObject( 1 )
oQuantiles = oStats.GetBlock( 2 )
oQuantiles.Dump( 15 )
print

print "# Calculate deciles:"
# Calculate deciles
os.SetNumberOfIntervals(10)
os.Update()

oStats = os.GetOutputDataObject( 1 )
oQuantiles = oStats.GetBlock( 2 )
oQuantiles.Dump( 9 )
print

print "# Calculate correlation and linear regression:"
# Calculate correlation and linear regression
cs = vtkCorrelativeStatistics()
cs.AddInputConnection(databaseToTable.GetOutputPort())
cs.AddColumnPair("Temp1","Temp2")
cs.SetAssessOption(1)
cs.Update()

cStats = cs.GetOutputDataObject( 1 )
cPrimary = cStats.GetBlock(0)
cDerived = cStats.GetBlock(1)
cPrimary.Dump( 15 )
cDerived.Dump( 15 )
print

print "# Report corresponding deviations (squared Mahalanobis distance):"
# Report corresponding deviations (squared Mahalanobis distance):"
cData = cs.GetOutput(0)
cData.Dump( 15 )
print
