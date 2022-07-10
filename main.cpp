#include <bits/stdc++.h>
#include "file_manager.h"
#include "errors.h"
#include "constants.h"

using namespace std;

FileManager fm;
FileHandler fh;
int dim;
ifstream in;
ofstream out;
int point_node_per_page;
int region_node_per_page;
int rootNode = -1;
// TODO: set true before submission
bool logs = true;

void printBufferlog() {
    if(!logs) {
        return ;
    }
    fm.PrintBuffer();
}

void createEmptyPointNode(PageHandler& ph, int splitDim, int parent) {
    ph = fh.NewPage();
    char* data = ph.GetData();
    memcpy(&data[0], &splitDim, sizeof(int));
    int x = 1;
    memcpy(&data[4], &x, sizeof(int));
    memcpy(&data[8], &parent, sizeof(int));
    int padding = -1000000;
    for(int i=12; i<PAGE_CONTENT_SIZE; i+=4) {
        memcpy(&data[i], &padding, sizeof(int));
    }
    return;
}

void createEmptyRangeNode(PageHandler& ph, int splitDim, int parent) {
    ph = fh.NewPage();
    char* data = ph.GetData();
    memcpy(&data[0], &splitDim, sizeof(int));
    int x = 0;
    memcpy(&data[4], &x, sizeof(int));
    memcpy(&data[8], &parent, sizeof(int));
    int padding = -1000000;
    for(int i=12; i<PAGE_CONTENT_SIZE; i+=4) {
        memcpy(&data[i], &padding, sizeof(int));
    }
    return;
}

int splitdownward(int childpagenum, int splitdim, int splitValue, int parent) {
    cout<<"splitdownward\n";
    PageHandler node = fh.PageAt(childpagenum);
    printBufferlog();
    char* data = node.GetData();
    PageHandler rp;
    int indicator;
    memcpy(&indicator, &data[4], sizeof(int));
    int copysplitdim;
    memcpy(&copysplitdim, &data[0], sizeof(int));
    if(indicator==1) {
        createEmptyPointNode(rp, copysplitdim, parent);
        char* rightdata = rp.GetData();
        int nl=0, nr=0;
        for(int x=0;x<point_node_per_page;x++){
            int value=0;
            memcpy(&value, &data[12+x*(dim+1)*4+splitdim*4], sizeof(int));
            if(value==-1000000){
                break;
            }
            if(value<splitValue) {
                for(int j=0;j<dim+1;j++) {
                    memcpy(&data[12+nl*(dim+1)*4+j*4], &data[12+x*(dim+1)*4+j*4], sizeof(int));
                }
                nl++;
            } else {
                for(int j=0;j<dim+1;j++) {
                    memcpy(&rightdata[12+nr*(dim+1)*4+j*4], &data[12+x*(dim+1)*4+j*4], sizeof(int));
                }
                nr++;
            }
        }
        int padding = -1000000;
        for(int i=12+nl*4*(dim+1); i<PAGE_CONTENT_SIZE; i+=4) {
            memcpy(&data[i], &padding, sizeof(int));
        }
        return rp.GetPageNum();
    } else {
        createEmptyRangeNode(rp, copysplitdim, parent);
        char* rightdata = rp.GetData();
        int nl=0, nr=0;
        for(int x=0;x<region_node_per_page;x++){
            int rmin=0;
            memcpy(&rmin, &data[12+x*(2*dim+1)*4+2*splitdim*4], sizeof(int));
            if(rmin==-1000000){
                break;
            }
            int rmax=0;
            memcpy(&rmax, &data[12+x*(2*dim+1)*4+2*splitdim*4+4], sizeof(int));
            if(rmax<=splitValue) {
                for(int j=0;j<2*dim+1;j++) {
                    memcpy(&data[12+nl*(2*dim+1)*4+j*4], &data[12+x*(2*dim+1)*4+j*4], sizeof(int));
                }
                nl++;
            } else if(rmin>=splitValue) {
                for(int j=0;j<2*dim+1;j++) {
                    memcpy(&rightdata[12+nr*(2*dim+1)*4+j*4], &data[12+x*(2*dim+1)*4+j*4], sizeof(int));
                }
                nr++;
            } else {
                // split node again
                for(int j=0;j<2*dim+1;j++) {
                    memcpy(&data[12+nl*(2*dim+1)*4+j*4], &data[12+x*(2*dim+1)*4+j*4], sizeof(int));
                }
                memcpy(&data[12+nl*(2*dim+1)*4+splitdim*4+4], &splitValue, sizeof(int));
                nl++;
                for(int j=0;j<2*dim+1;j++) {
                    memcpy(&rightdata[12+nr*(2*dim+1)*4+j*4], &data[12+x*(2*dim+1)*4+j*4], sizeof(int));
                }
                memcpy(&rightdata[12+nr*(2*dim+1)*4+splitdim*4], &splitValue, sizeof(int));
                nr++;
                int childpagenum;
                memcpy(&childpagenum, &data[12+x*(2*dim+1)*4+(2*dim)*4], sizeof(int));
                int nodenum = node.GetPageNum();
                fh.MarkDirty(nodenum);
                fh.UnpinPage(nodenum);
                int rightnum = rp.GetPageNum();
                fh.MarkDirty(rightnum);
                fh.UnpinPage(rightnum);
                int rightChild = splitdownward(childpagenum, splitdim, splitValue, rp.GetPageNum());
                node = fh.PageAt(nodenum);
                rp = fh.PageAt(rightnum);
                printBufferlog();
                data = node.GetData();
                rightdata = rp.GetData();
                memcpy(&rightdata[12+(nr-1)*(2*dim)*4], &rightChild, sizeof(int));
            }
        }
        int padding = -1000000;
        for(int i=12+nl*4*(2*dim+1); i<PAGE_CONTENT_SIZE; i+=4) {
            memcpy(&data[i], &padding, sizeof(int));
        }
        return rp.GetPageNum();
    }


}

