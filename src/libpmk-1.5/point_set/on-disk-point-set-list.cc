// Copyright 2007, Massachusetts Institute of Technology.
// The use of this code is permitted for research only. There is
// absolutely no warranty for this software.
//
// Author: John Lee (jjl@mit.edu)
//

#include <assert.h>
#include <string>
#include <iostream>
#include <list>
#include "point_set/on-disk-point-set-list.h"
#include "point_set/point-set.h"
#include "point_set/point-ref.h"
#include "point_set/feature.h"

namespace libpmk {

const int OnDiskPointSetList::DEFAULT_LRU_CACHE_SIZE(1500);
const int OnDiskPointSetList::DEFAULT_AREA_CACHE_SIZE(1500);

OnDiskPointSetList::OnDiskPointSetList(string filename) : 
   max_lru_size_(DEFAULT_LRU_CACHE_SIZE),
   max_area_size_(DEFAULT_AREA_CACHE_SIZE) {
   input_stream_.open(filename.c_str(), ios::binary);
   assert(input_stream_.good());
   GetListInfo();
   SeekAndFillAreaCache(0);
}

OnDiskPointSetList::OnDiskPointSetList(string filename, int lru_cache_size,
                                       int area_cache_size) : 
   max_lru_size_(lru_cache_size), max_area_size_(area_cache_size) {
   assert(max_lru_size_ > 0);
   assert(max_area_size_ > 0);
   input_stream_.open(filename.c_str(), ios::binary);
   assert(input_stream_.good());
   GetListInfo();
   SeekAndFillAreaCache(0);
}

OnDiskPointSetList::~OnDiskPointSetList() {
   if (input_stream_.is_open()) {
      input_stream_.close();
   }
   lru_cache_.clear();
   area_cache_.clear();
}

void OnDiskPointSetList::GetListInfo() const {
   streampos old_position = input_stream_.tellg();
   
   input_stream_.clear();
   input_stream_.seekg(0, ios::beg);

   int32_t num_pointsets, feature_dim;
   input_stream_.read((char *)&num_pointsets, sizeof(int32_t));
   input_stream_.read((char *)&feature_dim, sizeof(int32_t));

   num_point_sets_ = num_pointsets;
   feature_dim_ = feature_dim;
   num_points_ = 0;
   pointers_.reserve(num_pointsets);
   num_features_.reserve(num_pointsets);

   // Read in each point set
   for (int ii = 0; ii < num_pointsets; ++ii) {
      pointers_[ii] = input_stream_.tellg();
      
      // Number of features in this point set
      int32_t num_features;
      input_stream_.read((char *)&num_features, sizeof(int32_t));
      num_points_ += num_features;
      num_features_[ii] = num_features;

      // Read all of the features in this point set
      input_stream_.ignore(num_features * feature_dim * sizeof(float));
     
      // Read the weights for this point set
      input_stream_.ignore(num_features * sizeof(float));
   }

   input_stream_.clear();
   input_stream_.seekg(old_position);
}

void OnDiskPointSetList::SeekAndFillAreaCache(int index) const {
   assert(index <= num_point_sets_);
   area_cache_offset_ = index;
   area_cache_.clear();
   input_stream_.clear();
   input_stream_.seekg(pointers_[index]);

   int end_index = index + max_area_size_ - 1;
   if (end_index >= num_point_sets_) {
      end_index = num_point_sets_ - 1;
   }

   
   // Fill up the cache until we hit the end
   for (int ii = index; ii <= end_index; ++ii) {
      PointSet p(feature_dim_);
      
      // Number of features in this point set
      int32_t num_features;
      input_stream_.read((char *)&num_features, sizeof(int32_t));

      // Read each of the features in this point set
      for (int jj = 0; jj < num_features; ++jj) {
         Feature feature(feature_dim_);

         for (int kk = 0; kk < feature_dim_; ++kk) {
            float value;
            input_stream_.read((char *)&value, sizeof(float));
            feature[kk] = value;
         }
         p.AddFeature(feature);
      }
     
      // Read the weights for this point set
      for (int jj = 0; jj < num_features; ++jj) {
         float value;
         input_stream_.read((char *)&value, sizeof(float));
         p[jj].SetWeight(value);
      }
      area_cache_.push_back(p);
   }
}



int OnDiskPointSetList::GetNumPointSets() const {
   return num_point_sets_;
}

int OnDiskPointSetList::GetFeatureDim() const {
   return feature_dim_;
}

bool OnDiskPointSetList::IsInAreaCache(int index) const {
   int start = area_cache_offset_;
   int end = area_cache_offset_ + (int)area_cache_.size() - 1;
   if (index >= start && index <= end) {
      return true;
   }
   return false;
}

const PointSet& OnDiskPointSetList::operator[](int index) const {
   assert(index < GetNumPointSets());

   // Is it in the area cache?
   if (IsInAreaCache(index)) {
      return area_cache_.at(index - area_cache_offset_);
   } else {
      // Check if it's in the LRU cache.
      list<pair<int, PointSet> >::iterator iter(lru_cache_.begin());

      // If it's there, return it, and move it up to the front of the
      // LRU, and return it.
      while (iter != lru_cache_.end()) {
         if (iter->first == index) {
            PointSet p(iter->second);
            lru_cache_.erase(iter);
            lru_cache_.push_front(pair<int, PointSet>(index, p));
            return lru_cache_.front().second;
         }
         ++iter;
      }

      // If not, add it to the front of the LRU cache, evicting an
      // element from the back if necessary, and also refill the area
      // cache with the new location.
      SeekAndFillAreaCache(index);
      lru_cache_.push_front(pair<int, PointSet>(index, area_cache_.at(0)));
      if ((int)lru_cache_.size() > max_lru_size_) {
         lru_cache_.pop_back();
      }
      return area_cache_.at(0);
   }
}

int OnDiskPointSetList::GetNumPoints() const {
   return num_points_;
}

vector<int> OnDiskPointSetList::GetSetCardinalities() const {
   return num_features_;
}

}  // namespace libpmk
