# shell的实现

(1)运行一个程序

(2)建立一个程序

(3)等待exit()



# 系统调用

### fork()
允许一进程(父进程)创建一新进程(子进程)

具体做法是,新的子进程几近于对父进程的翻版:子进程获得父进程的栈、数据段、堆和执行文本段的拷贝

**函数介绍**

功能：

fork函数是从一个已经存在的进程中创建一个新的进程，新的进程称为子进程，原来的进程称为父进程。

参数：

无

返回值：

成功：子进程中返回 0，父进程中返回子进程 ID。pid_t，为无符号整型。

失败：返回 -1。

失败的两个主要原因是：

1）当前的进程数已经达到了系统规定的上限，这时 errno 的值被设置为 EAGAIN。

2）系统内存不足，这时 errno 的值被设置为 ENOMEM

### execvp() 

显示一个程序调用另一个程序，execvp将程序复制到内存然后运行它

```c
execvp(const char *file,const char *argv[])
```

成功没有返回值，失败则返回 -1

**file**：要运行的程序名

**aargv**：程序的命令行参数数组，其最后一个元素必须是null 

Unix运行程序的方法：

1.将制定程序复制到调用他的进程

2.将制定的字符串数组作为argv[]传给这个程序

3.运行程序



### waitpid()

用于等待指定的子进程终止或者获取子进程的状态信息
```c
pid_t waitpid(pid_t pid, int *status, int options);
```

**函数介绍**

*pid* 参数指定了要等待的子进程的进程ID。具体取值有：

如果 pid  > 0，则等待进程ID为 pid 的子进程

如果 pid  = -1，则等待任何一个子进程，等同于 wait 函数

如果 pid  = 0，则等待当前进程组中的任何一个子进程

如果 pid  < -1，则等待进程组ID等于 pid 绝对值的任何一个子进程

*status* 参数用于获取子进程的退出状态信息，它是一个指向整数的指针。如果不关心子进程的退出状态信息，可以传入NULL。

*options* 参数是一个位掩码，用于指定等待子进程的一些选项：

WNOHANG：非阻塞方式，即不挂起父进程，如果没有子进程退出，则立即返回

WUNTRACED：等待已经停止的子进程，但未报告其终止的子进程

WCONTINUED：等待已经停止的子进程，但之前已经被停止并且现在已经被继续的子进程

waitpid 函数会阻塞调用它的进程，直到满足指定的等待条件之一：子进程终止、被停止、被继续，或者出错（如果 pid 不合法）

### wait()

wait(&status)的目的有二:

其一,如果子进程尚未调用 exit()终止,那么 wait()会挂起父进程直至子进程终止

其二,子进程的终止状态通过 wait()的 status 参数返回

### exit()

库函数 exit(status)终止一进程,将进程占用的所有资源(内存、文件描述符等)归
还内核,交其进行再次分配。参数 status 为一整型变量,表示进程的退出状态。父进
程可使用系统调用 wait()来获取该状态。

>exit()位于系统调用_exit()之上，在调用 fork()之后
>
>父、子进程中一般只有一个会通过调用 exit()退出,而另一进程则应使用_exit()终止。




### 总结

系统调用 fork()通过复制一个与调用进程(父进程)几乎完全一致的拷贝来创建一个新进程
(子进程)
 。系统调用 vfork()是一种更为高效的 fork()版本,不过因为其语义独特—vfork()产生
的子进程将使用父进程内存,直至其调用 exec()或退出;于此同时,将会挂起(suspended)
父进程,所以应尽量避免使用。
调用 fork()之后,不应对父、子进程获得调度以使用 CPU 的先后顺序有所依赖。对执行
顺序做出假设的程序易于产生所谓“竞争条件”的错误。由于此类错误的产生依赖于诸如系
统负载之类的外部因素,故而其发现和调试将非常困难。

