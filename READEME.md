# epoll 的原理
pollable

首先，linux 的 file 有个 pollable 的概念，只有 pollable 的 file 才可以加入到 epoll 和 select 中。一个 file 是 pollable 的当且仅当其定义了 file->f_op->poll。file->f_op->poll 的形式如下

__poll_t poll(struct file *fp, poll_table *wait)
不同类型的 file 实现不同，但做的事情都差不多：

通过 fp 拿到其对应的 waitqueue
通过 wait 拿到外部设置的 callback[[1]]
执行 callback(fp, waitqueue, wait)，在 callback 中会将另外一个 callback2[[2]] 注册到 waitqueue[[3]]中，此后 fp 有触发事件就会调用 callback2
waitqueue 是事件驱动的，与驱动程序密切相关，简单来说 poll 函数在 file 的触发队列中注册了个 callback， 有事件发生时就调用callback。感兴趣可以根据文后 [[4]] 的提示看看 socket 的 poll 实现

了解了 pollable 我们看看 epoll 的三个系统调用 epoll_create, epoll_ctl, epoll_wait

epoll_create 只是在内核初始化一下数据结构然后返回个 fd

epoll_ctl 支持添加移除 fd，我们只看添加的情况。epoll_ctl 的主要操作在 ep_insert, 它做了以下事情：

初始化一个 epitem，里面包含 fd，监听的事件，就绪链表，关联的 epoll_fd 等信息
调用 ep_item_poll(epitem, ep_ptable_queue_proc[[1]])。ep_item_poll 会调用 vfs_poll， vfs_poll 会调用上面说的 file->f_op->poll 将 ep_poll_callback[[2]] 注册到 waitqueue
调用 ep_rbtree_insert(eventpoll, epitem) 将 epitem 插入 evenpoll 对象的红黑树，方便后续查找
ep_poll_callback

在了解 epoll_wait 之前我们还需要知道 ep_poll_callback 做了哪些操作

ep_poll_callback 被调用，说明 epoll 中某个 file 有了新事件
eventpoll 对象有一个 rdllist 字段，用链表存着当前就绪的所有 epitem
ep_poll_callback 被调用的时候将 file 对应的 epitem 加到 rdllist 里（不重复）
如果当前用户正在 epoll_wait 阻塞状态 ep_poll_callback 还会通过 wake_up_locked 将 epoll_wait 唤醒
epoll_wait 主要做了以下操作：

检查 rdllist，如果不为空则去到 7，如果为空则去到 2
设置 timeout
开始无限循环
设置线程状态为 TASK_INTERRUPTIBLE [参看 Sleeping in the Kernal](Kernel Korner - Sleeping in the Kernel)
检查 rdllist 如果不为空去到 7， 否则去到 6
调用 schedule_hrtimeout_range 睡到 timeout，中途有可能被 ep_poll_callback 唤醒回到 4，如果真的 timeout 则 break 去到 7
设置线程状态为 TASK_RUNNING，rdllist如果不为空时退出循环，否则继续循环
调用 ep_send_events 将 rdllist 返回给用户态

