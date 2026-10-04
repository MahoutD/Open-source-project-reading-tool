# CodeInsight Pro - 开源代码架构与依赖全景分析工具

基于 **C++ 17 + Qt 6.8 (QML) + MSVC 2022** 构建的专业级开源代码阅读、拓扑依赖图谱分析与工程诊断工具。

---

## 🎨 界面与交互重构亮点

1. **工业级开发工具界面风格 (VS Code / JetBrains Dark 现代深色体系)**
   - 采用现代化扁平卡片化与 Slate-900 系列深灰专业配色。
   - 三段式布局结构：
     - **顶部功能控制区**：品牌标志、仓库链接克隆区、本地工程入口、网络诊断入口、工程重析与使用帮助。
     - **左侧项目管理区**：支持根据实际窗口尺寸自适应拉伸或折叠，提供「源码文件（带快速关键词搜索检索过滤）」与「全景总结报告（可一键导出 Markdown 报告）」双面板。
     - **中央工作区**：上方为 Scintilla 风格带独立行号槽与精准语法着色的代码编辑器；下方为交互式依赖拓扑图；底部为实时操作诊断日志面板。
     - **底部状态栏**：全局运行状态指示与技术栈标记。

2. **Graphviz 规范与 Qt 交互式图元引擎 ([`InteractiveGraphView`](file:///d:/WorkSpace/AI_Work/Open-source%20project%20reading%20tool/src/InteractiveGraphView.h))**
   - **深层拓扑布局与贝塞尔曲线连接**：采用层次分层算法（Sugiyama 模型），根据入度与出度自动分配层级坐标。
   - **全量图元事件交互**：
     - **鼠标悬停高亮**：悬浮在任一文件节点时即时显示高亮光晕并同步显示元信息。
     - **图元单选与关联边缘发光**：单击选中图元卡片，与其相关的依赖连线会切换为青色高亮发光显示，并计算其入度与出度。
     - **双击联动代码查看**：双击拓扑图中的任意文件图元，上方代码编辑器将立即自动定位并加载对应源代码。
     - **平移、缩放与视角自适应**：支持按住拖拽、滚轮平滑缩放、一键复位与「适应窗口（Fit to View）」。
     - **水平/垂直布局切换**：支持 LR（水平从左到右）与 TB（垂直从上到下）一键切换。

3. **操作日志与诊断终端 ([`Logger`](file:///d:/WorkSpace/AI_Work/Open-source%20project%20reading%20tool/src/Logger.h))**
   - 界面底部内置实时滚动更新的操作日志终端。
   - 记录项目加载、Git 克隆命令执行、网络连通性探测结果、拓扑图元交互、代码读取警告等全部日志，支持按日志级别（INFO、SUCCESS、WARN、ERROR）着色区分，并提供「清空日志」按钮。

4. **代码托管平台网络诊断中心 ([`NetworkTester`](file:///d:/WorkSpace/AI_Work/Open-source%20project%20reading%20tool/src/NetworkTester.h))**
   - 内置针对 **GitHub、Gitee (码云)、GitLab、GitCode** 的并发非阻塞连通性诊断对话框。
   - 实时反馈往返延迟、HTTP 响应码及状态指示灯，并提供一键重新探测。

5. **全景分析与报告导出 ([`ProjectController`](file:///d:/WorkSpace/AI_Work/Open-source%20project%20reading%20tool/src/ProjectController.h))**
   - 多维度统计项目总代码行数、文件数、体积及各语言后缀所占百分比。
   - 自动检测并预警高耦合依赖模块，支持直接点击「导出报告」保存为本地 `PROJECT_ANALYSIS_REPORT.md`。

---

## 🛠️ 环境要求与编译指南

### 1. 软件环境
- **操作系统**: Windows 10 / 11 (x64)
- **编译器**: Visual Studio 2022 (MSVC 19.44+, C++17)
- **Qt SDK**: Qt 6.8.3 (`D:/Qt/6.8.3/msvc2022_64`)
- **构建工具**: CMake (>= 3.20) + Ninja
- **Git**: 系统 PATH 中可用

### 2. 编译与打包步骤

在 Visual Studio 2022 开发者命令提示符（Developer Command Prompt）中执行：

```powershell
# 1. 激活 MSVC x64 环境
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"

# 2. CMake 配置工程
cmake -B build -G "Ninja" -DCMAKE_PREFIX_PATH="D:/Qt/6.8.3/msvc2022_64" -DCMAKE_BUILD_TYPE=Release

# 3. 编译
cmake --build build --config Release

# 4. 部署 Qt 运行依赖
D:\Qt\6.8.3\msvc2022_64\bin\windeployqt.exe build\OpenSourceCodeReader.exe --qmldir qml

# 5. 启动软件
.\build\OpenSourceCodeReader.exe
```

---

## 📁 核心源码结构

```
Open-source project reading tool/
├── CMakeLists.txt                # MSVC 2022 构建与编译参数
├── resources.qrc                 # Qt 资源文件
├── README.md                     # 本文档
├── .gitignore                    # 严格排除构建产物与二进制文件
├── src/
│   ├── main.cpp                  # 应用程序入口与 QML 类型注册
│   ├── Logger.h/.cpp             # 全局操作日志与诊断记录系统
│   ├── InteractiveGraphView.h/.cpp # Graphviz 拓扑布局、贝塞尔曲线与可点击交互图元引擎
│   ├── DependencyAnalyzer.h/.cpp # C++/QML/Python 依赖与内部符号解析器
│   ├── NetworkTester.h/.cpp      # GitHub/Gitee/GitLab/GitCode 连通性测试模块
│   ├── CodeSyntaxHighlighter.h/.cpp # 语法高亮引擎 (C++/QML/Python)
│   ├── CodeEditorBridge.h/.cpp   # QML 编辑器桥接器
│   └── ProjectController.h/.cpp  # 工程控制器、Git 克隆与报告导出
└── qml/
    └── Main.qml                  # 工业级深色主题界面、自适应分栏与日志终端
```