void reorganize(PageHandler& node) {
    cout<<"reorganize\n";
    int splitdim = -1;
    char* data = node.GetData();
    memcpy(&splitdim, &data[0], sizeof(int));
    vector<int> values;
    for(int i=12;i<PAGE_CONTENT_SIZE;i+=(2*dim+1)*4) {
        int val;
        memcpy(&val, &data[i+(2*splitdim)*4], sizeof(int));
        if(val==-1000000){
            break;
        }
        cout<<val<<" rmins in reorg\n";
        values.push_back(val);
    }
    sort(values.begin(), values.end());
    int midInd = region_node_per_page/2 + (region_node_per_page%2);
    int splitValue = values[midInd];
    PageHandler rp;
    // taking node itself as left node
    int parentNode = -1;
    memcpy(&parentNode, &data[8], sizeof(int));
    int newsplitdim = (splitdim+1)%dim;
    createEmptyRangeNode(rp, newsplitdim, parentNode);
    char* rightdata = rp.GetData();
    memcpy(&data[0], &newsplitdim, sizeof(int));
    int nl=0, nr=0;
    int rightpagenum = rp.GetPageNum();
    for(int i=12;i<PAGE_CONTENT_SIZE;i+=(2*dim+1)*4) {
        int rmin;
        memcpy(&rmin, &data[i+(2*splitdim)*4], sizeof(int));
        int rmax;
        memcpy(&rmax, &data[i+(2*splitdim)*4+4], sizeof(int));
        if(rmax<=splitValue) {
            // go in left node
            for(int x=0;x<2*dim+1;x++) {
                memcpy(&data[12+nl*(2*dim+1)*4+x*4], &data[i+x*4], sizeof(int));
            }
            nl++;
        } else if(rmin>=splitValue) {
            // go in right node
            for(int x=0;x<2*dim+1;x++) {
                memcpy(&rightdata[12+nr*(2*dim+1)*4+x*4], &data[i+x*4], sizeof(int));
            }
            nr++;
        } else {
            // split region
            for(int x=0;x<2*dim+1;x++) {
                memcpy(&data[12+nl*(2*dim+1)*4+x*4], &data[i+x*4], sizeof(int));
            }
            memcpy(&data[12+nl*(2*dim+1)*4+splitdim*4+4], &splitValue, sizeof(int));
            nl++;
            for(int x=0;x<2*dim+1;x++) {
                memcpy(&rightdata[12+nr*(2*dim+1)*4+x*4], &data[i+x*4], sizeof(int));
            }
            memcpy(&rightdata[12+nr*(2*dim+1)*4+splitdim*4], &splitValue, sizeof(int));
            nr++;
            int childpagenum;
            memcpy(&childpagenum, &data[i+(2*dim)*4], sizeof(int));
            int nodenum = node.GetPageNum();
            fh.MarkDirty(nodenum);
            fh.UnpinPage(nodenum);
            int rightnum = rp.GetPageNum();
            fh.MarkDirty(rightnum);
            fh.UnpinPage(rightnum);
            int rightChild = splitdownward(childpagenum, splitdim, splitValue, rp.GetPageNum());
            node = fh.PageAt(nodenum);
            rp = fh.PageAt(rightnum);
            printBufferlog();
            data = node.GetData();
            rightdata = rp.GetData();
            memcpy(&rightdata[12+(nr-1)*(2*dim)*4], &rightChild, sizeof(int));
        }
    }
    int padding = -1000000;
    for(int i=12+nl*4*(2*dim+1); i<PAGE_CONTENT_SIZE; i+=4) {
        memcpy(&data[i], &padding, sizeof(int));
    }

    // New root creation
    if(parentNode == -1){
        cout<<splitValue<<" in reqorganize\n";
        PageHandler rootp;
        createEmptyRangeNode(rootp, splitdim, -1);
        char* rootdata = rootp.GetData();
        rootNode = rootp.GetPageNum();
        int rmin=-99999;
        int rmax = INT_MAX;
        for(int x=0;x<2*dim;x+=2){
            memcpy(&rootdata[12+x*4], &rmin, sizeof(int));
            memcpy(&rootdata[12+x*4+4], &rmax, sizeof(int));
        }
        memcpy(&rootdata[12+splitdim*4+4], &splitValue, sizeof(int));
        int leftChild = node.GetPageNum();
        memcpy(&rootdata[12+(2*dim)*4], &leftChild, sizeof(int));
        for(int x=0;x<2*dim;x+=2){
            memcpy(&rootdata[12+(2*dim+1)*4+x*4], &rmin, sizeof(int));
            memcpy(&rootdata[12+(2*dim+1)*4+x*4+4], &rmax, sizeof(int));
        }
        memcpy(&rootdata[12+(2*dim+1)*4+splitdim*4], &splitValue, sizeof(int));
        int rightChild = rp.GetPageNum();
        memcpy(&rootdata[12+(2*dim+1)*4+(2*dim)*4], &rightChild, sizeof(int));

        memcpy(&data[8], &rootNode, sizeof(int));
        memcpy(&rightdata[8], &rootNode, sizeof(int));
        fh.MarkDirty(node.GetPageNum());
        fh.UnpinPage(node.GetPageNum());
        fh.MarkDirty(rp.GetPageNum());
        fh.UnpinPage(rp.GetPageNum());
        fh.MarkDirty(rootNode);
        return;
    }

    PageHandler parent = fh.PageAt(parentNode);
    printBufferlog();
    char* parentdata = parent.GetData();
    memcpy(&parentdata[0], &splitdim, sizeof(int));
    int region = -1;
    for(int i=12;i<PAGE_CONTENT_SIZE;i+=(2*dim+1)*4) {
        int childpagenum=-1;
        memcpy(&childpagenum, &parentdata[i+(2*dim)*4], sizeof(int));
        if(childpagenum==node.GetPageNum()){
            region = i;
            break;
        }
    }

    // Find number of regions in the region node
    int numregions = 0;
    for(int i=12;i<PAGE_CONTENT_SIZE;i+=(2*dim+1)*4) {
        int value=0;
        memcpy(&value, &parentdata[i], sizeof(int));
        if(value==-1000000) {
            break;
        }
        numregions++;
    }
    for(int x=0;x<2*dim;x++) {
        memcpy(&parentdata[12+numregions*(2*dim+1)*4+x*4], &parentdata[region+x*4], sizeof(int));
    }
    rightpagenum = rp.GetPageNum();
    memcpy(&parentdata[12+numregions*(2*dim+1)*4+(2*dim)*4], &rightpagenum, sizeof(int));
    memcpy(&parentdata[12+numregions*(2*dim+1)*4+(2*splitdim)*4], &splitValue, sizeof(int));
    memcpy(&parentdata[region+(2*splitdim)*4+4], &splitValue, sizeof(int));
    numregions++;

    // region node overflowed
    if(numregions == region_node_per_page) {
        reorganize(parent);
        return;
    }

}

