# 凌枢云台 LingShu Cloud Platform

基于 Qt 5.12 的会员一卡通管理系统，融合 **刷卡考勤**（串口读卡器）与 **人脸识别考勤**（SeetaFace + 摄像头），覆盖开户注册、一卡通充值/扣费/挂失/补办、考勤规则管理、公告发布、终端日结等完整业务闭环。

## 功能特性

### 管理员端
- **开户注册**：录入用户信息并开通一卡通，支持通过串口刷卡自动读取卡号
- **一卡通管理**：搜索、充值、扣费、挂失、补办、终端日结
- **考勤管理**：签到/签退时间规则（超时签到标记迟到、未到签退时间禁止签退），考勤记录查询 / 修改 / 删除
- **人脸识别考勤**：摄像头实时画面 + 人脸签到 / 签退，支持注册新用户人脸
- **串口调试**：串口号、波特率、数据位、停止位、校验位、流控配置，数据收发日志
- **公告管理**：发布 / 编辑 / 删除公告，支持附件
- **终端管理**：终端设备管理
- **用户管理**：按用户名 / 卡号搜索，修改 / 重置密码，删除用户

### 用户端
- **我的考勤**：串口刷卡考勤、摄像头刷脸考勤，今日考勤状态与历史记录查询
- **我的一卡通**：卡片信息与余额查看
- **公告查看**：浏览公告并下载附件

## 技术栈

| 类别 | 选型 |
| ---- | ---- |
| 语言 / 框架 | C++11 / Qt 5.12（Widgets） |
| Qt 模块 | SQL、SerialPort、Multimedia、MultimediaWidgets、Concurrent |
| 数据库 | MySQL（QSqlDatabase） |
| 人脸识别 | SeetaFace（检测 / 关键点 / 特征提取 + 特征比对） |
| 界面 | QSS 自定义深色渐变主题（微软雅黑 UI） |

## 环境依赖

- Qt 5.12（MinGW 64-bit）或更高版本
- MySQL（需预先建库建表，连接参数在 `mysql.cpp` 中配置）
- 摄像头（人脸识别考勤）
- 串口读卡器（刷卡考勤，可选）

## 构建运行

```bash
# 方式一：Qt 命令行（qmake）
qmake LingShu.pro
mingw32-make
./debug/LingShu.exe

# 方式二：Qt Creator
# 直接打开 LingShu.pro 构建运行
```

> 人脸识别依赖 SeetaFace 模型文件（检测 / 关键点 / 识别三份），程序会按「exe 旁 `models/` → 环境变量 `SEETA_ROOT` → 默认路径」的顺序自动查找。

## 目录结构

```
LingShu/
├── LingShu.pro            # qmake 工程文件
├── main.cpp               # 入口：字体、摄像头、全局 QSS 主题
├── widget.{h,cpp,ui}      # 主窗口（管理员 / 用户登录）
├── mysql.{h,cpp}          # MySQL 数据库封装
├── gmwidget.*             # 管理员控制台
├── userwidget.*           # 用户中心
├── attendancewidget.*     # 考勤管理（管理员）
├── userattendancewidget.* # 我的考勤（用户）
├── faceattendancedialog.* # 人脸识别考勤对话框
├── facerecognizer.*       # SeetaFace 识别封装（单例 + Pimpl）
├── mycamera.*             # 摄像头封装
├── gmcardwidget.*         # 一卡通管理
├── usercardwidget.*       # 我的一卡通
├── consumedialog.*        # 消费（充值 / 扣费）
├── dailysummarydialog.*   # 终端日结
├── serialwidget.*         # 串口调试
├── terminalmanagement.*   # 终端管理
├── usermanagement.*       # 用户管理
├── noticegm.*             # 公告管理（管理员）
├── noticeuser.*           # 公告查看（用户）
└── notice.ui.autosave     # （编辑器自动保存文件，未参与构建）
```
