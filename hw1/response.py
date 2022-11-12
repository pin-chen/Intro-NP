with open('response.txt', 'r') as f:
	for str in f:
		if len(str) > 0 and str[0] == '(':
			arg = str.split()
			print("#define " + arg[1] + " " + arg[0][1:4])
