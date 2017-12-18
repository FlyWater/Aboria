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


#ifndef H2_TEST_H_
#define H2_TEST_H_

#include <cxxtest/TestSuite.h>

#include <random>
#include <time.h>
#include <chrono>
typedef std::chrono::system_clock Clock;
#include "Level1.h"
#include "Kernels.h"
#include "H2Matrix.h"
#include "H2Lib.h"
#ifdef HAVE_GPERFTOOLS
#include <gperftools/profiler.h>
#endif



using namespace Aboria;

class H2Test: public CxxTest::TestSuite {
    ABORIA_VARIABLE(source,double,"source");
    ABORIA_VARIABLE(target_manual,double,"target manual");
    ABORIA_VARIABLE(target_h2,double,"target h2");
    ABORIA_VARIABLE(inverted_source,double,"inverted source h2");

public:
#ifdef HAVE_EIGEN
    template <unsigned int N, typename ParticlesType, typename KernelFunction>
    void helper_fast_methods_calculate(ParticlesType& particles, const KernelFunction& kernel, const double scale) {
        typedef typename ParticlesType::position position;
        typedef typename ParticlesType::reference reference;
        const unsigned int dimension = ParticlesType::dimension;

        
        

        // perform the operation using h2 matrix
        auto t0 = Clock::now();
        auto h2_matrix = make_h2_matrix(particles,particles,
                make_black_box_expansion<dimension,N>(kernel));
        auto t1 = Clock::now();
        std::chrono::duration<double> time_h2_setup = t1 - t0;
        std::fill(std::begin(get<target_h2>(particles)), std::end(get<target_h2>(particles)),0.0);
        t0 = Clock::now();
        h2_matrix.matrix_vector_multiply(get<target_h2>(particles),get<source>(particles));
        t1 = Clock::now();
        std::chrono::duration<double> time_h2_eval = t1 - t0;
        
        double L2_h2 = std::inner_product(
                std::begin(get<target_h2>(particles)), std::end(get<target_h2>(particles)),
                std::begin(get<target_manual>(particles)), 
                0.0,
                [](const double t1, const double t2) { return t1 + t2; },
                [](const double t1, const double t2) { return (t1-t2)*(t1-t2); }
                );

        std::cout << "for h2 matrix class:" <<std::endl;
        std::cout << "dimension = "<<dimension<<". N = "<<N<<". L2_h2 error = "<<L2_h2<<". L2_h2 relative error is "<<std::sqrt(L2_h2/scale)<<". time_h2_setup = "<<time_h2_setup.count()<<". time_h2_eval = "<<time_h2_eval.count()<<std::endl;

        if (N == 3) {
            TS_ASSERT_LESS_THAN(L2_h2/scale,1e-2);
        }

        // perform the operation using h2lib matrix
        t0 = Clock::now();
        auto h2lib_matrix = make_h2lib_matrix(particles,particles,
                make_black_box_expansion<dimension,N>(kernel));
        t1 = Clock::now();
        time_h2_setup = t1 - t0;
        std::fill(std::begin(get<target_h2>(particles)), std::end(get<target_h2>(particles)),0.0);
        t0 = Clock::now();
        h2lib_matrix.matrix_vector_multiply(get<target_h2>(particles),1.0,false,get<source>(particles));
        t1 = Clock::now();
        time_h2_eval = t1 - t0;
        
        L2_h2 = std::inner_product(
                std::begin(get<target_h2>(particles)), std::end(get<target_h2>(particles)),
                std::begin(get<target_manual>(particles)), 
                0.0,
                [](const double t1, const double t2) { return t1 + t2; },
                [](const double t1, const double t2) { return (t1-t2)*(t1-t2); }
                );

        std::cout << "for h2lib matrix class:" <<std::endl;
        std::cout << "dimension = "<<dimension<<". N = "<<N<<". L2_h2 error = "<<L2_h2<<". L2_h2 relative error is "<<std::sqrt(L2_h2/scale)<<". time_h2_setup = "<<time_h2_setup.count()<<". time_h2_eval = "<<time_h2_eval.count()<<std::endl;

        

        // invert target_manual to get the source
        t0 = Clock::now();
        auto h2lib_lr = h2lib_matrix.lr();
        t1 = Clock::now();
        time_h2_setup = t1 - t0;
        std::copy(std::begin(get<target_manual>(particles)),
                  std::end(get<target_manual>(particles)),
                  std::begin(get<inverted_source>(particles)));
        t0 = Clock::now();
        h2lib_lr.solve(get<inverted_source>(particles));
        t1 = Clock::now();
        time_h2_eval = t1 - t0;
        
        L2_h2 = std::inner_product(
                std::begin(get<inverted_source>(particles)), std::end(get<inverted_source>(particles)),
                std::begin(get<source>(particles)), 
                0.0,
                [](const double t1, const double t2) { return t1 + t2; },
                [](const double t1, const double t2) { return (t1-t2)*(t1-t2); }
                );

        std::cout << "for h2lib lr invert:" <<std::endl;
        std::cout << "dimension = "<<dimension<<". N = "<<N<<". L2_h2 error = "<<L2_h2<<". L2_h2 relative error is "<<std::sqrt(L2_h2/scale)<<". time_h2_setup = "<<time_h2_setup.count()<<". time_h2_eval = "<<time_h2_eval.count()<<std::endl;

        // invert target_manual to get the source
        t0 = Clock::now();
        auto h2lib_chol = h2lib_matrix.chol();
        t1 = Clock::now();
        time_h2_setup = t1 - t0;
        std::copy(std::begin(get<target_manual>(particles)),
                  std::end(get<target_manual>(particles)),
                  std::begin(get<inverted_source>(particles)));
        t0 = Clock::now();
        h2lib_chol.solve(get<inverted_source>(particles));
        t1 = Clock::now();
        time_h2_eval = t1 - t0;
        
        L2_h2 = std::inner_product(
                std::begin(get<inverted_source>(particles)), std::end(get<inverted_source>(particles)),
                std::begin(get<source>(particles)), 
                0.0,
                [](const double t1, const double t2) { return t1 + t2; },
                [](const double t1, const double t2) { return (t1-t2)*(t1-t2); }
                );

        std::cout << "for h2lib chol invert:" <<std::endl;
        std::cout << "dimension = "<<dimension<<". N = "<<N<<". L2_h2 error = "<<L2_h2<<". L2_h2 relative error is "<<std::sqrt(L2_h2/scale)<<". time_h2_setup = "<<time_h2_setup.count()<<". time_h2_eval = "<<time_h2_eval.count()<<std::endl;


        if (N == 3) {
            TS_ASSERT_LESS_THAN(L2_h2/scale,1e-2);
        }

        t0 = Clock::now();
        auto h2_eigen = create_h2_operator<N>(particles,particles,
                                    kernel);
        t1 = Clock::now();
        time_h2_setup = t1 - t0;

        typedef Eigen::Map<Eigen::Matrix<double,Eigen::Dynamic,1>> map_type; 
        map_type target_eigen(get<target_h2>(particles).data(),particles.size());
        map_type source_eigen(get<source>(particles).data(),particles.size());

        t0 = Clock::now();
        target_eigen = h2_eigen*source_eigen; 
        t1 = Clock::now();
        time_h2_eval = t1 - t0;

        L2_h2 = std::inner_product(
                std::begin(get<target_h2>(particles)), std::end(get<target_h2>(particles)),
                std::begin(get<target_manual>(particles)), 
                0.0,
                [](const double t1, const double t2) { return t1 + t2; },
                [](const double t1, const double t2) { return (t1-t2)*(t1-t2); }
                );

        std::cout << "for h2 operator:" <<std::endl;
        std::cout << "dimension = "<<dimension<<". N = "<<N<<". L2_h2 error = "<<L2_h2<<". L2_h2 relative error is "<<std::sqrt(L2_h2/scale)<<". time_h2_setup = "<<time_h2_setup.count()<<". time_h2_eval = "<<time_h2_eval.count()<<std::endl;

        if (N == 3) {
            TS_ASSERT_LESS_THAN(L2_h2/scale,1e-2);
        }

    }

