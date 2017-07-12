/*

Copyright (c) 2005-2016, University of Oxford.
All rights reserved.

University of Oxford means the Chancellor, Masters and Scholars of the
University of Oxford, having an administrative office at Wellington
Square, Oxford OX1 2JD, UK.

This file is part of Aboria.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.
 * Neither the name of the University of Oxford nor the names of its
   contributors may be used to endorse or promote products derived from this
   software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/


#ifndef NEIGHBOURS_H_
#define NEIGHBOURS_H_

#include <cxxtest/TestSuite.h>
#include <chrono>
typedef std::chrono::system_clock Clock;

#include "Aboria.h"

using namespace Aboria;

class NeighboursTest : public CxxTest::TestSuite {
public:

    void test_documentation(void) {
//[neighbour_search
/*`
[section Neighbourhood Searching]

The [classref Aboria::Particles] container gives you neighbourhood searching 
functionality, using a variety of spatial data structures as described below.
All these data structure can be used in any number of dimensions, with arbitrary
periodicity. Any neighbour search is performed within a hypercube domain, with 
extents specified by the user.

To start with, we will create a particle set in three dimensions (the default) 
containing a few randomly placed particles
*/

        const size_t N = 100;
        typedef Particles<> particle_type;
        typedef particle_type::position position;
        particle_type particles(N);
        std::default_random_engine gen; 
        std::uniform_real_distribution<double> uniform(-1,1);
        for (int i=0; i<N; ++i) {
            get<position>(particles)[i] = double3(uniform(gen),uniform(gen),uniform(gen));
        }

/*`

Before you can use the neighbourhood searching, you need to initialise the
domain using the [memberref Aboria::Particles::init_neighbour_search] function.

In this case, we will initialise a domain from $(-1,-1,-1)$ to $(1,1,1)$, which
is periodic in all directions.

*/

        double3 min(-1);
        double3 max(1);
        bool3 periodic(true);
        particles.init_neighbour_search(min,max,periodic);
/*`

Once this is done you can begin using the neighbourhood search queries using the 
[funcref Aboria::euclidean_search] function. This returns a lightweight container with `begin()` and `end()` functions that return `const` forward only iterators to the particles that satisfy the neighbour search. For example, the following counts all the
particles within a distance `radius` of the point $(0,0,0)$. 

*/

        double radius = 0.2;
        int count = 0;
        for (const auto& i: euclidean_search(particles.get_query(),double3(0),radius)) {
            count++;
        }
        std::cout << "There are "<< count << " particles.\n";

/*`

Note that [funcref Aboria::euclidean_search] uses the euclidean or L2-norm distance ($\sqrt{\sum_i^d x^2}$), but there are other functions for other distance norms. [funcref Aboria::manhatten_search] uses the L1-norm ($\sum_i^d |x|$), [funcref Aboria::chebyshev_search] uses the Linf-norm ($\max_i^d |x|$), and you can use the generic [funcref Aboria::distance_search] for the Ln-norm ($(\sum_i^d x^n)^{1/n}$)

When dereferenced, the neighbourhood iterator returns a tuple of size 2 
containing 

# A constant reference to the found particle object, with type
`particle_type::const_reference`

# A vector $\mathbf{dx}\_{ij}$ pointing to the found point from the query 
point. I.e. if $\mathbf{x}\_i$ is the query point and $\mathbf{x}\_j$ is the
found point, then $\mathbf{dx}\_{ij} = \mathbf{x}\_j - \mathbf{x}\_i$.

The latter is useful for periodic domains, the returned vector
$\mathbf{dx}\_{ij}$ takes periodic domains into account and returns the
$\mathbf{dx}\_{ij}$ with the smallest length. 

For example, 

*/

        for (const auto& i: euclidean_search(particles.get_query(),double3(0),radius)) {
            particle_type::const_reference b = std::get<0>(i);
            const double3& dx = std::get<1>(i);
            std::cout << "Found a particle with dx = " << dx << " and id = " << get<id>(b) << "\n";
        }

