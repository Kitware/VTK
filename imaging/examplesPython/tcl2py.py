#!/usr/bin/env python
import sys, string, regex, re
from scanfile import scanner

#
# tcl2py.py - try to Pythonize a vtk tcl script
# 

name = sys.argv[1]
keepTcl = sys.argv[2]
print name
#print '------------------'
input = open(name + '.tcl', 'r')
output = open(name + '.py', 'w')


#>>> b = regex.compile( '[[]')
#>>> b.search( '    [   ')
#4
global inaModule

def processLine(line):
	if line[0] == '#':
		output.write(line)
	else:
		if keepTcl == 1:
			output.write('#' + line)

		line = re.sub('\[','',line)
		line = re.sub('\]','',line)
		line = re.sub('ren1','ren',line)
		line = re.sub(';','\n',line)
		line = re.sub('\$','',line)
		line = re.sub('{',' ',line)
		line = re.sub('}',' ',line)

		n = len(line)
#		if line[n-1:n] == ;
#			line = line[:-1]

		keys = string.split(line)
#		print 'length(keys) = ' + len(keys)
		keysLength = len(keys)
#		if keysLength != 0:
#			print keys

		inaModule = 0
		inaForLoop = 0
		if keysLength == 0:
			output.write(line)
#		 Catch some tcl-specific keywords and comment out
		elif keys[0] == 'proc':
			inaModule = 0
			output.write( 'def ' + keys[1] + '(')
			if len(keys) > 2:
				output.write(keys[2])
				for i in range(3,len(keys)):
					output.write(',' + keys[i])
			output.write('):\n')
		elif keys[0] == 'catch':
			output.write( '#' + line)
		elif keys[0] == 'source':
			output.write( '#' + line)
			if regex.search("colors.tcl",line) > -1:
				output.write("from colors import *")
		elif keys[0] == 'wm':
			output.write( '#' + line)
		elif keysLength > 1 and keys[1] == 'SetUserMethod':
			if keepTcl == 1:
				output.write( '#' + line)
		elif keys[0] == 'for' and keys[1]=='set':
			inaForLoop = 1
#			print '...Handling for loop'
			output.write( "for " + keys[2] + " in range(" + keys[3] +",")
			upper = keys[6]
			if keys[5] == "<=":
				output.write( upper + "+1):\n" )
			else:
				output.write( upper + "):\n" )

#		 Detect vtk class instance; Pythonize it.
		elif line[:3] == 'vtk':
			output.write( keys[1] + ' = ' + keys[0] + '()\n' )
		else:
			lparen = 0
			finishedFlag = 0
#			for i in range(len(keys)-1):
			for i in range(len(keys)):
				ls = len(keys[i])
				if keys[i] == 'eval':
					continue
				# continuation mark
				elif keys[i] == '\\':
					output.write( "  \\\n")
				elif keys[i] == 'SetFileName':
					if keys[i+1][0:1] == '"':
						output.write( "SetFileName(" + keys[i+1] + ")")
					else:
						output.write( "SetFileName(\"" + keys[i+1] + "\")")
					finishedFlag = 1
					break
				elif keys[i] == 'SetColor':
#					print '...doing SetColor'
#					print keys
					if regex.search(keys[i+1][0:1],"0123456789") == -1:
#						print '...got a named color'
						color = keys[i+1][0:]
#						print 'color = ' + color
						output.write( "SetColor(" + color+"[0]," + \
						color+"[1]," + color+"[2])" )
					else:
						output.write( "SetColor("+keys[i+1]+","+keys[i+2]+","+keys[i+3]+")")
					finishedFlag = 1
					break
				elif keys[i][:3]=='Set' or keys[i][:3]=='Add' or keys[i][:6]=='Insert':
					output.write( keys[i] + '(' )
					lparen = 1
				elif i < len(keys)-1 and \
				regex.search(keys[i+1][0:1],"0123456789-") > -1 and \
				regex.search(keys[i][ls-1:ls],"0123456789-")==-1:
					output.write( keys[i] + '(' )
					lparen = 1
				elif keys[i][:3] == 'Get':
					output.write( keys[i] + '()' )
					if i < len(keys)-1:
						output.write( '.' )
				else:
					if i < len(keys)-1:
						npos = regex.search(keys[i][0:1], "0123456789-") 
						if npos > -1 or keys[i][0:3] == 'VTK':
							output.write( keys[i] + ',' )
						else:
							output.write( keys[i] + '.' )
					else:
						if inaModule == 1:
							output.write( '\t' )
						output.write( keys[i] )

			if finishedFlag == 0:
				if keys[len(keys)-1][:3] != 'Get':
					if lparen == 0:
						output.write( '(' )
						lparen = 1
			else:
				output.write( '\n' )

#			 	Terminating right paren.
#				output.write( ')\n' )

			if lparen == 1:
				output.write( ')\n' )

output.write('#!/usr/local/bin/python\n')
output.write('\n')
output.write('from vtkpython import *\n')
output.write('from WindowLevelInterface import *\n')
output.write('\n')
for line in input.readlines():
	processLine(line)

#output.write('iren.Initialize()\n')
output.write('WindowLevelInterface(viewer)\n')






