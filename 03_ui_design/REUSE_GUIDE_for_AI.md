# MCHOSE HUB 风格 · 其他软件 UI 复用指南（AI 可直接执行版）

> **给阅读本文档的 AI**：本规范来自对 MCHOSE HUB（外设驱动中心）的逆向设计取证，已整理为**可直接套用**的设计令牌、组件规范、布局骨架、交互模式与构建工作流。
> 当你被要求为一个**新的软件项目**构建 UI 时，请按本文档的「构建工作流」执行：
> 1）先贴入【设计令牌】；2）按【组件规范】写控件；3）按【布局骨架】搭页面；4）按【交互模式】接状态；5）用【检验清单】自查。
> 若目标软件有明确品牌色，仅替换 `--brand` 一处即可，其余全部沿用本体系。

***

## 0. 这套风格的定位（一句话）

**消费级控制台：大量留白 + 低饱和蓝灰白 + 单一克制品牌蓝 + 一个「实体/物体」作为页面视觉中心。**
不靠炫技动效取胜，靠「形态克制、实体辨识清晰、状态与恢复路径完备」建立品质感。

**四条设计原则**

1. 实体优先：页面围绕「要管理的对象」展开，用产品/物体渲染图当页眼。
2. 能力驱动：同一套壳层承载多类对象，页面模块按对象能力动态渲染；没有的能力不出现。
3. 功能分层：常用在前，高级在后；复杂编辑拆「预览 → 编辑 → 应用」，编辑不破坏当前生效值。
4. 状态即恢复：错误/受限/无数据不灰掉整个实体，而是「显示原因 + 下一步动作」。

***

## 1. 设计令牌（复制这段到项目 `styles/tokens.css`）

```css
/* ================= MCHOSE-STYLE TOKENS ================= */
:root {
  /* 品牌色（唯一强调色，换品牌改这里即可） */
  --brand: #0053e2;
  --brand-active: #005ce6;
  --brand-10: rgba(0, 83, 226, .1);   /* hover/选中晕染 */
  --brand-20: rgba(0, 83, 226, .2);
  --brand-80: rgba(0, 83, 226, .8);

  /* 中性（浅色基底） */
  --text-main: #151515;
  --text-2: rgba(21, 21, 21, .5);      /* 次要文字/占位符/禁用文字 */
  --icon-2: #4a4b50;                   /* 图标次级色 */
  --border: #d7dee3;                   /* 边框/分割线 */
  --border-strong: #d0d6da;
  --hover-bg: #e5e9f0;                 /* 列表/项悬停、分隔 */
  --panel-bg: #f2f3f7;                 /* 面板/卡片底/drawer 底 */
  --surface: #ffffff;                  /* 白表面（卡片/弹窗/输入） */
  --side-active-bg: #d8dae3;           /* 侧边菜单选中底 */

  /* 语义色（各配 10%/20% 极淡晕染做「状态块」） */
  --success: #32aa70;  --success-10: rgba(50,170,112,.1);
  --danger:  #ea5e56;  --danger-10:  rgba(234,94,86,.1);   /* hover --danger-active:#dd4e45 */
  --warning: #f59c25;  --warning-10: rgba(245,156,37,.1);
  --info:    #909399;

  /* 深色模式（仅替换中性面，品牌/语义色不变） */
  --dark-panel: #212121;
  --dark-field: #2f2f2f;

  /* 字体 */
  --font: MiSans, "PingFang SC", Helvetica, "Microsoft YaHei", Arial, sans-serif;
  --font-icon: iconfont;   /* 图标专用字体族 */

  /* 字阶 */
  --fs-32:32px; --fs-24:24px; --fs-20:20px; --fs-18:18px;
  --fs-16:16px; --fs-14:14px; --fs-13:13px; --fs-12:12px;

  /* 字重（只用三档） */
  --w-normal:400; --w-medium:500; --w-bold:700;

  /* 图标容器基类 */
  --icon-wrap: flex-shrink:0;  /* svg{vertical-align:middle} */

  /* 圆角分级：层级越高圆角越大 */
  --r-sm:4px;   /* 标签/弹窗 */
  --r-md:6px;   /* 输入/选择 */
  --r-mid:8px;  /* 按钮/tab */
  --r-lg:16px;  /* 卡片 */

  /* 阴影（低透明、柔和、可叠加） */
  --shadow-0: 0 4px 12px 0 rgba(0,0,0,.08);
  --shadow-1: 0 3px 6px 0 rgba(0,0,0,.12);
  --shadow-2: 0 6px 16px 0 rgba(0,0,0,.18);
  --shadow-3: 0 9px 30px 0 rgba(0,0,0,.2);

  /* 控件尺寸节奏（统一高度保证整窗一致） */
  --ctl-h:40px; --ctl-r:6px; --ctl-px:12px;   /* 输入/选择 */
  --btn-r:8px;                                 /* 按钮 */
  --tab-h:36px;                                /* 分段tab */
  --side-w:272px;                              /* 侧边栏宽 */

  /* 动效 */
  --dur-fast:.2s; --dur-base:.3s;
  --ease-io:cubic-bezier(.23,1,.32,1);
}

/* 浅色/深色切换（跟随系统示例） */
@media (prefers-color-scheme: dark) {
  :root {
    --text-main:#f5f5f5; --text-2:rgba(255,255,255,.5); --icon-2:#c0c4cc;
    --border:#3a3a3a; --hover-bg:#2b2b2b; --panel-bg:#212121; --surface:#2a2a2a;
    --side-active-bg:#3a3a3a;
  }
}
```

