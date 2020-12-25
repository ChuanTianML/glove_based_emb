//  Tool to calculate word-word cooccurrence statistics
//
//  Copyright (c) 2014 The Board of Trustees of
//  The Leland Stanford Junior University. All Rights Reserved.
//
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.
//
//
//  For more information, bug reports, fixes, contact:
//    Jeffrey Pennington (jpennin@stanford.edu)
//    GlobalVectors@googlegroups.com
//    http://nlp.stanford.edu/projects/glove/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define TSIZE 1048576
#define SEED 1159241

static const int MAX_STRING_LENGTH = 1000;
typedef double real;

typedef struct cooccur_rec {
	int word1;
	int word2;
	real val;
} CREC;

typedef struct cooccur_rec_id {
	int word1;
	int word2;
	real val;
	int id;
} CRECID;

typedef struct hashrec {
	char	*word;
	long long id;
	struct hashrec *next;
} HASHREC;

int verbose = 2; // 0, 1, or 2
//long long max_product; // Cutoff for product of word frequency ranks below which cooccurrence counts will be stored in a compressed full array
//long long overflow_length; // Number of cooccurrence records whose product exceeds max_product to store in memory before writing to disk
//int window_size = 15; // default context window size
//int symmetric = 1; // 0: asymmetric, 1: symmetric
//real memory_limit = 3; // soft limit, in gigabytes, used to estimate optimal array sizes
//char *vocab_file, *file_head;

//TC: param
char *src_cooc, *dep_cooc; // 2 files' names
real dep_weight; // the weight of denpendency
int src_cooc_indx = 0;
int dep_cooc_indx = 1;

/* Efficient string comparison */
int scmp( char *s1, char *s2 ) {
	while(*s1 != '\0' && *s1 == *s2) {s1++; s2++;}
	return(*s1 - *s2);
}

/* Move-to-front hashing and hash function from Hugh Williams, http://www.seg.rmit.edu.au/code/zwh-ipl/ */

/* Check if two cooccurrence records are for the same two words */
int compare_crecid(CRECID a, CRECID b) {
	int c;
	if( (c = a.word1 - b.word1) != 0) return c;
	else return a.word2 - b.word2;
}

/* Swap two entries of priority queue */
void swap_entry(CRECID *pq, int i, int j) {
	CRECID temp = pq[i];
	pq[i] = pq[j];
	pq[j] = temp;
}

/* Insert entry into priority queue */
void insert(CRECID *pq, CRECID new, int size) {
	int j = size - 1, p;
	pq[j] = new;
	while( (p=(j-1)/2) >= 0 ) {
		if(compare_crecid(pq[p],pq[j]) > 0) {swap_entry(pq,p,j); j = p;}
		else break;
	}
}

/* Delete entry from priority queue */
void delete(CRECID *pq, int size) {
	int j, p = 0;
	pq[p] = pq[size - 1];
	while( (j = 2*p+1) < size - 1 ) {
		if(j == size - 2) {
			if(compare_crecid(pq[p],pq[j]) > 0) swap_entry(pq,p,j);
			return;
		}
		else {
			if(compare_crecid(pq[j], pq[j+1]) < 0) {
				if(compare_crecid(pq[p],pq[j]) > 0) {swap_entry(pq,p,j); p = j;}
				else return;
			}
			else {
				if(compare_crecid(pq[p],pq[j+1]) > 0) {swap_entry(pq,p,j+1); p = j + 1;}
				else return;
			}
		}
	}
}

/* Write top node of priority queue to file, accumulating duplicate entries */
int merge_write(CRECID new, CRECID *old, FILE *fout) {
	if(new.word1 == old->word1 && new.word2 == old->word2) {
		old->val += new.val;
		return 0; // Indicates duplicate entry
	}
	fwrite(old, sizeof(CREC), 1, fout);
	*old = new;
	return 1; // Actually wrote to file
}

