# Glyph

An in-process and network-accessible vector search engine implementing Hierarchical Navigable Small World (HNSW) graphs, metadata predicate filtering, write-ahead logging (WAL) for durability, and optional scalar (SQ8) or product (PQ) quantization.

## Project Overview

Glyph is a C++ vector database providing approximate nearest neighbor search. It isolates collections in namespaces and supports:

- **Distance Metrics**: Cosine, L2, Dot Product, L1, and bitwise Hamming.
- **SIMD Kernels**: AVX2 and ARM NEON hardware acceleration with automatic runtime CPU feature dispatch.
- **Metadata Filtering**: Attribute constraints evaluated during search traversals.
- **Quantization**: SQ8 coordinates compression and Product Quantization (PQ) for asymmetric distance computation.
- **Durability**: Write-Ahead Log (WAL) logging, sequential registry persistence, and periodic background snapshots.
- **Interfaces**: Simultaneous gRPC and HTTP/REST server interfaces.
- **Benchmarking**: In-process benchmark harness comparing search QPS and Recall@k.

## Installation

### Prerequisites

- CMake 3.20 or higher
- C++20 compiler
- Protobuf and gRPC development libraries

### Building the Project

```bash
cmake -S . -B build
cmake --build build
```

## Usage

### Starting the Server

```bash
./build/src/glyph_server --data-dir ./data
```

### Running the Benchmarks

```bash
./build/bench/glyph_bench --dim 128 --n-train 10000 --n-query 100 --ef-list 10,50,100 --metric L2 --quant pq --pq-m 8
```

### Running Tests

```bash
./build/tests/glyph_tests
```

### API Examples

#### Create Namespace

```bash
curl -X POST http://localhost:8080/namespaces \
  -H "Content-Type: application/json" \
  -d '{"name": "example", "config": {"dim": 4, "m": 16, "m0": 32, "ef_construction": 200, "metric": "L2", "max_elements": 1000, "quant_mode": "None", "pq_m": 8}}'
```

#### Upsert Vector

```bash
curl -X POST http://localhost:8080/namespaces/example/vectors \
  -H "Content-Type: application/json" \
  -d '{"id": 1, "vector": [1.0, 2.0, 3.0, 4.0], "metadata": {"tag": "test", "active": true}}'
```

#### Search

```bash
curl -X POST http://localhost:8080/namespaces/example/search \
  -H "Content-Type: application/json" \
  -d '{"query": [1.0, 2.0, 3.0, 4.0], "k": 10, "ef": 50, "filter": {"type": "eq", "key": "tag", "value": "test"}}'
```

## Configuration

- **CLI Options**:
  - `--data-dir <path>`: Specifies directory path for database file storage (defaults to `./data`).
- **Environment Variables**:
  - `GLYPH_DATA_DIR`: Overrides the default data directory if `--data-dir` is not specified.

## License

MIT License