bool insert(vector<int>& point) {
    cout<<"In insert\n";
    int location = -1;
    bool isEmpty = false;
    PageHandler ph;
    if(rootNode!=-1) {
        ph = fh.PageAt(rootNode);
        printBufferlog();
    } else {
        isEmpty = true;
    }

    // root is NULL
    if(isEmpty) {
        cout<<"NULL root\n";
        PageHandler ph;
        createEmptyPointNode(ph, 0, -1);
        char *data  = ph.GetData();
        for(int i=0;i<dim;i++){
            memcpy(&data[i*4+12], &point[i], sizeof(int));
        }
        memcpy(&data[dim*4+12], &location, sizeof(int));
        rootNode = ph.GetPageNum();
        fh.MarkDirty(rootNode);
        return true;
    }

    char* data;
    // Traverse down the tree to the point node
    while(true){
        data = ph.GetData();
        int indicator=-1;
        memcpy(&indicator, &data[4], sizeof(int));
        if(indicator==1) {
            break;
        }
        // cout<<"traverse down\n";
        for(int i=12;i<PAGE_CONTENT_SIZE;i+=(2*dim+1)*4) {
            bool correct=true;
            for(int x=0;x<dim;x++) {
                int rmin=0;
                memcpy(&rmin, &data[i+2*x*4], sizeof(int));
                int rmax=0;
                memcpy(&rmax, &data[i+2*x*4+4], sizeof(int));
                if(point[x]<rmax && point[x]>=rmin) {
                    continue;
                } else {
                    correct=false;
                    break;
                }
            }
            if(!correct) {
                continue;
            }
            int childPage=-1;
            memcpy(&childPage, &data[i+2*dim*4], sizeof(int));
            fh.UnpinPage(ph.GetPageNum());
            ph = fh.PageAt(childPage);
            printBufferlog();
        }
    }


    // Find number of point in the point node and check if point already in node
    // cout<<"num point\n";
    int numPoints = 0;
    for(int i=12;i<PAGE_CONTENT_SIZE;i+=(dim+1)*4) {
        int value=0;
        memcpy(&value, &data[i], sizeof(int));
        if(value==-1000000) {
            break;
        }
        bool pointalreadyin = true;
        for(int x=0;x<dim;x++) {
            memcpy(&value, &data[i+x*4], sizeof(int));
            if(point[x]!=value){
                pointalreadyin=false;
                break;
            }
        }
        if(pointalreadyin){
            fh.UnpinPage(ph.GetPageNum());
            // Point already in tree, print required error;
            out<<"DUPLICATE INSERTION\n";
            return false;
        }
        numPoints++;
    }

    // cout<<"put point in node\n";
    // put point in node
    for(int i=0;i<dim;i++){
        memcpy(&data[i*4+numPoints*4*(dim+1)+12], &point[i], sizeof(int));
    }
    memcpy(&data[dim*4+numPoints*4*(dim+1)+12], &location, sizeof(int));
    fh.MarkDirty(ph.GetPageNum());
    // fh.UnpinPage(ph.GetPageNum());
    numPoints++;

    // cannot fit point -> split node
    int split = -1;
    memcpy(&split, &data[0], sizeof(int));
    if(numPoints == point_node_per_page) {
        cout<<"split\n";
        vector<int> values;
        for(int i=12;i<PAGE_CONTENT_SIZE;i+=(dim+1)*4) {
            int value;
            memcpy(&value, &data[i+split*4], sizeof(int));
            if(value==-1000000) {
                break;
            }
            values.push_back(value);
        }
        sort(values.begin(),values.end());
        int midInd = point_node_per_page/2 + (point_node_per_page%2);
        int splitValue = values[midInd];
        cout<<splitValue<<" is split value\n";
        cout<<split<<" is split dimension\n";
        // assuming some values are less than median

        int parentNode = -1;
        memcpy(&parentNode, &data[8], sizeof(int));
        // used ph itself for left page
        PageHandler rp;
        int nl=0, nr=0;
        createEmptyPointNode(rp, (split+1)%dim, parentNode);
        char* rightdata = rp.GetData();
        for(int i=12;i<PAGE_CONTENT_SIZE;i+=(dim+1)*4){
            int value;
            memcpy(&value, &data[i+split*4], sizeof(int));
            if(value==-1000000){
                break;
            }
            if(value<splitValue) {
                for(int x=0;x<dim+1;x++) {
                    int temp;
                    memcpy(&temp, &data[i+x*4], sizeof(int));
                    memcpy(&data[12+nl*(dim+1)*4+x*4], &temp, sizeof(int));
                    // cout<<temp<<" ";
                }
                // cout<<"\n";
                nl++;
            } else {
                for(int x=0;x<dim+1;x++) {
                    memcpy(&rightdata[12+nr*(dim+1)*4+x*4], &data[i+x*4], sizeof(int));
                }
                nr++;
            }
        }
        int padding = -1000000;
        for(int i=12+nl*4*(dim+1); i<PAGE_CONTENT_SIZE; i+=4) {
            memcpy(&data[i], &padding, sizeof(int));
        }
        int newSplitDim = (split+1)%dim;
        memcpy(&data[0], &newSplitDim, sizeof(int));
        for(int i=12;i<PAGE_CONTENT_SIZE;i+=4){
            int value;
            memcpy(&value, &data[i], sizeof(int));
            if(value==-1000000){
                break;
            }
            cout<<value<<" ";
        }
        cout<<"\n";
        
        // New root creation
        if(parentNode == -1){
            cout<<"new root creation\n";
            PageHandler rootp;
            createEmptyRangeNode(rootp, split, -1);
            char* rootdata = rootp.GetData();
            rootNode = rootp.GetPageNum();
            int rmin=-99999;
            int rmax = INT_MAX;
            for(int x=0;x<2*dim;x+=2){
                memcpy(&rootdata[12+x*4], &rmin, sizeof(int));
                memcpy(&rootdata[12+x*4+4], &rmax, sizeof(int));
            }
            memcpy(&rootdata[12+split*4+4], &splitValue, sizeof(int));
            int leftChild = ph.GetPageNum();
            memcpy(&rootdata[12+(2*dim)*4], &leftChild, sizeof(int));
            for(int x=0;x<2*dim;x+=2){
                memcpy(&rootdata[12+(2*dim+1)*4+x*4], &rmin, sizeof(int));
                memcpy(&rootdata[12+(2*dim+1)*4+x*4+4], &rmax, sizeof(int));
            }
            memcpy(&rootdata[12+(2*dim+1)*4+split*4], &splitValue, sizeof(int));
            int rightChild = rp.GetPageNum();
            memcpy(&rootdata[12+(2*dim+1)*4+(2*dim)*4], &rightChild, sizeof(int));

            memcpy(&data[8], &rootNode, sizeof(int));
            memcpy(&rightdata[8], &rootNode, sizeof(int));
            // for(int i=12;i<PAGE_CONTENT_SIZE;i+=4) {
            //     int value;
            //     memcpy(&value, &data[i], sizeof(int));
            //     if(value==-1000000){
            //         break;
            //     }
            //     cout<<value<<" ";
            // }
            // cout<<"\n";
            fh.MarkDirty(ph.GetPageNum());
            fh.UnpinPage(ph.GetPageNum());
            fh.MarkDirty(rp.GetPageNum());
            fh.UnpinPage(rp.GetPageNum());
            fh.MarkDirty(rootNode);
            return true;
        }

        // cout<<"get parent node\n";
        PageHandler parent = fh.PageAt(parentNode);
        printBufferlog();
        char* parentdata = parent.GetData();
        memcpy(&parentdata[0], &split, sizeof(int));
        int region = -1;
        for(int i=12;i<PAGE_CONTENT_SIZE;i+=(2*dim+1)*4) {
            int childpagenum;
            memcpy(&childpagenum, &parentdata[i+(2*dim)*4], sizeof(int));
            // cout<<childpagenum<<"child page num \n";
            if(childpagenum==ph.GetPageNum()){
                region = i;
                break;
            }
        }

        // cout<<"process parent region node\n";
        // Find number of regions in the region node
        int numregions = 0;
        for(int i=12;i<PAGE_CONTENT_SIZE;i+=(2*dim+1)*4) {
            int value;
            memcpy(&value, &parentdata[i], sizeof(int));
            if(value==-1000000) {
                break;
            }
            numregions++;
        }
        cout<<numregions<<"regions in parent node\n";
        for(int x=0;x<2*dim;x++) {
            int transfer;
            memcpy(&transfer, &parentdata[region+x*4], sizeof(int));
            memcpy(&parentdata[12+numregions*(2*dim+1)*4+x*4], &transfer, sizeof(int));
            // cout<<transfer<<" ";
        }
        // cout<<"\n";
        // cout<<"before right pagenum\n";
        int rightpagenum = rp.GetPageNum();
        // cout<<"after right pagenum\n";
        memcpy(&parentdata[12+numregions*(2*dim+1)*4+(2*dim)*4], &rightpagenum, sizeof(int));
        memcpy(&parentdata[12+numregions*(2*dim+1)*4+(2*split)*4], &splitValue, sizeof(int));
        memcpy(&parentdata[region+(2*split)*4+4], &splitValue, sizeof(int));
        // for(int x=0;x<2*dim;x++) {
        //     int tempo;
        //     memcpy(&tempo, &parentdata[12+2*(2*dim+1)*4+x*4], sizeof(int));
        //     cout<<tempo<<" ";
        // }
        // cout<<"\n";
        numregions++;

        // region node overflowed
        // cout<<"before ph markdirty and unpin\n";
        fh.MarkDirty(ph.GetPageNum());
        fh.UnpinPage(ph.GetPageNum());
        // cout<<"after ph mark dirty and unpin\n";
        fh.MarkDirty(rp.GetPageNum());
        fh.UnpinPage(rp.GetPageNum());
        cout<<numregions<<" after rp mark dirty and unpin\n";
        if(numregions == region_node_per_page) {
            cout<<"reoragnize parent\n";
            reorganize(parent);
            return true;
        }
    } else {
        fh.UnpinPage(ph.GetPageNum());
    }

    // fh.UnpinPage(ph.GetPageNum());
    // cout<<"return \n";
    return true;

}