    template<unsigned int D, template <typename,typename> class StorageVector,template <typename> class SearchMethod>
    void helper_extended_matrix(size_t N) {
        typedef Vector<double,D> double_d;
        typedef Vector<int,D> int_d;
        typedef Vector<bool,D> bool_d;
        const double tol = 1e-10;
        // randomly generate a bunch of positions over a range 
        const double pos_min = 0;
        const double pos_max = 1;
        std::uniform_real_distribution<double> U(pos_min,pos_max);
        generator_type generator(time(NULL));
        auto gen = std::bind(U, generator);
        typedef Vector<double,D> double_d;
        typedef Vector<int,D> int_d;
        const int num_particles_per_bucket = 50;

        typedef Particles<std::tuple<source,target_manual,target_h2,inverted_source>,D,StorageVector,SearchMethod> ParticlesType;
        typedef typename ParticlesType::position position;
        ParticlesType particles(N);
        for (int i=0; i<N; i++) {
            for (int d=0; d<D; ++d) {
                get<position>(particles)[i][d] = gen();
                get<source>(particles)[i] = gen();
            }
        }
        particles.init_neighbour_search(int_d(pos_min),int_d(pos_max),bool_d(false),num_particles_per_bucket);

        // generate a source vector using a smooth cosine
        auto source_fn = [&](const double_d &p) {
            //return (p-double_d(0)).norm();
            double ret=1.0;
            const double scale = 2.0*detail::PI/(pos_max-pos_min); 
            for (int i=0; i<D; i++) {
                ret *= cos((p[i]-pos_min)*scale);
            }
            return ret/N;
        };
        std::transform(std::begin(get<position>(particles)), std::end(get<position>(particles)), 
                       std::begin(get<source>(particles)), source_fn);

        const double c = 0.01;
        auto kernel = [&c](const double_d &dx, const double_d &pa, const double_d &pb) {
            return std::sqrt(dx.squaredNorm() + c); 
        };

        auto h2_matrix = make_h2_matrix(particles,particles,
                                        make_black_box_expansion<D,2>(kernel));
        std::fill(std::begin(get<target_h2>(particles)), std::end(get<target_h2>(particles)),0.0);
        h2_matrix.matrix_vector_multiply(get<target_h2>(particles),get<source>(particles));

        auto internal_extended_vector = h2_matrix.get_internal_state();
        auto extended_vector = h2_matrix.gen_extended_vector(get<source>(particles));
        auto col_index = h2_matrix.gen_column_map();
        auto ext_matrix = h2_matrix.gen_extended_matrix();

        Eigen::Matrix<double,Eigen::Dynamic,1> mapped_extended_vector(ext_matrix.cols());
        for (int i = 0; i < col_index.size(); ++i) {
            mapped_extended_vector[col_index[i]] = get<source>(particles)[i];
        }
        // rest are zero
        for (int i = col_index.size(); i < ext_matrix.cols(); ++i) {
            mapped_extended_vector[i] = 0;
        }

        // check x in internal state and generated extended vector are the same
        for (int i = 0; i < particles.size(); ++i) {
            TS_ASSERT_DELTA(extended_vector[i],internal_extended_vector[i],1e-20);
            TS_ASSERT_DELTA(mapped_extended_vector[i],internal_extended_vector[i],1e-20);
        }
        // check rest of generated exteded vector is zero
        for (int i = particles.size(); i < extended_vector.size(); ++i) {
            TS_ASSERT_DELTA(extended_vector[i],0,1e-20);
            TS_ASSERT_DELTA(mapped_extended_vector[i],0,1e-20);
        }
        
        Eigen::Matrix<double,Eigen::Dynamic,1> result = ext_matrix*internal_extended_vector;
        /*
        std::ofstream myfile;
        myfile.open ("ext_matrix.csv");
        myfile << Eigen::Matrix<double,Eigen::Dynamic,Eigen::Dynamic>(ext_matrix);
        myfile.close();
        */


        Eigen::Matrix<double,Eigen::Dynamic,1> mapped_result(particles.size());
        auto row_index = h2_matrix.gen_row_map();
        for (int i = 0; i < row_index.size(); ++i) {
            mapped_result[i] = result[row_index[i]];
        }

        // check rest of result is 0
        for (int i = particles.size(); i < result.rows(); ++i) {
            TS_ASSERT_DELTA(result(i),0,1e-10);
        }

        // check filtered result is same as target_h2
        auto result_filtered = h2_matrix.filter_extended_vector(result);
        for (int i = 0; i < particles.size(); ++i) {
            TS_ASSERT_DELTA(result_filtered(i),get<target_h2>(particles)[i],1e-10);
            TS_ASSERT_DELTA(mapped_result(i),get<target_h2>(particles)[i],1e-10);
        }

        //auto st_ext_matrix = h2_matrix.gen_stripped_extended_matrix();
        /*
        myfile.open ("st_ext_matrix.csv");
        myfile << Eigen::Matrix<double,Eigen::Dynamic,Eigen::Dynamic>(st_ext_matrix);
        myfile.close();
        */


        //auto st_result = st_ext_matrix*internal_extended_vector;

    }


         
     

