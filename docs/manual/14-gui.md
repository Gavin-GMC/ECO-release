# 14 · Graphical User Interface

[English](14-gui.md) | [简体中文](../手册/14-使用-图形界面.md) | [Manual index](README.md)

The GUI lets you inspect problems, assemble configurations, connect a model service, run optimization, and analyze experiments without writing configuration files by hand. A local service and your default browser are supplied by the same `evoflow.exe`; no separate frontend installation is needed.

The GUI and model integration currently require the Windows x64 release. Linux core/CLI validation does not imply that these Windows API features have been ported. For batch scripts and complete command-line options, see the [CLI guide](13-command-line.md).

## 1. Starting and exiting

### 1.1 Start

Double-click `evoflow.exe`, or run:

```sh
evoflow
```

The program starts a service on a dynamic `127.0.0.1` port, generates a token for the session, opens your default browser, and shows the connection state in the lower-left corner. The actual product version appears beneath the logo.

When launched from File Explorer, the launch console closes after the browser opens. An existing CMD/PowerShell window is not closed; its prompt returns. The background GUI host has no console window.

The logo comes from `figs/Logo.png`. Its subtitle is “Modular evolutionary computation workbench” in English mode; the version is supplied by the local health endpoint. Keep the `figs` directory alongside the executable when moving it.

### 1.2 Exit

Click **Exit EvoFlow** to shut down the service and display the exit state. Closing the browser tab/window also sends a shutdown notification. After approximately two seconds of refresh grace, the service cancels active optimization and exits. A refreshed page's immediate heartbeat withdraws that shutdown request. Heartbeat timeout handles a forcibly terminated browser that cannot notify the service.

If the page reports a missing session token or disconnected service, start `evoflow.exe` again rather than continue using an old tab.

## 2. Three pages

| Page | Input | Purpose and output |
|---|---|---|
| Problem Definition | Natural-language description, standard problem file, or description plus nonstandard text data | Define/inspect a problem and save its original data |
| Optimizer Builder | v3.1/v3.2 configuration or blank skeleton | Edit, validate, save `.cfg`, and run the current problem/configuration |
| Statistical Analysis | `_log` or another log root | Scan batches, configure statistical reports, preview/download SVG/PNG |

Pages share one local service; switching pages does not start another process.

### 2.1 Global model settings

The top-right model settings apply across pages. Model features use OpenAI-compatible Chat Completions:

| Field | Meaning |
|---|---|
| Full endpoint | The complete Chat Completions endpoint, not merely the service root |
| Model name | An identifier accepted by the service |
| API Key | Optional for local services without authentication |

Public endpoints require HTTPS. Plain HTTP is allowed only for `127.0.0.1` or `localhost`. Connection testing sends a fixed check message, not problem/configuration content. States distinguish unconfigured, testing, connected, and failed. Transient network, timeout, rate-limit, or server failures allow at most three actual requests. With a 90-second receive timeout per request, the worst case can take about 4.5 minutes.

Settings remain in page memory only and disappear on refresh, close, or exit. They are not written to browser storage, configuration/problem files, or ordinary logs. Problem generation and existing-configuration recommendation were introduced in 2.4.2 and 2.4.3; 2.4.4 added runtime documentation discovery and bounded retries.

### 2.2 Language

The top-right **中 / EN** button immediately switches navigation, headings, forms, canvas states, statistics, and chart menus. User-entered names, file contents, tags, and results are not translated. The choice is page-local; refreshing returns the GUI to Chinese. This GUI behavior is independent of the default language of the GitHub README.

## 3. Problem definition

### 3.1 Unified input

Provide a description, a file, or both, then submit. Selecting a file only stages it; at least one input is required.

| Input | Processing | Model service? |
|---|---|---|
| Description only | Identify an existing template, generate data, validate, and load | Yes |
| File only | Locally load an EvoFlow standard problem file | No |
| Description and file | First attempt local standard loading; only on failure treat the file as model reference data | Depends on the file |

Standard files can be at most **64 MiB**. `TYPE:` and the existing `loadProblem` path select the family; see [problem modules](10-problems.md) and `_pdata/<family>/README.md`. A failed file-only load does not automatically upload the file; the UI asks for a natural-language description before another submission.

With a description and a valid standard file, loading succeeds locally without uploading either input. Only a failed standard load enters the reference-data path. Such reference data must be valid UTF-8 text, at most **1 MiB**, with no NUL characters. Plain text, CSV, TSV, and JSON are supported; `.xlsx`, images, archives, and other binary attachments are not parsed. For example, supply a coordinate CSV and request the shortest route visiting all locations.

