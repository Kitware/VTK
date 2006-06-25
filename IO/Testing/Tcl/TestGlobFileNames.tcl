package require vtk

vtkGlobFileNames globFileNames
globFileNames AddFileNames "$VTK_DATA_ROOT/Data/headsq/quarter.*"
set fileNames [globFileNames GetFileNames]

set n [$fileNames GetNumberOfValues]
if { $n != 93 } {
    puts "GetNumberOfValues should return 93"
    exit 1
}

for { set i 0 } { $i < $n } { incr i } {
    if { [$fileNames GetValue $i] != [globFileNames GetFileName $i] } {
        puts "mismatched filename for pattern quarter.*"
        puts [$fileNames GetValue $i]
        exit 1
    }
    if { [string match "*quarter.*" [$fileNames GetValue $i] ] == 0 } {
        puts "strings does not match pattern quarter.*"
        puts [$fileNames GetValue $i]
        exit 1
    }
}

# check that we can re-use the Glob object
globFileNames Reset
globFileNames AddFileNames "$VTK_DATA_ROOT/Data/financial.*"
set fileNames [globFileNames GetFileNames]

set n [$fileNames GetNumberOfValues]
for { set i 0 } { $i < $n } { incr i } {
    if { [$fileNames GetValue $i] != [globFileNames GetFileName $i] } {
        puts "mismatched filename for pattern financial.*"
        puts [$fileNames GetValue $i]
        exit 1
    }
    if { [string match "*financial.*" [$fileNames GetValue $i] ] == 0 } {
        puts "strings does not match pattern financial.*"
        puts [$fileNames GetValue $i]
        exit 1
    }
}

globFileNames Delete

exit
