#include <stdarg.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <pwd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdbool.h>
#include <signal.h>
#include <signal.h>

// 函数声明

void sh_exit();
void sh_cd(char *args[]);
void sh_prompt();
void sh_banner();
void sh_paths(char *args[]);
void sh_bg();
int sh_exe(const char *executable, char *args[]);
void sh_redirection(int p, int q, char *filepath);
void sh_pipe(int p, int q, int major);
void sh_run_in_bg(int p, int q);
void sh_make_token(char *input);
int sh_find_major(int p, int q);
bool sh_check_parentheses(int p, int q);
void sh_handle_cmd(int p, int q);
void sh_handle_child_signal(int signum);
void sh_register_signal_handler();

// 数据区
#define BUFFER_SIZE 1024
#define BUILTIN_FUNCTION_NUM 5

void (*functionPointers[BUILTIN_FUNCTION_NUM])() = {sh_exit, sh_cd, sh_paths, sh_bg};
char functionStrings[BUILTIN_FUNCTION_NUM][20] = {
    "exit",
    "cd",
    "paths",
    "bg",
};
char error_message[28] = "An error has occurred\n";
char PATH[1024][1024] = {
    "/bin",
};

char *tokens[64];
int nr_token = 0;

// 用链表来记录后台任务
struct BackgroundTask
{
  pid_t pid;
  char cmd[BUFFER_SIZE];
  struct BackgroundTask *next;
};
struct BackgroundTask *bgTaskList = NULL;
int numBgTasks = 0;

// 函数实现
void sh_banner()
{
  printf("Welcome to my shell!\n\n");
}