### 3.2 Display and save

The page displays filename, stable problem name, type, decision dimension, variable count and definitions, objectives, and constraints. Variable entries include name, kind, value type, length, and bounds. Objectives include direction, priority, and associated variables. Constraints include type, variables, penalty coefficient, and target objectives.

Ordinary metadata inspection reads definition specifications. It does not compile a handle, run an optimizer, or mix automatically generated compiled domain constraints into the definition list.

Definitions are read-only in this version. **Save Problem As** writes the original data to another location, falling back to a browser download when filesystem writing is unsupported. It does not reserialize the template: input and saved data remain byte-for-byte identical.

Manually loaded problems use the filename stem, for example `berlin52.tsp` → `berlin52`. Generated problems use `NAME:` from the generated data; the prompt asks for a concise stable name rather than copying the sample. Subsequent runs retain that name, so logs consistently go under `_log/<optimizer>/<problem>/`, not random temporary filenames.

### 3.3 Model-assisted construction

Configure a model and pass its connection test, then:

1. Describe the objects, data, objective, and constraints.
2. Optionally attach nonstandard UTF-8 text/CSV/TSV/JSON data.
3. Submit the description or description-plus-data request.
4. The service discovers registered templates and runtime `_pdata` documentation and identifies a type.
5. After exact whitelist validation, a second stage generates data for that template.
6. EvoFlow checks format, size, counts, and `TYPE`, loads through the standard path, and calls `compile()` once.
7. Successful output appears in the existing metadata tables.

Users are not asked to choose technical template types such as TSP/CVRP/QAP. The inferred type is progress information. Descriptions outside the registered template set fail rather than being forced into a vaguely similar type.

Submitting model-assisted construction sends the description and reference data to the configured service. Both are marked as untrusted data in the prompt; attachments supply facts and cannot change required output formats. The model proposes classifications/data. EvoFlow mechanically checks the exact type whitelist, strict JSON, data/count/matrix/graph limits, agreement between classified and embedded `TYPE`, successful template loading, and compilation.

Classification and generation each allow at most three actual model requests. Transient transport failures retry the same prompt. JSON, whitelist, preflight, loading, or compilation failures feed a bounded machine reason and previous response back to request a complete replacement. Authentication/settings errors, unavailable local documentation, valid `type:null`, or unwritable temporary files stop immediately. Final failure leaves the current problem intact. Generated data stays in a session temporary directory and can be saved through **Save Problem As**.

There are 13 exact file TYPE values and 12 documentation families. TSP and ATSP share `_pdata/tsp` and `PT_TSP`; their file TYPE distinguishes symmetric/directed semantics. No separate ATSP directory/example is created. Arbitrary external native benchmarks are accessed through C++/Python, not dynamically loaded by the GUI.

## 4. Optimizer builder

### 4.1 Files and drafts

| Action | Meaning |
|---|---|
| New | Create an editable blank skeleton |
| Open `.cfg` | Load v3.1 text or v3.2 JSON, at most 64 MiB |
| Save | Overwrite through an acquired file handle; otherwise use Save As |
| Save As | Choose a destination, or fall back to browser download |
| Format | Choose v3.1 text/v3.2 JSON; both use `.cfg` |
| Assembly validation | Construct an optimizer without running it |
| Run tag / seed / rounds | Session settings: tag up to 128 characters, seed 0–4294967295, 1–10000 independent rounds |
| Run Current Configuration | Execute the current problem/draft using configured termination budgets |

Both file formats become one canonical in-memory draft. Forms and blocks edit that same draft. Saving uses the existing codec, without a new file type.

The configuration's default tag is loaded/saved with the file. The session run tag is independent and is not replaced by opening, creating, or recommending another configuration. A nonempty session tag overrides the configuration tag; if both are empty, normal automatic tag generation applies. New/Open asks for confirmation when unsaved changes exist.

### 4.2 Recommend an existing configuration

After loading a problem and connecting a model, choose recommendation based on the current problem. The service reads problem type, dimensions, variables, objectives, and constraints; enumerates only direct `.cfg` children of the fixed `config` directory under the launch working directory; and decodes candidates into canonical drafts. The model selects one exact filename from that whitelist. The server checks it verbatim and supplies the mechanically loaded candidate to the editor.

The model cannot return arbitrary paths, new configuration text, algorithm definitions, changed parameters, components, or order. Wrong case, explanations, Markdown, extra JSON fields, and unknown filenames trigger bounded repair, with at most three actual requests.

