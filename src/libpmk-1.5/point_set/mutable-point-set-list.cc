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
#include "point_set/mutable-point-set-list.h"
#include "point_set/point-set.h"
#include "point_set/point-ref.h"
#include "point_set/feature.h"

namespace libpmk {

MutablePointSetList::~MutablePointSetList() {
   list_.clear();
}

void MutablePointSetList::ReadFromFile(const char* filename) {
   ifstream input_stream(filename, ios::binary);
   ReadFromStream(input_stream);
   input_stream.close();
}

void MutablePointSetList::ReadFromStream(istream& input_stream) {
   list_.clear();

   assert(input_stream.good());
   int32_t num_pointsets, feature_dim;

   input_stream.read((char *)&num_pointsets, sizeof(int32_t));
   input_stream.read((char *)&feature_dim, sizeof(int32_t));

   // Read in each point set
   for (int ii = 0; ii < num_pointsets; ++ii) {
      PointSet p(feature_dim);
      p.ReadFromStream(input_stream);
      list_.push_back(p);
   }
}

void MutablePointSetList::ReadSelectionFromStream(istream& input_stream,
                                                  int start,
                                                  int selection_size) {
   assert(input_stream.good());
   int32_t num_pointsets, feature_dim;
   
   input_stream.read((char *)&num_pointsets, sizeof(int32_t));
   input_stream.read((char *)&feature_dim, sizeof(int32_t));

   int end = start + selection_size - 1;
   assert(start >= 0);
   assert(start < num_pointsets);
   assert(end < num_pointsets);
   assert(start <= end);
   

   fprintf(stderr, "Reading %d point sets with dim %d\n",
           selection_size, feature_dim);

   // Read in each point set
   for (int ii = 0; ii < num_pointsets; ++ii) {

      // If this point set is in the selection:
      if (ii >= start && ii <= end) {
         PointSet p(feature_dim);
         
         // Number of features in this point set
         int num_features;
         input_stream.read((char *)&num_features, sizeof(int32_t));
         
         // Read each of the features in this point set
         for (int jj = 0; jj < num_features; ++jj) {
            Feature feature(feature_dim);
            
            for (int kk = 0; kk < feature_dim; ++kk) {
               float value;
               input_stream.read((char *)&value, sizeof(float));
               feature[kk] = value;
            }
            p.AddFeature(feature);
         }
         
         // Read the weights for this point set
         for (int jj = 0; jj < num_features; ++jj) {
            float value;
            input_stream.read((char *)&value, sizeof(float));
            p[jj].SetWeight(value);
         }
         list_.push_back(p);
      } else if (ii > end) { // if we're past the end of the selection
         break;
      } else { // if we're before the start of the selection
         int num_features;
         input_stream.read((char *)&num_features, sizeof(int32_t));

         // Ignore each of the features in this point set
         int num_values_to_ignore = feature_dim * num_features;
         input_stream.ignore(num_values_to_ignore * sizeof(float));

         // Ignore each of the weights
         input_stream.ignore(num_features * sizeof(float));
      }
   }
}

void MutablePointSetList::AddPointSet(const PointSet& point_set) {
   list_.push_back(point_set);
}

int MutablePointSetList::GetNumPointSets() const {
   return list_.size();
}

int MutablePointSetList::GetFeatureDim() const {
   return list_.at(0).GetFeatureDim();
}

PointSet& MutablePointSetList::operator[](int index) {
   assert(index < GetNumPointSets());
   return list_.at(index);
}

const PointSet& MutablePointSetList::operator[](int index) const {
   assert(index < GetNumPointSets());
   return list_.at(index);
}

vector<int> MutablePointSetList::GetSetCardinalities() const {
   vector<int> cardinalities;
   for (int ii = 0; ii < GetNumPointSets(); ++ii) {
      cardinalities.push_back(list_.at(ii).GetNumFeatures());
   }
   return cardinalities;
}

int MutablePointSetList::GetNumPoints() const {
   int count = 0;
   for (int ii = 0; ii < (int)list_.size(); ++ii) {
      count += list_.at(ii).GetNumFeatures();
   }
   return count;
}
}  // namespace libpmk
