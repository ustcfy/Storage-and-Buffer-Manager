#include "../include/BMgr.h"

BMgr::BMgr(CacheAlgorithm algorithm)
{
    // 构造函数初始化
    for (int i = 0; i < DEFBUFSIZE; ++i)
    {
        ftop[i] = -1; // 初始化 hash 表
        ptof[i] = nullptr;
        buf[i] = bFrame();
    }
    bufferHitCount = 0;
    bufferAccessCount = 0;
    switch (algorithm)
    {
    case LRU:
        this->algorithm = LRU;
        break;
    case CLOCK:
        this->algorithm = CLOCK;
        for (int i = 0; i < DEFBUFSIZE; ++i)
            circQueue[i] = 1;
        current = 0;
        break;
    case LRU2:
        this->algorithm = LRU2;
        break;
    default:
        std::cerr << "Invalid CacheAlgorithm: " << algorithm << std::endl;
        break;
    }
}

// prot=0表示读，prot=1表示写,prot = 2表示fixnewpage
int BMgr::FixPage(int page_id, int prot)
{
    // 检查是否在buffer中
    int fix_count = 0;
    int frame_id = -1; // 初始化 frame_id 为无效值
    int hash_value = Hash(page_id);

    BCB *bcb = ptof[hash_value];
    while (bcb != nullptr)
    {
        if (bcb->page_id == page_id)
        {
            frame_id = bcb->frame_id;
            break;
        }
        bcb = bcb->next;
    }

    if (frame_id != -1)
    { // 增加页面的 fix_count
        bcb->count++;
        bufferHitCount++;
        fix_count = bcb->count;
    }
    else // 未找到对应的 BCB，可能是未固定的页面
    {
        // 选择需要替换的页面
        frame_id = SelectVictim();
        ftop[frame_id] = page_id;
        if (ptof[hash_value] == nullptr)
        {
            ptof[hash_value] = new BCB();
            ptof[hash_value]->page_id = page_id;
            ptof[hash_value]->frame_id = frame_id;
            ptof[hash_value]->count = 1;
        }
        else
        {
            BCB *bcb = ptof[hash_value];
            while (bcb->next != nullptr)
                bcb = bcb->next;

            bcb->next = new BCB();
            bcb->next->page_id = page_id;
            bcb->next->frame_id = frame_id;
            bcb->next->count = 1;
        }
        // 从磁盘读取页面
        if (prot != 2)
            buf[frame_id] = dsmgr.ReadPage(page_id);
    }

    if (prot == 1)
        SetDirty(frame_id);
    bufferAccessCount++;
    return frame_id;
}

