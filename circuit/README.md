# Blue Pill SG90 Breadboard Carrier

本工程已从 `circuit/pcb.md` 重绘为 KiCad 9.0.9 可加载版本。

## 当前规则

- 板框：71.0 mm x 56.0 mm，来自 `circuit/pcb.md` 的 `Edge.Cuts`。
- 层数：2 层，F.Cu / B.Cu。
- Blue Pill：H1/H2 两条 1x20 排母，PCB 内显式分配网络。
- 面包板引出：B1A/B1B/B2A/B2B 四排 20 孔，共 80 个引出孔。
- SG90：J1/J2/J3/J4，物理顺序 G-V-S。
- UART：J7，丝印顺序 G PA10 PA9。
- 覆铜：B.Cu GND zone 已按当前板框重新生成，避免保留 KiCad 10 的不兼容填充块。

## 预览命令

```bash
SHARUN_DIR=/opt/kicad9 /opt/kicad9/sharun kicad /home/peter/dog/dog_dev/circuit/circuit.kicad_pro
SHARUN_DIR=/opt/kicad9 /opt/kicad9/sharun pcbnew /home/peter/dog/dog_dev/circuit/circuit.kicad_pcb
```

## 生成文件

- PCB：`circuit.kicad_pcb`
- 原理图说明：`circuit.kicad_sch`
- 校验报告：`reports/layout_validation.json`
- BOM：`reports/bom.csv`
