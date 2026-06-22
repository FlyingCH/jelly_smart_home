#!/bin/bash

cd "$(dirname "$0")" || exit
PROJECT_ROOT=$(pwd)
BACKEND_DIR="$PROJECT_ROOT/backend"
FRONTEND_DIR="$PROJECT_ROOT/frontend"

echo ""
echo " ███╗   ██╗ ██████╗     ██████╗ ██╗   ██╗ ██████╗ "
echo " ████╗  ██║██╔═══██╗    ██╔══██╗██║   ██║██╔════╝ "
echo " ██╔██╗ ██║██║   ██║    ██████╔╝██║   ██║██║  ███╗"
echo " ██║╚██╗██║██║   ██║    ██╔══██╗██║   ██║██║   ██║"
echo " ██║ ╚████║╚██████╔╝    ██████╔╝╚██████╔╝╚██████╔╝"
echo " ╚═╝  ╚═══╝ ╚═════╝     ╚═════╝  ╚═════╝  ╚═════╝ "
echo ""
echo "==================================="
echo "   果冻智能家 - 启动脚本"
echo "==================================="
echo ""

echo "[1/3] 检查 Python 环境..."
if ! command -v python3 &> /dev/null; then
    echo "错误: 未找到 Python,请先安装 Python 3.8+"
    exit 1
fi

echo "[2/3] 检查 Node.js 环境..."
if ! command -v node &> /dev/null; then
    echo "错误: 未找到 Node.js,请先安装 Node.js"
    exit 1
fi

echo "[3/3] 安装后端依赖..."
cd "$BACKEND_DIR" || { echo "错误: 找不到 backend 目录: $BACKEND_DIR"; exit 1; }

PYTHON_VERSION=$(python3 -c "import sys; print(f'{sys.version_info.major}.{sys.version_info.minor}')")

if ! python3 -m venv --help &>/dev/null; then
    echo "未安装 python${PYTHON_VERSION}-venv，正在安装..."
    sudo apt install -y "python${PYTHON_VERSION}-venv"
    if [ $? -ne 0 ]; then
        echo "错误: 安装 python${PYTHON_VERSION}-venv 失败，请手动执行:"
        echo "  sudo apt install python${PYTHON_VERSION}-venv"
        exit 1
    fi
fi

if [ -d "venv" ] && [ ! -f "venv/bin/activate" ]; then
    echo "检测到损坏的虚拟环境，正在清理..."
    rm -rf venv
fi

if [ ! -d "venv" ]; then
    echo "创建 Python 虚拟环境 (python${PYTHON_VERSION})..."
    python3 -m venv venv
    if [ $? -ne 0 ]; then
        echo "错误: 虚拟环境创建失败"
        exit 1
    fi
fi

source venv/bin/activate

pip install --upgrade pip -q

HAS_RUST=false
if command -v rustc &>/dev/null && command -v cargo &>/dev/null; then
    HAS_RUST=true
fi

PIP_EXTRA_ARGS="--prefer-binary"

echo "正在安装依赖 (优先使用预编译包)..."

if ! pip install $PIP_EXTRA_ARGS -r requirements.txt 2>&1; then
    echo ""
    echo "[警告] 预编译包安装失败，尝试源码编译..."

    if [ "$HAS_RUST" = false ]; then
        echo "安装 Rust 工具链 (pydantic-core 源码编译需要)..."
        if command -v apt &>/dev/null; then
            sudo apt install -y build-essential curl
        fi
        if ! command -v rustc &>/dev/null; then
            curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh -s -- -y
            source "$HOME/.cargo/env"
        fi
    fi

    pip install -r requirements.txt || {
        echo "错误: 依赖安装失败，请手动执行:"
        echo "  cd backend && source venv/bin/activate && pip install -r requirements.txt"
        exit 1
    }
fi

echo ""
echo "==================================="
echo "   安装前端依赖..."
echo "==================================="
cd "$FRONTEND_DIR" || { echo "错误: 找不到 frontend 目录: $FRONTEND_DIR"; exit 1; }
npm install

chmod 777 "$FRONTEND_DIR/node_modules/.bin/vite" 2>/dev/null || true

echo ""
echo "==================================="
echo "   检查 MySQL 数据库连接..."
echo "==================================="

