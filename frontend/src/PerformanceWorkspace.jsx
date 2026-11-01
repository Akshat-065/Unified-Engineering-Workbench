import { useEffect, useMemo, useState } from 'react';
import { Activity, Cpu, Play, RotateCw } from 'lucide-react';

export default function PerformanceWorkspace({ launchToReadyMs }) {
  const [hardware, setHardware] = useState(null); const [caseCount, setCaseCount] = useState(40); const [meshSize, setMeshSize] = useState(80); const [workers, setWorkers] = useState(2);
  const [result, setResult] = useState(null); const [error, setError] = useState(''); const [running, setRunning] = useState(false);
  useEffect(() => { fetch('/api/performance/hardware').then(async (r) => { const b=await r.json(); if(!r.ok) throw new Error(b.error||'Hardware information unavailable'); setHardware(b); }).catch((e)=>setError(e.message)); }, []);
  const workerChoices = useMemo(() => Array.from({length:Math.max(1,Math.min(4,hardware?.logical_cores??1))},(_,i)=>i+1),[hardware]);
  async function run() {
    setRunning(true); setError(''); setResult(null);
    try {
      const response=await fetch('/api/performance/benchmark',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({case_count:Number(caseCount),mesh_size:Number(meshSize),workers,seed:20261101,launch_to_ready_ms:launchToReadyMs??0})});
      const body=await response.json(); if(!response.ok) throw new Error(body.error||`Backend returned HTTP ${response.status}`); setResult(body);
    } catch(e) { setError(e instanceof Error?e.message:'Benchmark request failed'); }
    finally { setRunning(false); }
  }
  const chartMax=result?Math.max(result.median_serial_ms,result.median_parallel_ms,Number.EPSILON):1;
  return <section className="performance-page">
    <div className="workspace-heading"><div><div className="eyebrow muted">CPU COMPUTE</div><h2>Thermal batch benchmark</h2><p>Compare the existing thermal solver in sequential and bounded worker-pool modes.</p></div><div className="api-chip"><i className={hardware?'':'muted-dot'}/>{hardware?'Hardware detected':'Reading hardware'}</div></div>
    <div className="perf-layout"><div className="perf-main">
      <div className="panel benchmark-config"><div className="panel-heading"><div><div className="eyebrow muted">WORKLOAD</div><h2>Benchmark setup</h2></div><Cpu size={19} color="#788a65"/></div>
        <div className="config-fields"><label>Independent cases<input type="number" min="1" max="100000" value={caseCount} onChange={(e)=>setCaseCount(e.target.value)}/></label><label>Elements per mesh<input type="number" min="1" max="512" value={meshSize} onChange={(e)=>setMeshSize(e.target.value)}/></label><label>Parallel workers<select value={workers} onChange={(e)=>setWorkers(Number(e.target.value))}>{workerChoices.map((n)=><option key={n} value={n}>{n} {n===1?'worker':'workers'}</option>)}</select></label></div>
        <div className="seed-note">Fixed seed <code>20261101</code> · varied k = 5–25 W/(m·K), q = 5,000–15,000 W/m³ · one warm-up + five timed repetitions</div>
        <button className="start-benchmark" onClick={run} disabled={running||!hardware}><Play size={15} fill="currentColor"/>{running?'Running serial and parallel batches…':'Start benchmark'}</button>
        {running&&<div className="benchmark-progress" role="status"><span className="progress-spinner"/>C++ is solving the batch. Timing excludes request parsing and result comparison.</div>}
        {error&&<div className="parse-error" role="alert"><strong>Benchmark unavailable</strong><span>{error}</span></div>}
      </div>
      <div className="panel chart-panel"><div className="panel-heading"><div><div className="eyebrow muted">MEASURED WALL TIME</div><h2>Serial vs. parallel</h2></div>{result&&<span className="run-badge">5 repetitions · median</span>}</div>
        {!result?<div className="perf-empty"><Activity size={22}/><span>Run a benchmark to populate the timing comparison.</span></div>:<><div className="median-chart"><div className="chart-row"><span>Serial · 1 worker</span><div className="bar-track"><i className="serial-bar" style={{width:`${Math.max(1,result.median_serial_ms/chartMax*100)}%`}}/></div><b>{result.median_serial_ms.toFixed(3)} ms</b></div><div className="chart-row"><span>Parallel · {result.workers} {result.workers===1?'worker':'workers'}</span><div className="bar-track"><i className="parallel-bar" style={{width:`${Math.max(1,result.median_parallel_ms/chartMax*100)}%`}}/></div><b>{result.median_parallel_ms.toFixed(3)} ms</b></div></div>
          <div className="metric-strip"><Metric label="SPEEDUP" value={`${result.speedup.toFixed(3)}×`}/><Metric label="EFFICIENCY" value={`${(result.efficiency*100).toFixed(1)}%`}/><Metric label="MAX TEMP DELTA" value={`${result.max_temperature_difference_C.toExponential(2)} °C`}/></div>
          <div className="repetition-chart"><div className="model-panel-title">MEASURED REPETITIONS · MS</div>{result.repetitions.map((r)=><div className="rep-row" key={r.repetition}><span>R{r.repetition}</span><div><i className="serial-bar" style={{width:`${Math.max(1,r.serial_ms/chartMax*100)}%`}}/></div><div><i className="parallel-bar" style={{width:`${Math.max(1,r.parallel_ms/chartMax*100)}%`}}/></div></div>)}<div className="rep-legend"><span><i className="serial-swatch"/> Serial</span><span><i className="parallel-swatch"/> Parallel</span></div></div></>}
      </div>
    </div><aside className="panel hardware-panel"><div className="panel-heading"><div><div className="eyebrow muted">SYSTEM DETAILS</div><h2>Benchmark environment</h2></div><Cpu size={18} color="#788a65"/></div>
      {hardware?<><Info label="CPU" value={hardware.cpu_model}/><Info label="Physical cores" value={hardware.physical_cores}/><Info label="Logical cores" value={hardware.logical_cores}/><Info label="Installed RAM" value={hardware.ram_bytes?`${(hardware.ram_bytes/1073741824).toFixed(1)} GiB`:'Unavailable'}/><Info label="Compiler" value={hardware.compiler}/><Info label="Optimization" value={hardware.optimization_flags}/><Info label="Launch to API ready" value={launchToReadyMs==null?'Not recorded':`${launchToReadyMs.toFixed(1)} ms`}/>{result&&<><Info label="Workload" value={`${result.case_count} cases · ${result.mesh_size} elements each`}/><Info label="Workers used" value={`${result.workers} / ${result.logical_cores} logical CPUs`}/><Info label="Seed" value={result.seed}/></>}</>:<p className="info-placeholder">Waiting for backend hardware metadata.</p>}
      <div className="hardware-note">Worker count is capped by available logical CPUs. This dense direct solver can become memory and compute bound at larger meshes.</div>
    </aside></div>
  </section>;
}
function Metric({label,value}){return <div className="metric"><span>{label}</span><strong>{value}</strong></div>}
function Info({label,value}){return <div className="info-row"><span>{label}</span><strong>{value}</strong></div>}
