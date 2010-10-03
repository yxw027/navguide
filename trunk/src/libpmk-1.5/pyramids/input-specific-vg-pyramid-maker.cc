// Copyright 2007, Massachusetts Institute of Technology.
// The use of this code is permitted for research only. There is
// absolutely no warranty for this software.
//
// Author: John Lee (jjl@mit.edu)
//

#include <vector>
#include "pyramids/input-specific-vg-pyramid-maker.h"
#include "point_set/point-set-list.h"
#include "util/sparse-vector.h"
#include "histograms/multi-resolution-histogram.h"
#include "util/distance-computer.h"
#include "clustering/hierarchical-clusterer.h"

namespace libpmk {

InputSpecificVGPyramidMaker::
InputSpecificVGPyramidMaker(const HierarchicalClusterer& c,
                            const DistanceComputer& distance_computer) :
   VGPyramidMaker(c, distance_computer) { }

MultiResolutionHistogram* InputSpecificVGPyramidMaker::
MakePyramid(const PointSet& point_set) {
   MultiResolutionHistogram* pyramid = new MultiResolutionHistogram();

   for (int ii = 0; ii < point_set.GetNumFeatures(); ++ii) {
      // This describes the path down the hierarchical cluster tree that
      // this point is placed in.
      pair<LargeIndex, vector<double> > membership(
         GetMembershipPath(point_set[ii]));
      
      const LargeIndex& membership_path(membership.first);

      // Element jj of this vector is the distance from all_points[ii]
      // to its respective cluster center at the jj'th level.
      const vector<double>& distances(membership.second);

      LargeIndex path;
      Bin* finger;
      Bin root_bin(path);
      root_bin.SetCount(point_set[ii].GetWeight());
      root_bin.SetSize(distances.at(0));
      finger = pyramid->AddBin(root_bin);
      
      for (int jj = 1; jj < (int)membership_path.size(); ++jj) {
         path.push_back(membership_path[jj]);
         Bin new_bin(path);
         new_bin.SetCount(point_set[ii].GetWeight());
         new_bin.SetSize(distances.at(jj));
         finger = pyramid->AddBin(new_bin, finger);
      }
   }
   return pyramid;
}

}  // namespace libpmk
