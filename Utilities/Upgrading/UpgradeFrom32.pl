#!/usr/bin/env perl

# This script tries to find deprecated classes and methods and replace 
# them with new classes/methods. Please note that it can not fix all 
# possible problems. However, it should be relatively easy to trace 
# those problems from compilation errors.

use Getopt::Long;

if (!GetOptions("language:s" => \$language, 
		"help" => \$help,
		"update" => \$update,
		"print-messages" => \$print))
{
    die;
}

if (!$language)
{
    $language = "c++";
}

if ( !$print && ($#ARGV < 0 || $help) )
{
    print "Usage: $0 [--language {c++ | tcl | python | java}] ",
          "[--help] [--print-messages] file1 [file2 ...]\n";
    exit;
}


@cxxmessageids = 
    (
     'vtkScalars\s*\*\s*([a-zA-Z0-9_-]*)\s*=\s*vtkScalars::New\([ ]*\)', 0,
     'vtkScalars\s*\*\s*([a-zA-Z0-9_-]*)\s*=\s*vtkScalars::New\(\s*VTK_UNSIGNED_CHAR\s*,\s*([1-4])\s*\);', 4,
     'vtkScalars\s*\*\s*([a-zA-Z0-9_-]*)\s*=\s*vtkScalars::New\(\s*VTK_UNSIGNED_SHORT\s*,\s*([1-4])\s*\);', 5,
     'vtkScalars\s*\*\s*([a-zA-Z0-9_-]*)\s*=\s*vtkScalars::New\(\s*VTK_SHORT\s*,\s*([1-4])\s*\);', 25,
     'vtkScalars\s*\*\s*([a-zA-Z0-9_-]*)\s*=\s*vtkScalars::New\(\s*VTK_INT\s*,\s*([1-4])\s*\);', 17,
     'vtkScalars\.h', 1,
     '([a-zA-Z0-9_-]*)\s*=\s*vtkScalars::New\(\s*\)', 2,
     '([a-zA-Z0-9_-]*)\s*=\s*vtkScalars::New\(\s*VTK_UNSIGNED_CHAR\s*,\s*([1-4])\s*\);', 6,
     '([a-zA-Z0-9_-]*)\s*=\s*vtkScalars::New\(\s*VTK_UNSIGNED_SHORT\s*,\s*([1-4])\s*\);', 7,
     '([a-zA-Z0-9_-]*)\s*=\s*vtkScalars::New\(\s*VTK_SHORT\s*,\s*([1-4])\s*\);', 26,
     '([a-zA-Z0-9_-]*)\s*=\s*vtkScalars::New\(\s*VTK_INT\s*,\s*([1-4])\s*\);', 18,
     'vtkScalars\s*\*\s*([a-zA-Z0-9_-]*)\s*=\s*vtkScalars::New\(\s*VTK_INT\s*\);', 19,
     'vtkScalars\s*\*\s*([a-zA-Z0-9_-]*)\s*=\s*vtkScalars::New\(\s*VTK_UNSIGNED_CHAR\s*\);', 20,
     'vtkScalars\s*\*\s*([a-zA-Z0-9_-]*)\s*=\s*vtkScalars::New\(\s*VTK_UNSIGNED_SHORT\s*\);', 21,
     'vtkScalars\s*\*\s*([a-zA-Z0-9_-]*)\s*=\s*vtkScalars::New\(\s*VTK_SHORT\s*\);', 27,
     '([a-zA-Z0-9_-]*)\s*=\s*vtkScalars::New\(\s*VTK_UNSIGNED_CHAR\s*\);', 22,
     '([a-zA-Z0-9_-]*)\s*=\s*vtkScalars::New\(\s*VTK_UNSIGNED_SHORT\s*\);', 23,
     '([a-zA-Z0-9_-]*)\s*=\s*vtkScalars::New\(\s*VTK_SHORT\s*\);', 28,
     '([a-zA-Z0-9_-]*)\s*=\s*vtkScalars::New\(\s*VTK_INT\s*\);', 24,
     'vtkScalars\s*\*\s*([a-zA-Z0-9_-]*)', 3,
     'GetScalar\s*\(', 8,
     'SetScalar\s*\(', 9,
     'InsertScalar\s*\(', 10,
     'InsertNextScalar\s*\(', 11,
     '(GetScalars\s*\(\s*[-a-zA-Z0-9_\*]+\s*\))', 12,
     'SetNumberOfScalars\s*\(', 13,
     'GetNumberOfScalars\s*\(', 16,
     'GetActiveScalars', 14,
     'vtkScalars([^a-zA-Z0-9])', 15,

     
     'vtkVectors\s*\*\s*([a-zA-Z0-9_-]*)\s*=[ \t]vtkVectors::New\([ ]*\)\s*;', 100,
     'vtkVectors\.h', 1,
     '([a-zA-Z0-9_-]*)\s*=\s*vtkVectors::New\(\s*\)', 102,
     'vtkVectors\s*\*\s*([a-zA-Z0-9_-]*)\s', 3,
     'GetVector\s*\(', 108,
     'SetVector\s*\(([^,]*),([^\),]*)\)', 109,
     'SetVector\s*\(([^,]*),([^,]*),([^,]*),([^\),]*)\)', 115,
     'InsertVector\s*\(', 110,
     'InsertNextVector\s*\(', 111,
     '(GetVectors\s*\(\s*[-a-zA-Z0-9_\*]+\s*\))', 12,
     'SetNumberOfVectors\s*\(', 113,
     'GetNumberOfVectors\s*\(', 16,
     'GetActiveVectors', 114,
     'vtkVectors([^a-zA-Z0-9])', 15,
     
     'vtkNormals\s*\*\s*([a-zA-Z0-9_-]*)\s*=[ \t]vtkNormals::New\([ ]*\)\s*;', 100,
     'vtkNormals\.h', 1,
     '([a-zA-Z0-9_-]*)\s*=\s*vtkNormals::New\(\s*\)', 102,
     'vtkNormals\s*\*\s*([a-zA-Z0-9_-]*)\s', 3,
     'GetNormal\s*\(', 108,
     'SetNormal\s*\(([^,]*),([^\),]*)\)', 109,
     'SetNormal\s*\(([^,]*),([^,]*),([^,]*),([^\),]*)\)', 115,
     'InsertNormal\s*\(', 110,
     'InsertNextNormal\s*\(', 111,
     '(GetNormals\s*\(\s*[-a-zA-Z0-9_\*]+\s*\))', 12,
     'SetNumberOfNormals\s*\(', 113,
     'GetNumberOfNormals\s*\(', 16,
     'GetActiveNormals', 214,
     'vtkNormals([^a-zA-Z0-9])', 15,     

     'vtkTCoords\s*\*\s*([a-zA-Z0-9_-]*)\s*=[ \t]vtkTCoords::New\([ ]*\)\s*;', 300,
     'vtkTCoords\.h', 1,
     '([a-zA-Z0-9_-]*)\s*=\s*vtkTCoords::New\(\s*\)', 302,
     'vtkTCoords\s*\*\s*([a-zA-Z0-9_-]*)\s', 3,
     'GetTCoord\s*\(', 108,
     'SetTCoord\s*\(([^,]*),([^\),]*)\)', 109,
     'InsertTCoord\s*\(', 110,
     'InsertNextTCoord\s*\(', 111,
     '(GetTCoords\s*\(\s*[-a-zA-Z0-9_\*]+\s*\))', 12,
     'SetNumberOfTCoords\s*\(', 113,
     'GetNumberOfTCoords\s*\(', 16,
     'GetActiveTCoords', 314,
     'vtkTCoords([^a-zA-Z0-9])', 15,     

     'vtkTensors\s*\*\s*([a-zA-Z0-9_-]*)\s*=[ \t]vtkTensors::New\([ ]*\)\s*;', 400,
     'vtkTensors\.h', 1,
     '([a-zA-Z0-9_-]*)\s*=\s*vtkTensors::New\(\s*\)', 402,
     'vtkTensors\s*\*\s*([a-zA-Z0-9_-]*)\s', 3,
     'GetTensor\s*\(', 108,
     'SetTensor\s*\(([^,]*),([^\),]*)\)', 109,
     'InsertTensor\s*\(', 110,
     'InsertNextTensor\s*\(', 111,
     '(GetTensors\s*\(\s*[-a-zA-Z0-9_\*]+\s*\))', 12,
     'SetNumberOfTensors\s*\(', 113,
     'GetNumberOfTensors\s*\(', 16,
     'GetActiveTensors', 414,
     'vtkTensors([^a-zA-Z0-9])', 15,     

     'GetPointData\(\)->GetFieldData\(', 1000,
     'GetCellData\(\)->GetFieldData\(', 1001,
     'SaveImageAsPPM\s*\(', 1002,
     );


%cxxreps = 
    (
     0 => 'vtkFloatArray \*$1 = vtkFloatArray::New\(\)',
     1 => 'vtkFloatArray\.h',
     2 => '$1 = vtkFloatArray::New\(\)',
     3 => 'vtkDataArray \*$1',
     4 => 'vtkUnsignedCharArray \*$1 = vtkUnsignedCharArray::New\(\); $1->SetNumberOfComponents\($2\);',
     5 => 'vtkUnsignedShortArray \*$1 = vtkUnsignedShortArray::New\(\); $1->SetNumberOfComponents\($2\);',
     6 => '$1 = vtkUnsignedCharArray::New\(\); $1->SetNumberOfComponents\($2\);',
     7 => '$1 = vtkUnsignedShortArray::New\(\); $1->SetNumberOfComponents\($2\);',
     8 => 'GetTuple1\(',
     9 => 'SetTuple1\(',
     10 => 'InsertTuple1\(',
     11 => 'InsertNextTuple1\(',
     12 => '$1 \/\/ Use GetTuples here instead ',
     13 => 'SetNumberOfTuples\(',
     14 => 'GetScalars',
     15 => 'vtkDataArray$1',
     16 => 'GetNumberOfTuples\(',
     17 => 'vtkIntArray \*$1 = vtkIntArray::New\(\); $1->SetNumberOfComponents\($2\);',
     18 => '$1 = vtkIntArray::New\(\); $1->SetNumberOfComponents\($2\);',
     19 => 'vtkIntArray \*$1 = vtkIntArray::New\(\);',
     20 => 'vtkUnsignedCharArray \*$1 = vtkUnsignedCharArray::New\(\);',
     21 => 'vtkUnsignedShortArray \*$1 = vtkUnsignedShortArray::New\(\);',
     22 => '$1 = vtkUnsignedCharArray::New\(\);',
     23 => '$1 = vtkUnsignedShortArray::New\(\);',
     24 => '$1 = vtkIntArray::New\(\);',
     25 => 'vtkShortArray \*$1 = vtkShortArray::New\(\); $1->SetNumberOfComponents\($2\);',
     26 => '$1 = vtkShortArray::New\(\); $1->SetNumberOfComponents\($2\);',
     27 => 'vtkShortArray \*$1 = vtkShortArray::New\(\);',
     28 => '$1 = vtkShortArray::New\(\);',
     
     
     100 => 'vtkFloatArray \*$1 = vtkFloatArray::New\(\); $1->SetNumberOfComponents\(3\);',
     102 => '$1 = vtkFloatArray::New\(\); $1->SetNumberOfComponents\(3\)',
     108 => 'GetTuple\(',
     109 => 'SetTuple\($1,$2\)',
     110 => 'InsertTuple\(',
     111 => 'InsertNextTuple\(',
     113 => 'SetNumberOfTuples\(',
     114 => 'GetVectors',
     115 => 'SetTuple3\($1,$2,$3,$4\)',
     
     214 => 'GetNormals',
     
     300 => 'vtkFloatArray \*$1 = vtkFloatArray::New\(\); $1->SetNumberOfComponents\(2\);',
     302 => '$1 = vtkFloatArray::New\(\); $1->SetNumberOfComponents\(2\)',
     314 => 'GetTCoords',
     
     400 => 'vtkFloatArray \*$1 = vtkFloatArray::New\(\); $1->SetNumberOfComponents\(9\);',
     402 => '$1 = vtkFloatArray::New\(\); $1->SetNumberOfComponents\(9\)',
     414 => 'GetTensors',
     
     1000 => 'GetPointData\(',
     1001 => 'GetCellData\(',
     1002 => 'SaveImageAsPPM\( \/\/ Use a vtkWindowToImageFilter instead of SaveImageAsPPM',
     );


@tclmessageids = 
    (
     'vtkScalars\s+([a-zA-Z0-9\$_-]*)', 0,
     'SetScalar\s+(\S+)\s*(\S+)', 1,
     'InsertScalar\s+', 2,
     'InsertNextScalar\s+', 3,
     'SetNumberOfScalars\s+', 4,
     'GetNumberOfScalars\s+', 5,
     'GetActiveScalars\s+', 6,

     'vtkVectors\s+([a-zA-Z0-9\$_-]*)', 100,
     'SetVector\s+(\S+)\s*(\S+)\s*(\S+)\s*(\S+)', 101,
     'InsertVector\s+', 102,
     'InsertNextVector\s+', 103,
     'SetNumberOfVectors\s+', 4,
     'GetNumberOfVectors\s+', 5,
     'GetActiveVectors\+', 106,

     'vtkNormals\s+([a-zA-Z0-9\$_-]*)', 100,
     'SetNormal\s+(\S+)\s*(\S+)\s*(\S+)\s*(\S+)', 101,
     'InsertNormal\s+', 102,
     'InsertNextNormal\s+', 103,
     'SetNumberOfNormals\s+', 4,
     'GetNumberOfNormals\s+', 5,
     'GetActiveNormals\+', 206,

     'vtkTCoords\s+([a-zA-Z0-9\$_-]*)', 300,
     'SetTCoord\s+(\S+)\s*(\S+)\s*(\S+)\s*(\S+)', 101,
     'InsertTCoord\s+', 102,
     'InsertNextTCoord\s+', 103,
     'SetNumberOfTCoords\s+', 4,
     'GetNumberOfTCoords\s+', 5,
     'GetActiveTCoords\s+', 306,

     'GetPointData\s*\]\s+GetFieldData\s*\]', 1000,
     'GetCellData\s*\]\s+GetFieldData\s*\]', 1001,
     'SaveImageAsPPM\s*', 1002,
     'GetImageWindow', 1003,
     'catch\s*\{\s*load\s*vtktcl\s*\}', 1004,
     'source\s*\$VTK_TCL\/vtkInt\.tcl', 1005,
     'source\s*\$VTK_TCL\/colors\.tcl', 1006,
     );

%tclreps = 
    (
     0 => 'vtkFloatArray $1',
     1 => 'SetTuple1 $1 $2',
     2 => 'InsertTuple1 ',
     3 => 'InsertNextTuple1 ',
     4 => 'SetNumberOfTuples ',
     5 => 'GetNumberOfTuples ',
     6 => 'GetScalars ',
     
     100 => 'vtkFloatArray $1; $1 SetNumberOfComponents 3',
     101 => 'SetTuple3 $1 $2 $3 $4',
     102 => 'InsertTuple3 ',
     103 => 'InsertNextTuple3 ',
     106 => 'GetVectors ',
     
     206 => 'GetNormals ',
     
     300 => 'vtkFloatArray $1; $1 SetNumberOfComponents 2',
     306 => 'GetTCoords ',
     
     1000 => 'GetPointData\]',
     1001 => 'GetCellData\]',
     1002 => 'SaveImageAsPPM \# Use a vtkWindowToImageFilter instead of SaveImageAsPPM',
     1003 => 'GetRenderWindow',
     1004 => 'package require vtk',
     1005 => 'package require vtkinteraction',
     1006 => 'package require vtktesting',
     );

if ($language eq "c++")
{
    @messageids = @cxxmessageids;
    %reps = %cxxreps;
} 
elsif($language eq "tcl")
{
    @messageids = @tclmessageids;
    %reps = %tclreps;
} 

else
{
    die "Unsupported language: $language.\n";
}

if ( $print )
{
    $i = 0;
    foreach $key (@messages)
    {
	print "Message id $i:\n";
	print $key, "\n";
	$i++;
    }
    exit 0;
}

foreach $filename (@ARGV)
{
    
    open(FPTR, "<$filename") or die "Could not open file $filename";
    open(OPTR, ">$filename.update") or die "Could not open file $filename.update";

    $i = 1;
    while (<FPTR>)
    {
	$line = $_;
	$j = 0;
	while ($j < $#messageids)
	{
	    if ( $line =~ m($messageids[$j]) )
	    {
		eval "\$line =~ s/$messageids[$j]/$reps{$messageids[$j+1]}/g";
	    }
	    $j = $j + 2;
	}
	print OPTR $line;
	$i++;
    }
    close OPTR;
    close FPTR;
    if ( $update )
    {
	print $filename,"\n";
	rename("$filename.update","$filename");
    }
}

