#ifndef __FUSION_H
#define __FUSION_H

#include <fstream>
#include "lm.h"
#include "em.h"

class Fusion
{
  public:
    EBasicBlock * bb;
    Fusion(EBasicBlock * bb);
    void doFusion();
    bool fuse(PlatformLevel * & fuse_p, PlatformLevel * stmt_p);
    bool fuse(DeviceLevel * & fuse_p, DeviceLevel * stmt_p);
    bool fuse(BlockLevel * & fuse_p, BlockLevel * stmt_p);
    bool fuse(ThreadLevel * & fuse_p, ThreadLevel * stmt_p);
};

class Edge
{
  public:
    EStmt * src;
    EParam * src_param;
    EStmt * dst;
    EParam * dst_param;
    Edge(EStmt *src, EParam* src_param, EStmt * dst, EParam * dst_param) : 
                 src(src), src_param(src_param), dst(dst), dst_param(dst_param) {};
};



#endif
