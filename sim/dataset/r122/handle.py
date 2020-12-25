#coding=utf-8

ip_file = open('rel122-norms.txt', 'r')
op_file = open('r122', 'w+')
op_file.write('word1\tword2\tscore\n')
for line in ip_file:
	line_sp = line.split()
	op_line = line_sp[1] + '\t' + line_sp[3] + '\t' + line_sp[0] + '\n'
	op_file.write(op_line)
ip_file.close()
op_file.close()
print 'finish'