MYSQL_HOST="192.168.31.213"
MYSQL_PORT=3306
MYSQL_USER="root"
MYSQL_PASSWORD="85459939"
MYSQL_DATABASE="smart_home"

MYSQL_OK=false

echo "目标: ${MYSQL_USER}@${MYSQL_HOST}:${MYSQL_PORT}/${MYSQL_DATABASE}"

cd "$BACKEND_DIR" || exit
source venv/bin/activate

MYSQL_CHECK=$(
    python3 -c "
import pymysql, sys, time

MAX_RETRIES = 3         # 最大连接次数
CONNECT_TIMEOUT = 20    # 连接超时时间(秒)
last_error = ''

for attempt in range(1, MAX_RETRIES + 1):
    try:
        conn = pymysql.connect(
            host='${MYSQL_HOST}',
            port=${MYSQL_PORT},
            user='${MYSQL_USER}',
            password='${MYSQL_PASSWORD}',
            database='${MYSQL_DATABASE}',
            connect_timeout=CONNECT_TIMEOUT,
        )
        conn.close()
        print('OK')
        sys.exit(0)
    except Exception as e:
        last_error = str(e)
        if attempt < MAX_RETRIES:
            time.sleep(2)
            print(f'[尝试 {attempt}/{MAX_RETRIES}] 连接失败,2秒后重试...', file=sys.stderr)

print(f'FAIL: {last_error}', file=sys.stderr)
" 2>&1
)

if [ "$MYSQL_CHECK" = "OK" ]; then
    echo "MySQL 连接成功 ✓"
    MYSQL_OK=true
else
    echo "MySQL 连接失败 ✗"
    echo "  详情: $MYSQL_CHECK"
    echo ""
    echo "  常见原因与修复:"
    echo "    1. MySQL 服务未启动 → sudo systemctl start mysql"
    echo "    2. MySQL root 使用 auth_socket 插件 (Ubuntu/Debian 默认)"
    echo "       修复: sudo mysql -u root -e \"ALTER USER 'root'@'localhost' IDENTIFIED WITH mysql_native_password BY '${MYSQL_PASSWORD}'; FLUSH PRIVILEGES;\""
    echo "    3. 数据库未创建 → sudo mysql -u root -e 'CREATE DATABASE IF NOT EXISTS ${MYSQL_DATABASE};'"
    echo ""
fi

echo ""
echo "==================================="
echo "   果冻智能家 - 启动服务..."
echo "==================================="

echo ""
echo "正在启动果冻智能家后端服务 (http://localhost:8000)..."
cd "$BACKEND_DIR" || exit
source venv/bin/activate
python main.py &
BACKEND_PID=$!

sleep 4

if kill -0 $BACKEND_PID 2>/dev/null; then
    echo "果冻智能家后端服务启动成功 (PID: $BACKEND_PID)"
else
    echo "错误: 果冻智能家后端服务启动失败！请检查"
    echo "  - 果冻智能家依赖的 MySQL 是否在 ${MYSQL_HOST}:${MYSQL_PORT} 上运行"
    echo "  - 数据库 ${MYSQL_DATABASE} 是否已创建"
    echo "  - MySQL root 认证方式 (若报错 1698 需改为密码认证):"
    echo "    sudo mysql -u root -e \"ALTER USER 'root'@'localhost' IDENTIFIED WITH mysql_native_password BY '${MYSQL_PASSWORD}'; FLUSH PRIVILEGES;\""
    echo "  - 尝试手动运行: cd backend && source venv/bin/activate && python main.py"
    exit 1
fi

echo "正在启动果冻智能家前端服务 (http://localhost:8001)..."
cd "$FRONTEND_DIR" || exit
npm run dev &
FRONTEND_PID=$!

echo ""
echo "==================================="
echo "   果冻智能家 - 服务已启动！"
echo "==================================="
echo "果冻智能家前端: http://localhost:8001"
echo "果冻智能家后端: http://localhost:8000"
echo "果冻智能家 API 文档: http://localhost:8000/docs"
echo ""
echo "按 Ctrl+C 停止所有服务"
echo ""

trap "kill $BACKEND_PID $FRONTEND_PID 2>/dev/null; exit" INT TERM
wait
