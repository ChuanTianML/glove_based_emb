#!/bin/bash

make
VOCAB_FILE=/home/tianchuan/NLP/depend/data/wiki2017/src_cooc/src_weight/vocab.txt
COOCCURRENCE_FILE_SRC=/home/tianchuan/NLP/depend/data/wiki2017/src_cooc/src_weight/cooccurrence.src.bin
COOCCURRENCE_FILE_DEP=/home/tianchuan/NLP/depend/data/wiki2017/depend_cooc/cooccurrence.dep.bin
COOCCURRENCE_FILE_MERGE=cooccurrence.merge.bin
COOCCURRENCE_SHUF_FILE_MERGE=cooccurrence.shuf.merge.bin
BUILDDIR=build
SAVE_FILE=vectors
VERBOSE=2
MEMORY=4.0
VOCAB_MIN_COUNT=5
VECTOR_SIZE=50
MAX_ITER=50
WINDOW_SIZE=10
BINARY=2
NUM_THREADS=24
X_MAX=100
DEP_WEIGHT=1.0 # the weight of dependency

cp $VOCAB_FILE ./
$BUILDDIR/merge -verbose $VERBOSE -source_cooc $COOCCURRENCE_FILE_SRC -depend_cooc $COOCCURRENCE_FILE_DEP -depend_weight $DEP_WEIGHT > $COOCCURRENCE_FILE_MERGE
if [[ $? -eq 0 ]]; then
	$BUILDDIR/shuffle -memory $MEMORY -verbose $VERBOSE < $COOCCURRENCE_FILE_MERGE > $COOCCURRENCE_SHUF_FILE_MERGE # shuffle merged cooccurence
	if [[ $? -eq 0 ]]; then
		rm $COOCCURRENCE_FILE_MERGE
		if [[ $? -eq 0 ]]; then
			$BUILDDIR/glove -save-file $SAVE_FILE -threads $NUM_THREADS -input-file $COOCCURRENCE_SHUF_FILE_MERGE -x-max $X_MAX -iter $MAX_ITER -vector-size $VECTOR_SIZE -binary $BINARY -vocab-file $VOCAB_FILE -verbose $VERBOSE
			if [[ $? -eq 0 ]]; then
				rm $COOCCURRENCE_SHUF_FILE_MERGE
				rm vectors.bin
				python eval/python/evaluate.py
				python sim/test_sim.py
				python sim/test_sim_cosin.py
			fi
		fi
	fi
fi



