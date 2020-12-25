
ip_file = open('rw.txt', 'r')
op_file = open('rw.tab', 'w+')
op_file.write('word1\tword2\tscore\n')
for line in ip_file:
	line_seg = line.lower().strip().split('\t')
	w1 = line_seg[0]
	w2 = line_seg[1]
	sc = line_seg[2]
	op_file.write(w1 + '\t' + w2 + '\t' + sc + '\n')
ip_file.close()
op_file.close()