**文字排版规则**：标题用 700 加粗（20–24px），次级/强调 500，正文 400（13–14px），辅助/标签/状态 12px；中文一律无衬线；图标走独立 iconfont，与文字字体分离。

***

## 2. 组件规范（写控件时照此办）

> 所有表单类控件**统一高 40px、圆角 6–8px、hover 底色** **`#e5e9f0`、选中用品牌色、禁用透明 40%**。

| 组件         | 规格 / 状态                                                                            |
| ---------- | ---------------------------------------------------------------------------------- |
| **按钮**     | 高40、min-width104、radius8、字13、图标16；默认透明底透明边 + 主文字；主按钮用品牌底白字；hover 微亮度、active 缩放 .98 |
| **输入框/下拉** | 高40、radius6、内边距左右12、白底、`--border` 边；hover/focus 边 → 品牌色40%；disabled 透明度 .4         |
| **开关**     | 底色 `#d1d2d3`、滑块白；激活可随主题覆盖                                                          |
| **分段Tab**  | 高36、radius8、项间距16；选中：品牌色文字 + 图标点亮（或浅底/下划线），未选中灰                                    |
| **滑块**     | 填充品牌色，轨道浅灰                                                                         |
| **标签 Tag** | 高28、radius4、品牌底白字 或 语义色+晕染底                                                        |
| **进度条**    | 高6（细版3）、填充品牌色；异常态红                                                                 |
| **弹窗**     | radius4、宽400、遮罩 rgba(61,61,61,.5)                                                  |
| **下拉列表**   | 白底、hover `#e5e9f0`、图标次级色                                                           |
| **侧边菜单**   | 宽272、项高40、padding-x16、选中底 `#d8dae3`                                                |
| **状态块**    | 语义色字 + 对应 10% 晕染底（成功/危险/警告）                                                        |

统一状态定义（全窗一致）：`default / hover / active(press缩.98) / focus(品牌描边) / selected(品牌) / disabled(透明40%) / loading(旋转) / error(红描边)`。

***

## 3. 布局骨架（搭页面时套用）

### 3.1 全局壳层（每个页面的根）

```html
<div class="app-shell">
  <header class="title-bar">
    <div class="brand">Logo + 品名</div>
    <div class="tools">
      <!-- 图标优先，全部配 tooltip -->
      <button title="登录"/><button title="刷新"/><button title="换肤" class="is-theme"/><button title="设置"/>
      <span class="win-controls">— ▢ ✕</span>
    </div>
  </header>
  <div class="page"><!-- 按下方三种骨架之一填充 --></div>
</div>
```

壳层两条规则：**图标+tooltip** 工具栏；**整窗不自带滚动条**，滚动只在内容区出现（`html,body{overflow:hidden}`）。

### 3.2 骨架 A · 首页（实体聚焦卡）

大圆角白卡居中、四周留白、卡上产品渲染图下型号+状态行、右上视图切换。

```html
<main class="page home">
  <section class="entity-card">       <!-- 白底 r-lg 阴影-0，尺寸固定不随内容变 -->
    <div class="entity-visual"><img/></div>
    <div class="entity-meta">
      <h1 class="name">型号名</h1>
      <div class="status-row"><span class="state">🔋100%</span><span class="state">📶2.4G</span></div>
    </div>
  </section>
</main>
```

### 3.3 骨架 B · 对象详情（三栏，最具复刻价值）

**设备身份 + 配置档案 + 能力标签 + 可视化实体**：

```html
<div class="detail">
  <aside class="side">              <!-- 宽272 -->
    <h2>对象名</h2><div class="status-row">电量/连接</div>
    <button class="primary">+ 创建配置</button>
    <nav class="segment">自定义 / 云端</nav>
    <ul class="profile-list">
      <li class="item is-active">默认档案1</li>  <!-- 选中品牌色/浅底 -->
      <li class="item">默认档案2</li>
    </ul>
  </aside>
  <section class="config">
    <nav class="segment-primary">能力A / 能力B / 能力C</nav>
    <input class="mc-input is-search" placeholder="搜索"/>
    <nav class="segment-secondary">子能力x / 子能力y</nav>
    <details class="accordion" open><summary>分组</summary>…</details>
  </section>
  <section class="preview">
    <img class="entity-3d"/>         <!-- 3D或2D，带引导线标注可配置部位 -->
    <div class="annotation">部位A</div>
  </section>
</div>
```

### 3.4 骨架 C · 设置/参数面板

白卡容器 + 分区列表（`section` 间用 `--border` 细分割线）+ 每行「标签 + 控件 + 说明小字」。

***

