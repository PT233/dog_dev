# PCB 审查报告

审查对象：`circuit/circuit.kicad_pcb`、`circuit/circuit.kicad_sch`  
审查方式：KiCad MCP + KiCad 9 CLI/ERC + 当前 PCB 文本几何解析。  
重要限制：当前 PCB 文件头为 `version 20260206`、`generator_version 10.0`，本机 KiCad 为 9.0.9；KiCad 9 CLI、`pcbnew.LoadBoard()` 与 PCB 侧 MCP 均无法加载该 PCB，因此板级 DRC 无法由 KiCad 9 复跑。已保留复跑失败结论，未修改任何 KiCad 工程文件。

## 1. 基本信息

- 板层数：2 层，`F.Cu` / `B.Cu`。
- 当前 `Edge.Cuts` 实测板框：71.0 mm x 56.0 mm。
- 注意：`README.md`、标题栏和现有导出 SVG 标题仍显示约 85 mm 宽；当前 PCB 文件实际板框为 71 mm 宽，存在文档/导出视图与 PCB 实体不一致。
- 元件总数：16 个，全部在顶层；顶层 16 个，底层 0 个。
- 过孔总数：0。
- 网络总数：39 个，来自 PCB 焊盘/走线/覆铜对象解析。
- 走线对象：130 条 `segment`，另有 11 条带 net 的铜层 `gr_line`。
- 审查输出图：`top.png`、`bottom.png`、`silk.png`、`3d.png` 已输出到 `circuit/review/`。因 KiCad 9 无法加载 KiCad 10 PCB，2D 图由当前时间戳匹配的既有 `exports/*.svg` 转换，3D 图由既有 `exports/3d.png` 复制。

## 2. ERC 结果

- MCP `run_erc`：0 violation，Errors 0 / Warnings 0 / Info 0。
- KiCad CLI ERC JSON 已输出到 `circuit/review/erc.json`：Errors 0 / Warnings 0。
- 原理图 MCP 网表：0 个元件、0 个网络；当前 `circuit.kicad_sch` 是说明型空图，因此无法用原理图网表对 PCB 进行实质连接比对。
- 重点项：
  - 未连接引脚：0（原理图层面未发现）。
  - 电源冲突：0（原理图层面未发现）。
  - 未驱动网络：0（原理图层面未发现）。

## 3. DRC 结果

官方 KiCad 9 DRC 复跑失败：`Failed to load board`。原因是当前 PCB 为 KiCad 10 文件格式，本机 KiCad/MCP 后端为 9.0.9。现有 `circuit/reports/drc.json` 显示生成时 DRC 为 0 violations，但该报告不是本次复跑产生。

文本几何补充检查结果：

- clearance：未发现焊盘/元件最小间距 < 1.5 mm 的对象；元件外接框最小间距约 2.70 mm（B2A-H2）。
- silk-to-pad：0 处 < 0.3 mm。
- silk-to-silk：未发现实质重叠；解析中仅有丝印矩形框端点相接。
- courtyard：未发现外接框间距 < 1.5 mm 的冲突；因 KiCad 9 无法加载，不能替代官方 courtyard DRC。
- hole-to-hole：最小孔边距约 1.54 mm（B1B:12 - B1B:13），未发现 < 0.5 mm。
- track width：发现 11 条电源类走线不符合本审查清单的 0.5 mm 期望，详见第 6 节。
- annular ring：最小环宽约 0.35 mm（H1:1，1.7 mm pad / 1.0 mm drill），未发现 < 0.15 mm。

## 4. 布局问题（含严重等级）

