#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>
#include <sys/mman.h>
#include "fat32.h"

struct Fat32BPB *hdr; // 指向 BPB 的数据
int mounted = -1;     // 是否已挂载成功
int date_region_start =0;
// 定义打开文件表
#define MAX_OPEN_FILES 128
struct OpenFile
{
    int cluster;
    int size;
};
struct OpenFile open_files[MAX_OPEN_FILES];

// 辅助函数：将扇区号转换为字节偏移量
static inline int sector_to_offset(int sector)
{
    return sector * hdr->BPB_BytsPerSec;
}

// 辅助函数：在目录中查找文件或子目录
static int find_in_directory(const char *filename, struct FilesInfo *dir_files) {
    for (int i = 0; i < dir_files->size; ++i) {
        struct FileInfo *file_info = &dir_files->files[i];
        struct DirEntry entry;

        // 将 FileInfo 转换为 DirEntry
        memcpy(entry.DIR_Name, file_info->DIR_Name, 11);
        entry.DIR_FileSize = file_info->DIR_FileSize;

        // 检查文件名
        char dos_filename[11];
        memset(dos_filename, ' ', 11);
        strncpy(dos_filename, filename, strlen(filename));
        dos_filename[11] = '\0';

        if (strcmp(dos_filename, entry.DIR_Name) == 0) {
            return i; // 返回文件在目录中的索引
        }
    }

    return -1; // 表示没有找到文件
}
// 辅助函数：获取指定簇的数据
static void *get_cluster_data(int cluster)
{
    int sector = hdr->BPB_RsvdSecCnt + (hdr->BPB_NumFATs * hdr->BPB_FATSz32) + ((cluster - 2) * hdr->BPB_SecPerClus);
    return (void *)((char *)hdr + sector_to_offset(sector));
}

// 辅助函数：获取下一个簇号
static int next_cluster(int cluster)
{
    // 计算 FAT 表的起始扇区和偏移量
    int fat_sector = hdr->BPB_RsvdSecCnt + (cluster / (hdr->BPB_BytsPerSec / 4));
    int fat_offset = (cluster % (hdr->BPB_BytsPerSec / 4)) * 4;
    uint32_t *fat_entry = (uint32_t *)((char *)hdr + sector_to_offset(fat_sector) + fat_offset);
    return *fat_entry & 0x0FFFFFFF; // 清除最高位以获取下一个簇号
}

// 辅助函数：获取簇链中的第一个簇号
static int first_cluster(int cluster)
{
    // 计算 FAT 表的起始扇区和偏移量
    int fat_sector = hdr->BPB_RsvdSecCnt + (cluster / (hdr->BPB_BytsPerSec / 4));
    int fat_offset = (cluster % (hdr->BPB_BytsPerSec / 4)) * 4;
    uint32_t *fat_entry = (uint32_t *)((char *)hdr + sector_to_offset(fat_sector) + fat_offset);
    return *fat_entry; // 直接获取第一个簇号
}

// 辅助函数：解析路径，找到文件对应的起始簇号和大小
static int parse_path(const char *path, int *cluster, int *size)
{
    const char *token;
    struct FilesInfo *current_dir_files = fat_readdir("/"); // 假设根目录的路径是 "/"
    int file_index;

    // 解析路径中的每个部分
    token = strtok((char *)path, "/");
    while (token != NULL) {
        file_index = find_in_directory(token, current_dir_files);
        if (file_index == -1) {
            // 如果在目录中找不到文件或子目录，释放当前目录资源并返回错误
            free(current_dir_files->files);
            free(current_dir_files);
            return -1;
        }

        // 如果找到的是文件，则返回文件信息
        if ((current_dir_files->files[file_index].DIR_Name[0] & DIRECTORY) == 0) {
            *cluster = current_dir_files->files[file_index].DIR_FileSize; // 这里假设文件大小即为簇号
            *size = current_dir_files->files[file_index].DIR_FileSize;
            free(current_dir_files->files);
            free(current_dir_files);
            return 0; // 返回 0 表示解析成功
        }

        // 如果找到的是目录，则继续读取下一级目录
        struct FileInfo next_dir_info = current_dir_files->files[file_index];
        free(current_dir_files->files);
        free(current_dir_files);
        current_dir_files = fat_readdir((char *)next_dir_info.DIR_Name); // 假设目录名即为路径
        token = strtok(NULL, "/");
    }

    // 如果路径解析完毕，释放当前目录资源并返回错误
    free(current_dir_files->files);
    free(current_dir_files);
    return -1; // 返回错误，因为路径应该指向一个文件
}


