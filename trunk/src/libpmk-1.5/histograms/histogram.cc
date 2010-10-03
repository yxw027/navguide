// Copyright 2007, Massachusetts Institute of Technology.
// The use of this code is permitted for research only. There is
// absolutely no warranty for this software.
//
// Author: John Lee (jjl@mit.edu)
//

#include <assert.h>
#include <iostream>
#include "histograms/bin.h"
#include "histograms/histogram.h"

namespace libpmk {

Histogram::Histogram() : bins_(0), is_sorted_(true) { }

Histogram::~Histogram() {
   for(int ii = 0; ii < (int)bins_.size(); ++ii) {
      delete bins_[ii];
   }
   bins_.clear();
}

Bin* Histogram::AddBin(const Bin& new_bin) {
   int last_index = 0;
   if (!bins_.empty()) {
      last_index = bins_.size() - 1;
   }
   Bin* added = new Bin(new_bin.GetIndex());
   added->SetSize(new_bin.GetSize());
   added->SetCount(new_bin.GetCount());
   bins_.push_back(added);

   if (!bins_.empty()) {
      if (is_sorted_ && (added->GetIndex() < bins_[last_index]->GetIndex())) {
         is_sorted_ = false;
      }
   }

   return added;
}

void Histogram::SortBins() const {
   if (is_sorted_) {
      return;
   }

   sort(bins_.begin(), bins_.end(), Bin::CompareNodes);
   
   // Consolidate duplicate bins:
   for (vector<Bin*>::iterator iter = bins_.begin();
        iter + 1 != bins_.end(); ) {
      vector<Bin*>::iterator next = iter + 1;
      Bin* current = *iter;
      Bin* nextbin = *next;
      if ((next != bins_.end()) &&
          (nextbin->GetIndex() == current->GetIndex())) {
         current->SetCount(current->GetCount() + nextbin->GetCount());
         double max_size = current->GetSize() > nextbin->GetSize() ?
            current->GetSize() : nextbin->GetSize();
         current->SetSize(max_size);
         delete nextbin;
         bins_.erase(next);
      } else {
         ++iter;
      }
      
   }
   is_sorted_ = true;
}

double Histogram::ComputeIntersection(Histogram* first, Histogram* second) {
   double intersection = 0;
   first->SortBins();
   second->SortBins();

   vector<Bin*>::iterator iter1 = first->bins_.begin();
   vector<Bin*>::iterator iter2 = second->bins_.begin();
   while (iter1 != first->bins_.end() &&
          iter2 != second->bins_.end()) {
      // If the indices are not equal, increment the iterator with the
      // smaller index.
      const LargeIndex& index1 = (*iter1)->GetIndex();
      const LargeIndex& index2 = (*iter2)->GetIndex();
      if (index1 < index2) {
         ++iter1;
      } else if (index2 > index1) {
         ++iter2;
      } else {
         // If the indices are equal, add to the score and increment both
         // iterators.
         assert((*iter1)->GetSize() == (*iter2)->GetSize());
         double minimum = ((*iter1)->GetCount()) < ((*iter2)->GetCount()) ?
            ((*iter1)->GetCount()) : ((*iter2)->GetCount());
         intersection += ((*iter1)->GetSize()) * minimum;
         ++iter1;
         ++iter2;
      }
   }
   return intersection;
}

// File format:
// (int32_t) num_bins
// Then, num_bins Bins, which are:
//   int32_t index_size
//   (index_size * int32_t) the index
//   double size
//   double count
// The bins are written in sorted order.
void Histogram::WriteSingleHistogramToStream(ostream& output_stream,
                                             Histogram* h) {
   assert(output_stream.good());

   h->SortBins();
   int32_t num_bins = h->GetNumBins();
   output_stream.write((char *)&num_bins, sizeof(int32_t));

   for (int ii = 0; ii < h->GetNumBins(); ++ii) {
      Bin* current = h->bins_[ii];
      int32_t index_size = current->GetIndex().size();
      double size = current->GetSize();
      double count = current->GetCount();
      output_stream.write((char *)&index_size, sizeof(int32_t));
      for (int ii = 0; ii < index_size; ++ii) {
         int32_t element = current->GetIndex()[ii];
         output_stream.write((char *)&element, sizeof(int32_t));
      }
      output_stream.write((char *)&size, sizeof(double));
      output_stream.write((char *)&count, sizeof(double));
   }
}

Histogram* Histogram::ReadSingleHistogramFromStream(istream& input_stream) {
   assert(input_stream.good());

   Histogram* result = new Histogram();
   int32_t num_bins = 0;
   input_stream.read((char *)&num_bins, sizeof(int32_t));

   for (int ii = 0; ii < num_bins; ++ii) {
      int32_t index_size;
      LargeIndex index;
      double size;
      double count;
      
      input_stream.read((char *)&index_size, sizeof(int32_t));
      for (int jj = 0; jj < index_size; ++jj) {
         int32_t index_element;
         input_stream.read((char *)&index_element, sizeof(int32_t));
         index.push_back(index_element);
      }
      
      input_stream.read((char *)&size, sizeof(double));
      input_stream.read((char *)&count, sizeof(double));
      Bin temp(index);
      temp.SetSize(size);
      temp.SetCount(count);
      result->AddBin(temp);      
   }
   result->is_sorted_ = true;
   return result;
}

void Histogram::WriteToStream(ostream& output_stream,
                              const vector<Histogram*>& hists) {
   assert(output_stream.good());

   int32_t num_hists = hists.size();
   output_stream.write((char *)&num_hists, sizeof(int32_t));
   
   for (int ii = 0; ii < (int)hists.size(); ++ii) {
      WriteSingleHistogramToStream(output_stream, hists[ii]);
   }
}

vector<Histogram*> Histogram::ReadFromStream(istream& input_stream) {
   assert(input_stream.good());
   vector<Histogram*> histograms;

   int32_t num_hists;
   input_stream.read((char *)&num_hists, sizeof(int32_t));

   for (int ii = 0; ii < num_hists; ++ii) {
      Histogram* next = ReadSingleHistogramFromStream(input_stream);
      histograms.push_back(next);
   }
   return histograms;
}

int Histogram::GetNumBins() const {
   SortBins();
   return bins_.size();
}

const Bin* Histogram::GetBin(int ii) {
   SortBins();
   return bins_.at(ii);
}

const Bin* Histogram::GetBin(const LargeIndex& index) {
   SortBins();
   return GetBin(index, 0);
}

const Bin* Histogram::GetBin(const LargeIndex& index, int finger) {
   assert(finger < GetNumBins());
   SortBins();
   for (int ii = finger; ii < GetNumBins(); ++ii) {
      const Bin* bin = bins_[ii];
      if (bin->GetIndex() < index) {
         return NULL;
      } else if (bin->GetIndex() == index) {
         return bin;
      }
   }
   return NULL;
}
}  // namespace libpmk
