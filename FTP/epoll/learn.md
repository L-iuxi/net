# **关于epoll**

## **阻塞**

（Blocking） 是指在程序执行过程中，某些操作会导致当前执行线程（或进程）暂停，直到某个条件满足或事件发生之后，才会继续执行。

在计算机程序中，阻塞通常发生在需要等待某些资源或操作完成时。阻塞的本质是使程序处于一种等待状态，直到外部条件满足才继续执行。

#### 常见的阻塞场景：

1. **I/O 操作：**

   当进行输入/输出操作（如读写文件、网络请求、管道读写等）时，如果数据不可用，程序会进入阻塞状态，直到有数据可用或者操作完成。比如说：

      如果没有数据可读，`read()` 会阻塞，直到有数据可供读取。

      如果缓冲区已满，`write()` 会阻塞，直到有足够空间来写入数据。

2. **线程/进程等待：**

   当一个线程或进程等待另一个线程或进程的结果时，它会进入阻塞状态。例如，在多线程程序中，主线程可能会等待子线程完成某些任务，直到子线程执行完毕，主线程才能继续。

3. **同步机制：**

   使用锁（如互斥锁、信号量等）时，如果一个线程已经持有锁，而其他线程也试图获取这个锁，它们会被阻塞，直到锁被释放。

   
   
   阻塞可能导致资源利用不充分（等待某些操作完成时无法利用cpu资源），性能瓶颈等问题。
   
   

## 非阻塞：

**非阻塞**：在非阻塞模式下，操作不会导致程序暂停。如果操作无法立即完成，程序会返回一个错误或状态码，表示当前操作不可进行，允许程序继续执行其他任务。这种方式通常用来提高并发性，避免等待阻塞。

**I/O 多路复用（select和poll）、信号驱动 I/O** 以及 **epoll**

同时检查多个文件描述符，检查 I/O 系统调用是否可以非阻塞地执行

## 水平触发和边缘触发

**水平触发**通知：如果文件描述符上可以非阻塞地执行 I/O 系统调用，此时认为它已经就绪。

**边缘触发**通知：如果文件描述符自上次状态检查以来有了新的 I/O 活动（比如新的输入），此时需要触发通知。

在 **水平触发（Level-triggered）** 模式下，IO操作会不断检查文件描述符的状态，直到该状态变为可以进行操作为止。即使文件描述符已经处于可读或可写的状态，应用程序也可以在任何时刻去检查该状态。因此，它有更多的空间和时间来检查和响应IO操作。

而 **边缘触发（Edge-triggered）** 模式则不同，它仅在状态发生变化时才通知应用程序，即只有在状态从“不可操作”变为“可操作”的边缘时才会触发事件。这意味着如果应用程序没有及时处理IO事件，或者在状态变更之前未能读取完所有数据，那么这个事件就不会再被通知。因此，边缘触发的IO事件需要应用程序特别注意，不仅要及时处理，而且需要确保调用是非阻塞的，避免因为没有及时处理导致文件描述符处于阻塞状态。

下面要将的select和poll均为水平触发，而epoll默认水平触发但可以设置为边缘触发

## 解决阻塞

### 一.I/O 多路复用（select和poll）

##### select

```
#include <sys/select.h>
#include <unistd.h>

int select(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout);

```

nfds：监视的文件描述符的最大值加 1。
readfds：待检查是否可以读取的文件描述符集合。
writefds：待检查是否可以写入的文件描述符集合。
exceptfds：待检查是否发生异常的文件描述符集合。
timeout：指定 select() 阻塞的最长时间。如果为 NULL，则会一直阻塞直到有事件发生；如果为 {0, 0}，则立即返回检查结果。

1：

- **如果 `timeout` 为 `NULL`**：`select()` 会一直阻塞，直到某个文件描述符准备好（例如可读、可写或者发生异常）。这意味着如果没有文件描述符准备好，`select()` 会一直等待。

- **如果 `timeout` 为 `{0, 0}`**：`select()` 会立即返回，无论文件描述符是否准备好。它不会阻塞，快速检查所有文件描述符的状态，并返回。如果没有事件发生，返回时所有文件描述符的集合都将为空。

