#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>
#include <sys/mman.h> 
#include "fat32.h"

struct Fat32BPB *hdr; // ָ�� BPB ������
int mounted = -1; // �Ƿ��ѹ��سɹ�

static int open_file_num = 0;

typedef struct {
    uint32_t cluster;
    uint32_t size;
}open_file_table;
open_file_table OFT[128];

static uint32_t next_cluster(uint32_t cluster) //��FAT������һ����
{
    uint32_t *cluster_pointer = (uint32_t *)((char*)hdr+hdr->BPB_RsvdSecCnt*hdr->BPB_BytsPerSec); //�����ָ��ָ��FAT
    return *(cluster_pointer+cluster)&0x0FFFFFFF;
}

static void* loc_at_cluster(int cluster) //��λ�������غ�
{
    int sector = hdr->BPB_RsvdSecCnt+hdr->BPB_FATSz32*hdr->BPB_NumFATs+(cluster-2)*hdr->BPB_SecPerClus;
    return (void*)((char*)hdr + sector*hdr->BPB_BytsPerSec);
}

// ���ش��̾���
int fat_mount(const char *path) {
    // ֻ��ģʽ�򿪴��̾���
    int fd = open(path, O_RDWR);
    if (fd < 0){
        // ��ʧ��
        return -1;
    }
    // ��ȡ���̾���Ĵ�С
    off_t size = lseek(fd, 0, SEEK_END);
    if (size == -1){
        // ��ȡʧ��
        return -1;
    }
    // �����̾���ӳ�䵽�ڴ�
    hdr = (struct Fat32BPB *)mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0); //�ļ�ϵͳ�׵�ַ
    if (hdr == (void *)-1){
        // ӳ��ʧ��
        return -1;
    }
    // �ر��ļ�
    close(fd);

    assert(hdr->Signature_word == 0xaa55); // MBR ����������ֽ�Ӧ���� 0x55 �� 0xaa
    assert(hdr->BPB_TotSec32 * hdr->BPB_BytsPerSec == size); // ����ȱ����ļ��� FAT32 �ļ�ϵͳ�������ܲ���ȷ

    // ��ӡ BPB �Ĳ�����Ϣ
    printf("Some of the information about BPB is as follows\n");
    printf("OEM-ID \"%s\", \n", hdr->BS_oemName);
    printf("sectors/cluster %d, \n", hdr->BPB_SecPerClus);
    printf("bytes/sector %d, \n", hdr->BPB_BytsPerSec);
    printf("sectors %d, \n", hdr->BPB_TotSec32);
    printf("sectors/FAT %d, \n", hdr->BPB_FATSz32);
    printf("FATs %d, \n", hdr->BPB_NumFATs);
    printf("reserved sectors %d, \n", hdr->BPB_RsvdSecCnt);

    // ���سɹ�
    mounted = 0;
    for(int i=0;i<128;i++)
    {
        OFT[i].cluster = -1;
        OFT[i].size = 0;
    }
    return 0;
}

// ���ļ�
int fat_open(const char *path) {
    // TODO
    if(mounted == -1) return -1;
    if(open_file_num == 128) return -1;
    if(path[0] != '/') return -1;
    char* buf = (char*)malloc(strlen(path));
    strcpy(buf,path);
    char* paths_args[114];
    int count = 0;
    paths_args[0] = strtok(buf,"/");
    while(1)
    {
        if(paths_args[count]==NULL) break;
        paths_args[++count] = strtok(NULL,"/");
    }
    //���Ҹ�Ŀ¼��Ȼ������������Ŀ¼��ֱ���ҵ��ļ�
    int root_cluster = hdr->BPB_RootClus;
    struct DirEntry* Dir = (struct DirEntry*)loc_at_cluster(root_cluster);
    uint32_t file_cluster = -1;
    int max_DirEntry = hdr->BPB_BytsPerSec*hdr->BPB_SecPerClus/32;
    for(int i=0;i<count;i++)
    {
        char *road_args[2];
        road_args[0] = strtok(paths_args[i],".");
        road_args[1] = strtok(NULL,".");
        if(road_args[1]!=NULL)
        {
            if(strlen(road_args[1])>3) //��׺���Ϸ�
            {
                free(buf);
                return -1;                
            } 
        }
        char* compare_str = (char*)malloc(12); //���ڱȽ�
        compare_str[11] = '\0';
        for(int j=0;j<11;j++)
        {
            if(j<strlen(road_args[0]))
            {
                compare_str[j] = road_args[0][j];
            }
            else if(j<8)
            {
                compare_str[j] = ' ';
            }
            else if(road_args[1]!=NULL && j<8+strlen(road_args[1]))
            {
                compare_str[j] = road_args[1][j-8];
            }
            else
            {
                compare_str[j] = ' ';
            }
        }
        for(int j=0;j<11;j++)
        {
            compare_str[j] = toupper(compare_str[j]);
        }
        int count_dir = 0;
        while(count_dir<=max_DirEntry)
        {
            if(count_dir==max_DirEntry) //����һ����
            {
                root_cluster = next_cluster(root_cluster);
                if(root_cluster>=0x0FFFFFF8) {free(buf);
                return -1;}
                count_dir = 0;
                Dir = (struct DirEntry*)loc_at_cluster(root_cluster);
            }
            if(Dir->DIR_Name[0] == 0x00) { free(buf);free(compare_str);return -1;} //˵��������δ�õ�Ŀ¼��
            if(Dir->DIR_Name[0] == 0xE5) {Dir+=1;count_dir++;continue;} //˵��������ɾ����Ŀ¼��
            if(strncmp((char*)Dir->DIR_Name,compare_str,11)==0)
            {
                uint32_t high = ((uint32_t)Dir->DIR_FstClusHI)<<16;
                uint32_t low = (uint32_t)Dir->DIR_FstClusLO;
                root_cluster = high|low;
                if(i!=count-1) Dir = (struct DirEntry*)loc_at_cluster(high|low);
                else
                {    
                    if((Dir->DIR_Attr & DIRECTORY)){ free(compare_str); free(buf); return -1;} //�����ҵ�����Ŀ¼ 
                    file_cluster = high|low;
                }     
                break;
            }
            else {Dir+=1; count_dir++; }
        }
        free(compare_str);
    }
    int fd = -1;
    if(count == 0) {free(buf); return -1;}
    for(int i=0;i<128;i++)
    {
        if(OFT[i].cluster!=-1) continue;
        else
        {
            OFT[i].cluster = file_cluster;
            OFT[i].size = Dir->DIR_FileSize;
            fd = i;
            open_file_num++;
            break;
        }
    }
    free(buf);
    return fd;
}

