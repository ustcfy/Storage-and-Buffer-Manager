#include "../include/DSMgr.h"

DSMgr::DSMgr()
{
    // 构造函数初始化
    currFile = nullptr;
    numPages = 0;
    readCount = 0;
    writeCount = 0;
    for (int i = 0; i < MAXPAGES; ++i)
        pages[i] = 0; // 初始化页面使用信息
}

int DSMgr::OpenFile(std::string filename)
{
    // 检查文件是否已打开
    if (currFile)
    {
        std::cerr << "Error: File already open." << std::endl;
        return -1; // 返回错误码
    }
    // 尝试打开文件
    currFile = fopen(filename.c_str(), "rb+");
    if (!currFile)
    {
        std::cerr << "Error: Failed to open file." << std::endl;
        return -2; // 返回错误码
    }

    // 计算页面数量
    fseek(currFile, 0, SEEK_END);
    numPages = ftell(currFile) / sizeof(bFrame);
    
    return 0; // 返回0表示操作成功
}

int DSMgr::CloseFile()
{
    if (!currFile)
    {
        std::cerr << "Error: File not open." << std::endl;
        return -1;
    }

    // 尝试关闭文件
    if (fclose(currFile) != 0) // 如果关闭文件失败，返回错误码
        return -1;
    // 重置当前文件指针和页面计数器
    currFile = nullptr;
    numPages = 0;
    for (int i = 0; i < MAXPAGES; ++i)
        pages[i] = 0;

    return 0;
}

bFrame DSMgr::ReadPage(int page_id)
{
    bFrame frame;
    // 检查文件是否已打开
    if (!currFile)
    {
        std::cerr << "Error: File not open." << std::endl;
        // 返回一个表示错误的 bFrame
        return frame;
    }
    if (page_id < 0 || page_id >= numPages)
    {
        std::cerr << "Error: Invalid page_id." << std::endl;
        // 返回一个表示错误的 bFrame
        return frame;
    }
    // 将文件指针移动到正确的位置
    if (fseek(currFile, page_id * sizeof(bFrame), SEEK_SET) != 0)
    {
        std::cerr << "Error: Failed to move file pointer for reading." << std::endl;
        // 返回一个表示错误的 bFrame
        return frame;
    }

    // fread 函数的返回值是成功读取的数据块数。
    if (fread(&frame, sizeof(bFrame), 1, currFile) != 1)
    {
        std::cerr << "Error: Failed to read data from file." << std::endl;
        // 返回一个表示错误的 bFrame
        return frame;
    }
    // 如果需要，可以在这里更新一些元数据，例如页面使用信息等 
    readCount++;
    return frame; // 返回读取的页面
}
  
int DSMgr::WritePage(int page_id, bFrame frm)
{ // 检查文件是否已打开
    if (!currFile)
    {
        std::cerr << "Error: File not open for writing." << std::endl;
        return -1; // 返回错误码
    }
    
    // 将文件指针移动到对应位置
    if (fseek(currFile, page_id * sizeof(bFrame), SEEK_SET) != 0)
    {
        std::cerr << "Error: Failed to move file pointer for writing." << std::endl;
        return -3; // 返回错误码
    }

    // fwrite 函数的返回值是成功写入的数据块数。
    if (fwrite(&frm, sizeof(bFrame), 1, currFile) != 1)
    {
        std::cerr << "Error: Failed to write data to file." << std::endl;
        return -4; // 返回错误码
    }
    // 刷新文件缓冲区，确保数据被写入磁盘
    fflush(currFile);
    writeCount++;
    return sizeof(bFrame); // 返回写入的字节数
}

int DSMgr::Seek(int offset, int pos)
{
    if (!currFile)
    {
        std::cerr << "Error: File not open." << std::endl;
        return -1;
    }
    // 将文件指针移动到对应位置
    if (fseek(currFile, offset + pos, SEEK_SET) != 0)
    {
        std::cerr << "Error: Failed to move file pointer." << std::endl;
        return -2;
    }
    return 0;
}

FILE *DSMgr::GetFile()
{
    return currFile;
}

void DSMgr::IncNumPages()
{
    numPages++;
}

int DSMgr::GetNumPages()
{
    return numPages;
}

void DSMgr::SetUse(int index, int use_bit)
{
    // 处理索引越界
    if (index < 0 || index >= MAXPAGES)
    {
        std::cerr << "Error: Invalid index." << std::endl;
        return;
    }
    // 处理 use_bit 参数错误
    if (use_bit != 0 && use_bit != 1)
    {
        std::cerr << "Error: Invalid use_bit." << std::endl;
        return;
    }
    // 设置页面使用信息
    pages[index] = use_bit;
    return;
}

int DSMgr::GetUse(int index)
{
    // 处理索引越界
    if (index < 0 || index >= MAXPAGES)
    {
        std::cerr << "Error: Invalid index." << std::endl;
        return -1;
    }
    // 返回页面使用信息
    return pages[index];
}
int DSMgr::GetDiskIO()
{
    printf("readCount: %d\t", readCount);
    printf("writeCount: %d\t", writeCount);
    printf("total I/O: %d\n", readCount + writeCount);
    return readCount + writeCount;
}
