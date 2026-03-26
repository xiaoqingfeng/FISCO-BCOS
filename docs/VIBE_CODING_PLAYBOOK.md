# FISCO-BCOS × Codex Vibe Coding 使用手册（内部版）

> 适用对象：研发、测试、技术负责人、协作开发同学。  
> 目标：让团队可以稳定复用“人类 + Codex”协作方式，快速完成需求实现、问题定位、分支治理、构建验证与PR交付。

---

## 1. 这份手册解决什么问题？

在真实协作中，最常见的问题不是“代码不会写”，而是：

- 分支推到了 A 仓库，却在 B 仓库看分支；
- SSH 多账号混用，`Host` 别名不一致导致推送失败；
- 本地有改动，但 compare 页面显示 “There isn’t anything to compare”；
- 任务已经合入 master，却还在追历史分支；
- 构建成功但测试未发现（`No tests were found!!!`）不知道如何解释。

本手册把上述高频坑统一为标准流程，目标是：**可复现、可交接、可扩展**。

---

## 2. Vibe Coding 在本团队中的定义

在本项目中，Vibe Coding 不是“全自动写代码”，而是：

1. **人**负责目标、约束、业务判断；
2. **Codex**负责分解、命令、补丁、排障路径、PR文字；
3. 双方按短回路迭代：`执行 -> 回传输出 -> 修正策略`。

一句话：**高频反馈的人机协作开发模式**。

---

## 3. 协作模式（建议固定使用）

### 模式 A（推荐，效率最高）

- 开发同学提供：目标、评论、约束；
- Codex提供：
  - 变更计划；
  - 可直接执行命令；
  - commit message / PR 描述；
  - reviewer 回复模板；
- 开发同学执行并回传输出。

适用场景：日常开发、评论整改、发布前验证。

### 模式 B（精细排障模式）

- 一轮只执行一条命令；
- 适合 SSH、remote、branch、权限等环境问题；
- 优先用于“命令执行失败”的窄域调试。

适用场景：环境不稳定、上下文混乱、多人协作时。

---

## 4. 仓库关系治理（必须先做）

实际协作常同时涉及三类仓库：

- `upstream`：主仓（例如 `FISCO-BCOS/FISCO-BCOS`）
- 个人 fork（例如 `andrew199799/FISCO-BCOS`）
- 内部协作仓（例如 `xiaoqingfeng/FISCO-BCOS`）

### 核心规则

每次操作前先明确三件事：

1. **我要推到哪个 remote？**
2. **我要推到哪个分支名？**
3. **我要在 GitHub 哪个仓库页面看结果？**

否则很容易出现“我明明 push 成功了，但页面看不到分支”的错觉。

---

## 5. SSH 多账号标准配置（简体中文说明）

`~/.ssh/config` 推荐如下（示例）：

```ssh-config
# 账户1：个人账号
Host github.com-Andrew
HostName github.com
User git
IdentityFile ~/.ssh/id_ed25519_andrew199799
IdentitiesOnly yes

# 账户2：协作账号
Host github.com-Andrew-Ai-auto
HostName github.com
User git
IdentityFile ~/.ssh/id_ed25519
IdentitiesOnly yes
```

### 关键注意事项

- 你在 `git remote` 里使用的 Host，必须是上面声明过的别名；
- 若使用未声明别名（例如 `github.com-xiaoqingfeng`），会出现：
  - `Could not resolve hostname ...`

---

## 6. 常用命令手册（可直接复制）

### 6.1 远端与分支基础检查

```bash
git remote -v
git branch -vv
git status --short
git fetch --all --prune
```

### 6.2 添加内部协作仓 remote

```bash
git remote add xqf git@github.com-Andrew:xiaoqingfeng/FISCO-BCOS.git
```

### 6.3 推送当前分支到指定远端分支

```bash
git push -u xqf HEAD:feature/chainapi-he
```

### 6.4 基于远端 master 建立干净修复分支

```bash
git checkout -B feature/chainapi-he-fix xqf/master
git push -u xqf feature/chainapi-he-fix
```

