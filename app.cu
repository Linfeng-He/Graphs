#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>

#include "support/graph.h"
#include "support/params.h"
#include "support/timer.h"
#include "support/utils.h"


__global__ void priority_kernel(int* __restrict__ nodePtrs, 
                                int* __restrict__ neighborIdxs,
                                int* __restrict__ prioNodeIdx,
                                int* __restrict__ prioNodePtrs,
                                int numbernode) {

    const int i = blockIdx.x * blockDim.x + threadIdx.x; //node id

    if(i < numbernode){
        const int pos = nodePtrs[i + 1] - nodePtrs[i]; //degree
        const int th_prio = pos;
        prioNodeIdx[i] = pos; //priority
        __syncthreads();

        // go through the neighbor list and get number of priority nodes
        int temp = 0;
        for(int node = nodePtrs[i]; node < nodePtrs[i + 1]; node++){
            if(prioNodeIdx[neighborIdxs[node]] > th_prio || 
                (prioNodeIdx[neighborIdxs[node]] == th_prio && neighborIdxs[node] < i)){
                temp++;
            }
        }
        prioNodePtrs[i] = temp;
    }
}

__global__ void color_kernel(int* __restrict__ nodePtrs, 
                             int* __restrict__ neighborIdxs,
                             int* __restrict__ prioNodeIdx,
                             int* __restrict__ prioNodePtrs,
                             int* __restrict__ color_num, 
                             int* __restrict__ color_bitmap,
                             int threadnum){

    const int i = blockIdx.x * blockDim.x + threadIdx.x;

    if(i < threadnum){
        int check_bit = 0;
        int pos = nodePtrs[i + 1] - nodePtrs[i]; //degree + 1

        while(1){

            //when none of its neighbors' priority is larger than itself, then color this node
            if(prioNodePtrs[i] <= 0){

                //get the first color not used by its neighbors
                for(int bit = 0; bit < pos + 1; bit++){
                    if(!isSet(color_bitmap[i], bit)){
                        check_bit = bit;
                        break;
                    }
                }
                color_num[i] = check_bit;

                //-1 to the priority of its nrighbors
                for(int node = nodePtrs[i]; node < nodePtrs[i + 1]; node++){
                    if(prioNodePtrs[neighborIdxs[node]] > 0){

                        atomicAdd(color_bitmap + neighborIdxs[node], pow(2, check_bit));
                        __threadfence();
                        atomicSub(prioNodePtrs + neighborIdxs[node], 1);
                        __ldg(&prioNodePtrs[node]);
                    
                    }
                }
                break;
            }
            __syncwarp();
        }
    }
}

