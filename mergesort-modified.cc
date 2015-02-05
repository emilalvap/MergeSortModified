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
#include <time.h>
#include <mare/mare.h>

// Parallel mergesort using recursive fork-join parallelism.
// mare::finish_after allows easy expression of the parallelism in the
// algorithm in a non-blocking manner, yielding better performance than
// blocking parallelization using mare::wait_for.

/// Controls size of largest task executed sequentially
//const size_t GRANULARITY = 8192;
const size_t GRANULARITY = 8192;
//Granularities close to 0 may cause crashes

const size_t MERGE_GRANULARITY = 8192;
// Asynchronous mergesort, to be invoked in a task
template<typename Iterator, typename Compare>
void parallel_inplace_merge(Iterator begin, Iterator middle, Iterator end, Compare cmp){
size_t n1 = std::distance(begin,middle)/2;
size_t n2 = std::distance(middle,end)/2;

auto xmiddle = begin;
auto ymiddle = middle;
std::advance(xmiddle,n1);
ymiddle = std::upper_bound(middle,end,*xmiddle,cmp);

if(n1 <= MERGE_GRANULARITY || n2 <= MERGE_GRANULARITY){
	inplace_merge(begin,middle,end);	
}
	else{
		//swap
		//Copia auxiliar de x2
		std::vector<long> v;
		auto x = xmiddle;
		while(x!= middle){
			v.push_back(*x);
			x++;
		}
		
		auto y = middle;
		x = xmiddle;
		while(y!=ymiddle){
			*x = *y;
			x++;y++;
		}
		auto fin_izquierda = x;
		auto it = v.begin();
		while(it!= v.end()){
			*x=*it;
			x++;it++;
		}
		auto medio_derecha = x;
		//Recursive call
		auto left= mare::create_task([=]{parallel_inplace_merge(begin,xmiddle,fin_izquierda,cmp);});
		auto right = mare::create_task([=]{parallel_inplace_merge(fin_izquierda,medio_derecha,end,cmp);});
		auto merge = mare::create_task([=]{int w =0;});		
		left >> merge; right >> merge;
		mare::launch(left);
		mare::launch(right);
		mare::launch(merge);
		mare::finish_after(merge);
	}
//}
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
      mare::create_task([=]{parallel_inplace_merge(begin, middle, end, cmp);});
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
  clock_t t_ini, t_fin;

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
  t_ini = clock();
  mare::launch(t);
  mare::wait_for(t);
  t_fin = clock();

  if (!std::is_sorted(input.begin(), input.end())) {
    std::cout << "parallel mergesorting failed\n";

  }else{
	std::cout << "parallel mergesorting success\n";
	double seconds =  (double)(t_fin - t_ini)/ CLOCKS_PER_SEC;
	std::cout << "took " << seconds << " seconds\n";
	}

  return 0;




  

}