---

## 7. Git 结果判读手册（非常重要）

### 7.1 `src refspec xxx does not match any`

含义：本地没有该分支，或该分支没有提交。

### 7.2 Compare 页面显示 “There isn’t anything to compare”

含义：base/compare 指向同一历史，或你选错了仓库。

### 7.3 `git rev-list --left-right --count A...B` 判读

- `0 N`：B 比 A 领先 N 个提交；
- `N 0`：B 比 A 落后 N 个提交；
- `0 0`：A/B 历史一致。

示例：`200 0` 说明当前分支比 master 落后 200 个提交且无新增。

---

## 8. 代码状态追踪策略（避免“追错历史”）

当你怀疑“功能已经合了但找不到提交”时：

1. 先看提交是否已被拆分合入 master；
2. 再看目标文件是否已存在（例如 `ChainAPIPrecompiled.cpp`）；
3. 最后再决定是 `cherry-pick` 还是“从 master 新开修复分支”。

**结论优先级：当前 master 的真实状态 > 历史分支名称。**

---

## 9. 构建与运行验证（内部测试版）

### 9.1 构建

```bash
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . -j$(nproc)
```

### 9.2 测试发现

```bash
ctest --output-on-failure -j$(nproc)
```

如果输出 `No tests were found!!!`，在当前构建配置下可能是正常现象（未启用测试集）。

### 9.3 二进制运行验证

从仓库根目录执行：

```bash
./build/bin/fisco-bcos -v
```

建议记录以下信息：

- Version
- Build Type
- Git Branch
- Git Commit Hash

这四项是“可运行证据链”的核心。

---

## 10. 内部版本管理规范（不公开发布场景）

如果当前版本仅用于内部验证：

- 不打公开 release/tag；
- PR 标题标记：`[Internal]` / `[Do Not Merge]`；
- PR 顶部注明：仅内部验证，不对外发布；
- 结果同步到团队内，不对外公告。

建议文案：

```text
[内部验证版本]
本分支/PR仅用于内部测试与验证，当前阶段不用于公开发布。
```

---

## 11. 与 Codex 协作的标准输入模板（给同事用）

每轮请至少提供：

1. 目标（要达成什么）；
2. 当前分支和目标仓库；
3. 当前命令输出（完整错误）；
4. 约束（是否可改接口、是否允许 force push、是否内部版）。

这样 Codex 可以一次给出高质量可执行方案，减少来回。

---

## 12. Reviewer 回复模板（可直接复制）

### 12.1 功能已在 master 的场景

```text
Thanks for the review.
This ChainAPI/HE-related integration has already been merged into master through prior commits/PRs.
Current master build/runtime verification passed, and `./build/bin/fisco-bcos -v` confirms the expected commit state.
If extra follow-up changes are needed, please point to exact file/line and we will provide a focused patch.
```

### 12.2 内部验证状态同步

```text
Status: Internal validation in progress.
- Remote/branch alignment: done.
- Build: passed.
- Runtime version check: passed.
- Public release action: not started.
Next: targeted functional checks and reviewer-driven deltas.
```

---

## 13. 团队落地建议（实践版）

1. 新人先按本手册跑一次完整流程；
2. 每次问题都先跑“基础诊断四连”；
3. PR前必须补齐“构建 + 运行版本输出”；
4. 涉及多仓库时，先对齐 remote，再谈代码；
5. 所有内部版必须明确标注“非公开发布”。

---

## 14. 一页速查（TL;DR）

- 先看 `git remote -v`，再操作；
- push 成功但看不到分支，先确认“看的是不是同一个仓库”；
- `200 0` 代表分支落后且无新增；
- 构建通过 + `./build/bin/fisco-bcos -v` 是最硬验证；
- 内部版不公开发布，必须显式标注。

---

如果你希望，本手册下一版可以再增加：
- “新人 5 分钟上手流程图”；
- “典型错误到修复命令对照表”；
- “发布前 checklist（可打印版）”。
