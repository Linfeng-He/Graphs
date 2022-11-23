
#ifndef _GRAPH_H_
#define _GRAPH_H_

#include <assert.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>

#define ROUND_UP_TO_MULTIPLE_OF_2(x)    ((((x) + 1)/2)*2)
#define ROUND_UP_TO_MULTIPLE_OF_8(x)    ((((x) + 7)/8)*8)
#define ROUND_UP_TO_MULTIPLE_OF_64(x)   ((((x) + 63)/64)*64)

#define setBit(val, idx) (val) |= (1l << (idx))
#define isSet(val, idx)  ((val) & (1l << (idx)))

struct COOGraph {
    int numNodes;
    int numEdges;
    int* nodeIdxs;
    int* neighborIdxs;
};

struct CSRGraph {
    int numNodes;
    int numEdges;
    int p_numEdges;
    int* nodePtrs;
    int* neighborIdxs;

    int* prioInfo;
    int* prioNodeIdx;
    int* prioNodePtrs;
    int* prioNeighbor;
};

static struct COOGraph readCOOGraph(const char* fileName) {

    struct COOGraph cooGraph;

    // Initialize fields
    FILE* fp = fopen(fileName, "r");
    char c;
    int startNode, endNode;
    assert(fscanf(fp, "%c", &c));
    assert(fscanf(fp, "%u", &startNode));
    assert(fscanf(fp, "%u", &endNode));

    cooGraph.numNodes = endNode - startNode;
    assert(fscanf(fp, "%u", &cooGraph.numEdges));
    cooGraph.nodeIdxs = (int*) malloc(cooGraph.numEdges*sizeof(int));
    cooGraph.neighborIdxs = (int*) malloc(cooGraph.numEdges*sizeof(int));

    // Read the neighborIdxs
    for(int edgeIdx = 0; edgeIdx < cooGraph.numEdges; ++edgeIdx) {
        int nodeIdx;
        assert(fscanf(fp, "%u", &nodeIdx));
        cooGraph.nodeIdxs[edgeIdx] = nodeIdx;
        int neighborIdx;
        assert(fscanf(fp, "%u", &neighborIdx));
        cooGraph.neighborIdxs[edgeIdx] = neighborIdx;
    }

    return cooGraph;

}

static void freeCOOGraph(struct COOGraph cooGraph) {
    free(cooGraph.nodeIdxs);
    free(cooGraph.neighborIdxs);
}

static struct CSRGraph coo2csr(struct COOGraph cooGraph) {

    struct CSRGraph csrGraph;

    // Initialize fields
    csrGraph.numNodes = cooGraph.numNodes;
    csrGraph.numEdges = cooGraph.numEdges;
    csrGraph.nodePtrs = (int*) calloc((csrGraph.numNodes + 1), sizeof(int));
    csrGraph.neighborIdxs = (int*)malloc((csrGraph.numEdges*sizeof(int)));
    // csrGraph.prioInfo = (int*) malloc(csrGraph.numNodes*sizeof(int));
    // csrGraph.prioNodePtrs = (int*) calloc((csrGraph.numNodes + 1), sizeof(int));
    // csrGraph.prioNodeIdx = (int*) malloc(csrGraph.numNodes*sizeof(int));

    // Histogram nodeIdxs
    for(int i = 0; i < cooGraph.numEdges; ++i) {
        int nodeIdx = cooGraph.nodeIdxs[i];
        csrGraph.nodePtrs[nodeIdx]++;
    }

    // Generate priority for nodes
    // srand((unsigned)time(NULL));
    // for(int i = 0; i < cooGraph.numNodes; ++i){
    //     csrGraph.prioInfo[i] = rand()%(cooGraph.numNodes+1) + 1;
    //     //csrGraph.prioInfo[i] = csrGraph.nodePtrs[i] + 1;
    // }
    // for(int i = 0; i < cooGraph.numNodes; ++i){
    //     printf("%d ",csrGraph.prioInfo[i]);
    // }
    // printf("\n");

    // Prefix sum nodePtrs
    int sumBeforeNextNode = 0;
    for(int nodeIdx = 0; nodeIdx < csrGraph.numNodes; ++nodeIdx) {
        int sumBeforeNode = sumBeforeNextNode;
        sumBeforeNextNode += csrGraph.nodePtrs[nodeIdx];
        csrGraph.nodePtrs[nodeIdx] = sumBeforeNode;
    }
    csrGraph.nodePtrs[csrGraph.numNodes] = sumBeforeNextNode;

