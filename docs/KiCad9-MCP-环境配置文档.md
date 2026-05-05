# KiCad 9.x + MCP Server 环境配置文档

## 环境概览

- **系统**: Ubuntu 24.04 LTS (WSL2)
- **KiCad 版本**: 9.0.9 (AppImage)
- **MCP Server**: mixelpixx/KiCAD-MCP-Server v2.1.0
- **Python**: 3.11 (AppImage 内置)
- **Node.js**: v22.22.2

---

## 一、安装 KiCad 9.0.9

### 1.1 方案选择

| 方案 | 结果 |
|------|------|
| PPA (`ppa:kicad/kicad-9.0-releases`) | ❌ launchpad.net 不可达 |
| Snap (`sudo snap install kicad`) | ❌ snap store 超时 |
| Ubuntu 默认仓库 (`apt install kicad`) | ❌ 仅提供 7.0 版本 |
| **AppImage 下载** | ✅ 成功 |

### 1.2 下载与解压

```bash
# 下载
mkdir -p ~/apps
cd ~/apps
curl -L --connect-timeout 30 -o kicad-9.0.9-x86_64.AppImage.tar \
  "https://downloads.kicad.org/kicad/linux/explore/stable/download/kicad-9.0.9-x86_64.AppImage.tar"

# 解压 tar
tar xvf kicad-9.0.9-x86_64.AppImage.tar

# 提取 AppImage 内容
./kicad-9.0.9-x86_64.AppImage --appimage-extract
# 提取到 ./AppDir/ 目录
```

### 1.3 部署运行时

AppImage 使用 `sharun` 运行时管理库路径。提取后直接调用 Python 会因缺少 libwx 等库而失败。需要通过 `sharun` 启动：

```bash
# 将提取内容部署到 /opt/kicad9/
sudo mv ~/apps/AppDir /opt/kicad9
sudo chown -R root:root /opt/kicad9

# 创建 kicad-python 包装器
sudo tee /usr/local/bin/kicad-python << 'WRAPPER'
#!/bin/bash
export SHARUN_DIR="/opt/kicad9"
exec /opt/kicad9/sharun python3.11 "$@"
WRAPPER
sudo chmod +x /usr/local/bin/kicad-python
```

### 1.4 创建主程序入口

```bash
# AppImage 直接运行
sudo tee /usr/local/bin/kicad << 'EOF'
#!/bin/bash
exec /home/peter/apps/kicad-9.0.9-x86_64.AppImage "$@"
EOF
sudo chmod +x /usr/local/bin/kicad
```

### 1.5 验证

```bash
kicad-python -c "import pcbnew; print(pcbnew.Version())"
# 输出: 9.0.9
```

---

## 二、安装 MCP Server 依赖

### 2.1 克隆项目

```bash
cd ~
git clone https://github.com/mixelpixx/KiCAD-MCP-Server.git
cd KiCAD-MCP-Server
npm install && npm run build
```

### 2.2 安装 Python 依赖

AppImage 内置的 Python 不含 pip，需先安装：

```bash
# 下载 get-pip.py
curl -sS https://bootstrap.pypa.io/get-pip.py -o /tmp/get-pip.py

# 安装 pip（需跳过 externally-managed-environment 检查）
kicad-python /tmp/get-pip.py --no-setuptools --no-wheel --break-system-packages

# 安装依赖
PIP_BREAK_SYSTEM_PACKAGES=1 kicad-python -m pip install \
  -r ~/KiCAD-MCP-Server/requirements.txt
```

已安装的关键 Python 包：
- `kicad-skip` — 原理图操作
- `Pillow` — 图像处理
- `cairosvg` — SVG 渲染
- `pydantic` — 数据验证
- `requests` — HTTP 请求

---

## 三、配置 Claude Code MCP Server

### 3.1 注册 MCP Server

```bash
claude mcp add-json --scope user kicad '{
  "command": "node",
  "args": ["/home/peter/KiCAD-MCP-Server/dist/index.js"],
  "env": {
    "KICAD_PYTHON": "/usr/local/bin/kicad-python",
    "PYTHONPATH": "/opt/kicad9/shared/lib/python3.11/dist-packages",
    "LOG_LEVEL": "info"
  }
}'
```

### 3.2 验证

```bash
claude mcp list
claude mcp get kicad
```

期望输出: `Status: ✓ Connected`

---

## 四、关键路径速查

| 用途 | 路径 |
|------|------|
| KiCad 主程序 | `/usr/local/bin/kicad` |
| KiCad Python 入口 | `/usr/local/bin/kicad-python` |
| AppImage 文件 | `/home/peter/apps/kicad-9.0.9-x86_64.AppImage` |
| 提取的运行时 | `/opt/kicad9/` |
| Python 3.11 二进制 | `/opt/kicad9/shared/bin/python3.11` |
| sharun 运行时 | `/opt/kicad9/sharun` |
| pcbnew 模块 | `/opt/kicad9/shared/lib/python3.11/dist-packages/pcbnew.py` |
| MCP Server 代码 | `/home/peter/KiCAD-MCP-Server/` |
| MCP Server 入口 | `/home/peter/KiCAD-MCP-Server/dist/index.js` |

---

## 五、故障排查

### 5.1 `import pcbnew` 报 `libwx_gtk3u_gl-3.3.so.2` 缺失

原因：未通过 `sharun` 启动 Python，库路径未正确设置。

解决：必须使用 `kicad-python` 包装器，或手动设置 `SHARUN_DIR` 并通过 `sharun` 执行。

### 5.2 MCP Server 显示 `✗ Disconnected`

```bash
# 查看详细错误
claude mcp get kicad

# 检查 kicad-python 是否正常
kicad-python -c "import pcbnew; print('OK')"

# 检查 Node.js 入口是否存在
ls -la ~/KiCAD-MCP-Server/dist/index.js

# 重新注册
claude mcp remove kicad -s user
claude mcp add-json --scope user kicad '...'
```

### 5.3 pip 安装报 `externally-managed-environment`

需加 `PIP_BREAK_SYSTEM_PACKAGES=1` 环境变量。AppImage 内的 Python 是独立的，不会影响系统。

---

## 六、注意事项

1. **AppImage 更新**: 需重新下载新版 `.AppImage.tar`，解压后替换 `/opt/kicad9/` 内容
2. **Python 版本耦合**: AppImage 内 pcbnew 编译为 Python 3.11，无法使用系统的 Python 3.12
3. **重启生效**: 修改 MCP 配置后需重启 Claude Code 会话
4. **网络依赖**: `launchpad.net` 在此环境中不可达，未来如需 PPA 方式安装需解决网络问题
