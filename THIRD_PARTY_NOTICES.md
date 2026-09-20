# Third-party notices and source status

EvoFlow 3.0.1.5 is licensed under the BSD 3-Clause License in `LICENSE`.
That license covers EvoFlow-owned source and does not replace the licenses or
restrictions of external components listed below.

## Project-owned support libraries

The following legacy support libraries are owned as part of EvoFlow even
though their historical path is `thirdparty/`:

- `distancelib`
- `doublelib`
- `filelib`
- `metriclib`
- `randlib`
- `sortlib`
- `stringlib`
- `timelib`

They are covered by the project BSD 3-Clause License. Existing per-file
headers are deliberately frozen until the final publication identity and
citation are known; the repository-level license is authoritative meanwhile.

## Bundled external components with an identified license

| Component | Bundled version | Location | License and copyright source |
|---|---:|---|---|
| pybind11 | 3.0.4 | `thirdparty/pybind11` | BSD 3-Clause; original text in `thirdparty/pybind11/LICENSE` |
| muParserX | 4.0.12 | `thirdparty/muparserx` | BSD 2-Clause; Ingo Berg et al.; reproduced in `thirdparty/licenses/BSD-2-Clause.txt` |
| StatsLib | 3.4.0 | `thirdparty/statslib` | Apache License 2.0; Keith O'Hara; reproduced in `thirdparty/licenses/Apache-2.0.txt` |
| stb_image_write | 1.16 | `thirdparty/stb/stb_image_write.h` | Dual MIT/public-domain offer; EvoFlow selects MIT, reproduced in `thirdparty/licenses/MIT.txt` |
| stb_truetype | 1.26 | `thirdparty/stb/stb_truetype.h` | Dual MIT/public-domain offer; EvoFlow selects MIT, reproduced in `thirdparty/licenses/MIT.txt` |

Copyright and attribution notices embedded in those source files must be
retained. Binary distributions must include this file and the referenced
license texts.

## CEC 2013 benchmark implementations: not distributed

Earlier internal development baselines used two external benchmark
implementations:

- `CEC2013_N` under `_pdata/numerical/CEC2013_N`, associated with X. Li,
  A. Engelbrecht and M. G. Epitropakis, "Benchmark Functions for CEC'2013
  Special Session and Competition on Niching Methods for Multimodal Function
  Optimization," RMIT technical report, 2013. The official competition page
  is <https://titan.csit.rmit.edu.au/~e46507/cec13-niching/competition/>.
- `CEC2013_L` under `_pdata/numerical/CEC2013_L`, associated with X. Li,
  K. Tang, M. N. Omidvar, Z. Yang and K. Qin, "Benchmark Functions for the
  CEC'2013 Special Session and Competition on Large Scale Global
  Optimization," RMIT technical report, 2013. The official competition page
  credits Wenxiang Chen for the C++ implementation:
  <https://titan.csit.rmit.edu.au/~e46507/cec13-lsgo/competition/>.

The downloaded source bundles contain no project-wide license granting public
redistribution. In addition, `CEC2013_N/rand2.cpp` and `rand2.h` contain a
Radford M. Neal notice that grants copying, use and modification only for
research or education. Availability for download and permission to run a
competition benchmark are not treated as permission to redistribute it under
EvoFlow's BSD license.

No project-wide license granting public redistribution was found. Consequently:

- CEC source is not covered by EvoFlow's BSD 3-Clause License.
- Branch `2.7.2.1` removes the CEC source, data, prebuilt library, specialized
  template/API, and build dependencies from the project tree.
- EvoFlow source packages, binaries, and Python extensions do not contain or
  link those implementations.
- A user who lawfully obtains an external benchmark may connect it from
  user-owned C++ or Python code. EvoFlow's external-benchmark tutorials show
  the integration boundary but do not reproduce CEC code or data.

## Data and operating-system assets

Redistributable benchmark instance data is inventoried in `_pdata/DATA_LICENSES.md`.
The program package contains only project-owned samples; the optional full data
overlay contains 377 MIT-declared FrontierCO conversions and 324 MIT-licensed
OR-Library MKP conversions. TSPLIB, QAPLIB and GK data are not redistributed.
System fonts used for optional
PNG rendering are loaded from the operating system at runtime and are not
redistributed by EvoFlow.
