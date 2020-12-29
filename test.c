
#include "metadata.h"

int main() {
    yrx_init_lfs(&lfs);
    struct INode fnode, node;
    int tid = yrx_begintransaction(lfs);
    yrx_readinodefrompath(lfs, tid, '/', &fnode, 1000);
    yrx_createinode(lfs, tid, &node, 33279, 1000, 1000);
    yrx_linkfile(lfs, tid, &fnode, "a", &node);
    yrx_endtransaction(lfs, tid);
    char * file = malloc(100000);
    memset(file, 1, 100000);
    tid = yrx_begintransaction(lfs);
    yrx_readinodefrompath(lfs, tid, "/a", &node, 1000);
    yrx_writefile(lfs, tid, file, &node, 100000, 0);
    yrx_endtransaction(lfs, tid);
    tid = yrx_begintransaction(lfs);
    char * result = malloc(100000);
    yrx_readinodefrompath(lfs, tid, "/a", &node, 1000);
    yrx_readfilefrominode(lfs, tid, result, &node, 100000, 0);
    yrx_endtransaction(tid, lfs);
    return 0;
}