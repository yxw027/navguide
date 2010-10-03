// Copyright 2007, Massachusetts Institute of Technology.
// The use of this code is permitted for research only. There is
// absolutely no warranty for this software.
//
// Author: John Lee (jjl@mit.edu)
//

#include <stdio.h>
#include <string>
#include <fstream>
#include <vector>
#include "util/sparse-vector.h"
#include "point_set/point-set-list.h"
#include "clustering/hierarchical-clusterer.h"

using namespace std;
using namespace libpmk;

int main(int argc, char** argv) {
   if (argc != 2) {
      printf("Usage: %s <input_hc_file>\n", argv[0]);
      exit(0);
   }

   HierarchicalClusterer hc;
   string input_file(argv[1]);
   
   ifstream input_stream(input_file.c_str(), ios::binary);
   hc.ReadFromStream(input_stream);
   input_stream.close();
   
   const PointSetList& centers(hc.GetClusterCenters());
   const vector<LargeIndex>& memberships(hc.GetMemberships());

   for (int ii = 0; ii < centers.GetNumPointSets(); ++ii) {
      printf("Level %d has %d centers:\n", ii, centers[ii].GetNumFeatures());
      for (int jj = 0; jj < centers[ii].GetNumFeatures(); ++jj) {
         printf("\t");
         for (int kk = 0; kk < centers[ii].GetFeatureDim(); ++kk) {
            printf("%f ", centers[ii][jj][kk]);
         }
         printf("\n");
      }
   }
   
   for (int ii = 0; ii < (int)memberships.size(); ++ii) {
      LargeIndex index = memberships[ii];
      printf("%d: ", ii);
      for (int jj = 0; jj < (int)index.size(); ++jj) {
         printf("%d ", index[jj]);
      }
      printf("\n");
   }
   return 0;
}
