# 我的智能家 - 项目需求文档

## 1. 项目概述

**项目名称：** 我的智能家 (My Smart Home)

**项目类型：** 前后端分离的物联网监测系统

**核心功能：** 实时监测家庭环境数据（温湿度、用电数据等），提供数据可视化展示和历史记录查询。

**目标用户：** 家庭用户，需要监测家庭环境参数的用户群体

## 2. 功能需求

### 2.1 数据监测功能

- **温湿度监测**
  - 实时温度显示
  - 实时湿度显示
  - 温湿度历史曲线图

- **用电数据监测**
  - 实时功率显示
  - 今日用电量统计
  - 用电历史数据查询

### 2.2 数据展示功能

- 仪表盘展示核心数据
- 实时数据刷新
- 历史数据图表展示

### 2.3 用户界面

- 响应式设计，适配PC和移动端
- 现代化的数据可视化界面
- 直观的图表展示

## 3. 技术架构

### 3.1 前端技术栈

- **框架：** Vue 3 + TypeScript
- **构建工具：** Vite
- **UI框架：** Tailwind CSS
- **图表库：** ECharts / Recharts
- **状态管理：** Pinia / Zustand
- **路由：** Vue Router

### 3.2 后端技术栈

- **语言：** Python 3.8+
- **框架：** FastAPI
- **数据库：** SQLite (开发) / PostgreSQL (生产)
- **ORM：** SQLAlchemy
- **数据验证：** Pydantic

### 3.3 项目结构

```
mySmartHome/
├── frontend/           # 前端项目
│   ├── src/
│   │   ├── components/  # 组件
│   │   ├── views/      # 页面视图
│   │   ├── stores/     # 状态管理
│   │   ├── api/        # API调用
│   │   └── utils/      # 工具函数
│   └── package.json
│
├── backend/            # 后端项目
│   ├── app/
│   │   ├── api/        # API路由
│   │   ├── models/     # 数据模型
│   │   ├── schemas/    # Pydantic模型
│   │   └── core/       # 核心配置
│   ├── requirements.txt
│   └── main.py
│
└── README.md
```

## 4. 数据库设计

### 4.1 温湿度数据表 (temperature_humidity)

- id: 主键
- temperature: 温度值 (摄氏度)
- humidity: 湿度值 (百分比)
- recorded_at: 记录时间
- device_id: 设备ID

### 4.2 用电数据表 (electricity_usage)

- id: 主键
- power: 实时功率 (瓦特)
- daily_usage: 今日用电量 (千瓦时)
- recorded_at: 记录时间
- device_id: 设备ID

## 5. API接口设计

### 5.1 温湿度API

- `GET /api/temperature-humidity/latest` - 获取最新温湿度数据
- `GET /api/temperature-humidity/history` - 获取温湿度历史数据

### 5.2 用电数据API

- `GET /api/electricity/latest` - 获取最新用电数据
- `GET /api/electricity/history` - 获取用电历史数据

### 5.3 仪表盘API

- `GET /api/dashboard/summary` - 获取仪表盘汇总数据

## 6. 开发计划

### 第一阶段：框架搭建

- [x] 项目结构设计
- [ ] 前端项目初始化
- [ ] 后端项目初始化
- [ ] 数据库模型创建

### 第二阶段：基础功能

- [ ] 后端API接口实现
- [ ] 前端页面框架搭建
- [ ] 数据展示组件开发

### 第三阶段：高级功能

- [ ] 数据可视化完善
- [ ] 历史数据查询
- [ ] 响应式UI优化