Loading a recommendation asks before replacing an unsaved draft. Cancellation/failure preserves the draft. A recommendation holds no write handle to the built-in source file, so Save becomes Save As rather than overwriting it.

Recommendation sends a necessary structured problem summary and bounded candidate summaries to the user's endpoint. No service, model name, or Key is preconfigured.

### 4.3 Configuration tree

| Section | Settings |
|---|---|
| Optimizer | Name/default tag, FES/time/stagnation limits, result/history/full-history/population-snapshot logging, global archive and parameters |
| Workflows | Add/remove workflows; identifiers, initializer, ordered components; renaming a workflow updates referencing subpopulations |
| Subpopulations | Add/remove groups; identifier, size, workflow binding, local termination, archive |
| Cooperation | Manager, subpopulation builder, cooperation topology |

Keep at least one workflow and subpopulation. Identifiers written to v3.1 should not contain spaces; letters, digits, underscores, dots, and hyphens are recommended.

### 4.4 Form workflow

Add components, select category/tag, move up/down, duplicate, or delete. Workflow categories are topology, repair, strategy, generation, evaluation, and selection. Categories may repeat; component contracts and execution semantics determine legality. `SET` components generally precede generators that consume them.

The registry-derived component catalog covers 12 categories and searches tags, category keywords, and Chinese/English category names. Search filters the display only. `individual` and `feature` are inferred during assembly and are browse-only, not direct workflow entries.

### 4.5 Parameters

Controls are generated from `ParameterTemplate`, not a second frontend parameter table. Each shows name, Enum/Int/Real type, hard bounds, recommended range, current value, and whether it can be empty.

Using internal defaults omits the parameter array. Disabling that option fills registry-recommended defaults. Dynamic parameters are revealed according to earlier values. Some enums show numbers because their metadata supplies ranges but no friendly labels; semantic labels remain `UI-ENUM-LABELS` work, and the frontend does not invent them.

### 4.6 Validation

Every edit triggers static validation. The badge/report shows highest severity, Error/Warning/Info counts, optimizer/workflow/subpopulation location, component index, stable `reason`, and message. Clicking an issue locates its form item.

- Error: statically unbuildable or invalid execution chain.
- Warning: buildable, but normal execution/termination is not guaranteed.
- Info: execution and termination are possible, but semantics/results/efficiency may differ from intent or contain redundancy.
- Pass: no static issue detected; this is an aggregate state, not proof for every runtime input.

Assembly validation actually constructs the optimizer without running it, catching additional issues such as nonexistent workflow references. GUI, CLI, and Python share `ConfigValidator → AssertMatcher + ParamMatcher`. Only Error blocks assembly/execution. Warning/Info remain visible and allow saving/running. Saving a draft containing Error is permitted after confirmation, so a saved file is not necessarily runnable.

## 5. Block workflow

### 5.1 Enter block mode

Select a workflow and choose Blocks. Switching back to Forms preserves components/parameters through the shared draft. The initializer is a fixed `START` node; edit its type/parameters in Forms.

### 5.2 Canvas

Drag catalog components anywhere, double-click to append to a valid chain, reposition blocks freely, or edit category/tag/parameters in the inspector. Delete selected blocks with `Delete`/`Backspace`.

Positions do not define execution order; directed arrows do. Zoom ranges from 40% to 180%; clicking the percentage restores 100%. Below 90%, SVG actually shrinks. Auto Arrange creates a compact grid in directed-flow order. Fit View computes scale from actual bounds and returns to the top-left.

Fullscreen hides surrounding configuration and permanent parameter panels, fits the canvas, and provides a collapsible floating operator library sharing the search filter. Drag from it or double-click an operator to append and open its parameters. Double-click an existing block to open the floating inspector. `Esc` first closes the inspector; another `Esc` or Exit Fullscreen restores the normal layout. Oversized canvases remain scrollable.

Positions/zoom are page state and are not stored in `.cfg`. Reloading builds a default layout from component order.

### 5.3 Arrows and order

Drag a source's right output connector to a target's left input connector. A new outgoing connection replaces the old outgoing edge; connecting to an occupied input replaces its previous edge. Click a connection, or use `Delete`, `Backspace`, or `Enter` when it has keyboard focus, to remove it.

The canvas writes order back only when a path starts at `START`, visits every block exactly once, has no branch/cycle/omission, forms one directed chain, and passes existing connection validation. `.cfg` still stores a linear array, not an arbitrary DAG. It saves neither separate edges nor coordinates.

