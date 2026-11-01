import { useEffect, useMemo, useRef, useState } from 'react';
import { Activity, CircleAlert, Download, Play, Square } from 'lucide-react';

const defaults = {
  length_m: '1', area_m2: '0.0001', conductivity_w_per_m_k: '10',
  heat_generation_w_per_m3: '10000', left_temperature_c: '100', right_temperature_c: '20',
};
const fields = [
  ['length_m', 'Length L', 'm'], ['area_m2', 'Cross-section A', 'm²'],
  ['conductivity_w_per_m_k', 'Conductivity k', 'W/(m·K)'],
  ['heat_generation_w_per_m3', 'Heat generation q', 'W/m³'],
  ['left_temperature_c', 'Left temperature T(0)', '°C'], ['right_temperature_c', 'Right temperature T(L)', '°C'],
];
const meshChoices = [10, 20, 40, 80];

function downloadCsv(meshes) {
  const rows = ['mesh_elements,point_type,point_index,x_m,finite_element_temperature_C,analytical_temperature_C,absolute_error_C'];
  for (const mesh of meshes) {
    for (const type of ['nodes', 'midpoints']) {
      mesh[type].forEach((point, index) => rows.push([
        mesh.element_count, type === 'nodes' ? 'node' : 'midpoint', index, point.x_m,
        point.finite_element_temperature_C, point.analytical_temperature_C, point.absolute_error_C,
      ].join(',')));
    }
  }
  const url = URL.createObjectURL(new Blob([`${rows.join('\n')}\n`], { type: 'text/csv;charset=utf-8' }));
  const link = document.createElement('a'); link.href = url; link.download = 'thermal_analysis_results.csv'; link.click();
  window.setTimeout(() => URL.revokeObjectURL(url), 1000);
}

function TemperatureChart({ mesh }) {
  const points = useMemo(() => [
    ...mesh.nodes.map((point) => ({ ...point, point_type: 'node' })),
    ...mesh.midpoints.map((point) => ({ ...point, point_type: 'midpoint' })),
  ].sort((a, b) => a.x_m - b.x_m), [mesh]);
  const W = 900, H = 300, P = { l: 62, r: 25, t: 20, b: 48 };
  const temperatures = points.flatMap((point) => [point.finite_element_temperature_C, point.analytical_temperature_C]);
  const ymin = Math.min(...temperatures), ymax = Math.max(...temperatures);
  const x = (value) => P.l + value / (points.at(-1).x_m || 1) * (W - P.l - P.r);
  const y = (value) => H - P.b - (value - ymin) / (ymax - ymin || 1) * (H - P.t - P.b);
  const series = (key) => points.map((point, i) => `${i ? 'L' : 'M'} ${x(point.x_m).toFixed(2)} ${y(point[key]).toFixed(2)}`).join(' ');
  return <div className="thermal-chart-scroll"><svg className="thermal-chart" viewBox={`0 0 ${W} ${H}`} role="img" aria-label={`Numerical and analytical temperature along length for ${mesh.element_count} elements`}>
    <line x1={P.l} y1={P.t} x2={P.l} y2={H - P.b} className="chart-axis"/><line x1={P.l} y1={H - P.b} x2={W - P.r} y2={H - P.b} className="chart-axis"/>
    <path d={series('finite_element_temperature_C')} className="thermal-numerical-line"/><path d={series('analytical_temperature_C')} className="thermal-analytical-line"/>
    <text x="10" y="16" className="chart-label">Temperature (°C)</text><text x={W/2} y={H-8} textAnchor="middle" className="chart-label">Position x (m)</text>
    <text x={P.l-8} y={y(ymax)+4} textAnchor="end" className="chart-tick">{ymax.toFixed(1)}</text><text x={P.l-8} y={y(ymin)+4} textAnchor="end" className="chart-tick">{ymin.toFixed(1)}</text>
    <text x={P.l} y={H-P.b+20} className="chart-tick">0</text><text x={W-P.r} y={H-P.b+20} textAnchor="end" className="chart-tick">{points.at(-1).x_m.toPrecision(3)}</text>
    <line x1={W-252} y1="17" x2={W-226} y2="17" className="thermal-numerical-line"/><text x={W-220} y="21" className="chart-tick">FEA · {mesh.element_count} elements</text>
    <line x1={W-115} y1="17" x2={W-89} y2="17" className="thermal-analytical-line"/><text x={W-83} y="21" className="chart-tick">Analytical</text>
  </svg></div>;
}

