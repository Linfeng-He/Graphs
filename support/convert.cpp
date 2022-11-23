#include <iostream>
#include <stdio.h>
#include <getopt.h>
#include <stdlib.h>
#include <cstring>
#include <algorithm>

typedef struct graphnode{
    int node;
    graphnode* next;
    graphnode(int x) : node(x), next(NULL) {}
};

bool comparenode(graphnode a, graphnode b){
    if(a.node != b.node)
        return a.node < b.node;
    
    if(a.next->node != b.next->node)
        return a.next->node < b.next->node;
}

int main(int argc, char** argv){
    int opt;
    opt = getopt(argc, argv, "f:");
    const char* fileName = "datasets/g2.mtx";
    if(opt == 'f')
        fileName = optarg;
    
    FILE* fp = fopen(fileName, "r");
    char c;
    fscanf(fp, "%c", &c);
    int snode, enode, nedge, numnode;
    fscanf(fp, "%d %d %d", &snode, &enode, &nedge);
    numnode = enode - snode;
    graphnode* nodeinfo = (graphnode*) malloc(nedge * 2 * sizeof(graphnode));
    int temp, temp1;
    for(int i = 0; i < nedge; i++){
        fscanf(fp, "%u %u", &temp, &temp1);
        nodeinfo[i].node = temp;
        nodeinfo[i + nedge] = temp1;
        graphnode* nodetemp = new graphnode(temp);
        graphnode* nodetemp1 = new graphnode(temp1);
        nodeinfo[i].next = nodetemp1;
        nodeinfo[i + nedge].next = nodetemp;
    }
    fclose(fp);
    std::sort(nodeinfo, nodeinfo + 2*nedge-1, comparenode);
    // std::string newname = "datasets/n" + fileName;
    FILE *nfp = fopen("datasets/new.mtx", "w+");
    fprintf(nfp, "%c %d %d %d\n", c, snode, enode, 2*nedge);
    for(int i = 0; i < 2*nedge; i++){
        fprintf(nfp, "%d %d\n", nodeinfo[i].node, nodeinfo[i].next->node);
    }
    fclose(nfp);
    return 0;
}