int main(int argc, char** argv) {

    // Process parameters
    struct Params p = input_params(argc, argv);

    // Initialize data structures
    PRINT_INFO(p.verbosity >= 1, "Reading graph %s", p.fileName);
    struct COOGraph cooGraph = readCOOGraph(p.fileName);
    PRINT_INFO(p.verbosity >= 1, "    Graph has %d nodes and %d edges", cooGraph.numNodes, cooGraph.numEdges);
    struct CSRGraph csrGraph = coo2csr(cooGraph);
    //PRINT_INFO(p.verbosity >= 1, "    Graph has %d nodes and %d edges and %d priority edges", csrGraph.numNodes, csrGraph.numEdges, csrGraph.p_numEdges);

    //receive data from gpu
    int* color_gpu = (int*) malloc(csrGraph.numNodes*sizeof(int));
    int* prioNodeIdx_gpu = (int*) malloc (csrGraph.numNodes*sizeof(int));
    int* prioNodePtrs_gpu = (int*) malloc (csrGraph.numNodes*sizeof(int));
    int* prio_edge_gpu = (int*) malloc (csrGraph.numNodes*sizeof(int));
    for(int i = 0; i < csrGraph.numNodes; ++i) {
        color_gpu[i] = INT_MAX; // Unreachable
    }

    // Allocate GPU memory
    CSRGraph csrGraph_d;
    csrGraph_d.numNodes = csrGraph.numNodes;
    csrGraph_d.numEdges = csrGraph.numEdges;
    cudaMalloc((void**) &csrGraph_d.nodePtrs, (csrGraph_d.numNodes + 1)*sizeof(int));
    cudaMalloc((void**) &csrGraph_d.neighborIdxs, csrGraph_d.numEdges*sizeof(int));
    cudaMalloc((void**) &csrGraph_d.prioNodePtrs, (csrGraph_d.numNodes)*sizeof(int)); //store number of neighbors with larger priority 
    cudaMalloc((void**) &csrGraph_d.prioNodeIdx, csrGraph_d.numNodes*sizeof(int)); //store the random/degree based priority
    // cudaMalloc((void**) &csrGraph_d.prioNeighbor, csrGraph_d.numEdges*sizeof(int)); //store larger priority node's info
    int* color_num_d;
    cudaMalloc((void**) &color_num_d, csrGraph_d.numNodes*sizeof(int)); //color info for each node
    
    int* color_bitmap_d;
    cudaMalloc((void**) &color_bitmap_d, csrGraph_d.numNodes*sizeof(uint32_t)); //color bitmap for each node

    // Copy data to GPU
    cudaMemcpy(csrGraph_d.nodePtrs, csrGraph.nodePtrs, (csrGraph_d.numNodes + 1)*sizeof(int), cudaMemcpyHostToDevice);
    cudaMemcpy(csrGraph_d.neighborIdxs, csrGraph.neighborIdxs, csrGraph_d.numEdges*sizeof(int), cudaMemcpyHostToDevice);
    cudaDeviceSynchronize();

    // Calculating result on GPU
    PRINT_INFO(p.verbosity >= 1, "Calculating result on GPU");

    int numThreadsPerBlock = 256;
    int numBlocks = ceil ((double)csrGraph.numNodes / (double)numThreadsPerBlock);
    
    //kernel to give priority
    priority_kernel <<< numBlocks, numThreadsPerBlock >>> (csrGraph_d.nodePtrs, csrGraph_d.neighborIdxs, csrGraph_d.prioNodeIdx, csrGraph_d.prioNodePtrs, csrGraph_d.numNodes );
    cudaDeviceSynchronize();
    
    //get data from gpu 
    // cudaMemcpy(prioNodeIdx_gpu, csrGraph_d.prioNodeIdx, csrGraph_d.numNodes*sizeof(int), cudaMemcpyDeviceToHost);
    // cudaMemcpy(prioNodePtrs_gpu, csrGraph_d.prioNodePtrs, csrGraph_d.numNodes*sizeof(int), cudaMemcpyDeviceToHost);

    Timer timer;
    startTimer(&timer);

    //kernel to give colors
    color_kernel <<< numBlocks, numThreadsPerBlock >>> (csrGraph_d.nodePtrs, csrGraph_d.neighborIdxs, csrGraph_d.prioNodeIdx, csrGraph_d.prioNodePtrs, color_num_d, color_bitmap_d, csrGraph_d.numNodes);
    cudaDeviceSynchronize();

    stopTimer(&timer);
    if(p.verbosity == 0) PRINT("%f", getElapsedTime(timer)*1e3);
    PRINT_INFO(p.verbosity >= 1, "Elapsed time: %f ms", getElapsedTime(timer)*1e3);

    // Copy data from GPU
    cudaMemcpy(color_gpu, color_num_d, csrGraph_d.numNodes*sizeof(int), cudaMemcpyDeviceToHost);

    printf("color is: \n");
    for(int i = 0; i < csrGraph_d.numNodes; i++){
        printf("%d ", color_gpu[i]);
    }
    printf("\n");

    free(color_gpu);
    free(prio_edge_gpu);
    free(prioNodeIdx_gpu);
    free(prioNodePtrs_gpu);
    freeCOOGraph(cooGraph);
    freeCSRGraph(csrGraph);
    return 0;
}


