// Copyright 2007, Massachusetts Institute of Technology.
// The use of this code is permitted for research only. There is
// absolutely no warranty for this software.
//
// Author: John Lee (jjl@mit.edu)
//

#include <assert.h>
#include <iostream>
#include "point_set/point-set.h"
#include "point_set/feature.h"

namespace libpmk {

PointSet::PointSet(int feature_dim) : feature_dim_(feature_dim) { }

int PointSet::GetNumFeatures() const {
   return features_.size();
}

int PointSet::GetFeatureDim() const {
   return feature_dim_;
}

Feature PointSet::GetFeature(int index) const {
   assert(features_[index].GetDim() == feature_dim_);
   return features_.at(index);
}

void PointSet::AddFeature(const Feature& feature) {
   assert(feature.GetDim() == feature_dim_);
   features_.push_back(feature);
}

void PointSet::SetFeature(int index, const Feature& feature) {
   assert(index < GetNumFeatures());
   assert(feature.GetDim() == feature_dim_);
   features_.at(index) = feature;
}

void PointSet::RemoveFeature(int index) {
   assert(index < GetNumFeatures());
   features_.erase(features_.begin() + index);
}

Feature& PointSet::operator[](int index) {
   assert(index < GetNumFeatures());
   return features_.at(index);
}

const Feature& PointSet::operator[](int index) const {
   assert(index < GetNumFeatures());
   return features_.at(index);
}

void PointSet::WriteToStream(ostream& output_stream) const {
   // Write each point set:
   int32_t num_features = GetNumFeatures();
   int feature_dim = GetFeatureDim();
   output_stream.write((char *)(&num_features), sizeof(int32_t));
   
   // Write the features in this point set:
   for (int jj = 0; jj < num_features; ++jj) {
      const Feature& feature = features_[jj];
      for (int kk = 0; kk < feature_dim; ++kk) {
         float value = feature[kk];
            output_stream.write((char *)&value, sizeof(float));
      }
   }
   
   // Write the weights for this point set:
   for (int jj = 0; jj < num_features; ++jj) {
      float value = features_[jj].GetWeight();
      output_stream.write((char *)&value, sizeof(float));
   }
}

void PointSet::ReadFromStream(istream& input_stream) {
   features_.clear();
   int32_t num_features;
   input_stream.read((char *)(&num_features), sizeof(int32_t));

   for (int ii = 0; ii < num_features; ++ii) {
      Feature f(feature_dim_);
      for (int kk = 0; kk < feature_dim_; ++kk) {
         float value;
         input_stream.read((char *)(&value), sizeof(float));
         f[kk] = value;
      }
      features_.push_back(f);  
   }

   for (int ii = 0; ii < num_features; ++ii) {
      float value;
      input_stream.read((char *)(&value), sizeof(float));
      features_[ii].SetWeight(value);
   }

}
}  // namespace libpmk
