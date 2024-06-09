#ifndef BMGR_H
#define BMGR_H
#include <list>
#include "DSMgr.h"
struct NewPage
{
    int page_id;
    int frame_id;
};
// Buffer Control Block
struct BCB
{
    BCB()
    {
        page_id = -1;
        frame_id = -1;
        latch = 0;
        count = 0;
        dirty = 0;
        next = nullptr;
    }
    int page_id;
    int frame_id;
    int latch;
    int count;
    int dirty;
    BCB *next;
};
// 定义缓存置换算法的枚举类型
enum CacheAlgorithm
{
    LRU,
    CLOCK,
    LRU2,
};
class BMgr
{
public:
    BMgr(CacheAlgorithm algorithm);
    // Interface functions
    int FixPage(int page_id, int prot);
    NewPage FixNewPage();
    int UnfixPage(int page_id);
    int NumFreeFrames();
    // Internal Functions
    int SelectVictim();
    int Hash(int page_id);
    void RemoveBCB(BCB *ptr, int page_id);
    void RemoveLRUEle(int frid);
    void SetDirty(int frame_id);
    void UnsetDirty(int frame_id);
    void WriteDirtys();
    void PrintFrame(int frame_id);
    void PrintCacheAlgorithm();
    // 新增获取缓冲区命中率的接口
    double GetBufferHitRate();
    DSMgr dsmgr;

private:
    CacheAlgorithm algorithm;
    // Hash Table
    int ftop[DEFBUFSIZE];  // Frame to page mapping
    BCB *ptof[DEFBUFSIZE]; // Page to frame mapping
    bFrame buf[DEFBUFSIZE];
    int bufferHitCount;    // 记录缓冲区命中次数
    int bufferAccessCount; // 记录缓冲区访问总次数
    // LRU quene
    std::list<int> lruQueue;
    // LRU2 quene
    std::list<int> lru2Queue;
    // CLOCK quene
    int circQueue[DEFBUFSIZE];
    int current;
};
#endif // BMGR_H