    // Bin the neighborIdxs
    for(int i = 0; i < cooGraph.numEdges; ++i) {
        int nodeIdx = cooGraph.nodeIdxs[i];
        int neighborListIdx = csrGraph.nodePtrs[nodeIdx]++;
        csrGraph.neighborIdxs[neighborListIdx] = cooGraph.neighborIdxs[i];
    }

    // Restore nodePtrs
    for(int nodeIdx = csrGraph.numNodes - 1; nodeIdx > 0; --nodeIdx) {
        csrGraph.nodePtrs[nodeIdx] = csrGraph.nodePtrs[nodeIdx - 1];
    }
    csrGraph.nodePtrs[0] = 0;

    // // Prefix sum nodePtrs with priority info
    // sumBeforeNextNode = 0;
    // for(int nodeIdx = 0; nodeIdx < csrGraph.numNodes; ++nodeIdx) {
    //     for(int neighIdx = csrGraph.nodePtrs[nodeIdx]; neighIdx < csrGraph.nodePtrs[nodeIdx+1]; ++neighIdx){
    //         if(csrGraph.prioInfo[csrGraph.neighborIdxs[neighIdx]] > csrGraph.prioInfo[nodeIdx])
    //             //csrGraph.prioNodeIdx[nodeIdx]++;
    //             csrGraph.prioNodePtrs[nodeIdx]++;
    //     }
    // }
    // for(int nodeIdx = 0; nodeIdx < csrGraph.numNodes; ++nodeIdx) {
    //     for(int neighIdx = csrGraph.nodePtrs[nodeIdx]; neighIdx < csrGraph.nodePtrs[nodeIdx+1]; ++neighIdx){
    //         if(csrGraph.prioInfo[csrGraph.neighborIdxs[neighIdx]] > csrGraph.prioInfo[nodeIdx])
    //             csrGraph.prioNodeIdx[nodeIdx]++;
    //             //csrGraph.prioNodePtrs[nodeIdx]++;
    //     }
    // }

    // for(int nodeIdx = 0; nodeIdx < csrGraph.numNodes; ++nodeIdx) {
    //     int sumBeforeNode = sumBeforeNextNode;
    //     sumBeforeNextNode += csrGraph.prioNodePtrs[nodeIdx];
    //     csrGraph.prioNodePtrs[nodeIdx] = sumBeforeNode;
    // }
    // csrGraph.prioNodePtrs[csrGraph.numNodes] = sumBeforeNextNode;
    // csrGraph.p_numEdges = sumBeforeNextNode;


    // //give space to prioEdgeInfo
    // csrGraph.prioNeighbor = (int*)malloc(sumBeforeNextNode*sizeof(int));
    // int pos = 0;
    // for(int nodeIdx = 0; nodeIdx < csrGraph.numNodes; ++nodeIdx) {
    //     for(int neighIdx = csrGraph.nodePtrs[nodeIdx]; neighIdx < csrGraph.nodePtrs[nodeIdx+1]; ++neighIdx){
    //         if(csrGraph.prioInfo[csrGraph.neighborIdxs[neighIdx]] > csrGraph.prioInfo[nodeIdx]){
    //             csrGraph.prioNeighbor[pos] = csrGraph.neighborIdxs[neighIdx];
    //             pos++;
    //         }
                
    //     }
    // }

    // for(int i = 0; i < cooGraph.numNodes+1; ++i){
    //     printf("%d ",csrGraph.prioNodePtrs[i]);
    // }
    // printf("\n");

    // for(int i = 0; i < sumBeforeNextNode; ++i){
    //     printf("%d ",csrGraph.prioNeighbor[i]);
    // }
    // printf("\n");

    // for(int i = 0; i < cooGraph.numNodes; ++i){
    //     printf("%d ",csrGraph.prioNodeIdx[i]);
    // }
    // printf("\n");
    

    return csrGraph;

}

static void freeCSRGraph(struct CSRGraph csrGraph) {
    free(csrGraph.nodePtrs);
    free(csrGraph.neighborIdxs);
    // free(csrGraph.prioInfo);
    // free(csrGraph.prioNodeIdx);
    // free(csrGraph.prioNodePtrs);
    // free(csrGraph.prioNeighbor);
}

#endif


