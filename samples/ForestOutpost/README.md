# Forest Outpost

一个可在 Limen Editor 中打开的林地哨站样例。场景包含蓝顶城堡、林间岗亭、地台、树木与岩石，可用于查看 OBJ 材质、资源引用及场景编辑。

从 Limen Engine 仓库根目录启动已构建的编辑器。

```bash
./out/LimenEditor --project samples/ForestOutpost/ForestOutpost.lmproject
```

Editor 从 `Config/DefaultGame.toml` 读取启动场景的 AssetID，打开 Forest Outpost。可通过 `--scene <路径>` 临时打开另一个场景。可以选中 `Bluecrest Keep` 查看城堡的对象变换。场景中多棵树共用同一个已登记的模型资源。

## 素材来源

模型、MTL 和共用的 `hexagons_medieval.png` 来自 Kay Lousberg 的 **KayKit: Medieval Hexagon Pack 1.0**，采用 CC0 1.0。样例保留这六个模型的上游文件名和内容，将引用的贴图随模型一并放在 `assets/Models/`。场景的摆放和灯光配置由 Limen 样例定义。[作者发布页](https://kaylousberg.itch.io/kaykit-medieval-hexagon) · [上游版本](https://github.com/KayKit-Game-Assets/KayKit-Medieval-Hexagon-Pack-1.0/tree/84fa4e91af6a88989be7c99e0891cede11f2ca38) · [随附许可原文](LICENSE-KayKit.txt)