### 5.4 Temporary disconnected states

During dragging/rewiring, isolated blocks and incomplete paths are allowed in the canvas editor without replacing the configuration draft. The latest validated complete chain is retained.

Leaving for Forms, another page/workflow, or replacing a draft through New/Open/Recommendation prompts when temporary or invalid components remain. Cancel preserves the canvas; confirming departure discards temporary edits and restores the latest valid chain. Refresh/close also uses the browser's unsaved-change warning. Isolated components are never silently written into the draft.

### 5.5 Connection status

A complete candidate chain is validated server-side. Valid candidates update the draft; invalid candidates remain temporary with their original reasons.

| Status | Meaning |
|---|---|
| `satisfied` | Structural contract met |
| `informational` | Allowed, but semantics/results/efficiency may differ or be redundant |
| `notfullysatisfied` | Allowed, but normal execution/termination is uncertain |
| `failsatisfied` | Insufficient provision count |
| `unsatisfied` | Missing type, order, or another hard contract |
| `isolated` | Not connected to the current path from START |

The inspector shows category/tag, parameter-dependent requirements/provisions, validation reasons, controls, and a Locate in Form action. The shared report below always describes the latest valid draft, not temporary disconnected canvas state.

## 6. Run the current problem and configuration

Set the session tag, seed, and round count. Normally leave result start empty to continue after the largest existing index for the same optimizer/problem/tag. Run first performs static and assembly validation, encodes the draft to temporary v3.1 `.cfg`, and uses `App::runOptimization`, shared with the CLI. Configured budgets apply; the session tag overrides the default tag.

Progress shows current/completed rounds. Successful output records each round separately and displays actual tag, optimizer identity, invocation round, disk result index, seed, best fitness, objectives, and full solution. The temporary configuration is deleted after the request. Saving the draft first is unnecessary.

Editing the configuration hides old results. After all rounds complete, left/right controls page through each round's own index, fitness, objectives, and solution. Progress granularity is independent rounds, not inner iterations/FES; a long round can leave the displayed round unchanged for some time.

Overwrite becomes available only with a nonnegative explicit result start and requires another confirmation on Run. Without overwrite, any existing `.rslt/.prcs` in the range rejects the whole batch before execution. With overwrite, old files are backed up and restored on failure/cancellation/page closure; replacement commits only after full success. Execution/error logs remain append-only.

There is no manual pause or budget override in this entry point. Closing the page or exiting requests cancellation. For command-line controls, use:

```sh
evoflow run <problem-file> <config-file> [options]
evoflow validate <configuration-or-directory> --build
```

Recommended sequence: inspect problem → edit configuration → clear static errors → assembly validation → run. Saving `.cfg` and using the CLI is also supported.

## 7. Statistical analysis

### 7.1 Input

Analysis reads existing `.rslt` files; it does not rerun optimization. Use the same explicit tag for comparable algorithms:

```sh
evoflow run problemA configA --tag exp1 -n 30
evoflow run problemA configB --tag exp1 -n 30
```

### 7.2 Report settings

Enter a log root (default `_log`, relative to the launch directory), scan batches, choose a tag, select optimizers/problems, configure analysis, and generate.

Each report uses one within-run reduction: a descriptive statistic or percentile over a chosen objective, or HyperVolume/GD/IGD/niching quality with per-problem reference data. Across-run aggregation supports multiple columns such as Mean, Std, Median, Q1, Q3, SEM, CV, and multiple Percentiles. Keep at least one aggregation.

Multiple comparisons are allowed. Comparisons needing a representative value select an added statistic; Best/Rank/Win-Tie-Lose also select smaller/larger-is-better. Significance, Kruskal–Wallis, and Dunn use run distributions directly, without irrelevant basis/direction controls. Tests requiring a baseline use the selected baseline optimizer. Parameterized percentiles may appear as columns, but comparison bases currently accept named statistics only.

Defaults are per-run minimum → Mean/Std/Median → Best/Rank → table, cross-problem box plots, and average ranks.

| Chart | Settings/dependencies |
|---|---|
| Statistical table | No additional settings |
| Single/cross-problem box plots | Problem, normalization, jitter |
| Single/cross-problem statistic bars | Statistic, interval, normalization |
| Average-rank bars | Rank or Nemenyi comparison |
| Rank heatmap | An added named statistic |
| Win/tie/loss chart | Win/Tie/Lose comparison |
| Nemenyi critical-difference diagram | Nemenyi comparison |
| Convergence | Matching `.prcs`; problem, statistic, interval, sample grid |
| Pareto front | Multiobjective results; problem, axes, run, connecting lines |