    template<unsigned int D, template <typename,typename> class StorageVector,template <typename> class SearchMethod>
    void helper_fast_methods(size_t N) {
        typedef Vector<double,D> double_d;
        typedef Vector<int,D> int_d;
        typedef Vector<bool,D> bool_d;
        const double tol = 1e-10;
        // randomly generate a bunch of positions over a range 
        const double pos_min = 0;
        const double pos_max = 1;
        std::uniform_real_distribution<double> U(pos_min,pos_max);
        typedef Vector<double,D> double_d;
        typedef Vector<int,D> int_d;
        const int num_particles_per_bucket = 50;

        typedef Particles<std::tuple<source,target_manual,target_h2,inverted_source>,D,StorageVector,SearchMethod> ParticlesType;
        typedef typename ParticlesType::position position;
        ParticlesType particles(N);

        detail::for_each(particles.begin(),particles.end(),
                [=](typename ParticlesType::raw_reference p) { 
            detail::uniform_real_distribution<double> U(pos_min,pos_max);
            for (int d=0; d<D; ++d) {
                get<position>(p)[d] = U(get<generator>(p));
                get<source>(p) = U(get<generator>(p));
            }
        });

        particles.init_neighbour_search(int_d(pos_min),int_d(pos_max),bool_d(false),num_particles_per_bucket);

        // generate a source vector using a smooth cosine
        auto source_fn = [&](const double_d &p) {
            //return (p-double_d(0)).norm();
            double ret=1.0;
            const double scale = 2.0*detail::PI/(pos_max-pos_min); 
            for (int i=0; i<D; i++) {
                ret *= cos((p[i]-pos_min)*scale);
            }
            return ret/N;
        };
        std::transform(std::begin(get<position>(particles)), std::end(get<position>(particles)), 
                       std::begin(get<source>(particles)), source_fn);

        const double c = 0.01;
        auto kernel = [&c](const double_d &dx, const double_d &pa, const double_d &pb) {
            return std::sqrt(dx.squaredNorm() + c); 
        };


        // perform the operation manually
        std::fill(std::begin(get<target_manual>(particles)), std::end(get<target_manual>(particles)),
                    0.0);

        auto t0 = Clock::now();
        for (int i=0; i<N; i++) {
            const double_d pi = get<position>(particles)[i];
            for (int j=0; j<N; j++) {
                const double_d pj = get<position>(particles)[j];
                get<target_manual>(particles)[i] += kernel(pi-pj,pi,pj)*get<source>(particles)[j];
            }
        }
        auto t1 = Clock::now();
        std::chrono::duration<double> time_manual = t1 - t0;


        const double scale = std::accumulate(
            std::begin(get<target_manual>(particles)), std::end(get<target_manual>(particles)),
            0.0,
            [](const double t1, const double t2) { return t1 + t2*t2; }
        );

        std::cout << "MANUAL TIMING: dimension = "<<D<<". number of particles = "<<N<<". time = "<<time_manual.count()<<" scale = "<<scale<<std::endl;

        helper_fast_methods_calculate<1>(particles,kernel,scale);
        helper_fast_methods_calculate<2>(particles,kernel,scale);
        helper_fast_methods_calculate<3>(particles,kernel,scale);
    }

