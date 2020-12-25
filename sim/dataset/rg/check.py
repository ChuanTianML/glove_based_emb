data_path = 'rg.tab'

data_file = open(data_path, 'r')
for line in data_file:
	line_seg = line.split('\t')
	if not 3==len(line_seg):
		print line_seg
	
