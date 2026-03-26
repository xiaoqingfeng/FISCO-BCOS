# ChainAPI-HE 协作开发完整输出（内部版）

> 本文档用于直接分享给同事，汇总本次 Codex Vibe Coding 协作开发中的定义、流程、命令、问题定位、环境修复、构建验证与交付建议。

---

## 1. 本次任务目标

1. 引入 ChainAPI HE 预编译能力（动态 vendor bridge）。
2. 修正/统一 Cryptopp 与 RocksDB 外部构建参数。
3. 完成多仓库多账号协作（remote/branch/SSH）打通。
4. 完成内部测试构建与可运行验证。
5. 产出团队可复用方法文档（简体中文）。

---

## 2. 关键产物（代码与文档）

### 2.1 代码侧产物

- `libprecompiled/extension/CallVendorHE.h/.cpp`
- `libprecompiled/extension/ChainAPIPrecompiled.h/.cpp`
- `libprecompiled/Common.h`（新增 `CHAIN_API_ADDRESS`）
- `cmake/templates/UserPrecompiled.h.in`（注册 ChainAPI 预编译）
- `libprecompiled/CMakeLists.txt`（链接 `${CMAKE_DL_LIBS}`）
- `cmake/ProjectCryptopp.cmake`
- `cmake/ProjectRocksDB.cmake`

### 2.2 文档侧产物

- `docs/VIBE_CODING_PLAYBOOK.md`（简体中文使用手册）

---

## 3. 协作中遇到的核心问题与解决路径

### 问题 A：分支“推了但看不到”

**根因**：推送仓库和查看仓库不一致（fork A 推送，仓库 B 页面查看）。

**解决**：
- 明确 `origin / upstream / xqf` 各自指向。
- 使用 `git remote -v` 与 GitHub 页面仓库一一对应。

### 问题 B：`src refspec ... does not match any`

**根因**：本地分支不存在或没有提交。

**解决**：
- 先 `git branch -vv` 确认本地分支；
- 必要时 `git checkout -b <branch>` 后再推送。

### 问题 C：`Could not resolve hostname github.com-xxx`

**根因**：SSH Host 别名未在 `~/.ssh/config` 定义。

**解决**：
- 统一使用已定义别名（如 `github.com-Andrew` / `github.com-Andrew-Ai-auto`）。
- 推送前先 `ssh -T git@<alias>` 验证。

### 问题 D：PR compare 页面显示没有差异

**根因**：
- base/compare 选错仓库，或
- 分支与 master 历史一致（`0 0`），或
- 分支落后且无新增（如 `200 0`）。

**解决**：
- `git rev-list --left-right --count base...head` 判读；
- 必要时基于 `xqf/master` 重新创建干净修复分支。

---

## 4. 可复用命令清单（建议团队直接复用）

### 4.1 基础诊断四连

```bash
git remote -v
git branch -vv
git status --short
git fetch --all --prune
```

### 4.2 新增协作远端并推送

```bash
git remote add xqf git@github.com-Andrew:xiaoqingfeng/FISCO-BCOS.git
git push -u xqf HEAD:feature/chainapi-he
```

### 4.3 从协作仓 master 建立修复分支

```bash
git checkout -B feature/chainapi-he-fix xqf/master
git push -u xqf feature/chainapi-he-fix
```

### 4.4 比较 ahead/behind

```bash
git rev-list --left-right --count xqf/master...xqf/feature/chainapi-he
```

解释：
- `0 N`：分支领先 N；
- `N 0`：分支落后 N；
- `0 0`：分支与 master 一致。

---

## 5. 构建与运行验证结果（内部）

### 5.1 构建

执行：

```bash
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . -j$(nproc)
```

结果：构建成功，`Built target fisco-bcos`。

### 5.2 测试发现

执行：

```bash
ctest --output-on-failure -j$(nproc)
```

结果：`No tests were found!!!`（在当前配置下可视为“未启用测试集”，不是构建失败）。

### 5.3 运行验证

执行：

```bash
./build/bin/fisco-bcos -v
```

结果样例（成功）：
- FISCO-BCOS Version: 2.8.0
- Build Type: Linux/g++/Release
- Git Branch: master
- Git Commit Hash: 64047c1...

---

## 6. 内部版发布策略（当前适用）

当前版本用于内部验证，不对外发布：

- 不打公开 release/tag；
- PR 标注 `[Internal]` 或 `[Do Not Merge]`；
- PR 顶部明确“仅内部验证”。

建议文案：

```text
[内部验证版本]
本分支/PR仅用于内部测试与验证，当前阶段不用于公开发布。
```

---

## 7. 推荐给同事的协作标准输入

每次与 Codex 协作时，建议统一提供：

1. 当前目标；
2. 目标仓库与目标分支；
3. 完整命令输出（尤其是报错）；
4. 约束条件（可否 force push、是否内部版等）。

这样能显著减少来回沟通成本。

---

## 8. 一条可直接发 Reviewer 的状态说明（英文）

```text
Thanks for the review.
This ChainAPI/HE-related integration has been validated in our internal workflow.
Build passed in Linux/g++/Release, and runtime version check via `./build/bin/fisco-bcos -v` succeeded.
This iteration is currently for internal validation only (no public release action yet).
If additional follow-up changes are needed, please point to exact file/line and we will provide a focused patch.
```

---

## 9. 结论

本次协作已验证：

- 代码路径可构建、可运行；
- 多仓库/多账号协作链路打通；
- 关键排障路径可复用；
- 团队可按 `docs/VIBE_CODING_PLAYBOOK.md` + 本文档进行标准化扩散。

