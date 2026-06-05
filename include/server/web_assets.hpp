#pragma once

inline const char* INDEX_HTML = R"rawhtml(<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>Glyph Console</title>
<link rel="preconnect" href="https://fonts.googleapis.com">
<link rel="preconnect" href="https://fonts.gstatic.com" crossorigin>
<link href="https://fonts.googleapis.com/css2?family=Outfit:wght@300;400;500;600;700&family=JetBrains+Mono:wght@400;500&display=swap" rel="stylesheet">
<style>
* {
    box-sizing: border-box;
    margin: 0;
    padding: 0;
}
body {
    background-color: #0b0f19;
    color: #e2e8f0;
    font-family: 'Outfit', sans-serif;
    line-height: 1.5;
    padding-bottom: 40px;
}
header {
    background-color: #111827;
    border-bottom: 1px solid #1f2937;
    padding: 15px 30px;
    display: flex;
    justify-content: space-between;
    align-items: center;
    position: sticky;
    top: 0;
    z-index: 100;
}
.logo-container {
    display: flex;
    align-items: center;
    gap: 12px;
}
.logo-icon {
    width: 32px;
    height: 32px;
    background: linear-gradient(135deg, #6366f1, #3b82f6);
    border-radius: 8px;
    display: flex;
    align-items: center;
    justify-content: center;
    font-weight: 700;
    font-size: 18px;
    color: #ffffff;
}
h1 {
    font-size: 20px;
    font-weight: 600;
    letter-spacing: -0.5px;
    background: linear-gradient(to right, #f8fafc, #94a3b8);
    -webkit-background-clip: text;
    -webkit-text-fill-color: transparent;
}
.status-badge {
    background-color: #065f46;
    color: #34d399;
    padding: 4px 12px;
    border-radius: 9999px;
    font-size: 12px;
    font-weight: 500;
    display: flex;
    align-items: center;
    gap: 6px;
}
.status-indicator {
    width: 8px;
    height: 8px;
    background-color: #34d399;
    border-radius: 50%;
    display: inline-block;
}
.nav-tabs {
    display: flex;
    gap: 8px;
    padding: 20px 30px;
    border-bottom: 1px solid #1f2937;
    background-color: #0f172a;
}
.nav-tab {
    background: none;
    border: none;
    color: #94a3b8;
    padding: 8px 16px;
    font-size: 14px;
    font-weight: 500;
    cursor: pointer;
    border-radius: 6px;
    transition: all 0.2s;
}
.nav-tab:hover {
    color: #f8fafc;
    background-color: #1e293b;
}
.nav-tab.active {
    color: #ffffff;
    background-color: #3b82f6;
}
.container {
    max-width: 1200px;
    margin: 0 auto;
    padding: 20px 30px;
}
.tab-content {
    display: none;
}
.tab-content.active {
    display: block;
}
.stats-grid {
    display: grid;
    grid-template-columns: repeat(auto-fit, minmax(220px, 1fr));
    gap: 20px;
    margin-bottom: 30px;
}
.card {
    background-color: #151c2c;
    border: 1px solid #222f47;
    border-radius: 12px;
    padding: 20px;
    transition: transform 0.2s, box-shadow 0.2s;
}
.card:hover {
    transform: translateY(-2px);
    box-shadow: 0 10px 20px rgba(0,0,0,0.3);
}
.stat-title {
    color: #94a3b8;
    font-size: 12px;
    font-weight: 500;
    text-transform: uppercase;
    letter-spacing: 0.5px;
    margin-bottom: 8px;
}
.stat-value {
    font-size: 28px;
    font-weight: 700;
    color: #ffffff;
}
.section-title {
    font-size: 18px;
    font-weight: 600;
    margin-bottom: 20px;
    display: flex;
    justify-content: space-between;
    align-items: center;
}
.btn {
    padding: 8px 16px;
    font-size: 14px;
    font-weight: 500;
    border-radius: 6px;
    cursor: pointer;
    border: none;
    transition: all 0.2s;
    display: inline-flex;
    align-items: center;
    gap: 8px;
}
.btn-primary {
    background-color: #3b82f6;
    color: #ffffff;
}
.btn-primary:hover {
    background-color: #2563eb;
}
.btn-secondary {
    background-color: #1f2937;
    color: #e2e8f0;
    border: 1px solid #374151;
}
.btn-secondary:hover {
    background-color: #374151;
}
.btn-danger {
    background-color: #e11d48;
    color: #ffffff;
}
.btn-danger:hover {
    background-color: #be123c;
}
.btn-success {
    background-color: #10b981;
    color: #ffffff;
}
.btn-success:hover {
    background-color: #059669;
}
.btn-sm {
    padding: 4px 8px;
    font-size: 12px;
}
table {
    width: 100%;
    border-collapse: collapse;
    margin-bottom: 30px;
    background-color: #151c2c;
    border-radius: 12px;
    overflow: hidden;
    border: 1px solid #222f47;
}
th, td {
    padding: 14px 20px;
    text-align: left;
    border-bottom: 1px solid #222f47;
}
th {
    background-color: #1e293b;
    color: #94a3b8;
    font-size: 13px;
    font-weight: 600;
    text-transform: uppercase;
    letter-spacing: 0.5px;
}
tr:hover td {
    background-color: #1e293b;
}
.badge {
    display: inline-block;
    padding: 2px 8px;
    border-radius: 4px;
    font-size: 11px;
    font-weight: 600;
    text-transform: uppercase;
}
.badge-blue {
    background-color: rgba(59, 130, 246, 0.15);
    color: #60a5fa;
}
.badge-green {
    background-color: rgba(16, 185, 129, 0.15);
    color: #34d399;
}
.badge-purple {
    background-color: rgba(139, 92, 246, 0.15);
    color: #a78bfa;
}
.modal {
    display: none;
    position: fixed;
    top: 0;
    left: 0;
    width: 100%;
    height: 100%;
    background-color: rgba(0,0,0,0.6);
    z-index: 200;
    align-items: center;
    justify-content: center;
}
.modal.active {
    display: flex;
}
.modal-content {
    background-color: #151c2c;
    border: 1px solid #222f47;
    border-radius: 16px;
    width: 100%;
    max-width: 500px;
    padding: 30px;
    box-shadow: 0 20px 25px -5px rgba(0,0,0,0.5);
}
.modal-title {
    font-size: 18px;
    font-weight: 600;
    margin-bottom: 20px;
    color: #ffffff;
}
.form-group {
    margin-bottom: 16px;
}
label {
    display: block;
    font-size: 13px;
    font-weight: 500;
    color: #94a3b8;
    margin-bottom: 6px;
}
input, select, textarea {
    width: 100%;
    background-color: #1d273a;
    border: 1px solid #2e3f5b;
    border-radius: 8px;
    padding: 10px 14px;
    color: #f1f5f9;
    font-family: inherit;
    font-size: 14px;
    outline: none;
    transition: border-color 0.2s;
}
input:focus, select:focus, textarea:focus {
    border-color: #3b82f6;
}
.form-actions {
    display: flex;
    justify-content: flex-end;
    gap: 12px;
    margin-top: 24px;
}
.playground-layout {
    display: grid;
    grid-template-columns: 350px 1fr;
    gap: 30px;
}
.panel-title {
    font-size: 15px;
    font-weight: 600;
    margin-bottom: 16px;
    color: #ffffff;
    border-bottom: 1px solid #222f47;
    padding-bottom: 8px;
}
.filter-row {
    display: flex;
    gap: 8px;
    margin-bottom: 8px;
}
.result-card {
    background-color: #151c2c;
    border: 1px solid #222f47;
    border-radius: 8px;
    padding: 16px;
    margin-bottom: 12px;
}
.result-header {
    display: flex;
    justify-content: space-between;
    margin-bottom: 8px;
}
.result-id {
    font-weight: 600;
    color: #60a5fa;
}
.result-dist {
    font-size: 13px;
    color: #94a3b8;
}
.result-meta {
    background-color: #0b0f19;
    border-radius: 6px;
    padding: 10px;
    font-family: 'JetBrains Mono', monospace;
    font-size: 12px;
    color: #cbd5e1;
    overflow-x: auto;
}
.api-endpoint {
    background-color: #151c2c;
    border: 1px solid #222f47;
    border-radius: 8px;
    margin-bottom: 16px;
    overflow: hidden;
}
.endpoint-header {
    padding: 12px 20px;
    display: flex;
    align-items: center;
    gap: 12px;
    cursor: pointer;
    background-color: #1e293b;
}
.method {
    font-size: 11px;
    font-weight: 700;
    padding: 2px 8px;
    border-radius: 4px;
    width: 60px;
    text-align: center;
}
.method-get {
    background-color: rgba(16, 185, 129, 0.15);
    color: #34d399;
}
.method-post {
    background-color: rgba(59, 130, 246, 0.15);
    color: #60a5fa;
}
.method-delete {
    background-color: rgba(225, 29, 72, 0.15);
    color: #f43f5e;
}
.path {
    font-family: 'JetBrains Mono', monospace;
    font-size: 14px;
    font-weight: 500;
    color: #e2e8f0;
}
.endpoint-body {
    padding: 20px;
    border-top: 1px solid #222f47;
    display: none;
    grid-template-columns: 1fr 1fr;
    gap: 20px;
}
.endpoint-body.active {
    display: grid;
}
.response-box {
    background-color: #0b0f19;
    border: 1px solid #222f47;
    border-radius: 8px;
    padding: 15px;
    font-family: 'JetBrains Mono', monospace;
    font-size: 12px;
    color: #34d399;
    height: 300px;
    overflow-y: auto;
    white-space: pre-wrap;
}
</style>
</head>
<body>
<header>
    <div class="logo-container">
        <div class="logo-icon">G</div>
        <h1>Glyph Database Console</h1>
    </div>
    <div class="status-badge">
        <span class="status-indicator"></span>
        <span id="uptime-display">System Active</span>
    </div>
</header>

<div class="nav-tabs">
    <button class="nav-tab active" onclick="switchTab('dashboard')">Dashboard</button>
    <button class="nav-tab" onclick="switchTab('playground')">Query Playground</button>
    <button class="nav-tab" onclick="switchTab('api')">API Explorer</button>
</div>

<div class="container">
    <div id="tab-dashboard" class="tab-content active">
        <div class="stats-grid">
            <div class="card">
                <div class="stat-title">Namespaces</div>
                <div id="stat-namespaces" class="stat-value">0</div>
            </div>
            <div class="card">
                <div class="stat-title">Total Vectors</div>
                <div id="stat-vectors" class="stat-value">0</div>
            </div>
            <div class="card">
                <div class="stat-title">Total Queries</div>
                <div id="stat-queries" class="stat-value">0</div>
            </div>
            <div class="card">
                <div class="stat-title">Total Errors</div>
                <div id="stat-errors" class="stat-value">0</div>
            </div>
        </div>

        <div class="section-title">
            <span>Namespaces</span>
            <button class="btn btn-primary btn-sm" onclick="openCreateModal()">Create Namespace</button>
        </div>

        <table id="namespaces-table">
            <thead>
                <tr>
                    <th>Name</th>
                    <th>Size</th>
                    <th>Dimension</th>
                    <th>Metric</th>
                    <th>Quant Mode</th>
                    <th>Settings (M/M0/ef)</th>
                    <th>Actions</th>
                </tr>
            </thead>
            <tbody id="namespaces-list">
            </tbody>
        </table>
    </div>

    <div id="tab-playground" class="tab-content">
        <div class="playground-layout">
            <div class="card">
                <div class="panel-title">Query Parameters</div>
                <div class="form-group">
                    <label for="play-namespace">Namespace</label>
                    <select id="play-namespace" onchange="updatePlaygroundDim()"></select>
                </div>
                <div class="form-group">
                    <label for="play-vector">Query Vector (comma-separated)</label>
                    <textarea id="play-vector" rows="4" placeholder="0.5, -0.2, 0.1..."></textarea>
                    <button class="btn btn-secondary btn-sm" style="margin-top: 8px; width: 100%; justify-content: center;" onclick="generateRandomVector()">Generate Random Vector</button>
                </div>
                <div class="form-group">
                    <label for="play-k">Top K</label>
                    <input type="number" id="play-k" value="10" min="1">
                </div>
                <div class="form-group">
                    <label for="play-ef">ef Search</label>
                    <input type="number" id="play-ef" value="50" min="1">
                </div>
                <div class="form-group">
                    <label>Metadata Filter</label>
                    <div id="filters-container"></div>
                    <button class="btn btn-secondary btn-sm" style="width: 100%; justify-content: center; margin-top: 8px;" onclick="addFilterRow()">Add Filter Predicate</button>
                </div>
                <button class="btn btn-primary" style="width: 100%; justify-content: center; margin-top: 10px;" onclick="runPlaygroundQuery()">Execute Search</button>
            </div>
            <div>
                <div class="section-title">Query Results</div>
                <div id="query-results-list">
                    <div class="card" style="text-align: center; color: #94a3b8;">
                        Select a namespace and execute a search query to inspect results.
                    </div>
                </div>
            </div>
        </div>
    </div>

    <div id="tab-api" class="tab-content">
        <div class="section-title">Interactive API Documentation</div>

        <div class="api-endpoint">
            <div class="endpoint-header" onclick="toggleEndpoint('api-get-health')">
                <span class="method method-get">GET</span>
                <span class="path">/health</span>
            </div>
            <div id="api-get-health" class="endpoint-body">
                <div>
                    <div class="form-group">
                        <label>Endpoint Parameters</label>
                        <p style="font-size: 13px; color: #94a3b8;">No query or path parameters required.</p>
                    </div>
                    <button class="btn btn-primary" onclick="runClient('GET', '/health', null, 'resp-health')">Send Request</button>
                </div>
                <div>
                    <label>Response</label>
                    <div id="resp-health" class="response-box">Click Send Request to run.</div>
                </div>
            </div>
        </div>

        <div class="api-endpoint">
            <div class="endpoint-header" onclick="toggleEndpoint('api-get-stats')">
                <span class="method method-get">GET</span>
                <span class="path">/stats</span>
            </div>
            <div id="api-get-stats" class="endpoint-body">
                <div>
                    <div class="form-group">
                        <label>Endpoint Parameters</label>
                        <p style="font-size: 13px; color: #94a3b8;">No query or path parameters required.</p>
                    </div>
                    <button class="btn btn-primary" onclick="runClient('GET', '/stats', null, 'resp-stats')">Send Request</button>
                </div>
                <div>
                    <label>Response</label>
                    <div id="resp-stats" class="response-box">Click Send Request to run.</div>
                </div>
            </div>
        </div>

        <div class="api-endpoint">
            <div class="endpoint-header" onclick="toggleEndpoint('api-get-namespaces')">
                <span class="method method-get">GET</span>
                <span class="path">/namespaces</span>
            </div>
            <div id="api-get-namespaces" class="endpoint-body">
                <div>
                    <div class="form-group">
                        <label>Endpoint Parameters</label>
                        <p style="font-size: 13px; color: #94a3b8;">No query or path parameters required.</p>
                    </div>
                    <button class="btn btn-primary" onclick="runClient('GET', '/namespaces', null, 'resp-namespaces')">Send Request</button>
                </div>
                <div>
                    <label>Response</label>
                    <div id="resp-namespaces" class="response-box">Click Send Request to run.</div>
                </div>
            </div>
        </div>

        <div class="api-endpoint">
            <div class="endpoint-header" onclick="toggleEndpoint('api-post-namespaces')">
                <span class="method method-post">POST</span>
                <span class="path">/namespaces</span>
            </div>
            <div id="api-post-namespaces" class="endpoint-body">
                <div>
                    <div class="form-group">
                        <label for="req-post-namespaces-body">Request Body (JSON)</label>
                        <textarea id="req-post-namespaces-body" rows="10" style="font-family: 'JetBrains Mono', monospace; font-size: 12px;">{
  "name": "example",
  "config": {
    "dim": 4,
    "m": 16,
    "m0": 32,
    "ef_construction": 200,
    "metric": "L2",
    "max_elements": 0,
    "quant_mode": "None",
    "pq_m": 8
  }
}</textarea>
                    </div>
                    <button class="btn btn-primary" onclick="runClient('POST', '/namespaces', 'req-post-namespaces-body', 'resp-post-namespaces')">Send Request</button>
                </div>
                <div>
                    <label>Response</label>
                    <div id="resp-post-namespaces" class="response-box">Click Send Request to run.</div>
                </div>
            </div>
        </div>

        <div class="api-endpoint">
            <div class="endpoint-header" onclick="toggleEndpoint('api-post-vectors')">
                <span class="method method-post">POST</span>
                <span class="path">/namespaces/{name}/vectors</span>
            </div>
            <div id="api-post-vectors" class="endpoint-body">
                <div>
                    <div class="form-group">
                        <label for="path-post-vectors-name">Namespace Name</label>
                        <input type="text" id="path-post-vectors-name" value="example">
                    </div>
                    <div class="form-group">
                        <label for="req-post-vectors-body">Request Body (JSON)</label>
                        <textarea id="req-post-vectors-body" rows="10" style="font-family: 'JetBrains Mono', monospace; font-size: 12px;">{
  "id": 100,
  "vector": [1.0, 0.0, 0.0, 0.0],
  "metadata": {
    "category": "test",
    "active": true
  }
}</textarea>
                    </div>
                    <button class="btn btn-primary" onclick="runClientWithNamespace('POST', '/namespaces/', '/vectors', 'path-post-vectors-name', 'req-post-vectors-body', 'resp-post-vectors')">Send Request</button>
                </div>
                <div>
                    <label>Response</label>
                    <div id="resp-post-vectors" class="response-box">Click Send Request to run.</div>
                </div>
            </div>
        </div>

        <div class="api-endpoint">
            <div class="endpoint-header" onclick="toggleEndpoint('api-post-vectors-batch')">
                <span class="method method-post">POST</span>
                <span class="path">/namespaces/{name}/vectors/batch</span>
            </div>
            <div id="api-post-vectors-batch" class="endpoint-body">
                <div>
                    <div class="form-group">
                        <label for="path-post-vectors-batch-name">Namespace Name</label>
                        <input type="text" id="path-post-vectors-batch-name" value="example">
                    </div>
                    <div class="form-group">
                        <label for="req-post-vectors-batch-body">Request Body (JSON)</label>
                        <textarea id="req-post-vectors-batch-body" rows="10" style="font-family: 'JetBrains Mono', monospace; font-size: 12px;">{
  "entries": [
    {
      "id": 101,
      "vector": [0.0, 1.0, 0.0, 0.0],
      "metadata": {
        "category": "test",
        "active": true
      }
    },
    {
      "id": 102,
      "vector": [0.0, 0.0, 1.0, 0.0],
      "metadata": {
        "category": "other",
        "active": false
      }
    }
  ]
}</textarea>
                    </div>
                    <button class="btn btn-primary" onclick="runClientWithNamespace('POST', '/namespaces/', '/vectors/batch', 'path-post-vectors-batch-name', 'req-post-vectors-batch-body', 'resp-post-vectors-batch')">Send Request</button>
                </div>
                <div>
                    <label>Response</label>
                    <div id="resp-post-vectors-batch" class="response-box">Click Send Request to run.</div>
                </div>
            </div>
        </div>

        <div class="api-endpoint">
            <div class="endpoint-header" onclick="toggleEndpoint('api-post-search')">
                <span class="method method-post">POST</span>
                <span class="path">/namespaces/{name}/search</span>
            </div>
            <div id="api-post-search" class="endpoint-body">
                <div>
                    <div class="form-group">
                        <label for="path-post-search-name">Namespace Name</label>
                        <input type="text" id="path-post-search-name" value="example">
                    </div>
                    <div class="form-group">
                        <label for="req-post-search-body">Request Body (JSON)</label>
                        <textarea id="req-post-search-body" rows="10" style="font-family: 'JetBrains Mono', monospace; font-size: 12px;">{
  "query": [1.0, 0.0, 0.0, 0.0],
  "k": 5,
  "ef": 50,
  "filter": {
    "type": "eq",
    "key": "category",
    "value": "test"
  }
}</textarea>
                    </div>
                    <button class="btn btn-primary" onclick="runClientWithNamespace('POST', '/namespaces/', '/search', 'path-post-search-name', 'req-post-search-body', 'resp-post-search')">Send Request</button>
                </div>
                <div>
                    <label>Response</label>
                    <div id="resp-post-search" class="response-box">Click Send Request to run.</div>
                </div>
            </div>
        </div>

        <div class="api-endpoint">
            <div class="endpoint-header" onclick="toggleEndpoint('api-post-train')">
                <span class="method method-post">POST</span>
                <span class="path">/namespaces/{name}/train</span>
            </div>
            <div id="api-post-train" class="endpoint-body">
                <div>
                    <div class="form-group">
                        <label for="path-post-train-name">Namespace Name</label>
                        <input type="text" id="path-post-train-name" value="example">
                    </div>
                    <button class="btn btn-primary" onclick="runClientWithNamespace('POST', '/namespaces/', '/train', 'path-post-train-name', null, 'resp-post-train')">Send Request</button>
                </div>
                <div>
                    <label>Response</label>
                    <div id="resp-post-train" class="response-box">Click Send Request to run.</div>
                </div>
            </div>
        </div>

        <div class="api-endpoint">
            <div class="endpoint-header" onclick="toggleEndpoint('api-delete-namespaces')">
                <span class="method method-delete">DELETE</span>
                <span class="path">/namespaces/{name}</span>
            </div>
            <div id="api-delete-namespaces" class="endpoint-body">
                <div>
                    <div class="form-group">
                        <label for="path-delete-namespaces-name">Namespace Name</label>
                        <input type="text" id="path-delete-namespaces-name" value="example">
                    </div>
                    <button class="btn btn-primary" onclick="runClientWithNamespace('DELETE', '/namespaces/', '', 'path-delete-namespaces-name', null, 'resp-delete-namespaces')">Send Request</button>
                </div>
                <div>
                    <label>Response</label>
                    <div id="resp-delete-namespaces" class="response-box">Click Send Request to run.</div>
                </div>
            </div>
        </div>
    </div>
</div>

<div id="create-modal" class="modal">
    <div class="modal-content">
        <div class="modal-title">Create Namespace</div>
        <div class="form-group">
            <label for="create-name">Namespace Name</label>
            <input type="text" id="create-name" placeholder="example_namespace">
        </div>
        <div class="form-group">
            <label for="create-dim">Dimension</label>
            <input type="number" id="create-dim" value="128" min="1">
        </div>
        <div class="form-group">
            <label for="create-metric">Distance Metric</label>
            <select id="create-metric">
                <option value="L2">L2 (Euclidean)</option>
                <option value="Cosine">Cosine</option>
                <option value="DotProduct">DotProduct</option>
                <option value="L1">L1 (Manhattan)</option>
                <option value="Hamming">Hamming</option>
            </select>
        </div>
        <div class="form-group">
            <label for="create-m">M (HNSW links per node)</label>
            <input type="number" id="create-m" value="16" min="2">
        </div>
        <div class="form-group">
            <label for="create-m0">M0 (HNSW base layer links)</label>
            <input type="number" id="create-m0" value="32" min="2">
        </div>
        <div class="form-group">
            <label for="create-ef">ef Construction</label>
            <input type="number" id="create-ef" value="200" min="1">
        </div>
        <div class="form-group">
            <label for="create-quant">Quantization Mode</label>
            <select id="create-quant" onchange="toggleCreatePqField()">
                <option value="None">None</option>
                <option value="SQ8">SQ8</option>
                <option value="PQ">PQ</option>
            </select>
        </div>
        <div class="form-group" id="create-pq-field" style="display: none;">
            <label for="create-pq-m">PQ Subspaces (pq_m)</label>
            <input type="number" id="create-pq-m" value="8" min="1">
        </div>
        <div class="form-actions">
            <button class="btn btn-secondary" onclick="closeCreateModal()">Cancel</button>
            <button class="btn btn-primary" onclick="submitCreateNamespace()">Create</button>
        </div>
    </div>
</div>

<script>
let namespaces = {};

function switchTab(tabId) {
    document.querySelectorAll('.nav-tab').forEach(btn => btn.classList.remove('active'));
    document.querySelectorAll('.tab-content').forEach(content => content.classList.remove('active'));

    const clickedTab = Array.from(document.querySelectorAll('.nav-tab')).find(btn => btn.textContent.toLowerCase().includes(tabId));
    if (clickedTab) clickedTab.classList.add('active');

    const activeContent = document.getElementById('tab-' + tabId);
    if (activeContent) activeContent.classList.add('active');

    if (tabId === 'dashboard') {
        loadDashboardData();
    } else if (tabId === 'playground') {
        refreshPlaygroundNamespaces();
    }
}

function toggleEndpoint(id) {
    const el = document.getElementById(id);
    if (el) {
        el.classList.toggle('active');
    }
}

function openCreateModal() {
    document.getElementById('create-modal').classList.add('active');
}

function closeCreateModal() {
    document.getElementById('create-modal').classList.remove('active');
}

function toggleCreatePqField() {
    const mode = document.getElementById('create-quant').value;
    const field = document.getElementById('create-pq-field');
    if (mode === 'PQ') {
        field.style.display = 'block';
    } else {
        field.style.display = 'none';
    }
}

async function apiCall(method, path, body = null) {
    const options = { method };
    if (body) {
        options.headers = { 'Content-Type': 'application/json' };
        options.body = typeof body === 'string' ? body : JSON.stringify(body);
    }
    try {
        const res = await fetch(path, options);
        if (!res.ok) {
            const errText = await res.text();
            throw new Error(errText || res.statusText);
        }
        return await res.json();
    } catch (err) {
        console.error(err);
        throw err;
    }
}

async function runClient(method, path, bodyFieldId, respFieldId) {
    const el = document.getElementById(respFieldId);
    el.textContent = 'Loading...';
    el.style.color = '#cbd5e1';
    let body = null;
    if (bodyFieldId) {
        body = document.getElementById(bodyFieldId).value;
    }
    const t0 = performance.now();
    try {
        const data = await apiCall(method, path, body);
        const t1 = performance.now();
        el.style.color = '#34d399';
        el.textContent = 'Status: 200 OK (' + (t1 - t0).toFixed(1) + 'ms)\n\n' + JSON.stringify(data, null, 2);
    } catch (err) {
        const t1 = performance.now();
        el.style.color = '#f43f5e';
        el.textContent = 'Error (' + (t1 - t0).toFixed(1) + 'ms):\n\n' + err.message;
    }
}

async function runClientWithNamespace(method, prefix, suffix, nameFieldId, bodyFieldId, respFieldId) {
    const nsName = document.getElementById(nameFieldId).value;
    const path = prefix + encodeURIComponent(nsName) + suffix;
    await runClient(method, path, bodyFieldId, respFieldId);
}

async function loadDashboardData() {
    try {
        const health = await apiCall('GET', '/health');
        document.getElementById('uptime-display').textContent = 'System Active (' + health.uptime_s + 's uptime)';
    } catch (err) {
        document.getElementById('uptime-display').textContent = 'Connection Error';
    }

    try {
        const stats = await apiCall('GET', '/stats');
        document.getElementById('stat-namespaces').textContent = stats.namespace_count;
        document.getElementById('stat-vectors').textContent = stats.total_vectors;
        document.getElementById('stat-queries').textContent = stats.searches;
        document.getElementById('stat-errors').textContent = stats.errors;
    } catch (err) {
        console.error(err);
    }

    try {
        const nsList = await apiCall('GET', '/namespaces');
        const listEl = document.getElementById('namespaces-list');
        listEl.innerHTML = '';
        namespaces = {};

        for (const name of nsList) {
            try {
                const info = await apiCall('GET', '/namespaces/' + encodeURIComponent(name));
                namespaces[name] = info;

                const tr = document.createElement('tr');
                tr.innerHTML = '<td><strong>' + name + '</strong></td>' +
                             '<td>' + info.size.toLocaleString() + '</td>' +
                             '<td>' + info.config.dim + '</td>' +
                             '<td><span class="badge badge-blue">' + info.config.metric + '</span></td>' +
                             '<td><span class="badge badge-purple">' + info.config.quant_mode + '</span></td>' +
                             '<td>M=' + info.config.m + ', M0=' + info.config.m0 + ', ef=' + info.config.ef_construction + '</td>' +
                             '<td>' +
                             '<button class="btn btn-secondary btn-sm" onclick="trainNamespace(\'' + name + '\')">Train</button> ' +
                             '<button class="btn btn-danger btn-sm" onclick="dropNamespace(\'' + name + '\')">Drop</button>' +
                             '</td>';
                listEl.appendChild(tr);
            } catch (err) {
                console.error(err);
            }
        }
    } catch (err) {
        console.error(err);
    }
}

async function submitCreateNamespace() {
    const name = document.getElementById('create-name').value;
    const dim = parseInt(document.getElementById('create-dim').value);
    const metric = document.getElementById('create-metric').value;
    const m = parseInt(document.getElementById('create-m').value);
    const m0 = parseInt(document.getElementById('create-m0').value);
    const ef_construction = parseInt(document.getElementById('create-ef').value);
    const quant_mode = document.getElementById('create-quant').value;
    const pq_m = parseInt(document.getElementById('create-pq-m').value);

    const payload = {
        name,
        config: {
            dim,
            m,
            m0,
            ef_construction,
            metric,
            max_elements: 0,
            quant_mode,
            pq_m
        }
    };

    try {
        await apiCall('POST', '/namespaces', payload);
        closeCreateModal();
        loadDashboardData();
    } catch (err) {
        alert('Failed to create namespace: ' + err.message);
    }
}

async function dropNamespace(name) {
    if (!confirm('Are you sure you want to drop namespace "' + name + '"?')) return;
    try {
        await apiCall('DELETE', '/namespaces/' + encodeURIComponent(name));
        loadDashboardData();
    } catch (err) {
        alert('Failed to drop namespace: ' + err.message);
    }
}

async function trainNamespace(name) {
    try {
        await apiCall('POST', '/namespaces/' + encodeURIComponent(name) + '/train');
        alert('Quantization training completed successfully.');
        loadDashboardData();
    } catch (err) {
        alert('Training failed: ' + err.message);
    }
}

function refreshPlaygroundNamespaces() {
    const select = document.getElementById('play-namespace');
    select.innerHTML = '';
    Object.keys(namespaces).forEach(name => {
        const opt = document.createElement('option');
        opt.value = name;
        opt.textContent = name;
        select.appendChild(opt);
    });
    updatePlaygroundDim();
}

function updatePlaygroundDim() {
    const name = document.getElementById('play-namespace').value;
    if (!name || !namespaces[name]) return;
    const info = namespaces[name];
    document.getElementById('play-vector').placeholder = info.config.dim + ' dimensions: e.g. ' + Array(info.config.dim).fill(0).map((_, i) => (i % 2 === 0 ? '0.5' : '-0.2')).slice(0, 5).join(', ') + '...';
}

function generateRandomVector() {
    const name = document.getElementById('play-namespace').value;
    if (!name || !namespaces[name]) return;
    const info = namespaces[name];
    const dim = info.config.dim;
    const metric = info.config.metric;

    let vals = [];
    let sumSq = 0;
    for (let i = 0; i < dim; ++i) {
        const v = Math.random() * 2.0 - 1.0;
        vals.push(v);
        sumSq += v * v;
    }

    if (metric === 'Cosine' || metric === 'DotProduct') {
        const norm = Math.sqrt(sumSq);
        if (norm > 0) {
            vals = vals.map(v => v / norm);
        }
    }

    document.getElementById('play-vector').value = vals.map(v => v.toFixed(6)).join(', ');
}

function addFilterRow() {
    const container = document.getElementById('filters-container');
    const row = document.createElement('div');
    row.className = 'filter-row';
    row.innerHTML = '<input type="text" placeholder="Key" class="filter-key" style="flex: 2;">' +
                   '<select class="filter-type" onchange="toggleFilterTypeFields(this)" style="flex: 2;">' +
                       '<option value="eq">Equals</option>' +
                       '<option value="range">Range</option>' +
                   '</select>' +
                   '<input type="text" placeholder="Value" class="filter-val" style="flex: 3;">' +
                   '<input type="number" placeholder="Low" class="filter-low" style="flex: 1.5; display: none;">' +
                   '<input type="number" placeholder="High" class="filter-high" style="flex: 1.5; display: none;">' +
                   '<button class="btn btn-danger btn-sm" onclick="this.parentElement.remove()">X</button>';
    container.appendChild(row);
}

function toggleFilterTypeFields(select) {
    const row = select.parentElement;
    const type = select.value;
    if (type === 'range') {
        row.querySelector('.filter-val').style.display = 'none';
        row.querySelector('.filter-low').style.display = 'block';
        row.querySelector('.filter-high').style.display = 'block';
    } else {
        row.querySelector('.filter-val').style.display = 'block';
        row.querySelector('.filter-low').style.display = 'none';
        row.querySelector('.filter-high').style.display = 'none';
    }
}

async function runPlaygroundQuery() {
    const name = document.getElementById('play-namespace').value;
    if (!name) return;

    const rawVec = document.getElementById('play-vector').value;
    const query = rawVec.split(',').map(s => parseFloat(s.trim())).filter(n => !isNaN(n));
    const k = parseInt(document.getElementById('play-k').value);
    const ef = parseInt(document.getElementById('play-ef').value);

    const filterRows = document.querySelectorAll('.filter-row');
    let filter = null;
    if (filterRows.length > 0) {
        const row = filterRows[0];
        const key = row.querySelector('.filter-key').value.trim();
        const type = row.querySelector('.filter-type').value;
        if (key) {
            if (type === 'eq') {
                const valStr = row.querySelector('.filter-val').value.trim();
                let value = valStr;
                if (valStr === 'true') value = true;
                else if (valStr === 'false') value = false;
                else if (!isNaN(Number(valStr)) && valStr !== '') value = Number(valStr);
                filter = { type: 'eq', key, value };
            } else if (type === 'range') {
                const low = parseFloat(row.querySelector('.filter-low').value);
                const high = parseFloat(row.querySelector('.filter-high').value);
                filter = { type: 'range', key, low, high };
            }
        }
    }

    const payload = { query, k, ef };
    if (filter) {
        payload.filter = filter;
    }

    const resultsList = document.getElementById('query-results-list');
    resultsList.innerHTML = '<div class="card" style="text-align: center; color: #94a3b8;">Querying...</div>';

    try {
        const results = await apiCall('POST', '/namespaces/' + encodeURIComponent(name) + '/search', payload);
        resultsList.innerHTML = '';
        if (results.length === 0) {
            resultsList.innerHTML = '<div class="card" style="text-align: center; color: #94a3b8;">No matching results found.</div>';
            return;
        }
        results.forEach(hit => {
            const card = document.createElement('div');
            card.className = 'result-card';
            card.innerHTML = '<div class="result-header">' +
                                 '<span class="result-id">ID: ' + hit.id + '</span>' +
                                 '<span class="result-dist">Distance: ' + hit.distance.toFixed(6) + '</span>' +
                             '</div>' +
                             '<div class="result-meta">' + JSON.stringify(hit.metadata) + '</div>';
            resultsList.appendChild(card);
        });
    } catch (err) {
        resultsList.innerHTML = '<div class="card" style="text-align: center; color: #f43f5e;">Query failed: ' + err.message + '</div>';
    }
}

window.addEventListener('DOMContentLoaded', () => {
    loadDashboardData();
});
</script>
</body>
</html>
)rawhtml";
