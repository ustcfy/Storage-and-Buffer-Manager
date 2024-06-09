#ifndef DSMGR_H
#define DSMGR_H

#include <string>
#include <cstdio>
#include <iostream>

#define MAXPAGES 50000 // Assuming a maximum of 50,000 pages
#define FRAMESIZE 4096
#define DEFBUFSIZE 1024

struct bFrame
{
    char field[FRAMESIZE];
};

class DSMgr
{
public:
    DSMgr();
    int OpenFile(std::string filename);
    int CloseFile();
    bFrame ReadPage(int page_id);
    int WritePage(int page_id, bFrame frm); // 返回写入的字节数
    int Seek(int offset, int pos);
    FILE *GetFile();
    void IncNumPages();
    int GetNumPages();
    void SetUse(int index, int use_bit);
    int GetUse(int index);
    int GetDiskIO();
private:
    FILE *currFile;
    int numPages;        // 文件中的页面数量
    int pages[MAXPAGES]; // 页面的使用状态
    int readCount;
    int writeCount;
};

#endif // DSMGR_H
