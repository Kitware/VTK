def scanner(name,function):
	file = open(name,'r')
	while 1:
		line = file.readline()
		if not line: break
		function(line)
	file.close()