/*`

[section Cell Lists]

There are two cell list data structures within Aboria. Both divide the domain into 
a regular grid of hypercubes with side length set so that the average number of 
particles within each box is close to a given value. Each particle in the container is assigned to the cell that contains its position, and neighbourhood queries search within that cell and its neighbours within the given radius. 

For example, the following diagram illustrates a cell list data structure in two dimensions, shown as a regular array of grey squares each containing zero or more particles. The user wishes to find all the particles within a given euclidean distance around the red point. To accomplish this query efficiently, Aboria would then search all the red-shaded cells for particles that fall within the red circle.

[$images/neighbour/cell_lists.svg] 

The first cell list data structure supports serial insertion of particles, and parallel
queries. The relevant classes are [classref Aboria::bucket_search_serial] and [classref Aboria::bucket_search_serial_query]. This data structure can be selected on a per-particle-set basis, by setting the fourth template argument for [classref Aboria::Particles]. I.e.

*/

        typedef Particles<std::tuple<>,3,std::vector,bucket_search_serial> particle_bs_serial_type;

/*`

You will notice that we also need to specify the vector data structure that the particle
container uses, which in this case is a `std::vector`.

The alternative is a cell-list data structure that supports parallel insertion of 
points, and parallel queries. This constantly re-orders the particles in the particle 
container so that they are sorted into individual cells, so if particles are changing 
cells often this can be slower. But theoretically (this hasn't been tested yet) this 
should speed up neighbourhood search queries as the particles that are local in memory
are also local in space. The relevant classes are [classref Aboria::bucket_search_parallel] and [classref Aboria::bucket_search_parallel_query], and you can use this data structure like so:

*/
        
        typedef Particles<std::tuple<>,3,std::vector,bucket_search_parallel> particle_bs_parallel_type;

/*`


[endsect]

[section Kd-Tree]

A kd-tree builds up a hierarchical tree of cells, with only the leaf cells actually containing particles. It is an efficient data structure to use if your particles are clustered in certain regions of the domain, and so you wish to adapt the size of your cells with the local particle density.

Each level of the tree divides the cells in the parent level in half along a certain dimension (the dimension is chosen based on the distribution of particles within the cell). Any cells that contain a number of particles that is smaller than a given threshold (set in [memberref Aboria::Particles::init_neighbour_search]) are marked as leaf cells, and are not divided on subsequent levels.

The construction of the kd-tree in Aboria simply wraps the popular NanoFLANN library [@https://github.com/jlblancoc/nanoflann], and reorders the particle set according to which leaf cell a particle belongs to. However, Aboria's native neighbourhood queries are used instead of those provided with NanoFLANN.

The relevent classes within Aboria are [classref Aboria::nanoflann_adaptor] and [classref Aboria::nanoflann_adaptor_query]. You can create a particle set using a kd-tree by setting the [classref Aboria::Particles] template arguments accordingly.

*/
        
        typedef Particles<std::tuple<>,3,std::vector,nanoflann_adaptor> particle_kdtree_type;

/*`



[endsect]


[section Hyper Oct-Tree]

A hyper oct-tree is a generalisation of an oct-tree (in 3 dimensions) to $N$ dimensions. Is also builds up a hierarchical tree of cells, however in this case each level of the tree is split along [*all] dimensions, so that each cell has $2^N$ children. Any cells that contain less that the given number of particles (set in [funcref Aboria::Particles::init_neighbour_search]) are marked as leaf cells. Empty cells are included in the data structure, but are ignored by any queries.

For example, the diagram below shows the leaf cells of a hyper oct-tree in 2 dimensions (this is the same as a quad-tree). If the user wishes to find all the particles within a given euclidean distance of the red particle, then Aboria will search through all the red-shaded cells for matching particles.

[$images/neighbour/octtree.svg] 

The relevent classes within Aboria are [classref Aboria::octtree] and [classref Aboria::octtree_query].
You can create a particle set using a hyper oct-tree by setting the [classref Aboria::Particles] template arguments accordingly.

*/
        
        typedef Particles<std::tuple<>,3,std::vector,octtree> particle_octtree_type;