void searchAndPrintInsertion(vector<int>& point) {
    PageHandler ph = fh.PageAt(rootNode);
    char* data;
    // Traverse down the tree to the point node
    while(true){
        data = ph.GetData();
        int indicator=-1;
        memcpy(&indicator, &data[4], sizeof(int));
        if(indicator==1) {
            break;
        }
        // cout<<"traverse down\n";
        for(int i=12;i<PAGE_CONTENT_SIZE;i+=(2*dim+1)*4) {
            bool correct=true;
            for(int x=0;x<dim;x++) {
                int rmin=0;
                memcpy(&rmin, &data[i+2*x*4], sizeof(int));
                int rmax=0;
                memcpy(&rmax, &data[i+2*x*4+4], sizeof(int));
                if(point[x]<rmax && point[x]>=rmin) {
                    continue;
                } else {
                    correct=false;
                    break;
                }
            }
            if(!correct) {
                continue;
            }
            int childPage=-1;
            memcpy(&childPage, &data[i+2*dim*4], sizeof(int));
            fh.UnpinPage(ph.GetPageNum());
            ph = fh.PageAt(childPage);
        }
    }

    bool correct = false;
    for(int i=12;i<PAGE_CONTENT_SIZE;i+=(dim+1)*4) {
        int value=0;
        memcpy(&value, &data[i], sizeof(int));
        if(value==-1000000) {
            break;
        }
        bool pointalreadyin = true;
        for(int x=0;x<dim;x++) {
            memcpy(&value, &data[i+x*4], sizeof(int));
            // cout<<value<<" ";
            if(point[x]!=value){
                pointalreadyin=false;
                break;
            }
        }
        // cout<<"\n";
        if(pointalreadyin){
            correct= true;
            break;
        }
    }
    if(!correct) {
        cout<<"not present\n";
        out<<"INSERTION FALSE\n";
    } else {
        cout<<"present\n";
        out<<"INSERTION DONE:\n";
        for(int x=0;x<PAGE_CONTENT_SIZE;x++) {
            int value;
            memcpy(&value, &data[12+x*(dim+1)*4], sizeof(int));
            if(value==-1000000){
                break;
            }
            for(int j=0;j<dim;j++) {
                memcpy(&value, &data[12+x*(dim+1)*4+j*4], sizeof(int));
                out<<value<<" ";
            }
            out<<"\n";
        }
    }
    fh.UnpinPage(ph.GetPageNum());
    return;
}

