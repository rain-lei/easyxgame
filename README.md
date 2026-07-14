# C++ / EasyX 面向对象课程作业

本仓库用于管理“面向对象方法程序设计与实践”课程作业，开发环境为 Visual Studio、C++17 和 EasyX。

## 内容

- `homework1/`：1.1 基础练习与 1.2 进阶练习，共四个面向对象 EasyX 项目。
- `final_project/MoeBubbleBattle/`：原创单人 Q 版闯关游戏《萌泡大作战》及其 UI、角色、音乐、测试和设计资料。
- `EasyxGame参考/`、`面向对象实验参考/`：课程提供的参考材料；软件安装包不纳入版本控制。
- 根目录的 DOCX 文件：课程作业要求与报告模板。

## 打开工程

- 四个练习：使用 Visual Studio 打开 `homework1/EasyXExercises.sln`。
- 大作业：使用 Visual Studio 打开 `final_project/MoeBubbleBattle/MoeBubbleBattle.sln`。
- 建议选择 `x64` 平台，使用 `Ctrl+F5` 运行。

## 版本控制约定

- 提交源码、解决方案、原创图片、原创音频和 Markdown 设计文档。
- 不提交 `.vs`、`build`、`Debug`、`Release`、中间目标文件和本地可执行文件。
- 最终提交压缩包、报告和演示视频在完成后单独归档，不直接放入 Git 历史。

## 最终打包

录制并转换好本人讲解的 720P `.flv` 视频后，在仓库根目录执行：

```powershell
python tools\package_submission.py `
  --sequence 序号 `
  --student-id 学号 `
  --student-name 姓名 `
  --teacher 任课教师 `
  --material-link "材料链接" `
  --video "本人讲解视频.flv"
```

脚本会生成 `dist/序号-学号-姓名-萌泡大作战/` 和外部的
`dist/序号-学号-姓名.zip`，自动排除 `.vs`、`x64`、`build`、编译中间文件，
并调用本机 Microsoft Word 将报告转换为真正的 Word 97-2003
`程序设计课程实践报告-姓名.doc`。如需同时保留新版 Word 备份，可添加
`--keep-docx`。最终提交前仍需从 ZIP 解压后试运行程序、播放视频并检查
`.doc` 打印预览。