- 🔴严重：PCB 文件实际板框为 71.0 x 56.0 mm，但覆铜 polygon 和既有导出 SVG 仍按约 85 mm 宽范围表达；B.Cu GND 覆铜 polygon 点包含 `(84.7, 0.3)`、`(84.7, 55.7)`，超出当前 `Edge.Cuts` 的 x=71.0。
- 🟡中等：Blue Pill 中心为 `(42.5, 28.0)`，当前板几何中心为 `(35.5, 28.0)`，X 方向偏右 7.0 mm，不在实际板框几何中心。
- 🟢轻微：4 路 SG90 插座基本沿左侧纵向等距排列，J1 `(2.42,12.0)`、J2 `(2.46,20.0)`、J3 `(2.42,28.0)`、J4 `(2.42,36.0)`；J2 X 坐标比其他三路偏 0.04 mm。
- 🔴严重：MPU6050 预留位不存在；README 说明当前版本已用 J7 UART 取代 MPU6050。
- 🟢通过：4 个 M3 安装孔齐全，MH1 `(3.5,3.5)`、MH2 `(61.5,3.5)`、MH3 `(3.5,52.5)`、MH4 `(61.5,52.5)`；钻孔均为 3.2 mm，焊盘 6.0 mm。
- 🟢通过：未发现元件外接框间距 < 1.5 mm 的违规对。

## 5. 面包板引出孔问题

- Blue Pill 40 个引脚均有对应引出孔；未引出引脚：无。
- 引出孔数量：B1A/B1B/B2A/B2B 共 80 孔，每个 Blue Pill 引脚对应 2 个外部孔。
- 引出孔规格一致：钻孔 1.0 mm，焊盘 1.8 mm，间距 2.54 mm。
- 引出孔与对应排母引脚同行排列：未发现行偏移违规。

## 6. 走线问题

- 连续同网同层路径折角统计：0 折角 5 条 / 1 折角 19 条 / 2 折角 14 条 / >2 折角 7 条。注：该统计按同网同层连通路径合并，分支网络会按路径合并口径计入。
- >2 折角路径：
  - `5V_SERVO`：`(5.0,20.0)` -> `(5.0,44.0)`，3 折角，约 40.03 mm。
  - `PA0_PWM_FL`：`(28.56,11.0)` -> `(7.5,12.0)`，8 折角，约 51.29 mm（含引出支线）。
  - `PA1_PWM_FR`：`(31.10,11.0)` -> `(7.54,20.0)`，8 折角，约 49.73 mm（含引出支线）。
  - `PA2_PWM_RL`：`(33.64,11.0)` -> `(7.5,28.0)`，6 折角，约 47.63 mm（含引出支线）。
  - `PA3_PWM_RR`：`(36.18,11.0)` -> `(7.5,36.0)`，7 折角，约 51.23 mm（含引出支线）。
  - `PA10_RX`、`PA9_TX` 各 3 折角。
- 非 0/45/90/135 度走线：0 条。
- SG90 PWM 主路径长度（H1 到对应 SG90 SIG）：
  - FL `PA0_PWM_FL` H1:5 -> J1:3：43.16 mm。
  - FR `PA1_PWM_FR` H1:6 -> J2:3：41.61 mm。
  - RL `PA2_PWM_RL` H1:7 -> J3:3：39.51 mm。
  - RR `PA3_PWM_RR` H1:8 -> J4:3：43.11 mm。
  - 最大差值约 3.66 mm，不算严格等长。
- 走线宽度：
  - 信号线：0.25 mm，符合清单。
  - 一般电源期望 0.5 mm；`3V3` 2 条、`3V3_H1` 3 条、`GND` 5 条为 0.25 mm，偏窄。
  - `5V_SERVO` 有 1 条 0.8 mm 短段，若按“舵机主干 0.8 mm”可接受；其余 `5V_SERVO` 主干为 0.5 mm。
- 走线与丝印净距 < 0.3 mm：解析到 61 个同面几何接触/贴近项，主要来自连接器封装丝印框与同面走线交叉或重合。代表位置：`5V_SERVO` `(4.96,12.0)` -> `(4.96,43.96)` 与 J1/J2/J3/J4 F.SilkS；`PA0_PWM_FL` `(28.53,19.11)` -> `(28.53,28.47)` 与 H1 F.SilkS。完整列表见 `circuit/review/analysis.json`。

## 7. 丝印问题

