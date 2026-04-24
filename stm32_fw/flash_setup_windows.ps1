# STM32F103CB J-Link USB 透传脚本 (Windows PowerShell)
# 以管理员身份运行此脚本

# 检查管理员权限
if (-NOT ([Security.Principal.WindowsPrincipal][Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)) {
    Write-Host "❌ 错误：此脚本需要管理员权限" -ForegroundColor Red
    Write-Host "   请以管理员身份运行 PowerShell，然后重新执行此脚本"
    exit 1
}

Write-Host "========== STM32F103CB J-Link USB 透传 ==========" -ForegroundColor Cyan
Write-Host ""

# 第一步：检查 usbipd 安装
Write-Host "[1/3] 检查 usbipd 安装..." -ForegroundColor Yellow

try {
    $usbipd = Get-Command usbipd -ErrorAction Stop
    Write-Host "✓ usbipd 已安装" -ForegroundColor Green
    Write-Host "   位置: $($usbipd.Source)" -ForegroundColor Gray
}
catch {
    Write-Host "❌ 错误：usbipd 未安装" -ForegroundColor Red
    Write-Host ""
    Write-Host "   安装方法（选择一个）：" -ForegroundColor Yellow
    Write-Host ""
    Write-Host "   方法 1：使用 winget（推荐）" -ForegroundColor Cyan
    Write-Host "   PS> winget install dorssel.usbipd-win"
    Write-Host ""
    Write-Host "   方法 2：使用 chocolatey" -ForegroundColor Cyan
    Write-Host "   PS> choco install usbipd"
    Write-Host ""
    Write-Host "   方法 3：从 GitHub 下载" -ForegroundColor Cyan
    Write-Host "   https://github.com/dorssel/usbipd-win/releases"
    Write-Host ""
    exit 1
}

# 第二步：列出 USB 设备并找 J-Link
Write-Host "[2/3] 扫描 USB 设备..." -ForegroundColor Yellow
Write-Host ""

$devices = usbipd list
Write-Host $devices -ForegroundColor Gray
Write-Host ""

# 查找 J-Link
$jlink = $devices | Select-String -Pattern "1366:1015|Segger|J-Link"

if ($jlink) {
    Write-Host "✓ 找到 J-Link 设备" -ForegroundColor Green

    # 提取 BUSID
    $busid = $jlink -split '\s+' | Select-Object -First 1

    Write-Host "   BUSID: $busid" -ForegroundColor Cyan
    Write-Host ""

    # 第三步：透传 USB 设备给 WSL2
    Write-Host "[3/3] 透传 J-Link 给 WSL2..." -ForegroundColor Yellow
    Write-Host "   命令: usbipd attach --wsl default --busid $busid" -ForegroundColor Gray
    Write-Host ""

    try {
        usbipd attach --wsl default --busid $busid
        Write-Host ""
        Write-Host "========== USB 透传完成 ==========" -ForegroundColor Cyan
        Write-Host ""
        Write-Host "✓ J-Link 已透传给 WSL2" -ForegroundColor Green
        Write-Host ""
        Write-Host "下一步：在 WSL2 中运行烧录脚本" -ForegroundColor Yellow
        Write-Host "  cd /home/peter/dog/dog_dev/stm32_fw" -ForegroundColor Gray
        Write-Host "  ./flash.sh" -ForegroundColor Gray
        Write-Host ""
    }
    catch {
        Write-Host ""
        Write-Host "❌ 透传失败" -ForegroundColor Red
        Write-Host "   错误: $_" -ForegroundColor Yellow
        Write-Host ""
        Write-Host "   可能原因：" -ForegroundColor Yellow
        Write-Host "   1. WSL2 未运行，请启动 WSL2"
        Write-Host "   2. BUSID 错误，请检查上面的设备列表"
        Write-Host "   3. J-Link 已被占用，请断开其他连接"
        Write-Host ""
        exit 1
    }
}
else {
    Write-Host "❌ 未找到 J-Link 设备" -ForegroundColor Red
    Write-Host ""
    Write-Host "   故障排查：" -ForegroundColor Yellow
    Write-Host "   1. 检查 J-Link 是否连接到 USB 口"
    Write-Host "   2. 尝试拔出后重新插入 J-Link"
    Write-Host "   3. 检查 USB 线是否损坏"
    Write-Host "   4. 在设备管理器中查看是否有未知设备"
    Write-Host ""
    Write-Host "   如果仍然找不到，请运行以下命令查看所有设备："
    Write-Host "   PS> usbipd list" -ForegroundColor Gray
    Write-Host ""
    exit 1
}
