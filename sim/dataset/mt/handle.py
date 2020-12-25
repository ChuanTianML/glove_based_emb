#coding=utf-8
import csv


ip_path = 'Mtruk.csv'
op_path = 'mt'
op_file = open(op_path, 'w+')
op_file.write('word1\tword2\tscore\n')
csv_reader = csv.reader(open(ip_path, 'r'))
for row in csv_reader:
	op_oneline = str(row[0]) + '\t' + str(row[1]) + '\t' + str(row[2]) + '\n'
	op_file.write(op_oneline)
op_file.close()
print 'finish'