    template <typename Expansions>
    void helper_fmm_matrix_operators(Expansions& expansions) {
        const unsigned int D = Expansions::dimension;
        typedef Vector<double,D> double_d;
        typedef Vector<int,D> int_d;
        typedef typename Expansions::p_vector_type p_vector_type;
        typedef typename Expansions::m_vector_type m_vector_type;
        typedef typename Expansions::l2p_matrix_type l2p_matrix_type;
        typedef typename Expansions::p2m_matrix_type p2m_matrix_type;
        typedef typename Expansions::p2p_matrix_type p2p_matrix_type;
        typedef typename Expansions::l2l_matrix_type l2l_matrix_type;
        typedef typename Expansions::m2l_matrix_type m2l_matrix_type;

        // unit box
        detail::bbox<D> parent(double_d(0.0),double_d(1.0));
        detail::bbox<D> leaf1(double_d(0.0),double_d(1.0));
        leaf1.bmax[0] = 0.5;
        detail::bbox<D> leaf2(double_d(0.0),double_d(1.0));
        leaf2.bmin[0] = 0.5;
        std::cout << "parent = "<<parent<<" leaf1 = "<<leaf1<<" leaf2 = "<<leaf2<<std::endl;

        // create n particles, 2 leaf boxes, 1 parent box
        std::uniform_real_distribution<double> U(0,1);
        generator_type generator(time(NULL));
        const size_t n = 10;
        Particles<std::tuple<>,D> particles(2*n);
        typedef position_d<D> position;
        p_vector_type source_leaf1;
        source_leaf1.resize(n);
        std::vector<size_t> leaf1_indicies;
        p_vector_type field_just_self_leaf1;
        field_just_self_leaf1.resize(n);
        double field_all_leaf1[n];
        p_vector_type source_leaf2;
        source_leaf2.resize(n);
        std::vector<size_t> leaf2_indicies;
        p_vector_type field_just_self_leaf2;
        field_just_self_leaf2.resize(n);
        double field_all_leaf2[n];

        auto f = [](const double_d& p) {
            return p[0];
        };

        for (int i = 0; i < n; ++i) {
            get<position>(particles)[i][0] = 0.5*U(generator);
            get<position>(particles)[n+i][0] = 0.5*U(generator)+0.5;
            for (int j = 1; j < D; ++j) {
                get<position>(particles)[i][j] = U(generator);
                get<position>(particles)[n+i][j] = U(generator);
            }
            source_leaf1[i] = f(get<position>(particles)[i]);
            source_leaf2[i] = f(get<position>(particles)[n+i]);
            leaf1_indicies.push_back(i);
            leaf2_indicies.push_back(n+i);
        }

        for (int i = 0; i < n; ++i) {
            const double_d& pi_leaf1 = get<position>(particles)[i];
            const double_d& pi_leaf2 = get<position>(particles)[n+i];
            field_just_self_leaf1[i] = 0;
            field_just_self_leaf2[i] = 0;
            for (int j = 0; j < n; ++j) {
                const double_d& pj_leaf1 = get<position>(particles)[j];
                const double_d& pj_leaf2 = get<position>(particles)[n+j];
                field_just_self_leaf1[i] += source_leaf1[j]
                    *expansions.m_K(pj_leaf1-pi_leaf1,pi_leaf1,pj_leaf1);
                field_just_self_leaf2[i] += source_leaf2[j]
                    *expansions.m_K(pj_leaf2-pi_leaf2,pi_leaf2,pj_leaf2);
            }
            field_all_leaf1[i] = field_just_self_leaf1[i];
            field_all_leaf2[i] = field_just_self_leaf2[i];
            for (int j = 0; j < n; ++j) {
                const double_d& pj_leaf1 = get<position>(particles)[j];
                const double_d& pj_leaf2 = get<position>(particles)[n+j];
                field_all_leaf1[i] += source_leaf2[j]
                    *expansions.m_K(pj_leaf2-pi_leaf1,pi_leaf1,pj_leaf2);                    
                field_all_leaf2[i] += source_leaf1[j]
                    *expansions.m_K(pj_leaf1-pi_leaf2,pi_leaf2,pj_leaf1);                    
            }
        }

        // check P2M, and L2P
        m_vector_type expansionM_leaf1 = m_vector_type::Zero();

        p2m_matrix_type p2m_matrix_leaf1;
        expansions.P2M_matrix(p2m_matrix_leaf1,leaf1,leaf1_indicies,particles);
        expansionM_leaf1 = p2m_matrix_leaf1*source_leaf1;

        m_vector_type expansionL_leaf1 = m_vector_type::Zero();
        m2l_matrix_type m2l_leaf1;
        expansions.M2L_matrix(m2l_leaf1,leaf1,leaf1);
        expansionL_leaf1 = m2l_leaf1*expansionM_leaf1;

        l2p_matrix_type l2p_matrix_leaf1;
        expansions.L2P_matrix(l2p_matrix_leaf1,leaf1,leaf1_indicies,particles);
        p_vector_type result_leaf1 = l2p_matrix_leaf1*expansionL_leaf1;

        double L2 = 0;
        double scale = 0;
        for (int i = 0; i < n; ++i) {
            L2 += std::pow(result_leaf1[i]-field_just_self_leaf1[i],2);
            scale += std::pow(field_just_self_leaf1[i],2);
            TS_ASSERT_LESS_THAN(std::abs(result_leaf1[i]-field_just_self_leaf1[i]),2e-4);
        }

        TS_ASSERT_LESS_THAN(std::sqrt(L2/scale),1e-4);

        p2m_matrix_type p2m_matrix_leaf2;
        expansions.P2M_matrix(p2m_matrix_leaf2,leaf2,leaf2_indicies,particles);
        m_vector_type expansionM_leaf2 = p2m_matrix_leaf2*source_leaf2;

        m2l_matrix_type m2l_leaf2;
        expansions.M2L_matrix(m2l_leaf2,leaf2,leaf2);
        m_vector_type expansionL_leaf2 = m2l_leaf2*expansionM_leaf2;

        l2p_matrix_type l2p_matrix_leaf2;
        expansions.L2P_matrix(l2p_matrix_leaf2,leaf2,leaf2_indicies,particles);
        p_vector_type result_leaf2 = l2p_matrix_leaf2*expansionL_leaf2;

        L2 = 0;
        for (int i = 0; i < n; ++i) {
            L2 += std::pow(result_leaf2[i]-field_just_self_leaf2[i],2);
            scale += std::pow(field_just_self_leaf2[i],2);
        }
        TS_ASSERT_LESS_THAN(std::sqrt(L2/scale),1e-4);
        
        // check M2M and L2L
        l2l_matrix_type l2l_leaf1,l2l_leaf2;
        expansions.L2L_matrix(l2l_leaf1,leaf1,parent);
        expansions.L2L_matrix(l2l_leaf2,leaf2,parent);

        m_vector_type expansionM_parent = l2l_leaf1.transpose()*expansionM_leaf1 
                                        + l2l_leaf2.transpose()*expansionM_leaf2;
        m2l_matrix_type m2l_parent;
        expansions.M2L_matrix(m2l_parent,parent,parent);
        m_vector_type expansionL_parent = m2l_parent*expansionM_parent;

        m_vector_type reexpansionL_leaf1 = l2l_leaf1*expansionL_parent;
        p_vector_type total_result_leaf1 = l2p_matrix_leaf1*reexpansionL_leaf1;

        L2 = 0;
        scale = 0;
        for (int i = 0; i < n; ++i) {
            L2 += std::pow(total_result_leaf1[i]-field_all_leaf1[i],2);
            scale += std::pow(field_all_leaf1[i],2);
        }
        TS_ASSERT_LESS_THAN(std::sqrt(L2/scale),1e-4);
    }
#endif

        
    void test_fmm_matrix_operators() {
#ifdef HAVE_EIGEN
        const unsigned int D = 2;
        typedef Vector<double,D> double_d;
        auto kernel = [](const double_d &dx, const double_d &pa, const double_d &pb) {
            return std::sqrt(dx.squaredNorm() + 0.1); 
        };
        detail::BlackBoxExpansions<D,10,decltype(kernel)> expansions(kernel);
        helper_fmm_matrix_operators(expansions);
#endif
    }