void psearch(vector<int>& point) {
    bool isEmpty = false;
    PageHandler ph;
    if(rootNode!=-1) {
        ph = fh.PageAt(rootNode);
        printBufferlog();
    } else {
        isEmpty = true;
    }

    // root is NULL
    if(isEmpty) {
        // cout<<"NULL root\n";
        out<<"NUM REGION NODES TOUCHED: 0\n";
        out<<"FALSE\n";
        return;
    }

    // cout<<"non null search\n";

    int nodestouched = 0;
    char* data;
    // Traverse down the tree to the point node
    // cout<<"Traverse down\n";
    while(true){
        // cout<<"traverse\n";
        // fm.PrintBuffer();
        data = ph.GetData();
        int indicator=-1;
        memcpy(&indicator, &data[4], sizeof(int));
        if(indicator==1) {
            break;
        }
        nodestouched++;
        // cout<<"not break\n";
        for(int i=12;i<PAGE_CONTENT_SIZE;i+=(2*dim+1)*4) {
            bool correct=true;
            for(int x=0;x<dim;x++) {
                int rmin=0;
                memcpy(&rmin, &data[i+2*x*4], sizeof(int));
                int rmax=0;
                memcpy(&rmax, &data[i+2*x*4+4], sizeof(int));
                if(point[x]<rmax && point[x]>=rmin) {
                    continue;
                } else {
                    correct=false;
                    break;
                }
            }
            if(!correct) {
                continue;
            }
            for(int x=0;x<dim;x++) {
                int rmin=0;
                memcpy(&rmin, &data[i+2*x*4], sizeof(int));
                int rmax=0;
                memcpy(&rmax, &data[i+2*x*4+4], sizeof(int));
                cout<<rmin<<" "<<rmax<<" ";
            }
            cout<<"\n";
            int childPage=-1;
            memcpy(&childPage, &data[i+2*dim*4], sizeof(int));
            fh.UnpinPage(ph.GetPageNum());
            ph = fh.PageAt(childPage);
            printBufferlog();
        }
    }
    // cout<<"traversed down\n";

    for(int i=12;i<PAGE_CONTENT_SIZE;i+=(dim+1)*4) {
        int value=0;
        memcpy(&value, &data[i], sizeof(int));
        if(value==-1000000) {
            // cout<<"break at: "<<i<<"\n";
            break;
        }
        bool pointalreadyin = true;
        for(int x=0;x<dim;x++) {
            memcpy(&value, &data[i+x*4], sizeof(int));
            // cout<<value<<" ";
            if(point[x]!=value){
                pointalreadyin=false;
                break;
            }
        }
        // cout<<"\n";
        if(pointalreadyin){
            fh.UnpinPage(ph.GetPageNum());
            out<<"NUM REGION NODES TOUCHED: "<<nodestouched<<"\n";
            out<<"TRUE\n";
            return;
        }
    }
    out<<"NUM REGION NODES TOUCHED: 0\n";
    out<<"FALSE\n";
    return;
}

