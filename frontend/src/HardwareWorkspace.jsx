import { useCallback, useEffect, useState } from 'react';
import { Cable, CircleAlert, CircleCheck, PlugZap, Radio, RefreshCw, ShieldAlert, Unplug } from 'lucide-react';

export default function HardwareWorkspace({ backendAvailable }) {
  const [devices,setDevices]=useState([]); const [error,setError]=useState(''); const [loading,setLoading]=useState(true); const [pending,setPending]=useState(''); const [failureArmed,setFailureArmed]=useState({});
  const refresh=useCallback(async()=>{
    setLoading(true);setError('');
    try { const response=await fetch('/api/hardware/devices'); const body=await response.json(); if(!response.ok)throw new Error(body.error||`Backend returned HTTP ${response.status}`); setDevices(body.devices||[]); }
    catch(e){setError(e instanceof Error?e.message:'Unable to load simulated devices');}
    finally{setLoading(false);}
  },[]);
  useEffect(()=>{refresh();},[refresh]);
  async function act(device,action,simulateTimeout=false){
    setPending(device.id);setError('');
    try{
      const response=await fetch(`/api/hardware/devices/${encodeURIComponent(device.id)}/${action}`,{method:'POST',headers:{'Content-Type':'application/json'},body:action==='connect'?JSON.stringify({simulate_timeout:simulateTimeout}):'{}'});
      const body=await response.json();if(!response.ok)throw new Error(body.error||`Backend returned HTTP ${response.status}`);
      setDevices((current)=>current.map((d)=>d.id===body.id?body:d));
      window.dispatchEvent(new CustomEvent('workbench:operation',{detail:`${body.status==='error'?'Simulated device timeout':'Device '+action} · ${body.name}`}));
      if(action==='connect'&&simulateTimeout)setFailureArmed((current)=>({...current,[device.id]:false}));
    }catch(e){setError(e instanceof Error?e.message:'Device action failed');}
    finally{setPending('');}
  }
  return <section className="hardware-page">
    <div className="workspace-heading"><div><div className="eyebrow muted">EMBEDDED INTEGRATION</div><h2>Hardware workbench</h2><p>Host-connected reference architecture with simulated device adapters.</p></div><div className="api-chip"><i className={backendAvailable?'':'muted-dot'}/>{backendAvailable?'API connected':'API unavailable'}</div></div>
    <div className="simulation-banner"><ShieldAlert size={17}/><div><strong>SIMULATION ONLY</strong><span>No physical boards, debug adapter, firmware, JTAG chain, or electrical signals are connected or controlled by this dashboard.</span></div><button onClick={refresh} disabled={loading} aria-label="Refresh device discovery"><RefreshCw size={15} className={loading?'spinning':''}/></button></div>
    {error&&<div className="parse-error" role="alert"><strong>Hardware service error</strong><span>{error}</span></div>}
    {loading&&!devices.length?<div className="hardware-loading"><span className="progress-spinner"/>Discovering simulated adapters…</div>:<div className="device-grid">{devices.map((device)=><DeviceCard key={device.id} device={device} pending={pending===device.id} failureArmed={Boolean(failureArmed[device.id])} onFailureChange={(value)=>setFailureArmed((current)=>({...current,[device.id]:value}))} onConnect={(failed)=>act(device,'connect',failed)} onDisconnect={()=>act(device,'disconnect')}/>)}</div>}
    <div className="hardware-footnote"><Cable size={15}/><span>Before wiring real equipment, verify the exact board revision, connector pinout, power selection and voltage levels against its manufacturer documentation. See <code>docs/hardware-integration.md</code> for the block diagram and interface assumptions.</span></div>
  </section>;
}

function DeviceCard({device,pending,failureArmed,onFailureChange,onConnect,onDisconnect}){
  const connected=device.status==='connected'; const failed=device.status==='error';
  return <article className={`device-card ${failed?'device-failed':''}`}>
    <div className="device-card-head"><div className="device-icon"><Radio size={18}/></div><span className="sim-tag">SIMULATED</span></div>
    <div className="device-type">{device.device_type}</div><h3>{device.name}</h3>
    <div className={`device-status ${connected?'connected':failed?'failed':''}`}><i/>{device.status?.toUpperCase()||'UNKNOWN'}</div>
    {failed&&<div className="device-recovery"><strong><CircleAlert size={14}/> {device.error}</strong><span>Recovery: {device.recovery_action}</span></div>}
    <div className="device-detail"><span>Interface</span><strong>{device.communication_protocol}</strong></div>
    <div className="device-detail"><span>Logic voltage</span><strong>{device.logic_voltage}</strong></div>
    <div className="device-detail"><span>Power input</span><strong>{device.power_input}</strong></div>
    <div className="device-capabilities"><span>CAPABILITIES</span><div>{device.capabilities?.map((cap)=><i key={cap}>{cap}</i>)}</div></div>
    <div className="device-telemetry"><span>TELEMETRY · SIMULATED</span><p>{device.telemetry?.message}</p></div>
    <div className="device-controls">{connected?<button className="device-disconnect" onClick={onDisconnect} disabled={pending}><Unplug size={14}/>Disconnect</button>:<><button className="device-connect" onClick={()=>onConnect(false)} disabled={pending}><PlugZap size={14}/>{pending?'Connecting…':failed?'Retry connection':'Connect'}</button><label className="failure-toggle"><input type="checkbox" checked={failureArmed} onChange={(e)=>onFailureChange(e.target.checked)}/>Simulate timeout</label>{failureArmed&&<button className="simulate-failure" onClick={()=>onConnect(true)} disabled={pending}>Connect with failure</button>}</>}</div>
  </article>;
}