- **如果 `timeout` 设置为一个特定的时间**：`select()` 会阻塞最多 `timeout` 指定的时间，单位是秒和微秒。如果在这个时间内有文件描述符准备好，它会提前返回。如果在超时前没有事件发生，`select()` 会返回，表明超时了。

**fd_set** 通常是一个位集（bit set），它用来表示多个文件描述符的状态。每个文件描述符对应一个位，文件描述符在集合中的位置决定了它是否被监控

关于文件描述符集合的操作都是通过四个宏来完成的：FD_ZERO()，FD_SET()，FD_CLR()以及 FD_ISSET()。

```
// 清空文件描述符集合
#define FD_ZERO(fdset)    

// 将文件描述符添加到集合中
#define FD_SET(fd, fdset)  

// 从集合中移除文件描述符
#define FD_CLR(fd, fdset)  

// 检查文件描述符是否在集合中
#define FD_ISSET(fd, fdset) 

```

 FD_ZERO()将 fdset 所指向的集合初始化为空。
 FD_SET()将文件描述符 fd 添加到由 fdset 所指向的集合中。

FD_CLR()将文件描述符 fd 从 fdset 所指向的集合中移除。
如果文件描述符 fd 是 fdset 所指向的集合中的成员，FD_ISSET()返回 true

```
#include <sys/select.h>
#include <unistd.h>
#include <stdio.h>

int main() {
    fd_set readfds;
    
    // 清空文件描述符集合
    FD_ZERO(&readfds);

    // 将文件描述符添加到集合中
    FD_SET(0, &readfds);  // 假设 0 代表标准输入

    // 调用 select 检查标准输入是否可读
    int n = select(1, &readfds, NULL, NULL, NULL); 

    if (n == -1) {
        // 如果 select 返回 -1，表示发生了错误
        perror("select");
        return 1;
    } else {
        // 如果 n > 0，表示有文件描述符就绪
        if (FD_ISSET(0, &readfds)) {
            printf("标准输入有数据可以读取。\n");
        }
    }

    // 从集合中移除文件描述符
    FD_CLR(0, &readfds);

    return 0;
}

```

select返回值

返回**−1** 表示有错误发生。可能的错误码包括 EBADF 和 EINTR。EBADF 表示 readfds、writefds 或者 exceptfds 中有一个文件描述符是非法的（例如当前并没有打开）。EINTR表示该调用被信号处理例程中断了。

返回 **0** 表示在任何文件描述符成为就绪态之前 select()调用已经超时。在这种情况下，每个返回的文件描述符集合将被清空。

返回**一个正整数**表示有 1 个或多个文件描述符已达到就绪态。返回值表示处于就绪态的文件描述符个数。如果同一个文件描述符在readfds、writefds 和 exceptfds 中同时被指定，且它对于多个 I/O 事件都处于就绪态的话，那么就会被统计多次。

##### poll

```
#include <poll.h>

int poll(struct pollfd *fds, nfds_t nfds, int timeout);

```

- `fds`：指向 `pollfd` 结构体数组的指针。每个 `pollfd` 结构体代表一个文件描述符。
- `nfds`：`pollfd` 结构体数组的大小。数据类型 nfds_t 实际为无符号整形。
- `timeout`：等待事件的时间，单位为毫秒。如果为 `-1`，则无限等待；如果为 `0`，则立即返回；如果大于 `0`，则等待指定的时间。

##### poll fd结构体

```
struct pollfd {
    int fd;         // 文件描述符
    short events;   // 要监视的事件类型（例如 POLLIN、POLLOUT）
    short revents;  // 返回的事件类型（例如 POLLIN、POLLOUT）
};

```

**`poll()` 监视的事件类型：**

1. **`POLLIN`**：表示文件描述符可以读取。也就是数据可从该文件描述符读取。这个事件适用于标准输入、套接字、管道等。

2. **`POLLOUT`**：表示文件描述符可以写入。也就是可以将数据写入该文件描述符，适用于文件、套接字等。

3. **`POLLERR`**：表示文件描述符发生错误。

4. **`POLLHUP`**：表示文件描述符的挂起状态，通常表示连接关闭或被挂起。