/////////
// 挂载磁盘镜像
int fat_mount(const char *path)
{
    // 只读模式打开磁盘镜像
    int fd = open(path, O_RDWR);
    if (fd < 0)
    {
        // 打开失败
        return -1;
    }
    // 获取磁盘镜像的大小
    off_t size = lseek(fd, 0, SEEK_END);
    if (size == -1)
    {
        // 获取失败
        return -1;
    }
    // 将磁盘镜像映射到内存
    hdr = (struct Fat32BPB *)mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
    if (hdr == (void *)-1)
    {
        // 映射失败
        return -1;
    }
    // 关闭文件
    close(fd);

    assert(hdr->Signature_word == 0xaa55);                   // MBR 的最后两个字节应该是 0x55 和 0xaa
    assert(hdr->BPB_TotSec32 * hdr->BPB_BytsPerSec == size); // 不相等表明文件的 FAT32 文件系统参数可能不正确
    //Data Region Start = BPB_RsvdSecCnt + (BPB_NumFATs * BPB_FATSz32) + (BPB_RootEntCnt * sizeof(struct DirEntry))
    date_region_start = hdr->BPB_RsvdSecCnt + (hdr->BPB_NumFATs *hdr->BPB_FATSz32)+ (hdr->BPB_rootEntCnt*sizeof(struct  DirEntry));
    printf("data_region_start:%d\n",date_region_start);
    // 打印 BPB 的部分信息
    printf("Some of the information about BPB is as follows\n");
    printf("OEM-ID \"%s\", \n", hdr->BS_oemName);
    printf("sectors/cluster %d, \n", hdr->BPB_SecPerClus);
    printf("bytes/sector %d, \n", hdr->BPB_BytsPerSec);
    printf("sectors %d, \n", hdr->BPB_TotSec32);
    printf("sectors/FAT %d, \n", hdr->BPB_FATSz32);
    printf("FATs %d, \n", hdr->BPB_NumFATs);
    printf("reserved sectors %d, \n", hdr->BPB_RsvdSecCnt);

    // 挂载成功
    mounted = 0;
    return 0;
}

// 打开文件
int fat_open(const char *path)
{
    if (mounted != 0)
        return -1; // 未挂载

    // 解析路径，找到文件对应的起始簇号和大小
    // TODO: 实现路径解析逻辑
    int cluster, size;
    if (!parse_path(path, &cluster, &size))
    {
        return -1; // 路径解析失败
    }
    for (int i = 0; i < MAX_OPEN_FILES; ++i)
    {
        if (open_files[i].cluster == 0)
        {                                    // 找到一个空闲的文件描述符
            open_files[i].cluster = cluster; // 假定 cluster 是路径解析后得到的簇号
            open_files[i].size = size;       // 假定 size 是路径解析后得到的文件大小
            return i;                        // 返回文件描述符
        }
    }
    return -1; // 打开文件数达到上限
}

// 关闭文件
int fat_close(int fd)
{
    if (fd < 0 || fd >= MAX_OPEN_FILES || open_files[fd].cluster == 0)
        return -1;

    open_files[fd].cluster = 0; // 清空文件描述符
    open_files[fd].size = 0;
    return 0;
}

// 读取普通文件内容
int fat_pread(int fd, void *buffer, int count, int offset)
{
    if (fd < 0 || fd >= MAX_OPEN_FILES || open_files[fd].cluster == 0)
        return -1;

    // 计算读取的起始簇和偏移量
    int cluster = open_files[fd].cluster;
    int cluster_size = hdr->BPB_BytsPerSec * hdr->BPB_SecPerClus;
    int cluster_offset = offset % cluster_size;
    int start_cluster = offset / cluster_size;

    // 跳转到正确的簇
    for (int i = 0; i < start_cluster; ++i)
    {
        cluster = next_cluster(cluster); // TODO: 实现获取下一个簇号的逻辑
    }

    // 读取数据
    int bytes_read = 0;
    char *buf = (char *)buffer;
    while (bytes_read < count)
    {
        // 计算当前簇可读字节数
        int readable = cluster_size - cluster_offset;
        if (readable > count - bytes_read)
            readable = count - bytes_read;

        // 读取数据到缓冲区
        // TODO: 实现从簇读取数据的逻辑

        bytes_read += readable;
        cluster_offset = 0; // 下一个簇从开始位置读

        // 检查是否需要读取下一个簇
        if (bytes_read < count)
        {
            cluster = next_cluster(cluster); // TODO: 实现获取下一个簇号的逻辑
            if (cluster == 0)
                break; // 文件结束
        }
    }
    return bytes_read;
}

// 读取目录文件内容 (目录项)
struct FilesInfo *fat_readdir(const char *path) {
    struct FilesInfo *files_info = malloc(sizeof(struct FilesInfo));
    if (!files_info) {
        return NULL;
    }

    struct DirEntry *dir_entries;
    int num_entries;
    if (read_directory(0, &dir_entries, &num_entries) != 0) {
        // 读取目录失败
        free(files_info);
        return NULL;
    }

    struct FileInfo *file_info = convert_dir_entry_to_file_info(dir_entries, num_entries);
    if (!file_info) {
        // 转换失败
        free(dir_entries);
        free(files_info);
        return NULL;
    }

    files_info->files = file_info;
    files_info->size = num_entries;
    free(dir_entries);
    return files_info;
}