#!/usr/bin/env python
import sys, string, regex, re
from scanfile import scanner

name = sys.argv[1]
input = open(name, 'r')

#>>> b = regex.compile( '[[]')
#>>> b.search( '    [   ')
#4

def processLine(line):
	if line[0] == '#':
		output.write(line)
	else:
		output.write('#' + line)
		print 
		print 'line= ' + line,
#		lb = regex.compile( '[[]')
#		lbindex = lb.search(line)
#		print 'lbindex = ', lbindex

#		lbpos = regex.search("\[",line)
#		lbpos = -1
#		rbpos = regex.search("\]",line)
#		print 'lbpos,rbpos = ', lbpos,rbpos

		line = re.sub('\[','',line)
		line = re.sub('\]','',line)
		line = re.sub('ren1','ren',line)
		print '(new)line= ' + line,

		n = len(line)
		print 'n = ', n

		keys = string.split(line)
#		print 'length(keys) = ' + len(keys)
		keysLength = len(keys)
		if keysLength != 0:
			print keys

		if keysLength == 0:
			output.write(line)
#		 Catch some tcl-specific keywords and comment out
		elif keys[0] == 'catch':
			output.write( '#' + line)
		elif keys[0] == 'source':
			output.write( '#' + line)
		elif keys[0] == 'wm':
			output.write( '#' + line)

#		 Detect vtk class instance; Pythonize it.
		elif line[:3] == 'vtk':
			output.write( keys[1] + ' = ' + keys[0] + '()\n' )
		else:
#			for i in range(len(keys)-1):
			for i in range(len(keys)):
				lparen = 0
				if keys[i][:3] == 'Set' or keys[i][:3] == 'Add':
#					print '===Got a Set'
					output.write( keys[i] + '(' )
					lparen = 1
				elif keys[i][:3] == 'Get':
#					print '===Got a Get'
					output.write( keys[i] + '()' )
					print keys[i] + '()', 
					print 'i,len(keys)-1 = ', i,len(keys)-1
					if i < len(keys)-1:
						output.write( '.' )
						print '.' 
				else:
					if i < len(keys)-1:
#						print 'keys[i+1]=' + keys[i+1]
#						npos = regex.search(keys[i][1:1], "0123456789") 
#						if npos > -1:
#							print 'keys[i][0:1],npos=' + keys[i][0:1],npos
#							output.write( keys[i] + ',' )
#						else:
							output.write( keys[i] + '.' )
					else:
						output.write( keys[i] )

			if lparen == 0
				output.write( '(' )

#			 Terminating right paren.
#			output.write( keys[len(keys)-1] + ')\n' )
			output.write( ')\n' )

output.write('#!python\n')
output.write('\n')
output.write('from libVTKCommonPython import *\n')
output.write('from libVTKGraphicsPython import *\n')
output.write('\n')
for line in input.readlines():
	processLine(line)

output.write('iren.Initialize()\n')
output.write('iren.Start()\n')

