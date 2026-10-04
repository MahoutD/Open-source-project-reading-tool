# 开源代码阅读工具 (Open-Source Project Reader)

基于 **C++ 17 + Qt 6.8 (QML) + MSVC** 构建的跨平台开源代码阅读与架构全景分析工具。

---

## ✨ 核心功能与特性

1. **主流托管平台支持与自动化拉取解析**
   - 支持通过输入 GitHub、Gitee (码云)、GitLab、GitCode 等 Git 仓库链接，后台异步浅克隆（`--depth 1`）并自动进入项目进行深度语法解析。
   - 支持直接通过文件夹选择器加载本地已有源码目录。

2. **跨平台网络环境连通性探测 (网络诊断中心)**
   - 界面内置针对 GitHub、Gitee、GitLab、GitCode 等平台的即时网络连通性探测功能。
   - 采用 Qt 异步网络引擎测量延迟、HTTP 状态码及可用性，便于开发者在阅读远端代码前判断网络代理与联通状态。

3. **智能拓扑关系图谱分析 (文件依赖 & 文件内部符号)**
   - **全项目文件拓扑图**：动态解析 C/C++/QML/Python 等代码的头文件包含、导入关系，输出 Graphviz DOT 拓扑结构。
   - **单文件内部结构图**：提取文件内的 `class`、`struct`、`function`、`component`、`#include` 关键符号并生成内部层次关系图。
   - **双模渲染引擎**：
     - 若系统安装了 Graphviz，支持直接调用 `dot` 编译成高保真矢量 SVG；
     - 若未安装 Graphviz，内置基于 QQuickPaintedItem / 拓扑数学模型的交互式渲染器，支持平移拖拽、鼠标滚轮平滑缩放与视图重置。

4. **现代化高亮代码编辑器 (Scintilla 风格)**
   - 采用 QML 现代化暗色主题。
   - 集成独立行号显示条（Gutter）与代码视口精准对齐。
   - 基于 C++ 底层 `QSyntaxHighlighter` 驱动的语法高亮系统（支持 C/C++ 关键字、Qt 类名、预处理宏、字符串与注释高亮）。

5. **全项目宏观概况与多维度统计报告**
   - 自动生成 Markdown 格式的统计报告。
   - 统计项目代码总行数、总文件数、文件体积。
   - 生成各语言文件类型占比表格及高耦合模块依赖预警分析。

---

## 🛠️ 构建与环境要求

- **操作系统**: Windows 10 / 11 (x64)
- **编译工具链**: Visual Studio 2022 (MSVC 19.44+, C++17)
- **Qt 版本**: Qt 6.8.3 (msvc2022_64)
- **构建工具**: CMake (>= 3.20) + Ninja
- **代码版本控制**: Git (已配置在系统 PATH 中)
- *(可选)* **Graphviz**: 系统安装 `dot` 命令后可自动激活官方 SVG 渲染模式。

---

## 🚀 编译与运行命令

在项目根目录下通过 VS2022 开发者命令提示符执行：

```powershell
# 1. 初始化 VS2022 x64 编译环境
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"

# 2. CMake 配置工程
cmake -B build -G "Ninja" -DCMAKE_PREFIX_PATH="D:/Qt/6.8.3/msvc2022_64" -DCMAKE_BUILD_TYPE=Release

# 3. 编译构建
cmake --build build --config Release

# 4. 部署 Qt 运行时动态库
D:\Qt\6.8.3\msvc2022_64\bin\windeployqt.exe build\OpenSourceCodeReader.exe --qmldir qml

# 5. 运行软件
.\build\OpenSourceCodeReader.exe
```
