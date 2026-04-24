# STM32F103 SWD 烧录脚本

# 初始化
init

# 重置并停止
reset halt

# 擦除 Flash
flash erase_sector 0 0 127

# 烧录
program build/STM32.bin verify 0x08000000

# 复位运行
reset run

# 关闭
shutdown
