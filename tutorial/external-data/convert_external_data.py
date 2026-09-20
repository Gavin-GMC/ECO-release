"""Convert user-downloaded benchmark files into EvoFlow instance files.

This script never downloads or redistributes benchmark data. Obtain each input
from its publisher under terms that permit your intended use, then run one of
the subcommands documented by ``--help``.
"""

from __future__ import annotations

import argparse
from decimal import Decimal, InvalidOperation
import gzip
from pathlib import Path
import re
from typing import Iterable, List, Optional, Sequence, Tuple


NUMBER = re.compile(r"[-+]?(?:\d+(?:\.\d*)?|\.\d+)(?:[Ee][-+]?\d+)?")
TSP_EDGE_TYPES = {
    "EUC_2D", "EUC_3D", "MAN_2D", "MAX_2D", "CEIL_2D", "GEO", "ATT",
    "EXPLICIT",
}
TSP_MATRIX_FORMATS = {
    "FULL_MATRIX", "UPPER_ROW", "LOWER_COL", "LOWER_ROW", "UPPER_COL",
    "UPPER_DIAG_ROW", "LOWER_DIAG_COL", "LOWER_DIAG_ROW", "UPPER_DIAG_COL",
}


def _read_text(path: Path) -> str:
    if path.suffix.lower() == ".gz":
        with gzip.open(str(path), "rt", encoding="utf-8-sig") as source:
            return source.read()
    return path.read_text(encoding="utf-8-sig")


def _write_text(path: Path, text: str, overwrite: bool) -> None:
    if path.exists() and not overwrite:
        raise FileExistsError("output exists; pass --overwrite: {}".format(path))
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", encoding="utf-8", newline="\n") as target:
        target.write(text.rstrip() + "\n")


def _header(text: str, key: str) -> str:
    match = re.search(
        r"^\s*" + re.escape(key) + r"\s*:?\s*(\S+)",
        text,
        flags=re.IGNORECASE | re.MULTILINE,
    )
    if not match:
        raise ValueError("missing TSPLIB header: {}".format(key))
    return match.group(1).upper()


def convert_tsp(source: Path, output: Path, overwrite: bool) -> None:
    text = _read_text(source).replace("\r\n", "\n").replace("\r", "\n")
    problem_type = _header(text, "TYPE")
    if problem_type not in {"TSP", "ATSP"}:
        raise ValueError("TYPE must be TSP or ATSP, got {}".format(problem_type))
    dimension = int(_header(text, "DIMENSION"))
    if dimension < 2:
        raise ValueError("DIMENSION must be at least 2")
    edge_type = _header(text, "EDGE_WEIGHT_TYPE")
    if edge_type not in TSP_EDGE_TYPES:
        raise ValueError(
            "EDGE_WEIGHT_TYPE {} is outside EvoFlow's TSPLIB subset".format(edge_type)
        )
    if edge_type == "EXPLICIT":
        matrix_format = _header(text, "EDGE_WEIGHT_FORMAT")
        if matrix_format not in TSP_MATRIX_FORMATS:
            raise ValueError(
                "EDGE_WEIGHT_FORMAT {} is outside EvoFlow's TSPLIB subset".format(
                    matrix_format
                )
            )
        if "EDGE_WEIGHT_SECTION" not in text.upper():
            raise ValueError("EXPLICIT instance has no EDGE_WEIGHT_SECTION")
    elif "NODE_COORD_SECTION" not in text.upper():
        raise ValueError("coordinate instance has no NODE_COORD_SECTION")
    _write_text(output, text, overwrite)


def _numeric_tokens(path: Path) -> List[str]:
    text = _read_text(path)
    tokens = NUMBER.findall(text)
    if not tokens:
        raise ValueError("input contains no numeric data: {}".format(path))
    return tokens


def _positive_integer(token: str, label: str) -> int:
    try:
        value = Decimal(token)
    except InvalidOperation:
        raise ValueError("{} is not numeric: {}".format(label, token))
    integer = int(value)
    if value != integer or integer <= 0:
        raise ValueError("{} must be a positive integer: {}".format(label, token))
    return integer


def _rows(values: Sequence[str], size: int) -> Iterable[Sequence[str]]:
    for start in range(0, len(values), size):
        yield values[start:start + size]


def convert_qap(
    source: Path, output: Path, name: str, overwrite: bool
) -> None:
    tokens = _numeric_tokens(source)
    size = _positive_integer(tokens[0], "QAP size")
    expected = 1 + 2 * size * size
    if len(tokens) != expected:
        raise ValueError(
            "QAPLIB input needs {} numeric values, found {}".format(expected, len(tokens))
        )
    split = 1 + size * size
    flow = tokens[1:split]
    distance = tokens[split:]
    lines = [
        "NAME: {}".format(name),
        "TYPE: QAP",
        "FACILITIES: {}".format(size),
        "LOCATIONS: {}".format(size),
        "FLOW_SECTION",
    ]
    for index, row in enumerate(_rows(flow, size), 1):
        lines.append("{} {}".format(index, " ".join(row)))
    lines.append("Distance_SECTION")
    for index, row in enumerate(_rows(distance, size), 1):
        lines.append("{} {}".format(index, " ".join(row)))
    lines.append("EOF")
    _write_text(output, "\n".join(lines), overwrite)


