#coding=utf-8

ip_file = open('SimLex-999.txt', 'r')
op_file = open('sim999', 'w+')
op_file.write('word1\tword2\tscore\n')
idx = 0
for line in ip_file:
	if 0==idx: idx += 1
	else:
		line_sp = line.split()
		op_file.write(line_sp[0] + '\t' + line_sp[1] + '\t' + line_sp[3] + '\n')
		idx += 1
op_file.close()
ip_file.close()
print 'total pairs: ' + str(idx-1)
print 'finish.'

