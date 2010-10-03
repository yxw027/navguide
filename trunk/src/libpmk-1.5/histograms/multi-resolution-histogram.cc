// Copyright 2007, Massachusetts Institute of Technology.
// The use of this code is permitted for research only. There is
// absolutely no warranty for this software.
//
// Author: John Lee (jjl@mit.edu)
//
// All siblings are sorted by index.

#include <assert.h>
#include <string>
#include <iostream>
#include <fstream>
#include "histograms/multi-resolution-histogram.h"
#include "histograms/bin.h"
#include "util/sparse-vector.h"
#include "util/tree.cc"

using namespace std;

namespace libpmk {
int MultiResolutionHistogram::GetNumBins() const {
   return tree_.GetNumNodes();
}

Bin* MultiResolutionHistogram::GetRootBin() {
   return tree_.GetRootNode();
}

Bin* const MultiResolutionHistogram::GetRootBin() const {
   return tree_.GetRootNode();
}

Bin* MultiResolutionHistogram::GetBin(LargeIndex index) {
   return tree_.GetNode(index);
}

Bin* MultiResolutionHistogram::GetBin(LargeIndex index, Bin* finger) {
   return tree_.GetNode(index, finger);
}

void MultiResolutionHistogram::
WriteSingleHistogramToStream(ostream& output_stream,
                             const MultiResolutionHistogram* h) {
   h->tree_.WriteToStream(output_stream);
}


// Automatically increments num_bins_.
Bin* MultiResolutionHistogram::AddBin(const Bin& new_bin) {
   return tree_.AddNode(new_bin);
}

Bin* MultiResolutionHistogram::AddBin(const Bin& new_bin, Bin* parent) {
   return tree_.AddNode(new_bin, parent);
}


void MultiResolutionHistogram::DeleteBin(Bin* finger) {
   tree_.DeleteNode(finger);
}

MultiResolutionHistogram* MultiResolutionHistogram::
ReadSingleHistogramFromStream(istream& input_stream) {
   assert(input_stream.good());
   MultiResolutionHistogram* result = new MultiResolutionHistogram();
   result->tree_.ReadFromStream(input_stream);
   return result;
}


void MultiResolutionHistogram::
IgnoreSingleHistogramFromStream(istream& input_stream) {
   Bin ignored;
   ignored.ReadFromStream(input_stream);
}

vector<MultiResolutionHistogram*>
MultiResolutionHistogram::ReadFromStream(istream& input_stream) {
   assert(input_stream.good());
   vector<MultiResolutionHistogram*> histograms;

   int32_t num_hists;
   input_stream.read((char *)&num_hists, sizeof(int32_t));

   for (int ii = 0; ii < num_hists; ++ii) {
      MultiResolutionHistogram* next =
         ReadSingleHistogramFromStream(input_stream);
      histograms.push_back(next);
   }
   return histograms;
}

vector<MultiResolutionHistogram*>
MultiResolutionHistogram::ReadFromFile(const char* filename) {
   ifstream input_stream(filename, ios::binary);
   vector<MultiResolutionHistogram*>
      result(ReadFromStream(input_stream));
   input_stream.close();
   return result;
}

vector<MultiResolutionHistogram*>
MultiResolutionHistogram::ReadSelectionFromStream(istream& input_stream,
                                                  int start,
                                                  int selection_size) {
   assert(input_stream.good());
   vector<MultiResolutionHistogram*> histograms;
   int32_t num_hists;
   input_stream.read((char *)&num_hists, sizeof(int32_t));

   int end = start + selection_size - 1;
   assert(start >= 0);
   assert(start < num_hists);
   assert(end < num_hists);
   assert(start <= end);
   
   for (int ii = 0; ii < num_hists; ++ii) {
      if (ii >= start && ii <= end) {
         MultiResolutionHistogram* next =
            ReadSingleHistogramFromStream(input_stream);
         histograms.push_back(next);
      } else {
         IgnoreSingleHistogramFromStream(input_stream);
      }
   }
   return histograms;
}

vector<MultiResolutionHistogram*>
MultiResolutionHistogram::ReadSelectionFromFile(const char* filename,
                                                int start,
                                                int selection_size) {
   ifstream input_stream(filename, ios::binary);
   vector<MultiResolutionHistogram*>
      result(ReadSelectionFromStream(input_stream, start, selection_size));
   input_stream.close();
   return result;
}

void MultiResolutionHistogram::
WriteToStream(ostream& output_stream,
              const vector<MultiResolutionHistogram*>& hists) {
   assert(output_stream.good());
   int32_t num_hists = hists.size();
   output_stream.write((char *)&num_hists, sizeof(int32_t));

   for (int ii = 0; ii < (int)hists.size(); ++ii) {
      WriteSingleHistogramToStream(output_stream, hists[ii]);
   }
}

void MultiResolutionHistogram::
WriteToFile(const char* filename,
            const vector<MultiResolutionHistogram*>& hists) {
   ofstream output_stream(filename, ios::binary | ios::trunc);
   WriteToStream(output_stream, hists);
   output_stream.close();
}


}  // namespace libpmk