5. **`POLLNVAL`**：表示无效的文件描述符，通常用于错误检查。

   ```
   #include <stdio.h>
   #include <stdlib.h>
   #include <unistd.h>
   #include <poll.h>
   
   int main() {
       struct pollfd fds[1];  
       int n;
   
       // 设置文件描述符：标准输入（0）
       fds[0].fd = 0;        
       fds[0].events = POLLIN; 
   
       // 调用 poll() 等待事件
       n = poll(fds, 1, -1);  // -1 表示无限等待
   
       if (n == -1) {
           perror("poll");
           exit(1);
       }
   
       if (fds[0].revents & POLLIN) {
           printf("标准输入有数据可以读取\n");
       }
   
       return 0;
   }
   
   ```



select中的文件描述符有数量限制，并且在每次调用时，都要重新设置文件描述符集合。

poll没有文件描述符数量限制，但仍然需要逐个扫描文件描述符。

### 二.epoll

#####  epoll_create

```
int epoll_create(int size);
```

创建一个epoll实例，size指需要检查的文件描述符的个数，该函数返回一个文件描述符

##### epoll_ctl

```
int epoll_ctl(int epfd, int op, int fd, struct epoll_event *event);

```

向 `epoll` 实例添加、删除或修改文件描述符的监听事件



epfd`：`epoll_create或epoll_create1返回的epoll` 文件描述符。

op：操作类型，常用的值有：
- `EPOLL_CTL_ADD`：添加文件描述符到 `epoll` 实例中。
- `EPOLL_CTL_MOD`：修改已加入的文件描述符的事件。
- `EPOLL_CTL_DEL`：从 `epoll` 实例中删除文件描述符。

fd`：目标文件描述符（通常是 socket 或文件）。

event：指定感兴趣的事件（通过 epoll_event` 结构体）。

###### **`epoll_event 结构体：`**

```
struct epoll_event {
    uint32_t events;      // 事件类型 (例如 EPOLLIN, EPOLLOUT 等)
    epoll_data_t data;    // 附加数据（通常是文件描述符或其他信息）
};
//EPOLLIN：文件描述符可以读取数据。
//EPOLLOUT：文件描述符可以写入数据。
//EPOLLERR：发生错误。
//EPOLLHUP：文件描述符发生挂断。
//EPOLLET：边缘触发模式。
//EPOLLONESHOT：只触发一次事件。
```

###### epoll_data_t

```
typedef union epoll_data {
    void *ptr;        // 指向任意类型的数据指针
    int fd;           // 文件描述符
    uint32_t u32;     // 32位无符号整数
    uint64_t u64;     // 64位无符号整数
} epoll_data_t;

```



- **返回值**：成功返回 0，失败返回 -1。

#####  epoll_wait

```
int epoll_wait(int epfd, struct epoll_event *events, int maxevents, int timeout);

```

等待文件描述符上发生事件，并返回就绪的文件描述符和相应事件



epfd`：`epoll_create返回的 epoll 文件描述符。

`events`：存储发生的事件的数组。

maxevents`：`events`数组的大小。

timeout：等待时间（单位：毫秒）。可以设为 -1表示无限等待，0 表示立即返回。

- **返回值**：返回发生的事件数量。如果出错，返回 -1。如果超时没有事件发生，返回 0。



# REACTOR

**Reactor** 是一种常见的设计模式，广泛应用于事件驱动的编程中，特别是在网络编程和高并发系统中。它的核心思想是通过 **事件循环** 来处理多个并发的 I/O 操作（如网络请求、文件读写等），而不需要为每个操作创建独立的线程。它的主要作用是高效地分发事件和管理事件的处理。

#### 工作流程：

1. **事件注册**：客户端或应用程序向 Reactor 注册感兴趣的事件（例如，某个 socket 是否可读、可写）。

2. **事件监听**：Reactor 不断地监听注册的事件源，当某个事件发生时，Reactor 会将该事件通知给注册的处理器。

3. **事件分发**：一旦事件发生，Reactor 将事件分发给相应的处理器来处理

   

Reactor 模式常常结合非阻塞 I/O 使用，当处理某个事件时，如果事件不能立即完成，就可以立即返回，去处理其他的事件，而不需要一直等待。这避免了线程被阻塞，造成性能浪费。



