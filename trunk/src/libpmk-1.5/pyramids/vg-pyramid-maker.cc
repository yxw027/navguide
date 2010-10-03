// Copyright 2007, Massachusetts Institute of Technology.
// The use of this code is permitted for research only. There is
// absolutely no warranty for this software.
//
// Author: John Lee (jjl@mit.edu)
//
#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <values.h>
#include "pyramids/vg-pyramid-maker.h"
#include "point_set/point-set.h"
#include "point_set/feature.h"
#include "util/sparse-vector.h"
#include "histograms/multi-resolution-histogram.h"
#include "util/distance-computer.h"

namespace libpmk {

VGPyramidMaker::VGPyramidMaker(const HierarchicalClusterer& clusterer,
                               const DistanceComputer& distance_computer) :
   clusterer_(clusterer), centers_(clusterer.GetClusterCenters()),
   distance_computer_(distance_computer) { }

pair<LargeIndex, vector<double> > VGPyramidMaker::
GetMembershipPath(const Feature& f) {
   LargeIndex answer;
   vector<double> distances;

   // For each level, find the bin center closest to f, and add it
   // to <answer>. We can skip the first computation since it's always 0.
   answer.push_back(0);
   distances.push_back(distance_computer_.ComputeDistance(f, centers_[0][0]));

   for (int ii = 1; ii < centers_.GetNumPointSets(); ++ii) {
      double min_distance = DBL_MAX;
      int best_index = -1;

      pair<int, int> range = clusterer_.GetChildRange(ii - 1, answer.back());
      assert(range.first >= 0);
      assert(range.second >= 0);
      for (int jj = range.first; jj <= range.second; ++jj) {
         double distance =
            distance_computer_.ComputeDistance(f, centers_[ii][jj],
                                               min_distance);
         if (distance < min_distance) {
            min_distance = distance;
            best_index = jj;
         }
      }
      assert(best_index != -1);
      answer.push_back(best_index);
      distances.push_back(min_distance);
   }

   assert(answer.size() == distances.size());
   return pair<LargeIndex, vector<double> >(answer, distances);
}

}  // namespace libpmk
