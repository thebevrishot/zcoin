#include "../sigmaextended_prover.h"
#include "../sigmaextended_verifier.h"
#include <chrono>
#include <ctime>
#include <iostream>

void test( int N, int n, int index){
    int m = (int)(log(N) / log(n));;
    secp_primitives::GroupElement g;
    g.randomize();
    std::vector<secp_primitives::GroupElement> h_gens;
    h_gens.resize(n * m);
    for(int i = 0; i < n * m; ++i ){
        h_gens[i].randomize();
    }
    secp_primitives::Scalar v, r;
    v.randomize();
    r.randomize();
    lelantus::SigmaExtendedProver prover(g,h_gens, n, m);

    std::vector<secp_primitives::GroupElement> commits;
    for(int i = 0; i < N; ++i){
        if(i == index){
            secp_primitives::GroupElement c;
            secp_primitives::Scalar zero(uint64_t(0));
            c = lelantus::LelantusPrimitives::double_commit(g, zero, h_gens[1], v, h_gens[0], r);
            commits.push_back(c);

        }
        else{
            commits.push_back(secp_primitives::GroupElement());
            commits[i].randomize();
        }
    }

    std::clock_t proof_start = std::clock();

    lelantus::SigmaExtendedProof proof;
    prover.proof(commits, index, v, r, proof);
    std::cout <<"N = " << N << " n = " << n << "m = " <<m;
    std::cout << " Proof size  " << proof.memoryRequired() ;

    auto duration_clock = ( std::clock() - proof_start ) / (CLOCKS_PER_SEC / 1000);
    std::cout << " Proof time  " << duration_clock << " ms ";


    std::clock_t verify_start = std::clock();
    lelantus::SigmaExtendedVerifier verifier(g, h_gens, n, m);
    verifier.verify(commits, proof);

    duration_clock = ( std::clock() - verify_start ) / (CLOCKS_PER_SEC / 1000);
    std::cout << " Verify time  " <<  duration_clock << " ms \n";
}

int main(){
    test(8192, 2, 0);
    test(16384, 4, 0);
    test(32768, 8, 0);
    test(65536, 4, 0);
    test(65536, 16, 0);
    test(262144, 8, 0);
    test(262144, 64, 0);
    test(1048576, 10, 0);
	return 0;
}