- 冗余 GPIO 标注：`PA0 PA1 PA2 PA3` at `(27.53,8.93)`，`G PA10 PA9` at `(51.96,55.0)`。
- 必要丝印：
  - USB 方向箭头：存在。
  - Pin1 三角：存在。
  - SG90 标签 FL/FR/RL/RR：存在。
  - G-V-S 顺序：存在 4 处。
  - `5V_SERVO`：缺失。
  - `STAR GND`：缺失。
  - `MPU6050` 标签：缺失。
  - 标题栏：存在。
- 字高/线宽：线宽均为 0.15 mm；但 10 处文字字高不是 1.0 mm，包括 `PA0 PA1 PA2 PA3` 0.7 mm、4 处 `G-V-S` 0.8 mm、FL/FR/RL/RR 0.85 mm、`G PA10 PA9` 0.85 mm。
- 丝印压焊盘/压过孔：未发现 silk-to-pad < 0.3 mm；无过孔。

## 8. 电源与覆铜问题

- B.Cu GND 覆铜 polygon 面积按自身 polygon 约 4675.76 mm²，而实际板框面积为 3976.0 mm²，折算为 117.6%；这不是有效的“>70% 通过”，而是覆铜边界超出当前板框导致的异常。
- 孤岛铜区：无法由 KiCad 9 官方 DRC/zone fill 复核；文本解析未发现多个独立 zone，仅有 1 个 B.Cu GND zone。
- 大块未覆铜区域：无法由 KiCad 9 对当前 KiCad 10 PCB 进行可靠渲染/填充复核；从 polygon 定义看覆铜目标覆盖整板，但边界不匹配实际板框，需优先处理。

## 9. 电气连接核对

- 原理图为空图，MCP 生成网表为 0 组件 / 0 网络，无法做 schematic-to-PCB parity 的实质核对。
- PCB 内部网络核对：
  - 4 路 SG90 信号分别为 `PA0_PWM_FL`、`PA1_PWM_FR`、`PA2_PWM_RL`、`PA3_PWM_RR`，均连接到对应 J1/J2/J3/J4 的 SIG pad 3。
  - SG90 物理顺序为 pad1 `GND`、pad2 `5V_SERVO`、pad3 `SIG`，符合 G-V-S。
  - J7 UART：pad1 `GND`、pad2 `PA10_RX`、pad3 `PA9_TX`；丝印为 `G PA10 PA9`，与 PCB pad net 一致。
  - `5V_SERVO` 与 `5V_MCU` 是独立网络；`GND` 共地。
  - `3V3` 与 `3V3_H1` 同时存在，需确认这是否为有意分域；当前原理图无法提供依据。

## 10. 总评分（A/B/C/D/F）+ 关键修改建议清单

总评分：C。

关键原因：当前 PCB 文件格式超出本机 KiCad 版本导致官方 DRC 无法复跑；实际板框与覆铜/导出视图/文档存在明显不一致；MPU6050 与 STAR GND 等需求项缺失；Blue Pill 不在实际板框几何中心。

建议优先级：

1. 先统一 KiCad 版本或导出 KiCad 9 可加载文件，再重新跑官方 DRC，尤其是 clearance、courtyard、zone、silk 规则。
2. 修正实际板框、覆铜 polygon、导出图和 README/标题栏之间的 71 mm vs 85 mm 不一致。
3. 按需求决定是否恢复 MPU6050 预留位、`STAR GND` 和 `5V_SERVO` 丝印；如果当前 UART 替代 MPU6050 是设计决策，应更新审查清单或需求文档。
4. 评估 Blue Pill 是否应按当前 71 x 56 mm 板框重新居中。
5. 调整电源线宽：`3V3`、`3V3_H1`、部分 `GND` 从 0.25 mm 提升到目标 0.5 mm；确认 `5V_SERVO` 主干是否统一按 0.8 mm。
6. 清理连接器丝印框与同面走线的重合/贴近项，或确认制造规则允许该封装丝印布局。
