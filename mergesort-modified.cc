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
void sort_base_case(Iterator xbegin, Iterator xend, Iterator ybegin, Iterator yend){
	
	auto begin = xbegin;
	std::vector<long> xvector;
	std::vector<long> yvector;
	std::copy(xbegin,xend,xvector.begin());
	std::copy(ybegin,yend,yvector.begin());
	auto x = xvector.begin();
	auto y = yvector.begin();
		while(begin != yend){
			if(x==xvector.end()){
			}			
			else if(y==yvector.end()){
			}
			else if(*x <= *y){
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
template<typename Iterator, typename Compare>
void sort_ranges(Iterator x1begin, Iterator x1end, Iterator y1begin, Iterator y1end, Iterator x2end, Iterator y2end){
	auto begin = x1begin;
	std::vector<long> x1vector;
	std::vector<long> y1vector;
	std::vector<long> x2vector;
	std::vector<long> y2vector;
	std::copy(x1begin,x1end,x1vector.begin());
	std::copy(y1begin,y1end,y1vector.begin());
	std::copy(x1end,x2end,x2vector.begin());
	std::copy(y1end,y2end,y2vector.begin());
	auto x1 = x1vector.begin();
	auto y1 = x2vector.begin();
	auto x2 = y1vector.begin();
	auto y2 = y2vector.begin();
	while(begin != y2end){
		if(x1 == x2vector.end()){
			*begin = *y1;
			y1 = (y1++ != y1vector.end()) ? y1++ : y2;
		}
		else if(y1 == y2vector.end()){
			*begin = *x1;
			x1 = (x1++ != x1vector.end()) ? x1++ : x2;
		}
		else if(*x1 <= *y1){
			*begin = *x1;
			x1 = (x1++ != x1vector.end()) ? x1++ : x2;
		}
		else{
			*begin = *y1;
			y1 = (y1++ != y1vector.end()) ? y1++ : y2;
		}
		// Esto puede petar seriamente
		begin = (begin++ != x1end) ? begin++ : y1begin;
		begin = (begin++ != y1end) ? begin++ : x1end;
		begin = (begin++ != x2end) ? begin++ : y1end;
		
	}
	
}
template<typename Iterator, typename Compare>
void final_sort(Iterator x1begin, Iterator x1end, Iterator x2end, Iterator y1end, Iterator y2end){
	auto begin = x1begin;
	std::vector<long> x1vector;
	std::vector<long> y1vector;
	std::vector<long> x2vector;
	std::vector<long> y2vector;
	std::copy(x1begin,x1end,x1vector.begin());
	std::copy(x2end,y1end,y1vector.begin());
	std::copy(x1end,x2end,x2vector.begin());
	std::copy(y1end,y2end,y2vector.begin());
	auto x1 = x1vector.begin();
	auto y1 = x2vector.begin();
	auto x2 = y1vector.begin();
	auto y2 = y2vector.begin();
	while(begin != y2end){
		if(x1 == x2vector.end()){
			*begin = *y1;
			y1 = (y1++ != y1vector.end()) ? y1++ : y2;
		}
		else if(y1 == y2vector.end()){
			*begin = *x1;
			x1 = (x1++ != x1vector.end()) ? x1++ : x2;
		}
		else if(*x1 <= *y1){
			*begin = *x1;
			x1 = (x1++ != x1vector.end()) ? x1++ : x2;
		}
		else{
			*begin = *y1;
			y1 = (y1++ != y1vector.end()) ? y1++ : y2;
		}
		// Esto puede petar seriamente
		begin ++;
		
	}
}
template<typename Iterator, typename Compare>
void aux_parallel_merge(Iterator begin, Iterator middle, Iterator end, Compare cmp){

	size_t n1 = std::distance(begin,middle);
	size_t n2 = std::distance(middle,end);
	auto xmiddle = begin;
	std::advance(begin,n1/2);
	auto ymiddle = std::upper_bound(middle,end,xmiddle,cmp);
	/*if( n1 <= MERGE_GRANULARITY || n2 <= MERGE_GRANULARITY){
			sort_base_case(begin,xmiddle,ybegin,yend);	
	}
	else {*/
		auto left= mare::create_task([=]{parallel_inplace_merge(begin,xmiddle,middle,ymiddle,cmp);});
		auto right = mare::create_task([=]{parallel_inplace_merge(xmiddle,middle,ymiddle,end,cmp);});
		//Funcion de merge de dos regiones no continuas
		auto merge = mare::create_task([=]{sort_ranges(begin,xmiddle,middle,ymiddle,middle,end);});
		mare::launch(left);
		mare::launch(right);
		left >> merge;
		right >> merge;
		mare::finish_after(merge);
	//}
	final_sort(begin,xmiddle,middle,ymiddle,end);


}

template<typename Iterator, typename Compare>
void parallel_inplace_merge(Iterator xbegin, Iterator xend,Iterator ybegin, Iterator yend, Compare cmp){

size_t n1 = std::distance(xbegin,xend);
size_t n2 = std::distance(ybegin,yend);
auto xmiddle = xbegin;
auto ymiddle = ybegin;
std::advance(xmiddle,n1/2);
ymiddle = std::upper_bound(ybegin,yend,*xmiddle,cmp);

	if( n1 <= MERGE_GRANULARITY || n2 <= MERGE_GRANULARITY){
		sort_base_case(xbegin,xend,ybegin,yend);	
	}
	else {
		auto left= mare::create_task([=]{parallel_inplace_merge(xbegin,xmiddle,ybegin,ymiddle,cmp);});
		auto right = mare::create_task([=]{parallel_inplace_merge(xmiddle,xend,ymiddle,yend,cmp);});
		//Funcion de merge de dos regiones no continuas
		auto merge = mare::create_task([=]{sort_ranges(xbegin,xmiddle,ybegin,ymiddle,xend,yend);});
		mare::launch(left);
		mare::launch(right);
		left >> merge;
		right >> merge;
		mare::finish_after(merge);
	}
}
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
      mare::create_task([=]{aux_parallel_merge(begin, middle, end, cmp);});
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
