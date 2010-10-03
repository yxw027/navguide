// Copyright 2007, Massachusetts Institute of Technology.
// The use of this code is permitted for research only. There is
// absolutely no warranty for this software.
//
// Author: John Lee (jjl@mit.edu)
//

#include <stdio.h>
#include <string>
#include <fstream>
#include "kernel/kernel-matrix.h"
#include "point_set/on-disk-point-set-list.h"

using namespace std;
using namespace libpmk;

int main(int argc, char** argv) {
   if (argc != 2) {
      printf("Usage: %s <input_point_set_list>\n",
             argv[0]);
      exit(0);
   }

   string input_file(argv[1]);
   
   OnDiskPointSetList psl(input_file);

   for (int zz = 0; zz < psl.GetNumPointSets(); ++zz) {
      printf("POINT SET %d OF %d (%d points)\n", zz + 1,
             psl.GetNumPointSets(),
             psl[zz].GetNumFeatures());
      for (int ii = 0; ii < psl[zz].GetNumFeatures(); ++ii) {
         for (int dd = 0; dd < psl[zz].GetFeatureDim(); ++dd) {
            printf("\t%f", psl[zz][ii][dd]);
         }
         printf("\n");
      }
      printf("\n\n");
   }
   return 0;
}