    void test_fast_methods_bucket_search_serial(void) {
#ifdef HAVE_EIGEN
        const size_t N = 10000;
        /*
        std::cout << "BUCKET_SEARCH_SERIAL: testing extended matrix 1D..." << std::endl;
        helper_extended_matrix<1,std::vector,bucket_search_serial>(N);
        std::cout << "BUCKET_SEARCH_SERIAL: testing extended matrix 2D..." << std::endl;
        helper_extended_matrix<2,std::vector,bucket_search_serial>(N);
        std::cout << "BUCKET_SEARCH_SERIAL: testing extended matrix 3D..." << std::endl;
        helper_extended_matrix<3,std::vector,bucket_search_serial>(N);
        */

        std::cout << "BUCKET_SEARCH_SERIAL: testing 1D..." << std::endl;
        helper_fast_methods<1,std::vector,bucket_search_serial>(N);
        std::cout << "BUCKET_SEARCH_SERIAL: testing 2D..." << std::endl;
        helper_fast_methods<2,std::vector,bucket_search_serial>(N);
        std::cout << "BUCKET_SEARCH_SERIAL: testing 3D..." << std::endl;
        helper_fast_methods<3,std::vector,bucket_search_serial>(N);
#endif
    }


    void test_fast_methods_bucket_search_parallel(void) {
#ifdef HAVE_EIGEN
        const size_t N = 10000;
        /*
        std::cout << "BUCKET_SEARCH_PARALLEL: testing extended matrix 1D..." << std::endl;
        helper_extended_matrix<1,std::vector,bucket_search_parallel>(N);
        std::cout << "BUCKET_SEARCH_PARALLEL: testing extended matrix 2D..." << std::endl;
        helper_extended_matrix<2,std::vector,bucket_search_parallel>(N);
        std::cout << "BUCKET_SEARCH_PARALLEL: testing extended matrix 3D..." << std::endl;
        helper_extended_matrix<3,std::vector,bucket_search_parallel>(N);
        */

        std::cout << "BUCKET_SEARCH_PARALLEL: testing 1D..." << std::endl;
        helper_fast_methods<1,std::vector,bucket_search_parallel>(N);
        std::cout << "BUCKET_SEARCH_PARALLEL: testing 2D..." << std::endl;
        helper_fast_methods<2,std::vector,bucket_search_parallel>(N);
        std::cout << "BUCKET_SEARCH_PARALLEL: testing 3D..." << std::endl;
        helper_fast_methods<3,std::vector,bucket_search_parallel>(N);
#endif
    }


