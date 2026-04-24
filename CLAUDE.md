## Shell 命令约定

为避免触发权限确认弹窗,请遵循以下约定:

### 禁用的命令语法
以下语法会触发 Claude Code 的安全检测,**即使在 allow 列表中也会弹窗**,请主动避开:

- `find -exec`、`find -execdir`、`find -delete` → 改用 `find -printf`、纯 glob、或 `ls`
- `source`、`.`、`eval` → 把环境变量预置在 `.bashrc`,或封装进脚本文件
- `xargs` → 改用 `find -printf` 或循环
- `curl ... | bash`、`wget ... | sh` → 先下载到本地再执行

### 推荐替代
- 列文件名: `find <dir> -name "*.ext" -printf "%f\n"`,不要用 `-exec basename`
- 批量操作: 写进 `scripts/` 下的 .sh 脚本再执行,不要在命令行拼接复合命令
- ROS2 环境: 已在 .bashrc 中 source,无需再手动 source install/setup.bash

### 复合命令
避免在单行命令中使用 `&&`、`||`、`;` 连接多条操作。如需复合逻辑,封装为 `scripts/` 下的脚本文件。

## 常用命令示范
- 列 .msg 文件: `find ros2_ws/src -name "*.msg" -printf "%f\n"`
- 跑 ROS2 节点测试: `bash scripts/run_<name>_test.sh`
- 编译单个包: `colcon build --packages-select <pkg>`