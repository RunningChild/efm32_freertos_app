#### How to use

在src\，执行“git clone git@github.com:RunningChild/embeded_c_components.git”下载公共库，并重新命名为public_libs。

#### 目录结构介绍

	
	arm：             工程文件。
	bootloader：      用于引导app。
	common：
	    ——CMSIS：     cortex-m3的头文件。
	    ——DEVICE：    汇编.s文件。跟编译工具链有关。    
	    ——FreeRTOS：  操作系统相关。 
	script：          实现烧录相关脚本。
	src： 
	    ——app： 	  
		    	    ——app_manager：    每个事件任务具体的逻辑实现。
		    		——hal_manager：    管理bsp层所有的功能模块。
		    		——thread_manager： 线程监控，功耗管理。
		    		——module_config.h：app_manager和hal_manager的配置文件。 			
	
	    ——framework:
	    	        ——components：
	    			——app_schedule：    事件任务调度框架。
	   			    ——app_timer:        一个硬件定时器实现可配置的多路软件定时器。
	    			——delay:            自旋等待，不可睡眠。 
	    			——platform:         重写跟平台相关的操作系统相关接口，如内存管理，睡眠管理。  
	
	    ——bsp：
		    		——common：
		    			——emdrv:     EFM32GG官方外设驱动抽象层。
		    			——emlib:     EFM32GG官方所有外设驱动。 
		    		——fs：               片上文件系统，外部flash文件系统。     
		    		——system：
		    			——input:      输入设备，如按键。
		    			——misc:       其他杂设备，如看门狗，万年历。
		    			——output:     输出设备，如蜂鸣器。
		    		——uart_device：       ble，指纹，日志，zigbee通信控制。
		    		——bsp_config.h：      板级所有IO引脚配置。
		    		——interrupt_config.h：板级中断优先级配置。
	
	    ——public_libs:
	    		    ——common_libs：          与平台无关的小工具，包括万年历、系统错误码检查等。