    void test_fast_methods_kd_tree(void) {
#ifdef HAVE_EIGEN
        const size_t N = 10000;
        /*
        std::cout << "KD_TREE: testing extended matrix 1D..." << std::endl;
        helper_extended_matrix<1,std::vector,nanoflann_adaptor>(N);
        std::cout << "KD_TREE: testing extended matrix 2D..." << std::endl;
        helper_extended_matrix<2,std::vector,nanoflann_adaptor>(N);
        std::cout << "KD_TREE: testing extended matrix 3D..." << std::endl;
        helper_extended_matrix<3,std::vector,nanoflann_adaptor>(N);
        */

        std::cout << "KD_TREE: testing 1D..." << std::endl;
        helper_fast_methods<1,std::vector,nanoflann_adaptor>(N);
        std::cout << "KD_TREE: testing 2D..." << std::endl;
        helper_fast_methods<2,std::vector,nanoflann_adaptor>(N);
        std::cout << "KD_TREE: testing 3D..." << std::endl;
        helper_fast_methods<3,std::vector,nanoflann_adaptor>(N);
#endif
    }



    void test_fast_methods_octtree(void) {
#ifdef HAVE_EIGEN
        const size_t N = 10000;
        /*
        std::cout << "OCTTREE: testing extended matrix 1D..." << std::endl;
        helper_extended_matrix<1,std::vector,octtree>(N);
        std::cout << "OCTTREE: testing extended matrix 2D..." << std::endl;
        helper_extended_matrix<2,std::vector,octtree>(N);
        std::cout << "OCTTREE: testing extended matrix 3D..." << std::endl;
        helper_extended_matrix<3,std::vector,octtree>(N);
        */


        std::cout << "OCTTREE: testing 1D..." << std::endl;
        helper_fast_methods<1,std::vector,octtree>(N);
        std::cout << "OCTTREE: testing 2D..." << std::endl;
        helper_fast_methods<2,std::vector,octtree>(N);
        std::cout << "OCTTREE: testing 3D..." << std::endl;
        helper_fast_methods<3,std::vector,octtree>(N);
#endif
    }


};


#endif
