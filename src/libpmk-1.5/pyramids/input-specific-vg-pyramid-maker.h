// Copyright 2007, Massachusetts Institute of Technology.
// The use of this code is permitted for research only. There is
// absolutely no warranty for this software.
//
// Author: John Lee (jjl@mit.edu)
//
#ifndef PYRAMIDS_INPUT_SPECIFIC_VG_PYRAMID_MAKER_H
#define PYRAMIDS_INPUT_SPECIFIC_VG_PYRAMID_MAKER_H

#include <vector>
#include "pyramids/vg-pyramid-maker.h"
#include "point_set/point-set-list.h"
#include "util/sparse-vector.h"
#include "histograms/multi-resolution-histogram.h"
#include "util/distance-computer.h"
#include "clustering/hierarchical-clusterer.h"

namespace libpmk {
/// Makes pyramids with bin sizes that are specific to each input.
class InputSpecificVGPyramidMaker : public VGPyramidMaker {
 public:
   InputSpecificVGPyramidMaker(const HierarchicalClusterer& c,
                               const DistanceComputer& distance_computer);
   virtual MultiResolutionHistogram* MakePyramid(const PointSet& point_set);
};
}  // namespace libpmk

#endif  // PYRAMIDS_INPUT_SPECIFIC_VG_PYRAMID_MAKER_H
