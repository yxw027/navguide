// Copyright 2007, Massachusetts Institute of Technology.
// The use of this code is permitted for research only. There is
// absolutely no warranty for this software.
//
// Author: John Lee (jjl@mit.edu)
//

#include <assert.h>
#include <string>
#include <iostream>
#include <fstream>
#include "point_set/point-set-list.h"
#include "point_set/point-set.h"
#include "point_set/point-ref.h"
#include "point_set/feature.h"

namespace libpmk {

void PointSetList::WriteToFile(const char* output_file) const {
   ofstream output_stream(output_file, ios::binary | ios::trunc);
   WriteToStream(output_stream);
   output_stream.close();
}

void PointSetList::WriteToStream(ostream& output_stream) const {
   WriteHeaderToStream(output_stream);
   WritePointSetsToStream(output_stream);
}

void PointSetList::WriteHeaderToStream(ostream& output_stream) const {
   assert(output_stream.good());
   int32_t num_pointsets = GetNumPointSets();
   int32_t feature_dim = GetFeatureDim();

   // Header: num point sets and feature dim
   output_stream.write((char *)&num_pointsets, sizeof(int32_t));
   output_stream.write((char *)&feature_dim, sizeof(int32_t));

}

void PointSetList::WritePointSetsToStream(ostream& output_stream) const {
   assert(output_stream.good());
   assert(GetNumPointSets() > 0);
   int32_t num_pointsets = GetNumPointSets();

   for (int ii = 0; ii < num_pointsets; ++ii) {
      const PointSet& point_set = this->operator[](ii);
      point_set.WriteToStream(output_stream);
   }
}

const Feature& PointSetList::GetFeature(int index) const {
   pair<int, int> indices = GetFeatureIndices(index);
   return this->operator[](indices.first)[indices.second];
}

pair<int, int> PointSetList::GetFeatureIndices(int index) const {
   int sum = 0;
   bool found = false;
   int which_point_set = 0;
   int which_point = 0;
   int previous_sum = 0;

   // On the iith iteration, sum should contain the total number of points
   // in the 0 to iith point sets.
   for (int ii = 0; ii < GetNumPointSets(); ++ii) {
      sum += this->operator[](ii).GetNumFeatures();
      if (sum > index) {
         which_point_set = ii;
         which_point = index - previous_sum;
         found = true;
         break;
      }
      previous_sum = sum;
   }

   assert(found);
   return pair<int, int>(which_point_set, which_point);
}

void PointSetList::GetPointRefs(vector<PointRef>* out_refs) const {
   assert(out_refs != NULL);
   out_refs->clear();
   int feature_index = 0;
   int which_point_set = 0;
   int which_point = 0;
   for (int ii = 0; ii < GetNumPointSets(); ++ii) {
      which_point_set = ii;
      for (int jj = 0; jj < this->operator[](ii).GetNumFeatures(); ++jj) {
         which_point = jj;
         PointRef new_ref(this, feature_index, which_point_set, which_point);
         out_refs->push_back(new_ref);
         ++feature_index;
      }
   }
}

}  // namespace libpmk
