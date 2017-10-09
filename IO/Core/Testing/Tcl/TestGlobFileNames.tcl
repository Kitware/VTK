package require vtk

vtkGlobFileNames globFileNames
globFileNames SetDirectory "$VTK_DATA_ROOT/Data/"
# globs do not include Kleene star support for pattern repetitions thus
# we insert a pattern for both single and double digit file extensions.
globFileNames AddFileNames "headsq/quarter.\[1\-9\]"
globFileNames AddFileNames "headsq/quarter.\[1\-9\]\[0\-9\]"

set fileNames [globFileNames GetFileNames]
set n [globFileNames GetNumberOfFileNames]

if { $n != 93 } {
    for { set i 0 } { $i < $n } { incr i } {
        puts [$fileNames GetValue $i]
    }
    puts "GetNumberOfValues should return 93, returned $n"
    puts "Listing of $VTK_DATA_ROOT/Data/headsq"
    vtkDirectory directory
    directory Open "$VTK_DATA_ROOT/Data/headsq"
    set m [directory GetNumberOfFiles]
    for { set j 0 } { $j < $n } { incr j } {
        puts [directory GetFile $j]
    }
    exit 1
}

for { set i 0 } { $i < $n } { incr i } {
    set filename [$fileNames GetValue $i]
    if { "$filename" != [globFileNames GetNthFileName $i] } {
        puts "mismatched filename for pattern quarter.*: $filename"
        exit 1
    }
    if { [string match "*quarter.*" "$filename"] == 0 } {
        puts "strings does not match pattern quarter.*: $filename"
        exit 1
    }
}

# check that we can re-use the Glob object
globFileNames Reset
globFileNames SetDirectory "$VTK_DATA_ROOT/Data/"
globFileNames AddFileNames "$VTK_DATA_ROOT/Data/financial.*"
set fileNames [globFileNames GetFileNames]

set n [$fileNames GetNumberOfValues]
for { set i 0 } { $i < $n } { incr i } {
    set filename [$fileNames GetValue $i]
    if { "$filename" != [globFileNames GetNthFileName $i] } {
        puts "mismatched filename for pattern financial.*: $filename"
        exit 1
    }
    if { [string match "*financial.*" $filename] == 0 } {
        puts "strings does not match pattern financial.*: $filename"
        exit 1
    }
}

globFileNames Delete

exit