## 4. 交互模式（接状态时照此办）

- **导航**：返回按钮 + 保持上下文（不靠面包屑堆叠）；二级用可折叠分组；三层分段（壳层/一级/二级）。

- **能力驱动**：对象声明哪些能力，页面就渲染哪些模块；**关闭的能力不删除，只局部禁用（透明40%）并给原因**。

- **受限即恢复**：无数据/休眠/权限不足/升级中，展示「原因 + 连接/操作方式 + 下一步动作」组合，可第一个动作永远可点。功能受版本限制时：显示受限原因 + 所需版本 + 升级入口。

- **预览后应用**：复杂编辑（EQ/壁纸/配置/上传）先预览再应用；提供「直接应用/另存后应用」。

- **反馈**：成功提示用图标+短消息；长任务用阶段进度条（写明现在在做什么）；**表单单次校验绑定提交可用性**（无效则禁用提交）。

- **轻量反馈收集**：做成右下角通知而非全屏遮罩；先结构化作答（5级）+ 按需展开自由文本。

- **动效**：函数式界面用 .2–.3s 淡入淡出与滑动即可，不炫技；内容展示（3D/灯效/歌词）才用重动画。

***

## 5. 字体与图标规范

### 5.1 字体（排版）
- **字体族**：中文主栈 `MiSans, "PingFang SC", "Microsoft YaHei", sans-serif`；图标走独立 `font-family: iconfont`，与文字字体分离。
- **字阶**（照此取档，不随意自定义）：
  - 12px 辅助/标签计数/校验错误；13px 正文/输入下拉（主力）；14px 弹窗/说明；
  - 16px 主操作与标题起点/大按钮/a 弹窗标题；18px 菜单图标；20–24px 大标题；24px 消息状态图标。
- **字重**：标题 700、次级标题/强调 500、正文 400（三类取值即够，不要更粗）。
- **字距**：默认不用；仅点缀用 `letter-spacing:.3em`。
- 标题加粗、正文常规、中英文无衬线；行高 `normal`，高度交给组件尺寸控制。

### 5.2 图标（体系）
- 用一套 iconfont（SVG/WOFF2/WOFF/TTF 四格式），**命名约定 `名称_风格`**：
  - `*_linear`（线性/默认态）、`*_surface`（实心/选中点亮态）、`*_char`（键帽字符）；外加平台前缀 `a-*`。
  - 三风格并存，专为「默认描边 → 选中实心 → 键帽字符」的态切换设计——这是最有复刻价值的一点。
- **基类规范**：图标容器 `flex-shrink:0` + 居中；`svg{vertical-align:middle}`；随组件 `font-size:inherit`。
- **尺寸档**：按钮内 16px、菜单 18px、顶栏 20px、消息/状态 21–24px、小型控件 12px。
- **必备图标族**：`home/search/settings/plus/minus/delete/copy/export/import/refresh/save/edit/restore` + 视图切换 `grid/tile` + 语义 `success/danger/warning/info` + 各对象实体图标（`mouse/keyboard/screen/device…`）。

***

## 6. 从需求构建 UI 的工作流（AI 执行顺序）

1. **读目标软件需求**：列出「要管理的对象」（如设备、订单、文章、任务…），每个对象有哪些能力。
2. **贴入第1节令牌**到项目 token.css；若有品牌色，只改 `--brand*`。
3. **选页面骨架**：首页 → 骨架A；对象配置 → 骨架B；纯设置 → 骨架C。
4. **按第2节写控件**、按第3节排版；对象能力动态渲染模块。
5. **按第4节接状态**：给每个能力补「默认/hover/选中/禁用/错误/加载/为空/失败恢复」。
6. **用第7节清单自查**，通过后交付。

***

## 7. 复刻检验清单（AI 交付前自查）

- [ ] 全站一套 Token，无页面自造品牌色；中性色低饱和。

- [ ] 主色仅 `--brand`，语义色各配 10% 晕染。

- [ ] 圆角分级 4/6/8/16，控件高度统一 40。

- [ ] 标题加粗、正文常规、中文无衬线；整窗无全局滚动条。

- [ ] 页面以「实体 + 身份 + 档案 + 可视化」为核，不是纯表格堆砌。

- [ ] 卡片尺寸统一，内容多少不变形。

- [ ] 能力受限：显示原因 + 下一步，不隐藏、不整块灰掉。

- [ ] 复杂编辑走预览→应用，不破坏当前生效值。

- [ ] 8 种交互状态（含 loading/error/disabled/为空/失败恢复）齐备。

- [ ] 深色模式至少替换中性面与文字层。

***

## 8. 注意事项 / 边界

- 本规范是**风格与交互模式**的复刻指导，不含 MCHOSE 品牌资产与专有素材；落地时替换品牌色与图标即可合法复用其**设计范式**。

- 无 3D 资产时用高质量 2D 分层图替代，保留「点部位 → 编辑对应项」的映射。

- 若不基于 Element Plus，裸 CSS 用第1节变量 + 第2节规则即可还原视觉；若用 Vue/React，可将令牌赋给组件库的 `--el-*` 同名字段。

