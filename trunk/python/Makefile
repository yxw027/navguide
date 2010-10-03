

LCM_FILES:=$(shell ls ../lcmtypes/*.lcm)

.PHONY: lcmtypes

all: lcmtypes

lcmtypes:
	lcm-gen --python $(LCM_FILES)