// �ر��ļ�
int fat_close(int fd) {
    if(mounted == -1) return -1;
    if(fd<0 || fd>127) return -1;
    if(OFT[fd].cluster == -1) return -1;
    else
    {
        OFT[fd].cluster = -1;
        open_file_num--;
        return 0;
    }
}

// ��ȡ��ͨ�ļ�����
int fat_pread(int fd, void *buffer, int count, int offset) {
    if(mounted == -1) return -1;
    if(OFT[fd].cluster==-1 || fd<0 || fd>127 || count<0) return -1;
    if(count==0 || offset>=OFT[fd].size) return 0;
    if(count+offset>=OFT[fd].size) count = OFT[fd].size - offset; //ʵ���ܶ�ȡ���ֽ���
    
    uint32_t bytes_per_cluster = hdr->BPB_SecPerClus*hdr->BPB_BytsPerSec;//ÿ���ص�ʵ���ֽ���
    int index_of_start_cluster = offset/bytes_per_cluster;//������ļ��ĵڼ����ؿ�ʼ��
    uint32_t start_cluster = OFT[fd].cluster;
    for(int i=0;i<index_of_start_cluster;i++)
    {
        start_cluster = next_cluster(start_cluster);
    }
    uint32_t cluster_offset = offset % bytes_per_cluster;//��Ӧ��ƫ����
    
    int bytes_read = 0;
    while(bytes_read<count)
    {
        uint32_t remain = bytes_per_cluster - cluster_offset;//ʣ�¿ɶ��ֽ���
        if(remain>=count-bytes_read)//������������ж���
        {
            void* data = loc_at_cluster(start_cluster);//��ȡ��������λ�ò�ָ���������
            memcpy((char*)buffer+bytes_read,data+cluster_offset,count-bytes_read);
            bytes_read = count;
        }
        else//��ת�µĴ�
        {
            void* data = loc_at_cluster(start_cluster);
            memcpy((char*)buffer+bytes_read,data+cluster_offset,remain);
            cluster_offset = 0;
            start_cluster = next_cluster(start_cluster);
            bytes_read+=remain;
        }
    }
    return count;
}

