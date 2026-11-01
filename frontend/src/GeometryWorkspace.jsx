import { useCallback, useEffect, useRef, useState } from 'react';
import { Boxes, FileUp, Thermometer, RotateCcw } from 'lucide-react';
import ThreeViewport from './ThreeViewport.jsx';

const LIMIT = 16 * 1024 * 1024;
export default function GeometryWorkspace({ backend }) {
  const [kind, setKind] = useState('stl'); const [result, setResult] = useState(null); const [error, setError] = useState(''); const [busy, setBusy] = useState(false); const [inspected, setInspected] = useState(null);
  const requestRef = useRef(null); const [notice, setNotice] = useState('');
  const inspect = useCallback((node) => setInspected(node), []);
  useEffect(() => () => requestRef.current?.abort(), []);
  async function upload(file) {
    setError(''); setNotice('');
    if (!file) return;
    if (file.size > LIMIT) { setError('File exceeds the 16 MiB per-file limit.'); return; }
    const controller = new AbortController(); requestRef.current = controller;
    setBusy(true);
    try {
      const path = kind === 'stl' ? '/api/geometry/stl' : '/api/geometry/temperature';
      const response = await fetch(path, { method: 'POST', headers: { 'Content-Type': 'application/octet-stream' }, body: file, signal: controller.signal });
      const body = await response.json(); if (!response.ok) throw new Error(body.error || `Server returned HTTP ${response.status}`);
      if (requestRef.current !== controller) return;
      setResult(body); setInspected(null);
      window.dispatchEvent(new CustomEvent('workbench:operation', { detail: `${kind === 'stl' ? 'ASCII STL' : 'Temperature CSV'} loaded · ${kind === 'stl' ? `${body.triangle_count} triangles` : `${body.node_count} nodes`}` }));
    } catch (e) { if (requestRef.current === controller) { if (e.name === 'AbortError') setNotice('Upload cancelled. The last successfully parsed model remains displayed.'); else setError(e instanceof Error ? e.message : 'Unable to parse file.'); } }
    finally { if (requestRef.current === controller) { requestRef.current = null; setBusy(false); } }
  }
  function changeKind(next) { requestRef.current?.abort(); setKind(next); setResult(null); setError(''); setNotice(''); setInspected(null); }
  return <section className="geometry-page">
    <div className="workspace-heading"><div><div className="eyebrow muted">GEOMETRY & RESULTS</div><h2>3D file viewer</h2><p>Inspect ASCII STL surfaces or temperature nodes from the supported CSV schema.</p></div><div className={`api-chip ${backend ? 'api-up' : 'api-down'}`}><i />{backend ? 'API connected' : 'API unavailable'}</div></div>
    <div className="viewer-tabs"><button className={kind==='stl'?'active':''} onClick={() => changeKind('stl')}><Boxes size={16}/> ASCII STL</button><button className={kind==='csv'?'active':''} onClick={() => changeKind('csv')}><Thermometer size={16}/> Temperature CSV</button></div>
    <div className="viewer-layout"><div className="viewer-main">
      <div className="upload-row"><label className="upload-button"><FileUp size={17}/>{busy?'Parsing…':kind==='stl'?'Choose ASCII STL':'Choose temperature CSV'}<input type="file" disabled={busy} accept={kind==='stl'?'.stl,text/plain':'.csv,text/csv'} onChange={(e)=>{upload(e.target.files?.[0]);e.target.value='';}}/></label><span>Maximum file size: 16 MiB</span>{busy&&<button className="cancel-action" onClick={()=>requestRef.current?.abort()}><Square size={13}/>Cancel</button>}{result&&<button className="reset-button" onClick={()=>{setResult(null);setError('');setInspected(null);}}><RotateCcw size={14}/> Clear</button>}</div>
      {error&&<div className="parse-error" role="alert"><strong>Could not load file</strong><span>{error}</span></div>}
      {notice&&<div className="thermal-status" role="status">{notice}</div>}
      {busy&&<div className="benchmark-progress" role="status"><span className="progress-spinner"/>C++ is validating and parsing the selected file. Previous parsed geometry remains available until this upload succeeds.</div>}
      {result&&<ThreeViewport model={result} kind={kind} onInspect={inspect}/>}
      {!result&&!error&&<div className="empty-viewport"><div><Boxes size={30}/><strong>Select a file to begin</strong><span>{kind==='stl'?'ASCII STL only. Binary STL and native CAD files are not supported.':'Exact header: node_id,x_m,y_m,z_m,temperature_C'}</span></div></div>}
    </div><aside className="model-panel"><div className="model-panel-title">MODEL INFORMATION</div>{result?kind==='stl'?<><div className="model-name">{result.solid_name||'Unnamed solid'}</div><Info label="Format" value="ASCII STL"/><Info label="Triangles" value={result.triangle_count}/><Info label="File size" value={`${(result.file_size_bytes/1024).toFixed(1)} KiB`}/><Info label="Units" value="Unspecified in STL"/><Info label="Extent (m units assumed)" value={result.bounds?`${fmt(result.bounds.max[0]-result.bounds.min[0])} × ${fmt(result.bounds.max[1]-result.bounds.min[1])} × ${fmt(result.bounds.max[2]-result.bounds.min[2])}`:'—'}/></>:<><div className="model-name">Temperature point cloud</div><Info label="Nodes" value={result.node_count}/><Info label="File size" value={`${(result.file_size_bytes/1024).toFixed(1)} KiB`}/><Info label="Temperature range" value={`${fmt(result.temperature_range_C[0])}–${fmt(result.temperature_range_C[1])} °C`}/><Info label="Coordinates" value="m"/>{inspected&&<div className="inspection"><div className="model-panel-title">SELECTED NODE</div><Info label="Node ID" value={inspected.node_id}/><Info label="X / Y / Z" value={`${fmt(inspected.x_m)}, ${fmt(inspected.y_m)}, ${fmt(inspected.z_m)} m`}/><Info label="Temperature" value={`${fmt(inspected.temperature_C)} °C`}/></div>}<div className="legend"><div className="model-panel-title">TEMPERATURE · °C</div><div className="legend-bar"/><div className="legend-labels"><span>{fmt(result.temperature_range_C[0])} °C</span><span>{fmt(result.temperature_range_C[1])} °C</span></div><small>Click a point to inspect its node.</small></div></>:<p className="info-placeholder">Load a file to view validated model details.</p>}</aside></div>
  </section>;
}
function fmt(value){return Number(Number(value).toPrecision(5)).toString();}
function Info({label,value}){return <div className="info-row"><span>{label}</span><strong>{value}</strong></div>}
