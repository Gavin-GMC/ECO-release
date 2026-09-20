# 外部问题数据下载与转换

EvoFlow 的公开包不再重新分发 TSPLIB、QAPLIB 和 GK 数据。用户应直接从发布者或其他有权来源取得数据，并自行确认其使用条件。本目录的转换器只处理用户提供的本地文件，不会下载、内嵌或上传第三方数据。

运行环境只需要 CPython 3.8 以上；转换器不依赖 EvoFlow 的原生扩展或第三方 Python 包。

## TSPLIB

官方入口：<https://comopt.ifi.uni-heidelberg.de/software/TSPLIB95/>；TSP 文件列表：<https://comopt.ifi.uni-heidelberg.de/software/TSPLIB95/tsp.html>。

以 `burma14.tsp.gz` 为例，PowerShell 下载并解压/校验：

```powershell
New-Item -ItemType Directory -Force external-data | Out-Null
Invoke-WebRequest `
  https://comopt.ifi.uni-heidelberg.de/software/TSPLIB95/tsp/burma14.tsp.gz `
  -OutFile external-data/burma14.tsp.gz
python tutorial/external-data/convert_external_data.py tsp `
  external-data/burma14.tsp.gz _pdata/tsp/burma14.tsp
```

TSPLIB 文件本身已接近 EvoFlow TSP 格式，因此该步骤只解压、统一文本并拒绝当前解析器不支持的距离类型或矩阵布局。不要把 `.opt.tour` 放进 `_pdata`：它是参考解，不是问题实例。

## QAPLIB

官方入口及格式说明：<https://qaplib.mgi.polymtl.ca/>。QAPLIB 原始文件为 `n + n×n + n×n` 的纯数字布局，需补 EvoFlow 头部与矩阵行号。

```powershell
Invoke-WebRequest `
  https://qaplib.mgi.polymtl.ca/data.d/bur26a.dat `
  -OutFile external-data/bur26a.dat
python tutorial/external-data/convert_external_data.py qap `
  external-data/bur26a.dat _pdata/qap/bur26a.qap --name bur26a
```

## GK / 标准 MKP 数字格式

GK 数据通常采用标准单实例数字布局：`n m optimum`、`n` 个利润、按资源排列的 `m×n` 消耗和 `m` 个容量。由于当前未确认一个可审计的 GK 再分发授权，EvoFlow 不提供其副本或自动下载地址；请从作者或其他有权来源取得。

```powershell
python tutorial/external-data/convert_external_data.py mkp `
  external-data/gk01.dat _pdata/mkp/gk01.mkp --name gk01
```

对于首项为实例数量的 OR-Library 文件，增加 `--counted`，并用 `--instance-index` 选择其中一个实例：

```powershell
Invoke-WebRequest `
  https://people.brunel.ac.uk/~mastjjb/jeb/orlib/files/mknap1.txt `
  -OutFile external-data/mknap1.txt
python tutorial/external-data/convert_external_data.py mkp `
  external-data/mknap1.txt _pdata/mkp/mknap1-1.mkp `
  --counted --instance-index 1 --name mknap1-1
```

OR-Library 的格式说明见 <https://people.brunel.ac.uk/~mastjjb/jeb/orlib/mknapinfo.html>，许可声明见 <https://people.brunel.ac.uk/~mastjjb/jeb/orlib/legal.html>。

## 加载与检查

转换后可以直接传完整路径：

```powershell
evoflow run _pdata/tsp/burma14.tsp config/ACS.cfg -s 42
evoflow data _pdata
python -m evoflow --check-data
```

默认拒绝覆盖目标文件；确认需要覆盖时显式加 `--overwrite`。完整数据包安装与 manifest 校验见 [`../../docs/数据包与外部数据.md`](../../docs/数据包与外部数据.md)。