bool dfs_rsearch(PageHandler& ph, vector<int>& range, int depth) {
    char* data = ph.GetData();
    int indicator;
    memcpy(&indicator, &data[4], sizeof(int));
    if(indicator == 1) {
        cout<<"point node\n";
        // check all points if they lie in given range.
        bool found = false;
        for(int i=12;i<PAGE_CONTENT_SIZE;i+=(dim+1)*4) {
            bool inrange = true;
            int checkpad;
            memcpy(&checkpad, &data[i], sizeof(int));
            if(checkpad==-1000000) {
                inrange = false;
                break;
            }
            for(int x=0;x<dim;x++) {
                int value;
                memcpy(&value, &data[i+x*4], sizeof(int));
                if(value==-1000000) {
                    inrange = false;
                    break;
                }
                if(value<=range[x*2+1] && value>=range[x*2]) {
                    continue;
                } else {
                    inrange = false;
                    break;
                }
            }
            if(inrange) {
                found = true;
                out<<"POINT: ";
                for(int x=0;x<dim;x++) {
                    int value;
                    memcpy(&value, &data[i+x*4], sizeof(int));
                    out<<value<<" ";
                }
                out<<"NUM REGION NODES TOUCHED: "<<depth<<"\n";
            }
        }
        return found;
    } else {
        // recurse on overlapping range
        cout<<"region node\n";
        bool found = false;
        for(int i=12;i<PAGE_CONTENT_SIZE;i+=(2*dim+1)*4) {
            bool inrange = true;
            int checkpad;
            memcpy(&checkpad, &data[i], sizeof(int));
            if(checkpad==-1000000) {
                inrange = false;
                break;
            }
            for(int x=0;x<dim;x++) {
                int rmin;
                memcpy(&rmin, &data[i+x*4], sizeof(int));
                if(rmin==-1000000) {
                    inrange = false;
                    break;
                }
                int rmax;
                memcpy(&rmax, &data[i+x*4+4], sizeof(int));
                if((rmin>=range[x*2] && rmin<=range[x*2+1]) || (rmax>=range[x*2]+1 && rmax<=range[x*2+1]+1)) {
                    continue;
                } else {
                    inrange = false;
                    break;
                }
            }
            if(inrange) {
                int childpagenum;
                memcpy(&childpagenum, &data[i+(2*dim)*4], sizeof(int));
                cout<<childpagenum<<" in rsearch\n";
                int currentpage = ph.GetPageNum();
                fh.UnpinPage(currentpage);
                PageHandler child = fh.PageAt(childpagenum);
                printBufferlog();
                found = found || dfs_rsearch(child, range, depth+1);
                fh.UnpinPage(childpagenum);
                ph = fh.PageAt(currentpage);
                printBufferlog();
            }
        }
        return found;
    }
}