// 提示，显示当前工作路径
void sh_prompt()
{
  char buf[1024];
  if (getcwd(buf, sizeof(buf)) != NULL)
  {
    printf("%s> ", buf);
  }
  else
  {
    printf("Unknown CWD> ");
  }
}
// 退出
void sh_exit()
{
  exit(0);
}
// 改变当前工作目录，只接受1个参数
void sh_cd(char *args[])
{
  if (args[1] == NULL)
  {
    printf("Too few parameters!\n");
    return;
  }
  if (args[2] != NULL)
  {
    printf("Too many parameters!args[2]:%s\n", args[2]);
    return;
  }

  char *directory = args[1];
  directory[strcspn(directory, "\n\r")] = '\0';
  if (strcmp(directory, "~") == 0)
  {
    char *homeDir = getenv("HOME");
    strcpy(directory, homeDir);
  }
  if (chdir(directory) != 0)
  {
    perror("cd");
  }
}
// 环境变量PATH,ex: paths /usr/bin /bin
void sh_paths(char *args[])
{
  if (args[1] == NULL)
  {
    int index = 0;
    while (PATH[index][0] != '\0')
    {
      printf("%d\t%s\n", index + 1, PATH[index]);
      index++;
    }
  }
  else
  {
    int index = 0;
    while (args[index + 1] != NULL)
    {
      strncpy(PATH[index], args[index + 1], sizeof(PATH[index]) - 1);
      PATH[index][sizeof(PATH[index]) - 1] = '\0'; // 确保字符串以空字符结尾
      index++;
    }
    PATH[index][0] = '\0'; // 清空剩余的路径
  }
}
// 显示运行中的后台进程
void sh_bg()
{
  // printf("Index\tPID\tCommand\n");
  struct BackgroundTask *current = bgTaskList;
  int index = 1;
  while (current != NULL)
  {
    printf("%d\t%d\t%s\n", index, current->pid, current->cmd);
    current = current->next;
    index++;
  }
}
// 将某段指令的输出结果重定向到filepath
void sh_redirection(int p, int q, char *filepath)
{
  // 打开目标文件，获取文件描述符
  int fd = open(filepath, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
  if (fd == -1)
  {
    perror("open");
    exit(EXIT_FAILURE);
  }
  // 保存标准输出和标准错误的文件描述符
  int stdout_fd = dup(1);
  int stderr_fd = dup(2);
  if (stdout_fd == -1 || stderr_fd == -1)
  {
    perror("dup");
    exit(EXIT_FAILURE);
  }

  // 重定向标准输出和标准错误到文件
  if (dup2(fd, 1) == -1 || dup2(fd, 2) == -1)
  {
    perror("dup2");
    exit(EXIT_FAILURE);
  }

  sh_handle_cmd(p, q);
  // 恢复标准输出和标准错误
  if (dup2(stdout_fd, 1) == -1 || dup2(stderr_fd, 2) == -1)
  {
    perror("dup2");
    exit(EXIT_FAILURE);
  }

  close(fd); // 关闭文件描述符
}
// 管道，major是两段指令的分隔
void sh_pipe(int p, int q, int major)
{
  int pipefd[2];
  if (pipe(pipefd) == -1)
  {
    printf("Failed to create pipe\n");
    return;
  }
  pid_t pid1 = fork();
  if (pid1 == 0)
  { // 子进程1
    // printf("in pid1\n");
    close(pipefd[0]);
    // 将标准输出重定向到管道写入端
    if (dup2(pipefd[1], STDOUT_FILENO) == -1)
    {
      printf("Failed to redirect output to pipe\n");
      close(pipefd[1]);
      exit(EXIT_FAILURE);
    }

    sh_handle_cmd(p, major - 1);

    printf("Failed to execute command: %s\n", tokens[p]);
    close(pipefd[1]);
    exit(EXIT_FAILURE);
  }
  else if (pid1 > 1)
  {
    // 父进程
    //  创建子进程2，用于执行命令2
    pid_t pid2 = fork();
    if (pid2 == 0)
    {
      // 子进程2
      close(pipefd[1]);
      // 将标准输入重定向到管道读取端
      if (dup2(pipefd[0], STDIN_FILENO) == -1)
      {
        printf("Failed to redirect input from pipe\n");
        close(pipefd[0]);
        exit(EXIT_FAILURE);
      }

      // sh_exe(tokens[major + 1], tokens + major + 1);
      sh_handle_cmd(major + 1, q);
      exit(EXIT_SUCCESS);
    }
    else if (pid2 > 0)
    {
      // 父进程
      close(pipefd[0]); // 关闭管道两端
      close(pipefd[1]);

      // 等待子进程2执行完毕
      waitpid(pid2, NULL, 0);
    }
    else
    {
      printf("Failed to create child process\n");
      close(pipefd[0]);
      close(pipefd[1]);
      return;
    }

    // 等待子进程1执行完毕
    waitpid(pid1, NULL, 0);
  }
  else
  {
    printf("Failed to create child process\n");
    close(pipefd[0]);
    close(pipefd[1]);
    return;
  }
}

// 将指定段指令（可以是复杂的指令）以后台方式运行
void sh_run_in_bg(int p, int q)
{
  // printf("run in bg:%s  last args:%s\n",tokens[p],tokens[q]);
  pid_t pid = fork(); // 创建子进程

  if (pid < 0)
  {
    // 创建子进程失败
    perror("fork() failed");
    exit(EXIT_FAILURE);
  }
  else if (pid == 0)
  {
    // 子进程
    sh_handle_cmd(p, q);
    exit(EXIT_SUCCESS);
  }
  else
  {
    // 父进程

    struct BackgroundTask *new_task = malloc(sizeof(struct BackgroundTask));
    if (new_task == NULL)
    {
      perror("malloc() failed");
      exit(EXIT_FAILURE);
    }
    new_task->pid = pid;
    new_task->next = NULL;

    // 拼接命令字符串
    memset(new_task->cmd, 0, sizeof(new_task->cmd));
    int length = 0;
    for (int i = p; i <= q; i++)
    {
      int remaining = sizeof(new_task->cmd) - length;
      int written = snprintf(new_task->cmd + length, remaining, "%s ", tokens[i]);
      if (written >= remaining)
      {
        printf("Command too long, truncated.\n");
        break;
      }
      length += written;
    }
    int index = 1;
    // 将任务添加到链表
    if (bgTaskList == NULL)
    {
      bgTaskList = new_task;
    }
    else
    {
      struct BackgroundTask *current_task = bgTaskList;
      while (current_task->next != NULL)
      {
        current_task = current_task->next;
        index++;
      }
      current_task->next = new_task;
      index++;
    }

    printf("[%d] %d %s\n", index, pid, new_task->cmd);
  }
}

int sh_exe(const char *executable, char *args[])
{
  // printf("exe:%s args[1]:%s  args[2]:%s\n", executable, args[1], args[2]);
  for (int i = 0; i < BUILTIN_FUNCTION_NUM; i++)
  {
    if (strcmp(executable, functionStrings[i]) == 0)
    {
      functionPointers[i](args);
      return 0;
    }
  }
  char filepath[2048];
  int is_access = 0;
  if (access(executable, X_OK) == 0) // 本身就可以执行
  {
    strcpy(filepath, executable);
    is_access = 1;
  }
  else // 如果本身不能执行，去环境变量尝试执行
  {
    int i = 0;
    while (PATH[i][0] != '\0')
    {
      snprintf(filepath, sizeof(filepath), "%s/%s", PATH[i], executable);
      if (access(filepath, X_OK) == 0)
      {
        is_access = 1;
        break;
      }
      i++;
    }
  }

  if (is_access == 1)
  {
    // printf("can exe\n");
    pid_t pid = fork();
    if (pid == -1)
    {
      perror("fork");
      return EXIT_FAILURE;
    }
    else if (pid == 0)
    {
      // 子进程
      execv(filepath, args);
      exit(EXIT_SUCCESS);
    }
    else
    {
      // 父进程
      int status;
      waitpid(pid, &status, 0); // 等待子进程完成
      return 0;
    }
  }
  else
  {
    printf("Unknown shell command!\n");
    return 1;
  }
}
// 递归地处理指令
void sh_handle_cmd(int p, int q)
{
  if (p > q)
  {
    return;
  }
  else if (sh_check_parentheses(p, q) == true)
  {

    sh_handle_cmd(p + 1, q - 1);
  }
  else
  {
    int major = sh_find_major(p, q);
    if (major == -1) // 这说明这里直接就是一条指令了，可以直接执行
    {
      int num_elements = q - p + 1;
      char **new_tokens = malloc((num_elements + 1) * sizeof(char *));
      for (int i = 0; i < num_elements; i++)
      {
        new_tokens[i] = strdup(tokens[p + i]);
      }
      new_tokens[num_elements] = NULL;
      sh_exe(new_tokens[0], new_tokens);
      for (int i = 0; i < num_elements; i++) // 这里内存释放有待商榷
      {
        free(new_tokens[i]);
      }
      free(new_tokens);
      return;
    }

    if (strcmp(tokens[major], "|") == 0)
    {
      sh_pipe(p, q, major);
    }
    else if (strcmp(tokens[major], "&") == 0)
    {
      sh_run_in_bg(p, major - 1);
      if (tokens[major + 1] != NULL)
      {
        sh_handle_cmd(major + 1, q);
      }
    }
    else if (strcmp(tokens[major], ">") == 0)
    {
      sh_redirection(p, major - 1, tokens[major + 1]);
    }
    else if (strcmp(tokens[major], ";") == 0)
    {
      sh_handle_cmd(p, major - 1);
      sh_handle_cmd(major + 1, q);
    }
  }
}

// 一些工具
// 将用户input拆分成tokens
void sh_make_token(char *input)
{
  int max_tokens = strlen(input);
  int input_len = strlen(input);
  nr_token = 0;
  int in_quotes = 0; // 标记是否在引号内部
  int start = 0;     // 引号内部的起始位置

  for (int i = 0; i < input_len; i++)
  {
    if (input[i] == ' ' || input[i] == '\n')
    {
      if (!in_quotes)
      {
        continue; // 如果不在引号内部，跳过空格和换行符
      }
    }

    if (input[i] == '\"') // 如果遇到引号
    {
      if (!in_quotes)
      {
        in_quotes = 1; // 标记进入引号内部
        start = i + 1; // 记录引号内部的起始位置
      }
      else
      {
        in_quotes = 0; // 标记退出引号内部
        int end = i;   // 引号内部的结束位置

        if (end > start) // 如果引号内部有内容
        {
          tokens[nr_token] = malloc((end - start + 1) * sizeof(char));
          strncpy(tokens[nr_token], &input[start], end - start);
          tokens[nr_token][end - start] = '\0';
          nr_token++;
        }
      }
      continue;
    }

    if (!in_quotes && (input[i] == '(' || input[i] == ')'))
    {
      tokens[nr_token] = malloc(2 * sizeof(char));
      tokens[nr_token][0] = input[i];
      tokens[nr_token][1] = '\0';
      nr_token++;
      continue;
    }

    if (!in_quotes && (input[i] == '|' || input[i] == '&' || input[i] == '>' || input[i] == ';'))
    {
      tokens[nr_token] = malloc(2 * sizeof(char));
      tokens[nr_token][0] = input[i];
      tokens[nr_token][1] = '\0';
      nr_token++;
      continue;
    }

    if (!in_quotes)
    {
      start = i; // 记录非引号内部的起始位置
      while (i < input_len && input[i] != ' ' && input[i] != '(' && input[i] != ')' &&
             input[i] != '|' && input[i] != '&' && input[i] != '>' && input[i] != ';' &&
             input[i] != '\"') // 遇到引号时停止
      {
        i++;
      }
      int end = i;

      if (end > start)
      {
        tokens[nr_token] = malloc((end - start + 1) * sizeof(char));
        strncpy(tokens[nr_token], &input[start], end - start);
        tokens[nr_token][end - start] = '\0';
        nr_token++;
      }

      i--;
    }
  }
  // 将每个token可能存在的末尾换行符给换成\0，有待优化
  for (int j = 0; j < nr_token; j++)
  {
    tokens[j][strcspn(tokens[j], "\n\r")] = '\0';

    // printf("token[%d]: %s\n", j, tokens[j]);
  }
}
// 这个函数是用来检测表达式两侧的括号的，如果两侧括号属于同一对，可以去掉
bool sh_check_parentheses(int p, int q)
{
  if (strcmp(tokens[p], "(") == 0 && strcmp(tokens[q], ")") == 0)
  {

    int parentheseCount = 0;
    for (int i = p + 1; i < q; i++)
    {
      if (strcmp(tokens[i], "(") == 0)
      {
        parentheseCount++;
      }
      else if (strcmp(tokens[i], ")") == 0)
      {
        parentheseCount--;
        if (parentheseCount < 0)
        {

          return false;
        }
      }
    }

    return parentheseCount == 0;
  }
  return false;
}
// 查找指定段tokens的主操作符（最后一个执行的操作符）
int sh_find_major(int p, int q)
{
  // for (int j = 0; j < nr_token; j++) {
  //   printf("token[%d]: %s\n", j, tokens[j]);
  // }
  int mainOpIndex = -1;
  int highestPrecedence = 0;
  int currentPrecedence = 0;
  int parenthesesCount = 0;
  for (int i = p; i < q + 1; i++)
  {

    if (strcmp(tokens[i], "(") == 0)
    {
      parenthesesCount++;
    }
    else if (strcmp(tokens[i], ")") == 0)
    {
      parenthesesCount--;
      if (parenthesesCount < 0)
      {
        printf("Unclosed parentheses!\n");
        return -1;
      }
    }
    else if (parenthesesCount == 0)
    {

      if (strcmp(tokens[i], "|") == 0)
      {
        currentPrecedence = 1;
      }
      else if (strcmp(tokens[i], "&") == 0)
      {
        currentPrecedence = 2;
      }
      else if (strcmp(tokens[i], ">") == 0)
      {
        currentPrecedence = 3;
      }
      else if (strcmp(tokens[i], ";") == 0)
      {
        currentPrecedence = 4;
      }

      if (currentPrecedence > highestPrecedence)
      {
        highestPrecedence = currentPrecedence;
        mainOpIndex = i;
      }
    }
  }
  // printf("mainOP: %d\n", mainOpIndex);
  return mainOpIndex;
}

void sh_handle_child_signal(int signum)
{
  pid_t pid;
  int status;

  while ((pid = waitpid(-1, &status, WNOHANG)) > 0)
  {
    // 处理子进程结束
    if (WIFEXITED(status))
    {
      //printf("Child process %d exited with status %d\n", pid, WEXITSTATUS(status));
      // 更新任务状态、从链表中移除任务

      if (bgTaskList == NULL)
      {
        //printf("Error: Background task list is empty.\n");
        ;
      }
      else if (bgTaskList->pid == pid)
      {
        struct BackgroundTask *p = bgTaskList;
        bgTaskList = bgTaskList->next;
        free(p);
      }
      else
      {
        struct BackgroundTask *p, *prev = NULL;
        for (p = bgTaskList; p != NULL; prev = p, p = p->next)
        {
          if (p->pid == pid)
          {
            prev->next = p->next;
            free(p);
            break;
          }
        }

        if (p == NULL)
        {
          // 未找到匹配的节点
          printf("Error: Failed to find the matching background task.\n");
        }
      }
    }
    else if (WIFSIGNALED(status))
    {
      //printf("Child process %d terminated by signal %d\n", pid, WTERMSIG(status));

      // 更新任务状态、从链表中移除任务

      if (bgTaskList == NULL)
      {
        //printf("Error: Background task list is empty.\n");
        ;
      }
      else if (bgTaskList->pid == pid)
      {
        struct BackgroundTask *p = bgTaskList;
        bgTaskList = bgTaskList->next;
        free(p);
      }
      else
      {
        struct BackgroundTask *p, *prev = NULL;
        for (p = bgTaskList; p != NULL; prev = p, p = p->next)
        {
          if (p->pid == pid)
          {
            prev->next = p->next;
            free(p);
            break;
          }
        }

        if (p == NULL)
        {
          // 未找到匹配的节点
          printf("Error: Failed to find the matching background task.\n");
        }
      }
    }
  }
}
// 注册信号处理函数
void sh_register_signal_handler()
{
  signal(SIGCHLD, sh_handle_child_signal);
}

void main()
{
  sh_banner();
  sh_register_signal_handler();
  while (1)
  {
    sh_prompt();
    char input[BUFFER_SIZE];
    fgets(input, BUFFER_SIZE, stdin);
    sh_make_token(input);
    sh_handle_cmd(0, nr_token - 1);
  }

  return;
}