进程的终止分为正常和异常两种。异常终止可能是由于某些信号引起,其中的一些信号
还可能导致进程产生一个核心转储文件。
正常的终止可以通过调用_exit()完成,更多的情况下,则是使用_exit()的上层函数 exit()完
成。_exit()和 exit()都需要一个整型参数,其低 8 位定义了进程的终止状态。依照惯例,状态 0
用来表示进程成功完成,非 0 则表示异常退出。
不管进程正常终止与否,内核都会执行多个清理步骤。调用 exit()正常终止一个进程,将
会引发执行经由 atexit()和 on_exit()注册的退出处理程序(执行顺序与注册顺序相反),同时刷
新 stdio 缓冲区。

使用 wait()和 waitpid()(以及其他相关函数)
 ,父进程可以得到其终止或停止子进程的状
态。该状态表明子进程是正常终止(带有表示成功或失败的退出状态)
 ,还是异常中止,因收到
某个信号而停止,还是因收到 SIGCONT 信号而恢复执行。
如果子进程的父进程终止,那么子进程将变为孤儿进程,并为进程 ID 为 1 的 init 进程接管。
子进程终止后会变为僵尸进程,仅当其父进程调用 wait()(或类似函数)获取子进程退出
状态时,才能将其从系统中删除。在设计长时间运行的程序,诸如 shell 程序以及守护进程
(daemon)时,应总是捕获其所创建子进程的状态,因为系统无法杀死僵尸进程,而未处理的
僵尸进程最终将塞满内核进程表。
捕获终止子进程的一般方法是为信号 SIGCHLD 设置信号处理程序。当子进程终止时(也
可选择子进程因信号而停止时),其父进程会收到 SIGCHLD 信号。还有另一种移植性稍差的
处理方法,进程可选择将对 SIGCHLD 信号的处置置为忽略(SIG_IGN)
 ,这时将立即丢弃终
止子进程的状态(因此其父进程从此也无法获取到这些信息)
 ,子进程也不会成为僵尸进程。

 进程可使用 execve()以一新程序替换当前增长运行的程序。execve()的参数允许为新程序
指定参数列表(argv)和环境列表。构建于 execve()之上,存在多种命名相似的函数,功能相
同,但提供的接口不同。
所有的 exec()函数均可用于加载二进制的可执行文件或是执行解释器脚本。当进程执行脚
本时,脚本解释器程序将替换进程当前执行的程序。脚本的起始行(以#!开头)指定了解释器
的路径名,供识别解释器之用。如果没有这一起始行,那么只能通过 execlp()或 execvp()来执
行脚本,并默认把 shell 作为脚本解释器。
本章还展示了如何组合使用 fork()、exec()、exit()和 wait()来实现 system()函数,该函数可
用于执行任意 shell 命令

当打开进程记账功能时,内核会在系统中每一进程终止时将其账单记录写入一个文件。
该记录包含进程使用资源的统计数据。
如同函数 fork(),Linux 特有的 clone()系统调用也会创建一个新进程,但其对父子间的共
享属性有更为精确的控制。该系统调用主要用于线程库的实现。
本章对 fork()、vfork()和 clone()的进程创建速度做了比较。尽管 vfork()要快于 fork(),但
较之于子进程随后调用 exec()所耗费的时间,二者间的时间差异也就微不足道了。
fork()创建的子进程会从其父进程处继承(有时是共享)某些进程属性的副本,而对其他
进程属性则不做继承。例如,子进程继承了父进程文件描述符表和信号处置的副本,但并不
继承父进程的间隔定时器、记录锁或是挂起信号集合。相应地,进程执行 exec()时,某些进程
属性保持不变,而会将其他属性重置为缺省值。例如,进程 ID 保持不变,文件描述符保持打
开(除非设置了执行时关闭标志)
 ,间隔定时器得以保存,挂起信号依然挂起,不过会将对已
处理信号的处置重置为默认设置,同时与共享内存段“脱钩”。