/*`



[endsect]
[endsect]

*/
//]
    }



    template<template <typename,typename> class Vector,template <typename> class SearchMethod>
    void helper_single_particle(void) {
        ABORIA_VARIABLE(scalar,double,"scalar")
    	typedef Particles<std::tuple<scalar>,3,Vector,SearchMethod> Test_type;
        typedef position_d<3> position;
    	Test_type test;
    	double3 min(-1,-1,-1);
    	double3 max(1,1,1);
    	double3 periodic(true,true,true);
    	double radius = 0.1;
    	test.init_neighbour_search(min,max,periodic);
    	typename Test_type::value_type p;

        get<position>(p) = double3(0,0,0);
    	test.push_back(p);

    	int count = 0;
    	for (auto tpl: euclidean_search(test.get_query(),double3(radius/2,radius/2,0),radius)) {
    		count++;
    	}
    	TS_ASSERT_EQUALS(count,1);

    	auto tpl = euclidean_search(test.get_query(),double3(radius/2,radius/2,0),radius);
    	TS_ASSERT_EQUALS(std::distance(tpl.begin(),tpl.end()),1);

    	tpl = euclidean_search(test.get_query(),double3(2*radius,0,0),radius);
    	TS_ASSERT_EQUALS(std::distance(tpl.begin(),tpl.end()),0);
    }

    template<template <typename,typename> class Vector,template <typename> class SearchMethod>
    void helper_two_particles(void) {
        ABORIA_VARIABLE(scalar,double,"scalar")
    	typedef Particles<std::tuple<scalar>,3,Vector,SearchMethod> Test_type;
        typedef position_d<3> position;
    	Test_type test;
    	double3 min(-1,-1,-1);
    	double3 max(1,1,1);
    	double3 periodic(true,true,true);
    	double radius = 0.1;
    	test.init_neighbour_search(min,max,periodic);
    	typename Test_type::value_type p;

        get<position>(p) = double3(0,0,0);
    	test.push_back(p);

        get<position>(p) = double3(radius/2,0,0);
    	test.push_back(p);

    	auto tpl = euclidean_search(test.get_query(),double3(1.1*radius,0,0),radius);
    	TS_ASSERT_EQUALS(std::distance(tpl.begin(),tpl.end()),1);
    	typename Test_type::const_reference pfound = tuple_ns::get<0>(*tpl.begin());
    	TS_ASSERT_EQUALS(get<id>(pfound),get<id>(test[1]));

    	tpl = euclidean_search(test.get_query(),double3(0.9*radius,0,0),radius);
    	TS_ASSERT_EQUALS(std::distance(tpl.begin(),tpl.end()),2);

    	tpl = euclidean_search(test.get_query(),double3(1.6*radius,0,0),radius);
    	TS_ASSERT_EQUALS(std::distance(tpl.begin(),tpl.end()),0);

    	tpl = euclidean_search(test.get_query(),double3(0.25*radius,0.9*radius,0),radius);
    	TS_ASSERT_EQUALS(std::distance(tpl.begin(),tpl.end()),2);

    	tpl = euclidean_search(test.get_query(),double3(0.25*radius,0.99*radius,0),radius);
    	TS_ASSERT_EQUALS(std::distance(tpl.begin(),tpl.end()),0);
    }

    template <typename Particles, int LNormNumber>
    struct has_n_neighbours {
        typedef typename Particles::query_type query_type;
        typedef typename Particles::position position;
        typedef typename Particles::reference reference;
        unsigned int n;
        double max_distance;
        query_type query;

        CUDA_HOST_DEVICE 
        has_n_neighbours(const query_type& query, 
                const double max_distance, const unsigned int n):
            query(query),n(n),max_distance(max_distance) {}

        CUDA_HOST_DEVICE 
        void operator()(reference i) {
            auto tpl = distance_search<LNormNumber>(query,get<position>(i),max_distance);
            TS_ASSERT_EQUALS(tpl.end()-tpl.begin(),n);
        }
    };

    template<unsigned int D, 
             template <typename,typename> class VectorType,
             template <typename> class SearchMethod>
    void helper_d(const int n, const double r, const int neighbour_n) {
        ABORIA_VARIABLE(scalar,double,"scalar")
    	typedef Particles<std::tuple<scalar>,D,VectorType,SearchMethod> Test_type;
        typedef position_d<D> position;
        typedef Vector<double,D> double_d;
        typedef Vector<bool,D> bool_d;
        typedef Vector<unsigned int,D> uint_d;
    	Test_type test;
    	double_d min(0);
    	double_d max(n);
    	bool_d periodic(true);
    	typename Test_type::value_type p;
        uint_d index(0);
        double dx = 1.0;

        bool finished = false;
        while (finished != true) {
            double_d pos = index*dx+min+dx/2;
            get<position>(p) = pos;
            test.push_back(p);
            index[0]++;
            for (int i=0; i<D; i++) {
                if (index[i] >= n) {
                    if (i==D-1) {
                        finished = true;
                    } else {
                        index[i+1]++;
                        index[i] = 0;
                    }
                } else {
                    break;
                }
            }
        }

    	test.init_neighbour_search(min,max,periodic,neighbour_n);
        if (D==2) {
            // Gauss circle problem (L2 in D=2)
            int n_expect = 0;
            for (int i = 0; i < 100; ++i) {
                n_expect += int(std::floor(std::pow(r,2)/(4*i+1))) - int(std::floor(std::pow(r,2)/(4*i+3)));
            }
            n_expect = 1 + 4*n_expect;
            std::cout << "L2 norm test (r="<<r<<"): expecting "<<n_expect<<" points"<<std::endl;
            Aboria::detail::for_each(test.begin(),test.end(),
                    has_n_neighbours<Test_type,2>(test.get_query(),r,n_expect));
        }
        // Box search (Linf)
        int n_expect = std::pow(2*int(std::floor(r)) + 1,D);
        std::cout << "Linf norm test (r="<<r<<", D="<<D<<"): expecting "<<n_expect<<" points"<<std::endl;
        Aboria::detail::for_each(test.begin(),test.end(),
                has_n_neighbours<Test_type,-1>(test.get_query(),r,n_expect));

    }

    template<unsigned int D, 
             template <typename,typename> class VectorType,
             template <typename> class SearchMethod>
    void helper_d_random(const int N, const double r, const int neighbour_n, const bool is_periodic) {
        ABORIA_VARIABLE(neighbours,int,"number of neighbours")
    	typedef Particles<std::tuple<neighbours>,D,VectorType,SearchMethod> particles_type;
        typedef position_d<D> position;
        typedef Vector<double,D> double_d;
        typedef Vector<bool,D> bool_d;
        typedef Vector<int,D> int_d;
        typedef Vector<unsigned int,D> uint_d;
    	double_d min(-1);
    	double_d max(1);
    	bool_d periodic(is_periodic);
        particles_type particles(N);
        double r2 = r*r;


        std::cout << "random test (D="<<D<<" periodic= "<<is_periodic<<"  N="<<N<<" r="<<r<<"):" << std::endl;

        unsigned seed1 = std::chrono::system_clock::now().time_since_epoch().count();
        std::default_random_engine gen(seed1); 
        std::uniform_real_distribution<double> uniform(-1,1);
        for (int i=0; i<N; ++i) {
            for (int d = 0; d < D; ++d) {
                get<position>(particles)[i][d] = uniform(gen);
            }
        }

    	particles.init_neighbour_search(min,max,periodic,neighbour_n);

        // brute force search
        auto t0 = Clock::now();
        Aboria::detail::for_each(particles.begin(),particles.end(),
                [&](typename particles_type::reference i) {
                    int count = 0;
                    for (typename particles_type::const_reference j: particles) {
                        typename particles_type::double_d pi = get<position>(i);
                        typename particles_type::double_d pj = get<position>(j);
                        if (is_periodic) {
                            for (lattice_iterator<D> periodic_it(int_d(-1),int_d(2)); 
                                    periodic_it != false; ++periodic_it) {
                                if ((pi+(*periodic_it)*(max-min)-pj).squaredNorm() <= r2) {
                                    count++;
                                }
                            }
                        } else {
                            if ((pi-pj).squaredNorm() <= r2) {
                                count++;
                            }
                        }
                    }
                    get<neighbours>(i) = count;
                    
                });
        auto t1 = Clock::now();
        std::chrono::duration<double> dt_brute = t1 - t0;


        // Aboria search
        
        t0 = Clock::now();
        Aboria::detail::for_each(particles.begin(),particles.end(),
                [&](typename particles_type::reference i) {
                    int count = 0;
                    //std::cout << "position of i = "<<get<position>(i)<<std::endl;
                    for (auto tpl: euclidean_search(particles.get_query(),get<position>(i),r)) {
                        typename particles_type::const_reference j = std::get<0>(tpl);
                        typename particles_type::double_d dx = std::get<1>(tpl);
                        //std::cout << "position of j = "<<get<position>(j)<<std::endl;
                        TS_ASSERT_LESS_THAN_EQUALS(dx.squaredNorm(),r2);
                        count++;
                    }
                    TS_ASSERT_EQUALS(count,get<neighbours>(i));
                    if (get<id>(i)==0) {
                        std::cout << "\tfor id = 0 found "<<count 
                                  <<" neighbours and expected "<<get<neighbours>(i)
                                  <<" neighbours"<<std::endl;
                    }
                });
        t1 = Clock::now();
        std::chrono::duration<double> dt_aboria = t1 - t0;

        std::cout << "\ttiming result: Aboria = "<<dt_aboria.count()
                  <<" versus brute force = "<<dt_brute.count()<<std::endl;
    }

    

    template<template <typename,typename> class VectorType,
             template <typename> class SearchMethod>
    void helper_d_test_list_regular() {
        helper_d<1,VectorType,SearchMethod>(100,1.5,10);
        helper_d<2,VectorType,SearchMethod>(50,1.0001,10);
        helper_d<2,VectorType,SearchMethod>(50,1.5,10);
        helper_d<2,VectorType,SearchMethod>(20,2.1,10);
        helper_d<3,VectorType,SearchMethod>(10,1.9,10);
        helper_d<3,VectorType,SearchMethod>(10,1.0001,10);
        helper_d<4,VectorType,SearchMethod>(10,1.0001,10);
    }

    template<template <typename,typename> class VectorType,
             template <typename> class SearchMethod>
    void helper_d_test_list_random() {
        helper_d_random<1,VectorType,SearchMethod>(10,0.1,1,false);
        helper_d_random<1,VectorType,SearchMethod>(10,0.1,1,true);
        helper_d_random<1,VectorType,SearchMethod>(1000,0.1,10,true);
        helper_d_random<1,VectorType,SearchMethod>(1000,0.1,10,false);
        helper_d_random<1,VectorType,SearchMethod>(1000,0.1,100,true);
        helper_d_random<1,VectorType,SearchMethod>(1000,0.1,100,false);
        helper_d_random<2,VectorType,SearchMethod>(1000,0.1,10,true);
        helper_d_random<2,VectorType,SearchMethod>(1000,0.1,10,false);
        helper_d_random<2,VectorType,SearchMethod>(1000,0.5,10,true);
        helper_d_random<2,VectorType,SearchMethod>(1000,0.5,10,false);
        helper_d_random<2,VectorType,SearchMethod>(1000,0.2,1,true);
        helper_d_random<2,VectorType,SearchMethod>(1000,0.2,1,false);
        helper_d_random<3,VectorType,SearchMethod>(1000,0.2,100,true);
        helper_d_random<3,VectorType,SearchMethod>(1000,0.2,100,false);
        helper_d_random<3,VectorType,SearchMethod>(1000,0.2,10,true);
        helper_d_random<3,VectorType,SearchMethod>(1000,0.2,10,false);
        helper_d_random<3,VectorType,SearchMethod>(1000,0.2,1,true);
        helper_d_random<3,VectorType,SearchMethod>(1000,0.2,1,false);
        helper_d_random<4,VectorType,SearchMethod>(1000,0.2,10,true);
        helper_d_random<4,VectorType,SearchMethod>(1000,0.2,10,false);
    }

    void test_std_vector_bucket_search_serial(void) {
        helper_single_particle<std::vector,bucket_search_serial>();
        helper_two_particles<std::vector,bucket_search_serial>();
       
        helper_d_test_list_regular<std::vector,bucket_search_serial>();
        helper_d_test_list_random<std::vector,bucket_search_serial>();
    }

    void test_std_vector_bucket_search_parallel(void) {
        helper_single_particle<std::vector,bucket_search_parallel>();
        helper_two_particles<std::vector,bucket_search_parallel>();

        helper_d_test_list_regular<std::vector,bucket_search_parallel>();
        helper_d_test_list_random<std::vector,bucket_search_parallel>();
    }

    void test_std_vector_nanoflann_adaptor(void) {
        helper_d_test_list_random<std::vector,nanoflann_adaptor>();
        helper_d_test_list_regular<std::vector,nanoflann_adaptor>();
    }

    void test_std_vector_octtree(void) {
        helper_d_test_list_random<std::vector,octtree>();
        helper_d_test_list_regular<std::vector,octtree>();
    }

    void test_thrust_vector_bucket_search_serial(void) {
#if defined(__CUDACC__)
        helper_d_test_list_regular<thrust::device_vector,bucket_search_serial>();
        helper_d_test_list_random<thrust::device_vector,bucket_search_serial>();
#endif
    }

    void test_thrust_vector_bucket_search_parallel(void) {
#if defined(__CUDACC__)
        helper_d_test_list_regular<thrust::device_vector,bucket_search_parallel>();
        helper_d_test_list_random<thrust::device_vector,bucket_search_parallel>();
#endif
    }

};



#endif /* NEIGHBOURS_H_ */
