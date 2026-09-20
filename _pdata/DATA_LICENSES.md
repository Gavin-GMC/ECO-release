# EvoFlow data notices

This notice applies to the optional EvoFlow 3.0.1.5 full data overlay. EvoFlow's
program package contains only the 13 project-owned `sample.*` instances listed
by `manifest-minimal.json`; those samples are covered by the repository's BSD
3-Clause License.

## FrontierCO converted instances

- Source: CO-Bench/FrontierCO, <https://huggingface.co/datasets/CO-Bench/FrontierCO>
- Included selection: 377 converted instances: CFLP 40, CPMP 51, CVRP 39,
  FJSP 104, MDS 30, MIS 73, and STP 40.
- Transformation: source formats were converted into the corresponding
  EvoFlow text formats; selection was bounded by the development dataset
  policy and does not imply that the source set is complete.
- License evidence: the dataset card declares the dataset to be released under
  the MIT License. At the 2026-08-03 release audit, that card referred to a
  `LICENSE` file but no such file appeared in the repository file listing.
  This notice records both the explicit card declaration and that packaging
  limitation instead of silently treating the data as EvoFlow-owned.
- Citation requested by the dataset card: Shengyu Feng, Weiwei Sun, Shanda Li,
  Ameet Talwalkar, and Yiming Yang, "A Comprehensive Evaluation of
  Contemporary ML-Based Solvers for Combinatorial Optimization," 2025.

## OR-Library multidimensional knapsack instances

- Source and format: <https://people.brunel.ac.uk/~mastjjb/jeb/orlib/mknapinfo.html>
- Legal notice: <https://people.brunel.ac.uk/~mastjjb/jeb/orlib/legal.html>
- Included selection: 324 converted MKP instances: 270 Chu-Beasley instances
  from `mknapcb1` through `mknapcb9`, plus 54 selected instances from
  `mknap1` and `mknap2`.
- Attribution: Copyright (c) 2010 J E Beasley.

The OR-Library legal page applies the following MIT terms to all material made
available from OR-Library:

> Permission is hereby granted, free of charge, to any person obtaining a copy
> of this software and associated documentation files (the "Software"), to deal
> in the Software without restriction, including without limitation the rights
> to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
> copies of the Software, and to permit persons to whom the Software is
> furnished to do so, subject to the following conditions:
>
> The above copyright notice and this permission notice shall be included in
> all copies or substantial portions of the Software.
>
> THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
> IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
> FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
> AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
> LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
> OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
> SOFTWARE.

## Data not redistributed

EvoFlow does not redistribute TSPLIB, QAPLIB, or the Glover-Kochenberger (GK)
MKP set. Their formats remain supported. Users may obtain instances directly
from an authorized source and convert them with
`tutorial/external-data/convert_external_data.py`; source availability alone is
not represented here as a redistribution license.
