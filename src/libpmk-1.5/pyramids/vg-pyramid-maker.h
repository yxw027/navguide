// Copyright 2007, Massachusetts Institute of Technology.
// The use of this code is permitted for research only. There is
// absolutely no warranty for this software.
//
// Author: John Lee (jjl@mit.edu)
//

#ifndef PYRAMIDS_VG_PYRAMID_MAKER_H
#define PYRAMIDS_VG_PYRAMID_MAKER_H

#include "pyramids/pyramid-maker.h"
#include "point_set/point-set.h"
#include "point_set/point-set-list.h"
#include "point_set/feature.h"
#include "util/sparse-vector.h"
#include "histograms/multi-resolution-histogram.h"
#include "util/distance-computer.h"
#include "clustering/hierarchical-clusterer.h"

namespace libpmk {
/// Abstract interface for vocabulary-guided pyramid makers.
/**
 * The vocabulary-guided pyramid makers here use the output of a
 * hierarchical clustering algorithm to generate pyramids.
 */ 
class VGPyramidMaker : public PyramidMaker {
 public:
   /**
    * Requires a HierarchicalCluster which has already been
    * Preprocess()'d (or ReadFromStream()), and a DistanceComputer
    * which computes the same distances that were used to generate the
    * HierarchicalClusterer's data.
    */
   VGPyramidMaker(const HierarchicalClusterer& c,
                  const DistanceComputer& distance_computer);
   
   virtual MultiResolutionHistogram*
      MakePyramid(const PointSet& point_set) = 0;

 protected:
   const HierarchicalClusterer& clusterer_;

   /// The centers extracted from clusterer_.
   const PointSetList& centers_;
   const DistanceComputer& distance_computer_;

   /// \brief For a new feature, find which bin it would be placed in
   /// according to the HierarchicalClusterer.
   /**
    * The LargeIndex returned is the same size as the number of levels
    * in the tree. Each element of the LargeIndex tells you which
    * element in #centers_ that point belongs to. That is, if the
    * returned LargeIndex is [0 3 9], then centers_[0][0] is the root
    * bin, centers_[1][3] is the center corresponding to this point at
    * level 1, centers_[2][9] is the center corresponding to this
    * point at level 2.
    *
    * The returned vector of doubles gives the distance to the
    * corresponding center at each level, according to
    * #distance_computer_.
    */
   pair<LargeIndex, vector<double> > GetMembershipPath(const Feature& f);
};
}  // namespace libpmk

#endif  // PYRAMIDS_INPUT_SPECIFIC_VG_PYRAMID_MAKER_H
