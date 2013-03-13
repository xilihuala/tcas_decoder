工程说明：
1. workspace
  1）如果需要直接使用tcas.wks，需要将所有目录和文件拷贝到鷇:\tcas_decoder中。
  2）如果需要在其他目录放置文件，则需要按照下面步骤重建workspace：
     - 打开proj/test_edma_lld.pjt
     - 在gel分支中打开 gel\evmc6424.gel
     - 打开flashburn\C642x\CCS\NORWriter\NORWriter.pjt
     - 保存新的workspace

2. 烧写bootcode的方法是：（执行下列步骤前需确认proj/c642x.ini是否正确）
  - 编译test_edma_lld工程，生成用于烧写的ais文件proj/ais/tcas_decoder.ais
  - 编译NORWriter工程，生成写flash程序。
  - 下载并运行NORWriter 
  - 等待操作完成，重启设备即可。
  
NOTE: 上述步骤缺省烧录proj/ais/tcas_decoder.ais,如果需要烧录其他文件，可将编译选项中的-d"INPUT_FILE"去掉，并在程序运行时输入文件名（全路径）
  