/* Merge [num = 2] sorted files of cooccurrence records */
int merge_two_files() {
	fprintf(stderr, "MERGE COOCCORENCE\n");
	int num = 2;
	int i, size;
	long long counter = 0;
	CRECID *pq, new, old;
	char filename[200];
	FILE **fid, *fout;
	fid = malloc(sizeof(FILE) * num);
	pq = malloc(sizeof(CRECID) * num);
	fout = stdout;
	if(verbose > 1) fprintf(stderr, "Merging cooccurrence files: processed 0 lines.");

	//fprintf(stderr, "run to line 147+-");
	/* Open all files and add first entry of each to priority queue */
	for(i = 0; i < num; i++) {
		//sprintf(filename,"%s_%04d.bin",file_head,i);
		if(src_cooc_indx==i) sprintf(filename,"%s", src_cooc);
		else if(dep_cooc_indx==i) sprintf(filename,"%s", dep_cooc);
		fid[i] = fopen(filename,"rb");
		if(fid[i] == NULL) {fprintf(stderr, "Unable to open file %s.\n",filename); return 1;}
		fread(&new, sizeof(CREC), 1, fid[i]);
		if(dep_cooc_indx==i) new.val *= dep_weight;
		new.id = i;
		insert(pq,new,i+1);
	}

	//fprintf(stderr, "run to line 160+-");
	/* Pop top node, save it in old to see if the next entry is a duplicate */
	size = num;
	old = pq[0];
	i = pq[0].id;
	delete(pq, size);
	fread(&new, sizeof(CREC), 1, fid[i]);
	if(dep_cooc_indx==i) new.val *= dep_weight;
	if(feof(fid[i])) size--;
	else {
		new.id = i;
		insert(pq, new, size);
	}

	//fprintf(stderr, "run to line 174+-");
	/* Repeatedly pop top node and fill priority queue until files have reached EOF */
	while(size > 0) {
		counter += merge_write(pq[0], &old, fout); // Only count the lines written to file, not duplicates
		if((counter%100000) == 0) if(verbose > 1) fprintf(stderr,"\033[39G%lld lines.",counter);
		i = pq[0].id;
		delete(pq, size);
		fread(&new, sizeof(CREC), 1, fid[i]);
		if(dep_cooc_indx==i) new.val *= dep_weight;
		if(feof(fid[i])) size--;
		else {
			new.id = i;
			insert(pq, new, size);
		}
	}
	//fprintf(stderr, "run to line 190+-");
	fwrite(&old, sizeof(CREC), 1, fout);
	fprintf(stderr,"\033[0GMerging cooccurrence files: processed %lld lines.\n",++counter);
	//for(i=0;i<num;i++) {
	//    sprintf(filename,"%s_%04d.bin",file_head,i);
	//    remove(filename);
	//}
	//fprintf(stderr, "run to line 197+-");
	fprintf(stderr,"\n");
	return 0;
}

int find_arg(char *str, int argc, char **argv) {
	//fprintf(stderr, "go in find_arg: %s\n", str);
	int i;
	for (i = 1; i < argc; i++) {
		if(!scmp(str, argv[i])) {
			if (i == argc - 1) {
				printf("No argument given for %s\n", str);
				exit(1);
			}
			return i;
		}
	}
	return -1;
}

int main(int argc, char **argv) {
	int i;
	src_cooc = malloc(sizeof(char) * MAX_STRING_LENGTH);
	dep_cooc = malloc(sizeof(char) * MAX_STRING_LENGTH);

	if (argc == 1) {
		printf("Tool to merge 2 cooccurrence files\n");
		printf("Author: TianChuan\n\n");
		printf("Usage options:\n");
		printf("\t-verbose <int>\n");
		printf("\t\tSet verbosity: 0, 1, or 2 (default)\n");
		printf("\t-source_cooc_filename <file>\n");
		printf("\t\ttc: to do.\n");
		printf("\t-depend_cooc_filename <file>\n");
		printf("\t\ttc: to do.\n");
		printf("\nExample usage:\n");
		printf("\ttc: to do.\n");
		return 0;
	}

	if ((i = find_arg((char *)"-verbose", argc, argv)) > 0) verbose = atoi(argv[i + 1]);

	//TC: get params
	if ((i = find_arg((char *)"-source_cooc", argc, argv)) > 0) strcpy(src_cooc, argv[i + 1]);
	if ((i = find_arg((char *)"-depend_cooc", argc, argv)) > 0) strcpy(dep_cooc, argv[i + 1]);
	if ((i = find_arg((char *)"-depend_weight", argc, argv)) > 0) dep_weight = atof(argv[i + 1]);

	return merge_two_files();
}

