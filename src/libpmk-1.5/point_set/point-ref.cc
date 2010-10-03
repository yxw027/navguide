// Copyright 2007, Massachusetts Institute of Technology.
// The use of this code is permitted for research only. There is
// absolutely no warranty for this software.
//
// Author: John Lee (jjl@mit.edu)
//

#include <assert.h>
#include "point_set/point-ref.h"
#include "point_set/point-set-list.h"
#include "point_set/feature.h"

namespace libpmk {

PointRef::PointRef(const PointSetList* data, int feature_index) :
   data_(data), feature_index_(feature_index) {
   pair<int, int> indices = data_->GetFeatureIndices(feature_index);
   which_point_set_ = indices.first;
   which_point_ = indices.second;
}

PointRef::PointRef(const PointSetList* data, int which_point_set,
                   int which_point) :
   data_(data), which_point_set_(which_point_set), which_point_(which_point) {
   int sum = 0;
   assert(which_point_set < data_->GetNumPointSets());
   for (int ii = 0; ii < which_point_set - 1; ++ii) {
      sum += data_->operator[](ii).GetNumFeatures();
   }
   sum += which_point;
   feature_index_ = sum;
}

PointRef::PointRef(const PointSetList* data, int feature_index,
                   int which_point_set, int which_point) :
   data_(data), feature_index_(feature_index),
   which_point_set_(which_point_set), which_point_(which_point) { }

const Feature& PointRef::GetFeature() const {
   return data_->operator[](which_point_set_)[which_point_];
}


int PointRef::GetWhichPointSet() const {
   return which_point_set_;
}

int PointRef::GetWhichPoint() const {
   return which_point_;
}

int PointRef::GetFeatureIndex() const {
   return feature_index_;
}

}  // namespace libpmk