Add multiple charts of the same kind with different settings. Single-problem charts bind an explicit selected problem. Missing dependencies are rejected instead of producing empty charts. SVG comes from the shared `buildReport → ECAnalyzer` path and is embedded directly; the browser does not recompute statistics.

### 7.3 Incomplete data

Missing optimizer/problem combinations, unequal counts, or noncontiguous run indices mark a batch as incomplete. The GUI's documented reporting path requires common contiguous runs beginning at zero across the selected rectangle. It uses the smallest common count and reports excluded extra runs; missing run zero is rejected. This is a GUI-specific constraint; the Python batch API separately documents its supported common run-index sets.

### 7.4 Downloads

SVG is the same vector chart shown in the page. PNG is rendered server-side from the same chart data. PNG settings include English/Chinese fonts and resolution multiplier. Defaults:

```text
C:/Windows/Fonts/times.ttf
C:/Windows/Fonts/simsun.ttc
```

Commercial fonts are not redistributed. Inline SVG does not require these files; invalid font paths fail PNG generation explicitly.

## 8. Local service and data boundaries

The service listens only on loopback, generates a new random token per launch, and passes it through the URL fragment rather than ordinary HTTP paths/logs. APIs validate token, Host, and Origin, with header/body limits.

Problem temporary files are removed on replacement/exit/destruction and do not determine problem/log identity. Closing the page sends a token-authenticated shutdown request; optimization stops at termination checkpoints rather than waiting for the full budget. Chart temporary directories are cleaned after response construction. Downloads accept validated chart IDs from the current report, not arbitrary paths.

Model keys remain in request memory and are excluded from disk, browser storage, logs, and responses. Automatic redirects are disabled to avoid forwarding authentication headers elsewhere. Public endpoints require HTTPS; HTTP exceptions are loopback-only.

Standard local file loading, configuration editing, and statistics do not upload content. Connection tests send a fixed message. Model construction sends the description/reference data. Recommendation sends the necessary structured problem/candidate summaries and accepts a candidate identifier, never model-authored final configuration text.

## 9. Troubleshooting

| Symptom | Action |
|---|---|
| Missing session token | Close the old/bookmarked URL and launch `evoflow.exe` again |
| Local service disconnected | Relaunch; do not continue editing a disconnected page |
| Saved file not found | Check the browser download directory when File System Access API is unavailable |
| Saved configuration cannot run | Inspect errors and perform assembly validation or `evoflow validate <config> --build` |
| No batches found | Check launch-relative log root, `.rslt` generation, shared explicit tag, and the internal configuration optimizer name rather than its filename |
| PNG download fails | Check font paths and scale `0.25–8`; SVG does not require server font files |
| Model connection fails | Check full endpoint/model/key, HTTPS or loopback HTTP; inspect authentication, rate-limit, timeout, network, server, or response-format error |

Some nominally compatible model services return different fields and are rejected by the fixed protocol.

For problem generation, distinguish type identification from generation/mechanical validation. Failures may involve the whitelist, strict JSON, size/format, TYPE agreement, loading, or compilation. Attempts are shown during retries; final failure preserves the current problem. Reference attachments must be UTF-8, at most 1 MiB, and contain no NUL. File-only failures require an added description before any model path.

For recommendation, first load a problem and pass the model connection test. Unreadable candidate directories, undecodable candidates, or excessive summaries fail locally. Model replies must be exactly the expected JSON shape, for example `{"config":"filename.cfg"}`, and match a whitelisted name byte-for-byte. Bounded retries never fall back to generating a new configuration.

## 10. Scope

Implemented: loading/inspecting/saving original problem data; form and linear-block editors; v3.1/v3.2 open/save; static/deep validation; running current in-memory drafts with independent rounds; log discovery and SVG/PNG analysis; model settings/testing; template-constrained natural-language problem construction; and selection of existing whitelisted configurations.

Not implemented or deferred: editing problem definitions and reserializing them in their original format; manual pause/cancel controls, multi-configuration batch orchestration, and live inner-iteration monitoring; friendly enum labels; non-template code generation, adaptive configuration generation, and automatic feedback loops (planned for 3.5).

Historical acceptance: manual acceptance of the analysis and block-workflow pages was deferred at their development closeout while automated contracts/tests remained in place. The user completed that acceptance on 2026-08-02.
