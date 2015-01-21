// --~--~--~--~----~--~--~--~----~--~--~--~----~--~--~--~----~--~--~--~--
// Copyright 2013-2014 Qualcomm Technologies, Inc.  All rights reserved.
// Confidential & Proprietary – Qualcomm Technologies, Inc. ("QTI")
// 
// The party receiving this software directly from QTI (the "Recipient")
// may use this software as reasonably necessary solely for the purposes
// set forth in the agreement between the Recipient and QTI (the
// "Agreement").  The software may be used in source code form solely by
// the Recipient's employees (if any) authorized by the Agreement.
// Unless expressly authorized in the Agreement, the Recipient may not
// sublicense, assign, transfer or otherwise provide the source code to
// any third party.  Qualcomm Technologies, Inc. retains all ownership
// rights in and to the software.
// --~--~--~--~----~--~--~--~----~--~--~--~----~--~--~--~----~--~--~--~--
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <algorithm>
#include <functional>
#include <iostream>
#include <iterator>
#include <sstream>
#include <vector>

#include <mare/mare.h>

// Parallel mergesort using recursive fork-join parallelism.
// mare::finish_after allows easy expression of the parallelism in the
// algorithm in a non-blocking manner, yielding better performance than
// blocking parallelization using mare::wait_for.

/// Controls size of largest task executed sequentially
const size_t GRANULARITY = 8192;
//Granularities close to 0 may cause crashes
const size_t MERGE_GRANULARITY = 20;
// Asynchronous mergesort, to be invoked in a task
template<typename Iterator, typename Compare>
void
mergesort(Iterator begin, Iterator end, Compare cmp)
{
  size_t n = std::distance(begin, end);
  if (n <= GRANULARITY) {
    sort(begin, end, cmp);
  } else {
    auto middle = begin;
    std::advance(middle, n / 2);
    auto left = mare::create_task([=]{mergesort(begin, middle, cmp);});
    auto right = mare::create_task([=]{mergesort(middle, end, cmp);});
    auto merge =
      mare::create_task([=]{std::inplace_merge(begin, middle, end, cmp);});
    // The left subtree and right subtree tasks must finish before the merge
    // task can execute
    left >> merge; right >> merge;
    mare::launch(left);
    mare::launch(right);
    mare::launch(merge);
    // mergesort(begin, end, cmp) logically finishes after the merge task
    // finishes
    mare::finish_after(merge);
  }
}

void sort_base_case(Iterator xbegin, Iterator xend, Iterator ybegin, Iterator yend){
	
	auto begin = xbegin;
	std::vector<long> xvector;
	std::vector<long> yvector;
	std::copy(xbegin,xend,xvector.begin());
	std::copy(ybegin,yend,yvector.begin());
	x = xvector.begin();
	y = yvector.begin()

	while(begin != yend){
		if(*x <= *y){
			*begin = *x;
			x++;
		}
		else{
			*begin = *y;
			y++;
		}
		// Esto puede petar seriamente
		begin = (begin++ == xend) ? begin++ : ybegin;
	}
}
void sort_ranges(Iterator x1begin, Iterator x1end, Iterator y1begin, Iterator y1end, Iterator x2end, Iterator y2end){
	std::cout << "Not implemented yet";
}
void parallel_inplace_merge(Iterator xbegin, Iterator xend,Iterator ybegin, Iterator yend, Compare cmp){

size_t n1 = std::distance(begin,middle);
size_t n2 = std::distance(middle,end);
auto xmiddle = std::advance(begin,n1/2);
auto ymiddle = std::upper_bound(middle,end,xmiddle,cmp);

	if( n1 <= MERGE_GRANULARITY || n2 <= MERGE_GRANULARITY){
		sort_base_case(xbegin,xend,ybegin,yend);	
	}
	else {
		auto lef= mare::create_task([=]{parallel_inplace_merge(xbegin,xmiddle,ybegin,ymiddle,cmp);});
		auto right = mare::create_task([=]{parallel_inplace_merge(xmiddle,xend,ymiddle,yend,cmp);});
		//Funcion de merge de dos regiones no continuas
		auto merge = mare::create_task([=]{sort_ranges(xbegin,xmiddle,ybegin,ymiddle,xend,yend;});
		mare::launch(left);
		mare::launch(right);
		left >> merge;
		right >> merge;
		mare::finish_after(merge);
	}
}
int
main(int argc, const char* argv[])
{
  std::vector<long> input;
  size_t n_def = 1 << 16;
  size_t n = n_def;

  if (argc >= 2) {
    std::istringstream istr(argv[1]);
    istr >> n;
  }

  // Create a random array of integers
  for (size_t i = 0; i < n; i++) {
    input.push_back(rand());
  }

  // Launch mergesort inside a task since it has an asynchronous interface (due
  // to use of mare::finish_after)
  auto t = mare::create_task([&]{mergesort(input.begin(),
                                  input.end(), std::less<long>());});
  mare::launch(t);
  mare::wait_for(t);

  if (!std::is_sorted(input.begin(), input.end())) {
    std::cerr << "parallel mergesorting failed\n";
  }

  return 0;
}