// ��ȡĿ¼�ļ����� (Ŀ¼��)
struct FilesInfo* fat_readdir(const char *path) {
    if(mounted == -1) return NULL;
    if(path[0] != '/') return NULL;
    char* buf = (char*)malloc(strlen(path));
    strcpy(buf,path);
    char* paths_args[114];
    int count = 0;
    uint32_t root_cluster = hdr->BPB_RootClus;
    struct DirEntry* Dir = (struct DirEntry*)loc_at_cluster(root_cluster);
    int max_DirEntry = hdr->BPB_BytsPerSec*hdr->BPB_SecPerClus/32;
    uint32_t dir_size = 0;
    paths_args[0] = strtok(buf,"/");
    if(paths_args[0]==NULL)
    {
        int num_cl = 0;
        uint32_t n_cl = root_cluster;
        while(n_cl<0x0FFFFFF8)
        {
            num_cl++;
            n_cl = next_cluster(n_cl);
        }
        dir_size = hdr->BPB_BytsPerSec*hdr->BPB_SecPerClus*num_cl;
    }
    else
    {
    while(1)
    {
        if(paths_args[count]==NULL) break;
        paths_args[++count] = strtok(NULL,"/");
    }
    for(int i=0;i<count;i++)
    {
        char *road_args[2];
        road_args[0] = strtok(paths_args[i],".");
        road_args[1] = strtok(NULL,".");
        if(road_args[1]!=NULL)//�����ļ�
        {
            return NULL;
        }
        char* compare_str = (char*)malloc(12); //���ڱȽ�
        compare_str[11] = '\0';
        for(int j=0;j<11;j++)
        {
            if(j<strlen(road_args[0]))
            {
                compare_str[j] = road_args[0][j];
            }
            else if(j<8)
            {
                compare_str[j] = ' ';
            }
            else if(road_args[1]!=NULL && j<8+strlen(road_args[1]))
            {
                compare_str[j] = road_args[1][j-8];
            }
            else
            {
                compare_str[j] = ' ';
            }
        }
        for(int j=0;j<11;j++)
        {
            compare_str[j] = toupper(compare_str[j]);
        }
        int count_dir = 0;
        while(count_dir<=max_DirEntry)
        {
            if(count_dir==max_DirEntry) //����һ����
            {
                root_cluster = next_cluster(root_cluster);
                if(root_cluster>=0x0FFFFFF8) {free(buf);
                return NULL;}
                count_dir = 0;
                Dir = (struct DirEntry*)loc_at_cluster(root_cluster);
            }
            if(Dir->DIR_Name[0] == 0x00) { free(buf);return NULL;} //˵��������δ�õ�Ŀ¼��
            if(Dir->DIR_Name[0] == 0xE5) {Dir+=1;count_dir++;continue;} //˵��������ɾ����Ŀ¼��
            if(strncmp((char*)Dir->DIR_Name,compare_str,11)==0)
            {
                if((Dir->DIR_Attr & DIRECTORY)==0) return NULL; //��ʾ����Ŀ¼
                uint32_t high = ((uint32_t)Dir->DIR_FstClusHI)<<16;
                uint32_t low = (uint32_t)Dir->DIR_FstClusLO;
                if(i==count-1) dir_size = Dir->DIR_FileSize;
                Dir = (struct DirEntry*)loc_at_cluster(high|low);
                //if(i==count-1) root_cluster = high|low;
                root_cluster = high|low;
                break;
            }
            else {Dir+=1; count_dir++; }
        }
        free(compare_str);
    }
    }
    free(buf);
    //���������Ѱ��Ŀ¼��λ�ú͸����д��������洢����
    int count_dir = 0;
    int cnt = 0;//��ס�����
    struct DirEntry* DIR = Dir;
    uint32_t Root_cluster = root_cluster;
    while(count_dir<=max_DirEntry)
    {
        if(count_dir==max_DirEntry) //����һ����
        {
            Root_cluster = next_cluster(Root_cluster);
            if(Root_cluster>=0x0FFFFFF8) { 
            break;}
            count_dir = 0;
            DIR = (struct DirEntry*)loc_at_cluster(Root_cluster);
        }
        if(DIR->DIR_Name[0] == 0x00) //������
        {
            break;
        }
        if(DIR->DIR_Name[0] == 0xE5 || (DIR->DIR_Attr & LONG_NAME_MASK) == LONG_NAME) {DIR+=1;count_dir++;continue;}
        //strncpy(files_info->files[files_info->size].DIR_Name,(char*)DIR->DIR_Name,11);
        //files_info->files[files_info->size].DIR_FileSize = DIR->DIR_FileSize;
        count_dir++;
        cnt++;
        //files_info->size++;
        DIR++;
    }
    struct FilesInfo* files_info = (struct FilesInfo*)malloc(sizeof(struct FilesInfo));
    files_info->size = 0;
    files_info->files = (struct FileInfo *)malloc(sizeof(struct FileInfo)*cnt); //��̫���ˣ����޸�
    count_dir = 0;
    while(count_dir<=max_DirEntry)
    {
        if(count_dir==max_DirEntry) //����һ����
        {
            root_cluster = next_cluster(root_cluster);
            if(root_cluster>=0x0FFFFFF8) { 
            break;}
            count_dir = 0;
            Dir = (struct DirEntry*)loc_at_cluster(root_cluster);
        }
        if(Dir->DIR_Name[0] == 0x00) //������
        {
            break;
        }
        if(Dir->DIR_Name[0] == 0xE5 || (Dir->DIR_Attr & LONG_NAME_MASK) == LONG_NAME) {Dir+=1;count_dir++;continue;}
        strncpy(files_info->files[files_info->size].DIR_Name,(char*)Dir->DIR_Name,11);
        files_info->files[files_info->size].DIR_FileSize = Dir->DIR_FileSize;
        count_dir++;
        files_info->size++;
        Dir++;
    }
    return files_info;
}