NewPage BMgr::FixNewPage()
{
    NewPage newPage;
    int i;
    // 找到第一个空闲的页面
    for (i = 0; i < dsmgr.GetNumPages(); i++)
    {
        if (dsmgr.GetUse(i) == 0)
            break;
    }
    // 如果没有空闲的页面，需要增加一个页面
    if (i == dsmgr.GetNumPages())
        dsmgr.IncNumPages();
    dsmgr.SetUse(i, 1);
    newPage.page_id = i;
    newPage.frame_id = FixPage(i, 2);

    return newPage;
}
// 返回 frame_id
int BMgr::UnfixPage(int page_id)
{
    // 检查是否在buffer中
    int frame_id = -1; // 初始化 frame_id 为无效值
    int flag = 0;
    int hash_value = Hash(page_id);

    BCB *bcb = ptof[hash_value];
    while (bcb != nullptr)
    {
        if (bcb->page_id == page_id)
        {
            frame_id = bcb->frame_id;
            break;
        }
        bcb = bcb->next;
    }

    if (frame_id == -1)
    {
        std::cerr << "Error: Page " << page_id << " is not in buffer." << std::endl;
        return -1;
    }
    // 减少页面的 fix_count
    bcb->count--;

    // 如果页面的 fix_count 为 0，
    if (bcb->count == 0)
    {
        // 更新
        switch (algorithm)
        {
        case LRU:
            lruQueue.remove(frame_id);
            lruQueue.push_back(frame_id);
            break;
        case LRU2:
            for (auto it = lru2Queue.begin(); it != lru2Queue.end(); ++it)
            {
                if (*it == frame_id)
                {
                    lru2Queue.remove(frame_id);
                    lru2Queue.push_back(frame_id);
                    flag = 1;
                    break;
                }
            }
            // 如果在 lru2Queue 中没有找到 frame_id
            if (flag == 0)
            {
                // 在 lruQueue 中查找 frame_id
                for(auto it = lruQueue.begin(); it != lruQueue.end(); ++it)
                {
                    if (*it == frame_id)
                    {
                        lruQueue.remove(frame_id);
                        lru2Queue.push_back(frame_id);
                        flag = 1;
                        break;
                    }
                }
                // 如果在 lruQueue 中也没有找到 frame_id
                if(flag == 0)
                    lruQueue.push_back(frame_id);
            }
            break;
        default:
            break;
        }
    }

    return frame_id;
}
// 返回可用的 buffer 页面数
int BMgr::NumFreeFrames()
{
    int freeFrames = 0;

    for (int i = 0; i < DEFBUFSIZE; ++i)
        if (ftop[i] == -1)
            freeFrames++;
    return freeFrames;
}
// 返回 frame_id
int BMgr::SelectVictim()
{
    int victim_frame_id;
    // 1.检查是否有空闲的 frame
    if (NumFreeFrames() != 0)
    {
        for (int i = 0; i < DEFBUFSIZE; ++i)
        {
            if (ftop[i] == -1)
                return i;
        }
    }
    // 2.如果没有空闲的 frame，需要选择一个页面进行替换
    switch (algorithm)
    {
    case LRU:
        victim_frame_id = lruQueue.front();
        lruQueue.pop_front();
        break;
    case CLOCK:
        while (1)
        {
            int page_id = ftop[current];
            BCB *bcb = ptof[Hash(page_id)];
            for (; bcb != nullptr && bcb->page_id != page_id; bcb = bcb->next)
                ;

            if (bcb != nullptr)
            {
                if (circQueue[current] == 1)
                {
                    circQueue[current] = 0;
                    current = (current + 1) % DEFBUFSIZE;
                }
                else if (bcb->count == 0)
                {
                    victim_frame_id = current;
                    current = (current + 1) % DEFBUFSIZE;
                    break;
                }
            }
        }
        break;
    case LRU2:
        if (lruQueue.size() == 0)
        {
            victim_frame_id = lru2Queue.front();
            lru2Queue.pop_front();
        }
        else
        {
            victim_frame_id = lruQueue.front();
            lruQueue.pop_front();
        }
        break;
    default:
        break;
    }
    // 如果选择的页面是脏的，需要写回磁盘
    int page_id = ftop[victim_frame_id];
    int hash_value = Hash(page_id);
    for (BCB *bcb = ptof[hash_value]; bcb != nullptr; bcb = bcb->next)
    {
        if (bcb->page_id == page_id)
        {
            if (bcb->dirty)
                dsmgr.WritePage(page_id, buf[victim_frame_id]);
            break;
        }
    }
    // 移除 BCB
    RemoveBCB(ptof[hash_value], page_id);
    // 返回需要替换的 frame_id
    return victim_frame_id;
}
// 返回 frame_id
int BMgr::Hash(int page_id)
{
    // 使用静态哈希表的取模运算
    return page_id % DEFBUFSIZE;
}
void BMgr::PrintCacheAlgorithm() 
{
    switch (algorithm)
    {
    case LRU:
        std::cout << "Cache Algorithm: LRU" << std::endl;
        break;
    case CLOCK:
        std::cout << "Cache Algorithm: CLOCK" << std::endl;
        break;
    case LRU2:
        std::cout << "Cache Algorithm: LRU2" << std::endl;
        break;
    default:
        std::cerr << "Invalid CacheAlgorithm: " << algorithm << std::endl;
        break;
    }
}
void BMgr::RemoveBCB(BCB *ptr, int page_id)
{
    // 如果 BCB 链表为空，直接返回
    int hash_value = Hash(page_id);
    if (ptof[hash_value] == nullptr)
        return;

    // 如果 BCB 链表的第一个元素就是要删除的元素
    if (ptof[hash_value]->page_id == page_id)
    {
        // 删除第一个元素
        BCB *temp = ptof[hash_value];
        ptof[hash_value] = ptof[hash_value]->next;
        delete temp;
        return;
    }

    BCB *bcb = ptof[hash_value];
    while (bcb->next != nullptr)
    {
        if (bcb->next->page_id == page_id)
        {
            // 删除 bcb->next
            BCB *temp = bcb->next;
            bcb->next = bcb->next->next;
            delete temp;
            return;
        }
        bcb = bcb->next;
    }
}

void BMgr::RemoveLRUEle(int frid)
{
}

void BMgr::SetDirty(int frame_id)
{
    // 检查 frame_id 是否在合法范围内
    if (frame_id < 0 || frame_id >= DEFBUFSIZE)
    {
        std::cerr << "Invalid frame_id: " << frame_id << std::endl;
        return;
    }
    int page_id = ftop[frame_id];
    int hash_value = Hash(page_id);
    for (BCB *bcb = ptof[hash_value]; bcb != nullptr; bcb = bcb->next)
    {
        if (bcb->page_id == page_id)
        {
            bcb->dirty = 1;
            break;
        }
    }
}

void BMgr::UnsetDirty(int frame_id)
{
    // 检查 frame_id 是否在合法范围内
    if (frame_id < 0 || frame_id >= DEFBUFSIZE)
    {
        std::cerr << "Invalid frame_id: " << frame_id << std::endl;
        return;
    }
    int page_id = ftop[frame_id];
    int hash_value = Hash(page_id);
    for (BCB *bcb = ptof[hash_value]; bcb != nullptr; bcb = bcb->next)
    {
        if (bcb->page_id == page_id)
        {
            bcb->dirty = 0;
            break;
        }
    }
}

void BMgr::WriteDirtys()
{
    // 遍历缓冲区，将脏页写回磁盘
    for (int i = 0; i < DEFBUFSIZE; ++i)
    {
        BCB *currentBCB = ptof[i];
        while (currentBCB != nullptr)
        {
            if (currentBCB->dirty) // 如果是脏页，则写回磁盘
            {
                dsmgr.WritePage(currentBCB->page_id, buf[currentBCB->frame_id]);
                currentBCB->dirty = 0; // 重置 dirty 位
            }
            currentBCB = currentBCB->next;
        }
    }
}

void BMgr::PrintFrame(int frame_id)
{
    if (frame_id < 0 || frame_id >= DEFBUFSIZE)
    {
        std::cerr << "Invalid frame_id: " << frame_id << std::endl;
        return;
    }

    // 打印帧的内容
    std::cout << "Frame ID: " << frame_id << std::endl;
    std::cout << "Frame Content: " << buf[frame_id].field << std::endl;
}
double BMgr::GetBufferHitRate()
{
    return (double)bufferHitCount / bufferAccessCount;
}
