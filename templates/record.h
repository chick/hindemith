#ifndef __RECORD_H
#define __RECORD_H
#include <fstream>
#include "em.h"

void openLog(const char * fname);
void closeLog();

void startDigraph(int blockId);
void endDigraph();

void emit_tree(EBasicBlock * bb);
void emit_tree(PlatformLevel * p);
void emit_tree(DeviceLevel * p);
void emit_tree(BlockLevel * b);
void emit_tree(ThreadLevel * t);

#endif