void rsearch(vector<int>& range) {
    bool isEmpty = false;
    PageHandler ph;
    if(rootNode!=-1) {
        ph = fh.PageAt(rootNode);
        printBufferlog();
    } else {
        isEmpty = true;
    }

    // root is NULL
    if(isEmpty) {
        cout<<"NULL root\n";
        out<<"NO POINT FOUND\n";
        return;
    }

    bool found = dfs_rsearch(ph, range, 0);
    if(!found) {
        out<<"NO POINT FOUND\n";
        return;
    }
    
}

int main(int argc, char* argv[]) {

    if(argc !=4 ) {
        cout<<"INVALID ARGUMENTS!!!\n";
    }

    const char* input_file = argv[1];
    const char* output_file = argv[3];
    cout<<argv[2]<<"\n";
    dim = stoi(argv[2]);
    point_node_per_page = (PAGE_CONTENT_SIZE - 3*sizeof(int) ) / ((dim+1)*sizeof(int));
    region_node_per_page = (PAGE_CONTENT_SIZE - 3*sizeof(int) ) / ((dim*2+1)*sizeof(int));
    // TODO: comment before submission
    // point_node_per_page = 4;
    // region_node_per_page = 4;
    cout<<"point per node: "<<point_node_per_page<<"\n";
    cout<<"region per node: "<<region_node_per_page<<"\n";

    in.open(input_file);
    if ( ! in.is_open() ) {                 
        cout <<" Failed to open input file" << endl;
    }
    else {
        cout <<"Input file Opened OK" << endl;
    }
    out.open(output_file);
    if ( ! out.is_open() ) {                 
        cout <<" Failed to open output file" << endl;
    }
    else {
        cout <<"Output file Opened OK" << endl;
    }

	// Create a brand new file
	fh = fm.CreateFile("temp.txt");
	cout << "Temp File created " << endl;

    string line;
    while(getline(in,line)) {
        cout<<line<<"\n";
        int n=line.size();
        char str[n+1];
        strcpy(str,line.c_str());
        char* temp = strtok(str," ");
        vector<string> tokens;
        while(temp != NULL){
            tokens.push_back(temp);
            temp = strtok(NULL," ");
        }
        
        if(tokens[0] == "INSERT") {
            // cout<<"Insert called\n";
            vector<int> point(dim);
            for(int i=0;i<dim;i++) {
                point[i] = stoi(tokens[i+1].c_str());
                // cout<<point[i]<<"\n";
            }
            bool flag=insert(point);
            if(!flag){
                cout<<"continue code due to duplicate insertion";
            } else {
                searchAndPrintInsertion(point);
            }
        }
        else if(tokens[0] == "PQUERY") {
            vector<int> point(dim);
            for(int i=0;i<dim;i++) {
                point[i] = stoi(tokens[i+1].c_str());
            }
            psearch(point);
        }
        else {
            vector<int> range(dim*2);
            for(int i=0;i<dim*2;i++) {
                range[i] = stoi(tokens[i+1].c_str());
            }
            rsearch(range);
        }
        out<<"\n";
        out<<"\n";
    }

    fh.FlushPages();

	// Close the file and destory it
	fm.CloseFile (fh);
	fm.DestroyFile ("temp.txt");

    in.close();
    out.close();

    return 0;
}
