"""Create a tiny persisted-result batch and analyze it natively."""

from pathlib import Path
from tempfile import TemporaryDirectory

from evoflow import Statistic, analyze, discover_batches


def write_result(root: Path, optimizer: str, run: int, value: float) -> None:
    directory = root / optimizer / "demo"
    directory.mkdir(parents=True, exist_ok=True)
    path = directory / f"{optimizer}_demo(python_example)_{run}.rslt"
    path.write_text(
        "\n".join(
            (
                "<ver= 1.1 >",
                "Exe_Time:\t0\tFES:\t10",
                "----------------------------------------",
                "Object_Number:\t1\tSolution_Number:\t1",
                f"1:\tfitness\t{value}",
                "v1-\tx:\t0",
                "EOS",
                "",
            )
        ),
        encoding="utf-8",
    )


with TemporaryDirectory(prefix="evoflow-analysis-example-") as temporary:
    logs = Path(temporary) / "logs"
    for optimizer, offset in (("A", 0.0), ("B", 10.0)):
        for run in range(2):
            write_result(logs, optimizer, run, offset + run + 1.0)

    batches = discover_batches(logs)
    result = analyze(
        logs,
        tag="python_example",
        statistics=(Statistic.MEAN, Statistic.MEDIAN),
        comparisons=(),
        charts=(),
    )
    print("batches=", len(batches), "rows=", result.table.rows)
    for row, cells in zip(result.table.rows, result.table.cells):
        print(row, cells)
