# KiCad 自检报告

## 工程
- PCB: `/home/peter/dog/dog_dev/circuit/circuit.kicad_pcb`
- 板框: 85.0 mm x 56.0 mm
- 层数: 2 (F.Cu, B.Cu)
- Blue Pill 中心: 42.5 mm x 28.0 mm
- Blue Pill 排距: 17.78 mm
- Raspberry Pi 4B 安装孔: [{'ref': 'MH1', 'x': 3.5, 'y': 3.5, 'drill_mm': 3.2, 'pad_mm': 6.0}, {'ref': 'MH2', 'x': 61.5, 'y': 3.5, 'drill_mm': 3.2, 'pad_mm': 6.0}, {'ref': 'MH3', 'x': 3.5, 'y': 52.5, 'drill_mm': 3.2, 'pad_mm': 6.0}, {'ref': 'MH4', 'x': 61.5, 'y': 52.5, 'drill_mm': 3.2, 'pad_mm': 6.0}]
- 面包板引出孔: 80 个，孔径 1.0 mm，焊盘 1.8 mm
- GND: 全部共地
- MPU6050: 已删除；UART: GND / PA9_TX / PA10_RX
- 过孔数: 0
- 最长走线: PWM_FL_matched / 45.0 mm / 2 折角
- 底层 GND 覆铜面积占比: 81.14%

## ERC
- error/warning 总数: 0
- 无 ERC 违规。

## DRC
- violation 总数: 0
- unconnected items: 0
- 检查项: clearance=0, silk-to-pad/silk-over-copper=0, silk-to-silk=0, courtyard=0, hole-to-hole/hole-clearance=0, unconnected=0。
- 无 DRC 违规。

## 走线折角统计
- 0 折角: 82
- 1 折角: 2
- 2 折角: 9
- >2 折角: 0

## SG90 PWM 等长
- 目标长度: 45.0 mm
- PWM_FL_matched: 45.0 mm
- PWM_FR_matched: 45.0 mm
- PWM_RL_matched: 45.0 mm
- PWM_RR_matched: 45.0 mm
- 最大长度偏差: 0.0 mm

## Blue Pill 引出连通性抽查
- H1:1=VBAT -> B1A:1=VBAT(OK), B1B:1=VBAT(OK)
- H1:5=PA0_PWM_FL -> B1A:5=PA0_PWM_FL(OK), B1B:5=PA0_PWM_FL(OK)
- H1:8=PA3_PWM_RR -> B1A:8=PA3_PWM_RR(OK), B1B:8=PA3_PWM_RR(OK)
- H1:13=PB0 -> B1A:13=PB0(OK), B1B:13=PB0(OK)
- H1:20=GND -> B1A:20=GND(OK), B1B:20=GND(OK)
- H2:1=3V3 -> B2A:1=3V3(OK), B2B:1=3V3(OK)
- H2:2=GND -> B2A:2=GND(OK), B2B:2=GND(OK)
- H2:14=PA10_RX -> B2A:14=PA10_RX(OK), B2B:14=PA10_RX(OK)
- H2:15=PA9_TX -> B2A:15=PA9_TX(OK), B2B:15=PA9_TX(OK)
- H2:20=PB12 -> B2A:20=PB12(OK), B2B:20=PB12(OK)

## 生成文件
- 顶层: `circuit/exports/top.svg`
- 底层: `circuit/exports/bottom.svg`
- 丝印: `circuit/exports/silkscreen.svg`
- 3D: `circuit/exports/3d.png`
- BOM: `circuit/reports/bom.csv`