export default function ThermalWorkspace({ backendAvailable }) {
  const [parameters, setParameters] = useState(defaults);
  const [selectedMeshes, setSelectedMeshes] = useState(meshChoices);
  const [viewMesh, setViewMesh] = useState(80);
  const [result, setResult] = useState(null);
  const [error, setError] = useState('');
  const [status, setStatus] = useState('idle');
  const requestRef = useRef(null);
  const running = status === 'running';

  useEffect(() => () => requestRef.current?.abort(), []);

  const validationError = (() => {
    for (const [name, label] of fields) {
      const value = Number(parameters[name]);
      if (parameters[name].trim() === '' || !Number.isFinite(value)) return `${label} must be a finite number.`;
      if (['length_m', 'area_m2', 'conductivity_w_per_m_k', 'heat_generation_w_per_m3'].includes(name) && value <= 0)
        return `${label} must be greater than zero.`;
    }
    if (!selectedMeshes.length) return 'Select at least one mesh size.';
    return '';
  })();

  async function solve() {
    if (validationError || !backendAvailable) { setError(validationError || 'Backend is unavailable. Start the C++ workbench service and retry.'); return; }
    const controller = new AbortController(); requestRef.current = controller;
    setError(''); setStatus('running');
    try {
      const payload = { parameters: Object.fromEntries(Object.entries(parameters).map(([key, value]) => [key, Number(value)])), meshes: selectedMeshes };
      const response = await fetch('/api/thermal/solve', { method: 'POST', headers: { 'Content-Type': 'application/json' }, body: JSON.stringify(payload), signal: controller.signal });
      const body = await response.json();
      if (!response.ok) throw new Error(body.error || `Backend returned HTTP ${response.status}`);
      if (requestRef.current !== controller) return;
      setResult(body);
      if (!selectedMeshes.includes(viewMesh)) setViewMesh(selectedMeshes[selectedMeshes.length - 1]);
      setStatus('complete');
      window.dispatchEvent(new CustomEvent('workbench:operation', { detail: `Thermal FEA completed · ${selectedMeshes.join(', ')} element meshes` }));
    } catch (e) {
      if (requestRef.current === controller) {
        if (e.name === 'AbortError') setStatus('cancelled');
        else { setError(e instanceof Error ? e.message : 'Thermal solve failed. Check inputs and backend status.'); setStatus('error'); }
      }
    } finally { if (requestRef.current === controller) requestRef.current = null; }
  }

  function cancel() { requestRef.current?.abort(); setStatus('cancelled'); }
  const activeMesh = result?.meshes.find((mesh) => mesh.element_count === viewMesh) ?? result?.meshes[0];

  return <section className="thermal-page">
    <div className="workspace-heading"><div><div className="eyebrow muted">THERMAL ENGINEERING</div><h2>Steady-state 1D analysis</h2><p>Run the C++ two-node linear finite element solver with analytical comparison.</p></div><div className={`api-chip ${backendAvailable ? 'api-up' : 'api-down'}`}><i />{backendAvailable ? 'API connected' : 'API unavailable'}</div></div>
    <div className="thermal-layout">
      <section className="panel thermal-input-panel"><div className="panel-heading"><div><div className="eyebrow muted">MODEL INPUT</div><h2>Material and boundary conditions</h2></div><Activity size={18}/></div>
        <div className="thermal-field-grid">{fields.map(([name, label, unit]) => <label key={name}>{label}<span>{unit}</span><input type="number" step="any" value={parameters[name]} onChange={(event) => setParameters((current) => ({ ...current, [name]: event.target.value }))}/></label>)}</div>
        <div className="thermal-mesh-select"><strong>Uniform mesh sizes</strong><span>Choose one or more assignment meshes</span><div>{meshChoices.map((mesh) => <label key={mesh}><input type="checkbox" checked={selectedMeshes.includes(mesh)} onChange={(event) => setSelectedMeshes((current) => event.target.checked ? [...current, mesh].sort((a,b)=>a-b) : current.filter((item) => item !== mesh))}/>{mesh} elements</label>)}</div></div>
        {validationError && <div className="thermal-validation" role="status"><CircleAlert size={15}/>{validationError}</div>}
        {error && <div className="parse-error" role="alert"><strong>Analysis could not complete</strong><span>{error}</span></div>}
        <div className="thermal-actions">{running ? <button className="cancel-action" onClick={cancel}><Square size={14}/>Cancel request</button> : <button className="start-benchmark" disabled={Boolean(validationError) || !backendAvailable} onClick={solve}><Play size={15} fill="currentColor"/>Run thermal analysis</button>}
          {result && <button className="export-action" onClick={() => downloadCsv(result.meshes)}><Download size={15}/>Export CSV</button>}</div>
        {running && <div className="benchmark-progress" role="status"><span className="progress-spinner"/>C++ is assembling and solving the selected meshes. The previous completed result remains available until this run succeeds.</div>}
        {status === 'cancelled' && <div className="thermal-status" role="status">Request cancelled. Any previous completed result is retained.</div>}
        {status === 'complete' && <div className="thermal-status success" role="status">Analysis complete. Numerical results were returned by the C++ solver.</div>}
      </section>
      <section className="panel thermal-result-panel"><div className="panel-heading"><div><div className="eyebrow muted">RESULTS</div><h2>Temperature profile</h2></div>{result && <select aria-label="Mesh displayed in chart" value={activeMesh.element_count} onChange={(event) => setViewMesh(Number(event.target.value))}>{result.meshes.map((mesh) => <option key={mesh.element_count} value={mesh.element_count}>{mesh.element_count} elements</option>)}</select>}</div>
        {!result ? <div className="thermal-empty"><Activity size={22}/><strong>No completed analysis yet</strong><span>Set the model inputs and run the C++ thermal solver.</span></div> : <>
          {activeMesh && <><TemperatureChart mesh={activeMesh}/><div className="thermal-chart-legend"><span><i className="thermal-legend-numerical"/>Finite element interpolation</span><span><i className="thermal-legend-analytical"/>Analytical solution</span></div></>}
          <div className="thermal-error-table"><div className="model-panel-title">MAXIMUM ABSOLUTE MIDPOINT ERROR</div>{result.meshes.map((mesh) => <div className="thermal-error-row" key={mesh.element_count}><span>N = {mesh.element_count}</span><div className="thermal-error-track"><i style={{ width: `${Math.max(2, mesh.maximum_midpoint_error_C / Math.max(...result.meshes.map((item) => item.maximum_midpoint_error_C)) * 100)}%` }}/></div><strong>{mesh.maximum_midpoint_error_C.toExponential(4)} °C</strong></div>)}</div>
        </>}
      </section>
    </div>
  </section>;
}
