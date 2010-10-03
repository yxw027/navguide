CC = g++
LD = ld
CFLAGS = -Wall -Werror -O3
LDFLAGS = -lm 
LIBPMK_NAME = libpmk.o
LIBPMK_UTIL_NAME = libpmk_util.o

POINTSET_OBJ = \
	point_set/feature.o \
	point_set/point-set.o \
	point_set/point-set-list.o \
	point_set/point-ref.o \
	point_set/mutable-point-set-list.o \
	point_set/on-disk-point-set-list.o

UTIL_OBJ = \
	util/sparse-vector.o \
	util/labeled-index.o \
	util/distance-computer.o \
	util/tree-node.o \
	util/tree.o

HISTOGRAMS_OBJ = \
	histograms/bin.o \
	histograms/multi-resolution-histogram.o \
	histograms/histogram.o

PYRAMIDS_OBJ = \
	pyramids/pyramid-maker.o \
	pyramids/uniform-pyramid-maker.o \
	pyramids/vg-pyramid-maker.o \
	pyramids/global-vg-pyramid-maker.o \
	pyramids/pyramid-matcher.o \
	pyramids/input-specific-vg-pyramid-maker.o

KERNEL_OBJ = \
	kernel/matrix.o \
	kernel/kernel-matrix.o

SVM_OBJ = \
	svm/svm.o

EXPERIMENT_OBJ = \
	experiment/experiment.o \
	experiment/svm-experiment.o \
	experiment/prob-svm-experiment.o \
	experiment/example-selector.o \
	experiment/random-selector.o

CLUSTERING_OBJ = \
	clustering/clusterer.o \
	clustering/k-means-clusterer.o \
	clustering/hierarchical-clusterer.o 

TOOLS = \
	tools/random-sampler.out \
	tools/random-sampling-merger.out \
	tools/pointset-merger.out \
	tools/pointsets-to-uniform-pyramids.out \
	tools/hierarchical-cluster-point-set.out \
	tools/pyramid-match-kernel.out \
	tools/clusters-to-pyramids.out
DIAGNOSTIC = \
	diagnostic/pointset-viewer.out \
	diagnostic/pyramid-viewer.out \
	diagnostic/hc-viewer.out \
	diagnostic/cluster-viewer.out \
	diagnostic/kernel-viewer.out

LIBPMK_OBJS = \
	$(POINTSET_OBJ) \
	$(UTIL_OBJ) \
	$(HISTOGRAMS_OBJ) \
	$(PYRAMIDS_OBJ) \
	$(CLUSTERING_OBJ)

LIBPMK_UTIL_OBJS = \
	$(KERNEL_OBJ) \
	$(SVM_OBJ) \
	$(EXPERIMENT_OBJ)

ALLOBJS = \
	$(LIBPMK_OBJS) \
	$(LIBPMK_UTIL_OBJS) \

INC = -I.

libpmk: $(LIBPMK_OBJS)
	$(LD) -r -o $(LIBPMK_NAME) $(LIBPMK_OBJS)

libpmk_util: $(LIBPMK_UTIL_OBJS)
	$(LD) -r -o $(LIBPMK_UTIL_NAME) $(LIBPMK_UTIL_OBJS)

all: $(LIBPMK_OBJS) $(LIBPMK_UTIL_OBJS)
	$(LD) -r -o $(LIBPMK_NAME) $(LIBPMK_OBJS); done
	$(LD) -r -o $(LIBPMK_UTIL_NAME) $(LIBPMK_UTIL_OBJS)

objs: $(ALLOBJS)
tools: $(ALLOBJS) $(TOOLS)
diag: $(ALLOBJS) $(DIAGNOSTIC)

.cc.o:
	$(CC) $(CFLAGS) $(INC) -c $< -o $@

.c.o:
	$(CC) $(CFLAGS) $(INC) -c $< -o $@

.cc.out:
	$(CC) $(CFLAGS) $(INC) $(LDFLAGS) $(ALLOBJS) $< -o $@

clean:
	rm -f $(ALLOBJS)
	rm -f $(TOOLS)
	rm -f $(LIBPMK_NAME) $(LIBPMK_UTIL_NAME)
	rm -f $(DIAGNOSTIC)
