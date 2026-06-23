# Parquet Viewer

A Linux-first desktop Parquet viewer written in C++.

The app is designed for Fedora and Ubuntu users who want to open Parquet files, inspect them as tables, keep multiple files open, and filter rows without writing SQL.

## Current features

- Native Qt 6 desktop application
- VS Code-style file explorer for `.parquet` / `.parq` files
- Multiple open files through tabs
- Paged table display for large files
- Global search across all rows and columns
- Optional all-rows display by typing `-1` into the row count control
- Quick filters such as `target = 1`, `score > 10`, and `active = true`
- Visual filter builder:
  - equals / does not equal
  - greater than / less than
  - at least / at most
  - contains / starts with / ends with
  - is true / is false
  - is empty / is not empty
- DuckDB-backed Parquet queries under the hood

Users never need to type a `WHERE` clause. The UI builds safe filter expressions internally from dropdowns and input fields.

## Build

### Requirements

- CMake 3.24 or newer
- A C++20 compiler
- Qt 6 Widgets development files
- DuckDB CLI available as `duckdb`

The app uses Qt for the desktop UI and the DuckDB CLI for Parquet scanning, filtering, and paging.

### DuckDB

CMake looks for the `duckdb` executable in `PATH`.

If DuckDB is installed somewhere custom:

```bash
cmake -S . -B build -DDUCKDB_EXECUTABLE=/path/to/duckdb
```

The app embeds that executable path at build time and runs commands like:

```bash
duckdb -no-init -json -c "SELECT ..."
```

### Fedora

Install the Qt/C++ toolchain with your normal Fedora package manager. Install DuckDB so `duckdb --version` works in a terminal.

### Ubuntu

Install the Qt/C++ toolchain with your normal Ubuntu package manager. Install DuckDB so `duckdb --version` works in a terminal.

### Commands

```bash
cmake -S . -B build
cmake --build build
./build/parquet-viewer
```

Open files from the toolbar, by double-clicking `.parquet` files in the explorer, or by passing file paths:

```bash
./build/parquet-viewer data/example.parquet
```

## How filtering works

The UI exposes simple filter rows:

```text
[column] [operator] [value]
```

For example:

```text
status equals 1
active is true
country contains text de
```

Internally, those choices are converted to DuckDB expressions and applied to the open Parquet file.

The find box also accepts simple quick filters when the left side is a real column name:

```text
target = 1
target=1
score > 10
active = true
name = Alice
```

If the text is not recognized as a quick filter, it is treated as a normal search value across all columns.

## Roadmap

- Column-specific search from header menus
- Schema/details side panel
- Saved filter presets
- Export filtered rows
- Optional persistent cache/index metadata for repeated searches
- Sorting support
