# FreeRTOS系统管理中断优先级


参考链接[http://blog.sina.com.cn/s/blog_7fbb077f0102wrb1.html](http://blog.sina.com.cn/s/blog_7fbb077f0102wrb1.html)


- **Kernel级别**

- **Syscall级别**

- **系统可管理的中断优先级**



-------------------

### Kernel级别
FreeRTOSConfig.h宏定义configKERNEL_INTERRUPT_PRIORITY

### Syscall级别
FreeRTOSConfig.h宏定义configMAX_SYSCALL_INTERRUPT_PRIORITY

### 系统可管理的中断优先级
范围在Syscall~Kernel，能够调用_ISR后缀名的API函数，不会被内核延迟并且可嵌套。

### note
本demo所有中断配置，详见：`interrupt_config.h`。