def _parse_mkp_instance(tokens: Sequence[str], offset: int) -> Tuple[dict, int]:
    if offset + 3 > len(tokens):
        raise ValueError("truncated MKP instance header")
    items = _positive_integer(tokens[offset], "MKP item count")
    resources = _positive_integer(tokens[offset + 1], "MKP resource count")
    optimal = tokens[offset + 2]
    offset += 3
    end = offset + items
    profits = tokens[offset:end]
    offset = end
    end = offset + items * resources
    weights = tokens[offset:end]
    offset = end
    end = offset + resources
    capacities = tokens[offset:end]
    if len(profits) != items or len(weights) != items * resources or len(capacities) != resources:
        raise ValueError("truncated MKP instance body")
    return {
        "items": items,
        "resources": resources,
        "optimal": optimal,
        "profits": profits,
        "weights": weights,
        "capacities": capacities,
    }, end


def convert_mkp(
    source: Path,
    output: Path,
    name: str,
    instance_index: int,
    counted: bool,
    overwrite: bool,
) -> None:
    tokens = _numeric_tokens(source)
    offset = 0
    count = 1
    if counted:
        count = _positive_integer(tokens[0], "MKP instance count")
        offset = 1
    if instance_index < 1 or instance_index > count:
        raise ValueError("--instance-index must be between 1 and {}".format(count))
    selected = None
    for index in range(1, count + 1):
        instance, offset = _parse_mkp_instance(tokens, offset)
        if index == instance_index:
            selected = instance
    if offset != len(tokens):
        raise ValueError("unexpected trailing numeric data after MKP instance set")
    assert selected is not None
    items = selected["items"]
    resources = selected["resources"]
    lines = [
        "NAME: {}".format(name),
        "TYPE: MKP",
        "ITEMS: {}".format(items),
        "BACKPACKS: 1",
        "RESOURCES: {}".format(resources),
        "OPTIMAL: {}".format(selected["optimal"]),
        "PROFIT_SECTION",
    ]
    for item, profit in enumerate(selected["profits"], 1):
        lines.append("{} {}".format(item, profit))
    lines.append("RESOURCE_SECTION")
    weights = selected["weights"]
    for item in range(items):
        item_weights = [weights[resource * items + item] for resource in range(resources)]
        lines.append("{} {}".format(item + 1, " ".join(item_weights)))
    lines.append("CAPACITY_SECTION")
    lines.append("1 {}".format(" ".join(selected["capacities"])))
    lines.append("EOF")
    _write_text(output, "\n".join(lines), overwrite)


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Convert lawfully obtained external data into EvoFlow formats"
    )
    subparsers = parser.add_subparsers(dest="command", required=True)

    tsp = subparsers.add_parser("tsp", help="validate/decompress a TSPLIB .tsp[.gz]")
    tsp.add_argument("source", type=Path)
    tsp.add_argument("output", type=Path)
    tsp.add_argument("--overwrite", action="store_true")

    qap = subparsers.add_parser("qap", help="convert one raw QAPLIB .dat file")
    qap.add_argument("source", type=Path)
    qap.add_argument("output", type=Path)
    qap.add_argument("--name")
    qap.add_argument("--overwrite", action="store_true")

    mkp = subparsers.add_parser("mkp", help="convert standard MKP numeric data")
    mkp.add_argument("source", type=Path)
    mkp.add_argument("output", type=Path)
    mkp.add_argument("--name")
    mkp.add_argument("--instance-index", type=int, default=1)
    mkp.add_argument(
        "--counted",
        action="store_true",
        help="input starts with the number of instances (OR-Library style)",
    )
    mkp.add_argument("--overwrite", action="store_true")
    return parser


def main(argv: Optional[Sequence[str]] = None) -> int:
    parser = build_parser()
    options = parser.parse_args(argv)
    try:
        if options.command == "tsp":
            convert_tsp(options.source, options.output, options.overwrite)
        elif options.command == "qap":
            convert_qap(
                options.source,
                options.output,
                options.name or options.source.stem,
                options.overwrite,
            )
        else:
            default_name = options.source.stem
            if options.counted:
                default_name += "-{}".format(options.instance_index)
            convert_mkp(
                options.source,
                options.output,
                options.name or default_name,
                options.instance_index,
                options.counted,
                options.overwrite,
            )
    except (OSError, ValueError) as error:
        parser.exit(1, "error: {}\n".format(error))
    print("wrote {}".format(options.output))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
