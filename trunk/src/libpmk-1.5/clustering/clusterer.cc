// Copyright 2007, Massachusetts Institute of Technology.
// The use of this code is permitted for research only. There is
// absolutely no warranty for this software.
//
// Author: John Lee (jjl@mit.edu)
//

#include <assert.h>
#include <vector>
#include <iostream>
#include <fstream>
#include "clustering/clusterer.h"
#include "point_set/point-set.h"
#include "point_set/point-ref.h"

using namespace std;

namespace libpmk {

Clusterer::Clusterer() : done_(false) { }

PointSet Clusterer::GetClusterCenters() const {
   assert(done_);
   // Copy the data in cluster_centers_ to a new point set.
   PointSet to_return(cluster_centers_->GetFeatureDim());
   for (int ii = 0; ii < cluster_centers_->GetNumFeatures(); ++ii) {
      to_return.AddFeature(cluster_centers_->operator[](ii));
   }
   return to_return;
}

int Clusterer::GetNumCenters() const {
   assert(done_);
   return cluster_centers_->GetNumFeatures();
}

vector<int> Clusterer::GetMembership() const {
   assert(done_);
   return membership_;
}

void Clusterer::Cluster(const vector<PointRef>& data) {
   assert((int)data.size() > 0);
   cluster_centers_.reset(new PointSet(data[0].GetFeature().GetDim()));
   membership_.resize((int)data.size());
   DoClustering(data);
}


// Output format:
// (int32_t) num centers, c
// (int32_t) num points,  p
// (int32_t) feature dim, f
//   (f * float) feature_1
//   (f * float) feature_2
//   ...
//   (f * float) feature_c
//   (int32_t) membership_1
//   (int32_t) membership_2
//   ...
//   (int32_t) membership_p
// 
// 
void Clusterer::WriteToStream(ostream& output_stream) const {
   assert(done_);
   assert(output_stream.good());
   int32_t num_centers = (int32_t)cluster_centers_->GetNumFeatures();
   int32_t num_points = (int32_t)membership_.size();
   int32_t feature_dim = (int32_t)cluster_centers_->GetFeatureDim();
   output_stream.write((char*)&num_centers, sizeof(int32_t));
   output_stream.write((char*)&num_points, sizeof(int32_t));
   output_stream.write((char*)&feature_dim, sizeof(int32_t));
   for (int ii = 0; ii < num_centers; ++ii) {
      const Feature& feature = cluster_centers_->GetFeature(ii);
      for (int jj = 0; jj < feature_dim; ++jj) {
         float value = feature[jj];         
         output_stream.write((char *)&value, sizeof(float));
      }
   }

   for (int ii = 0; ii < num_points; ++ii) {
      int32_t member = membership_[ii];
      output_stream.write((char *)&member, sizeof(int32_t));
   }
}

void Clusterer::WriteToFile(const char* filename) const {
   ofstream output_stream(filename, ios::binary | ios::trunc);
   WriteToStream(output_stream);
   output_stream.close();
}

void Clusterer::ReadFromFile(const char* filename) {
   ifstream input_stream(filename, ios::binary);
   ReadFromStream(input_stream);
   input_stream.close();
}

void Clusterer::ReadFromStream(istream& input_stream) {
   assert(input_stream.good());
   int32_t num_centers, num_points, feature_dim;
   input_stream.read((char*)&num_centers, sizeof(int32_t));
   input_stream.read((char*)&num_points, sizeof(int32_t));
   input_stream.read((char*)&feature_dim, sizeof(int32_t));

   cluster_centers_.reset(new PointSet(feature_dim));
   membership_.resize(num_points);
   
   for (int ii = 0; ii < num_centers; ++ii) {
      Feature feature(feature_dim);
      for (int jj = 0; jj < feature_dim; ++jj) {
         float read_data;
         input_stream.read((char*)&read_data, sizeof(float));
         feature[jj] = read_data;
      }
      cluster_centers_->AddFeature(feature);
   }

   for (int ii = 0; ii < num_points; ++ii) {
      int32_t member;
      input_stream.read((char*)&member, sizeof(int32_t));
      membership_[ii] = member;
   }
   done_ = true;
}

}  // namespace libpmk
