2010.4.19
1. 时间戳存在周期性的反跳，导致程序认为此数据已经处理过。
2. 不包含frame处理时，间歇性的发生DMA失败，目前尚未从DMA寄存器中找到失败的原因。(可能是EMIF接口速度较慢，导致DMA队列溢出）
3. 包含frame处理时，等待frame处会等不到frame，但上一个阶段DMA发送已完成。

2010.4.20
调试建议：
1. 将QWMTHRA中每个队列的门限设置为最大值16
（0x01C0 0620 QWMTHRA Queue Watermark Threshold A Register for Q[2:0]）
2.从错误寄存器检查DMA过程的错误：
  CC:  
    CCERR（0x01C00318）， 
    EMCR（0x01C00308），
    EMRH（0x01C00304） 
    QSTAT0（0x01C00600）， 
    QSTAT1（0x01C00604）， 
    QSTAT2（0x01C00608）
    CCSTAT（0x01C00640）
    
  TC0:  
    TCSTAT （0x01C10100）
    ERRSTAT（0x01C10120）
    ERRDET（0x01C1012C）
    RDRATE（0x01C10140）
  
  TC1:
    TCSTAT （0x01C1 0500）
    ERRSTAT（0x01C1 0520）
    ERRDET（0x01C1 052C）
    RDRATE（0x01C1 0540）
    
  TC2:
    TCSTAT （0x01C1 0900）
    ERRSTAT（0x01C1 0920）
    ERRDET（0x01C1 092C）
    RDRATE（0x01C1 0940） 
    
3. cache：禁止gFrameOut对应的环形缓冲池的cache。
4. 解决edma中断每次需要清除的问题: 需要检查IER/IERH寄存器
5. ccsstudio看不到局部变量？  

2010.4.21
1. ccs3.3中的一个bug导致局部变量在特定条件下无法查看，这个条件是：存在不必要的{}界限 （去掉程序中不必要的{}）
2. 解决frame DMA发送后，等待接收部分无法接收的问题。(当前面的过程出现错误时，不应该移动接收指针）
3. 去掉二级缓冲中数据的128位对齐指示。

2010.5.11
  增加了中断支持，需要使用中断模式时，需要屏蔽adsb.h中的USE_POOL定义
  需要在编译选项中linker->code Entry Point设置为rst
  