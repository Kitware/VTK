from vtk import *
import os.path

data_dir = "../../../../VTKData/Data/Infovis/SQLite/"
if not os.path.exists(data_dir):
  data_dir = "../../../../../VTKData/Data/Infovis/SQLite/"
sqlite_file = data_dir + "temperatures.db"

# Pull the table from the database
databaseToTable = vtkSQLDatabaseTableSource()
databaseToTable.SetURL("sqlite://" + sqlite_file)
databaseToTable.SetQuery("select * from main_tbl")


# Start with descriptive statistics
print "# Start with descriptive statistics:"
ds = vtkDescriptiveStatistics()
ds.AddInputConnection(databaseToTable.GetOutputPort())
ds.AddColumn("Temp1")
ds.AddColumn("Temp2")
ds.Update()

dStats = ds.GetOutput(1)
dStats.Dump( 10 )
print

print "# Now calculate 5-point statistics:"
# Now calculate 5-point statistics
os = vtkOrderStatistics()
os.AddInputConnection(databaseToTable.GetOutputPort())
os.AddColumn("Temp1")
os.AddColumn("Temp2")
os.Update()

oStats = os.GetOutput(1)
oStats.Dump( 15 )
print

print "# Continue with deciles:"
# Continue with deciles
os.SetNumberOfIntervals(10)
os.Update()

oStats = os.GetOutput(1)
oStats.Dump( 11 )
print

print "# Finally, calculate correlation and linear regression:"
# Finally, calculate correlation and linear regression
cs = vtkCorrelativeStatistics()
cs.AddInputConnection(databaseToTable.GetOutputPort())
cs.AddColumnPair("Temp1","Temp2")
cs.SetAssessOption(1)
cs.Update()

cStats = cs.GetOutput(1)
cStats.Dump( 10 )
print

print "# And report corresponding deviations (squared Mahalanobis distance):"
# And report corresponding deviations (squared Mahalanobis distance):"
cData = cs.GetOutput(0)
cData.Dump( 14 )
print
