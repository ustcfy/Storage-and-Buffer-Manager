#include <iostream>
#include <fstream>
#include <string>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <ctime>
#include "src/DSMgr.cpp"
#include "src/BMgr.cpp"
using namespace std;

int main()
{
    BMgr *bmgr = new BMgr(CacheAlgorithm::LRU);
    int diskIO = 0;
    double BufferHitRate = 0;
    clock_t start, end;
    string line;

    // 创建一个 5万个page（页号从0到49999）的堆文件
    bmgr->dsmgr.OpenFile("data/data.dbf");
    for (int i = 0; i < 50000; i++)
    {
        bmgr->FixNewPage();
        bmgr->UnfixPage(i);
    }
    bmgr->WriteDirtys();
    bmgr->dsmgr.CloseFile();
    delete bmgr;

    // 读取trace文件
    bmgr = new BMgr(CacheAlgorithm::CLOCK);
    bmgr->PrintCacheAlgorithm();
    bmgr->dsmgr.OpenFile("data/data.dbf");
    ifstream trace("data/data-5w-50w-zipf.txt");
    if (!trace.is_open())
    {
        cerr << "Error: Failed to open trace file." << endl;
        return -1;
    }
    start = clock();
    while (getline(trace, line))
    {
        // 读取trace文件中的每一行 mode,page_id
        int mode, page_id;
        sscanf(line.c_str(), "%d,%d", &mode, &page_id);
        page_id--;
        if (mode == 0) // 读
            bmgr->FixPage(page_id, 0);
        else // 写
            bmgr->FixPage(page_id, 1);
        bmgr->UnfixPage(page_id);
    }
    bmgr->WriteDirtys();
    bmgr->dsmgr.CloseFile();
    delete bmgr;
    trace.close();
    end = clock();

    bmgr->dsmgr.GetDiskIO();
    cout << "BufferHitRate: " << bmgr->GetBufferHitRate() << endl;
    cout << "Time: " << (double)(end - start) / CLOCKS_PER_SEC << "s" << endl;

    return 0;
}