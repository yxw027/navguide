// Copyright 2007, Massachusetts Institute of Technology.
// The use of this code is permitted for research only. There is
// absolutely no warranty for this software.
//
// Author: John Lee (jjl@mit.edu)
//

#include <assert.h>
#include "point_set/feature.h"

namespace libpmk {

Feature::Feature(int dim) : weight_(1), feature_(dim, 0) { }

Feature::~Feature() {
   feature_.clear();
}

int Feature::GetDim() const {
   return feature_.size();
}

float Feature::GetValue(int index) const {
   return feature_.at(index);
}

float& Feature::operator[](int index) {
   return feature_.at(index);
}

float Feature::operator[](int index) const {
   return feature_.at(index);
}

float Feature::GetWeight() const {
   return weight_;
}

float Feature::SetWeight(float weight) {
   float old_weight = weight_;
   weight_ = weight;
   return old_weight;
}

}  // namespace libpmk
