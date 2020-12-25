#coding=utf-8

# input: the file path of word vectors
# ouput: the test result 

import os
import numpy as np

vec_path = 'vectors.txt'

# params
score_type = 'cosin'
#score_type = 'euclidean'
little_vec_path = 'vectors.temp'
temp_dataset_path = 'dataset.temp'
dataset_path_list = [
	'./sim/dataset/wordsim353/combined.tab',
	'./sim/dataset/rg/rg.tab',
	'./sim/dataset/scws/scws.tab',
    './sim/dataset/rw/rw.tab',
    './sim/dataset/men/men',
    './sim/dataset/mt/mt',
    './sim/dataset/r122/r122',
    './sim/dataset/simlex999/sim999',
]

# get all words in dataset as a dict
def get_all_words_from_dataset(dataset_path, has_title=False):
	#print 'getting all words from ' + dataset_path
	all_words = set()
	lines = open(dataset_path, 'r').readlines()
	for i in range(len(lines)):
		if 0==i and has_title: 
			continue
		line_seg = lines[i].strip().lower().split()
		#line_seg = lines[i].strip().lower().split('\t')
		all_words.add(line_seg[0].strip())
		all_words.add(line_seg[1].strip())
	#ip_file.close()
	#print
	return all_words

def get_little_vec(vec_path, out_path, all_words):
	#print 'getting little vectors file from ' + vec_path
	vec_file = open(vec_path, 'r')
	op_file = open(out_path, 'w+')
	count = 0
	found_set = set()
	vec_count = 0
	for line in vec_file:
		vec_count += 1
		line_seg = line.split()
		if line_seg[0].strip() in all_words:
			found_set.add(line_seg[0].strip())
			op_file.write(line.strip()+'\n')
			count += 1
		if vec_count >=400000: break;# limit vocab as 400000
		#if vec_count >=484601: break;# limit vocab as 400000

	vec_file.close()
	op_file.close()
	#print 'get ' + str(count) + '/' + str(len(all_words))
	#print 'not found:'
	#print all_words - found_set
	#print
	return all_words - found_set

#get dataset
def get_dataset(dataset_path, has_title=False):
	lines = open(dataset_path, 'r').readlines()
	k = 1
	id2pair=[]
	id2score_test=[]
	for i in range(len(lines)):
		if 0==i and has_title:
			continue
		line_seg = lines[i].strip().lower().split()
		#line_seg = lines[i].strip().lower().split('\t')
		pair = {'key':k, 'word1':line_seg[0].strip(), 'word2':line_seg[1].strip()}
		score = {'key':k, 'score':float(line_seg[2]), 'rank':1}
		k += 1
		id2pair.append(pair)
		id2score_test.append(score)
	
	return id2pair, id2score_test

#get word2vec
def get_w2v(little_vecs_path):
	w2v={}
	lines = open(little_vecs_path, 'r').readlines()
	for i in range(len(lines)):
		line_seg = lines[i].strip().split()
		vec = np.asarray(line_seg[1:], dtype=np.float64)
		w2v[line_seg[0]] = vec
	return w2v	

#get old and new score
#def get_score(id2pair, w2v, score_type):
def get_score(id2pair, w2v):
	id2score_evl=[]
	for pair in id2pair:
		k = pair['key']
		vec1 = np.asarray(w2v[pair['word1']])
		vec2 = np.asarray(w2v[pair['word2']])
		if 'cosin' == score_type:
			score_value = np.dot(vec1, vec2)/np.sqrt(vec1.dot(vec1) * vec2.dot(vec2))
		elif 'euclidean' == score_type:
			diff_vec = vec1 - vec2
			score_value = -np.sqrt(diff_vec.dot(diff_vec))
		score = {'key':k, 'score':score_value, 'rank':1}
		id2score_evl.append(score)
	return id2score_evl

#sort
def cmp_score(item1, item2):
	s1 = float(item1['score'])
	s2 = float(item2['score'])
	if s2-s1>0:
		return 1
	elif s2-s1<0:
		return -1
	else:
		return 0

def cmp_id(item1, item2):
	id1 = int(item1['key'])
	id2 = int(item2['key'])
	return id1-id2

def get_rank(id2score):
	rank = 1
	for item in id2score:
		item['rank'] = rank
		rank += 1


def test_similarity(vecs_path, dataset_path, src_dataset_path):
	id2pair, id2score_official = get_dataset(dataset_path, True)
	w2v = get_w2v(vecs_path)
	#score_type = 'cosin'
	#score_type = 'euclidean'
	#print 'score type； ' + score_type
	id2score = get_score(id2pair, w2v)
	#id2score = get_score(id2pair, w2v, score_type)
	#print id2score_official

	#sort by score
	#print '\nsort by score'
	id2score_official.sort(cmp_score)
	id2score.sort(cmp_score)

	#get rank
	#print '\nget rank'
	get_rank(id2score_official)
	get_rank(id2score)

	#sort by id
	#print '\nsort by id'
	id2score_official.sort(cmp_id)
	id2score.sort(cmp_id)

	#evaluate similarity
	def eval_simi(id2score1, id2score2):
		assert len(id2score1)==len(id2score2)
		d_squre_sum = 0
		for i in range(len(id2score1)):
			d = id2score1[i]['rank'] - id2score2[i]['rank']
			d_squre_sum += d*d
		N = len(id2score1)
		evl = 1.0 - 6.0 * d_squre_sum / float(N * (N*N-1))
		return evl

	evl = eval_simi(id2score_official, id2score)
	print src_dataset_path + ':\t' + str(evl)


# main
print
print 'score type:\t' + score_type
for dataset_path in dataset_path_list:
	#print '\n\n\n----------' + dataset_path + '-----------'

	# get all words in dataset as a set
	all_words = get_all_words_from_dataset(dataset_path, True)

	# get little vectors file
	not_found_words = get_little_vec(vec_path, little_vec_path, all_words)

	# remove test items contain not-found-word
	# get true test dataset
	#if len(not_found_words) > 0:
	if True:
		has_title = True
		lines = open(dataset_path, 'r').readlines()
		op_file = open(temp_dataset_path, 'w+')
		for i in range(len(lines)):
			if 0==i and has_title:
				op_file.write(lines[i].strip()+'\n')
				continue
			line_seg = lines[i].strip().split()
			#line_seg = lines[i].strip().split('\t')
			if not (line_seg[0] in not_found_words or 
				line_seg[1] in not_found_words):
				op_file.write(lines[i].strip()+'\n')
		op_file.close()



	# test and output
	test_similarity(little_vec_path, temp_dataset_path, dataset_path)


	# remove temp files, little vector file, true test dataset
	if os.path.exists(little_vec_path):
		os.remove(little_vec_path)
	if os.path.exists(temp_dataset_path):
		os.remove(temp_dataset_path)
	# done
	#print '----------------------------------------\n\n\n'




