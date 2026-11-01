import { useEffect, useState } from 'react';
import { Activity, Boxes, Cpu, LayoutDashboard, Radio, Network, Settings, Search, Bell, ChevronDown, CircleCheck, CircleAlert, RefreshCw, ArrowUpRight, Command } from 'lucide-react';
import GeometryWorkspace from './GeometryWorkspace.jsx';
import PerformanceWorkspace from './PerformanceWorkspace.jsx';
import HardwareWorkspace from './HardwareWorkspace.jsx';
import BackpropagationWorkspace from './BackpropagationWorkspace.jsx';

const modules = [
  { name: 'Overview', icon: LayoutDashboard },
  { name: 'Geometry Viewer', icon: Boxes },
  { name: 'Thermal Analysis', icon: Activity },
  { name: 'Performance', icon: Cpu },
  { name: 'Hardware', icon: Radio },
  { name: 'Backpropagation', icon: Network },
];

const cards = [
  { name: 'Geometry Viewer', description: 'Inspect CAD models and geometry', icon: Boxes, state: 'Workspace ready', tone: 'blue' },
  { name: 'Thermal Analysis', description: 'Set up and review thermal studies', icon: Activity, state: 'Awaiting model', tone: 'orange' },
  { name: 'Performance', description: 'Explore CPU compute benchmarks', icon: Cpu, state: 'No runs yet', tone: 'purple' },
  { name: 'Hardware', description: 'Review embedded system designs', icon: Radio, state: 'Design workspace', tone: 'green' },
  { name: 'Backpropagation', description: 'Configure neural network experiments', icon: Network, state: 'Workspace ready', tone: 'pink' },
];

export default function App() {
  const [active, setActive] = useState('Overview');
  const [backend, setBackend] = useState({ loading: true, error: '', health: null, status: null, launchReadyMs: null });
  const [appStartedAt] = useState(() => performance.now());

  async function refreshBackend() {
    setBackend((current) => ({ ...current, loading: true, error: '' }));
    try {
      const [healthResponse, statusResponse] = await Promise.all([fetch('/api/health'), fetch('/api/status')]);
      if (!healthResponse.ok || !statusResponse.ok) throw new Error(`Backend returned HTTP ${!healthResponse.ok ? healthResponse.status : statusResponse.status}`);
      const [health, status] = await Promise.all([healthResponse.json(), statusResponse.json()]);
      setBackend((current) => ({ loading: false, error: '', health, status, launchReadyMs: current.launchReadyMs ?? performance.now() - appStartedAt }));
    } catch (error) {
      setBackend((current) => ({ ...current, loading: false, error: error instanceof Error ? error.message : 'Unable to reach backend', health: null, status: null }));
    }
  }

  useEffect(() => { refreshBackend(); }, []);

  return <div className="app-shell">
    <aside className="sidebar">
      <div className="brand"><div className="brand-mark"><Command size={19} strokeWidth={2.4} /></div><div><strong>UE Workbench</strong><span>ENGINEERING SUITE</span></div></div>
      <div className="nav-label">WORKSPACE</div>
      <nav>{modules.map(({ name, icon: Icon }) => <button key={name} className={`nav-item ${active === name ? 'selected' : ''}`} onClick={() => setActive(name)}><Icon size={18} /><span>{name}</span>{name === 'Overview' && <i className="nav-dot" />}</button>)}</nav>
      <div className="sidebar-bottom"><div className="nav-label">SYSTEM</div><button className="nav-item"><Settings size={18} /><span>Settings</span></button><div className="profile"><div className="avatar">UE</div><div><strong>Engineering Team</strong><span>Local workspace</span></div><ChevronDown size={16} /></div></div>
    </aside>
    <main className="main-area">
      <header className="topbar"><div className="breadcrumbs"><span>Workspace</span><b>/</b><strong>{active}</strong></div><div className="top-actions"><div className="search"><Search size={16} /><span>Search workspace</span><kbd>⌘ K</kbd></div><button className="icon-button" aria-label="Notifications"><Bell size={18} /><i /></button><div className="top-divider" /><div className="user-chip"><div className="avatar small">UE</div><ChevronDown size={15} /></div></div></header>
      <div className="content">{active === 'Geometry Viewer' ? <GeometryWorkspace backend={Boolean(backend.health?.healthy)} /> : active === 'Performance' ? <PerformanceWorkspace launchToReadyMs={backend.launchReadyMs} /> : active === 'Hardware' ? <HardwareWorkspace backendAvailable={Boolean(backend.health?.healthy)} /> : active === 'Backpropagation' ? <BackpropagationWorkspace backendAvailable={Boolean(backend.health?.healthy)} /> : <>
        <section className="welcome"><div><div className="eyebrow"><span className="eyebrow-line" /> ENGINEERING WORKSPACE <span className="eyebrow-line" /></div><h1>{active === 'Overview' ? <>Build with <em>confidence.</em></> : active}</h1><p>{active === 'Overview' ? 'Your tools for designing, analyzing, and understanding complex systems.' : `Your ${active.toLowerCase()} workspace is ready to configure.`}</p></div><button className="date-pill">NOVEMBER 01, 2026 <ChevronDown size={14} /></button></section>
        <section className={`connection-panel ${backend.health?.healthy ? 'online' : backend.error ? 'offline' : ''}`}>
          <div className="connection-icon">{backend.error ? <CircleAlert size={19} /> : <Activity size={19} />}</div>
          <div className="connection-copy"><strong>Backend connection</strong><span>{backend.loading ? 'Checking local service…' : backend.error ? backend.error : 'Connected to the local engineering service'}</span></div>
          <div className="connection-meta"><span className={`status-dot ${backend.health?.healthy ? 'good' : backend.error ? 'bad' : ''}`} />{backend.loading ? 'CONNECTING' : backend.error ? 'UNAVAILABLE' : 'CONNECTED'}{backend.status?.version && <span className="version">v{backend.status.version}</span>}</div>
          <button className="refresh-button" onClick={refreshBackend} disabled={backend.loading} aria-label="Retry connection"><RefreshCw size={16} className={backend.loading ? 'spinning' : ''} /></button>
        </section>
        <div className="section-heading"><div><div className="eyebrow muted">YOUR TOOLKIT</div><h2>Engineering modules</h2></div><button className="text-button">View all modules <ArrowUpRight size={15} /></button></div>
        <section className="module-grid">{cards.map(({ name, description, icon: Icon, state, tone }) => <button className="module-card" key={name} onClick={() => setActive(name)}><div className={`module-icon ${tone}`}><Icon size={20} /></div><ArrowUpRight className="card-arrow" size={16} /><h3>{name}</h3><p>{description}</p><div className="card-footer"><span className="mini-status"><i />{state}</span><span className="open-label">OPEN <span>↗</span></span></div></button>)}</section>
        <section className="lower-grid"><div className="panel recent-panel"><div className="panel-heading"><div><div className="eyebrow muted">ACTIVITY</div><h2>Recent work</h2></div><button className="quiet-button">View activity <ArrowUpRight size={14} /></button></div><div className="empty-state"><div className="empty-icon"><Boxes size={20} /></div><strong>Your workspace is clear</strong><span>Open a module to begin your first engineering session.</span></div></div>
          <div className="panel system-panel"><div className="panel-heading"><div><div className="eyebrow muted">SERVICE STATUS</div><h2>System overview</h2></div><span className="live-badge"><i /> LIVE</span></div><div className="system-row"><span>API service</span><span className={backend.health?.healthy ? 'value-good' : backend.error ? 'value-bad' : ''}>{backend.loading ? 'Connecting…' : backend.error ? 'Unavailable' : 'Operational'} {backend.health?.healthy ? <CircleCheck size={15} /> : null}</span></div><div className="system-row"><span>Server version</span><span>{backend.status?.version ?? (backend.error ? 'Unavailable' : '—')}</span></div><div className="system-row"><span>Workspace mode</span><span><i className="mode-dot" /> Local development</span></div></div>
        </section>
        <footer><span>UNIFIED ENGINEERING WORKBENCH <i>·</i> FOUNDATION BUILD</span><span>ALL SYSTEMS DESIGNED FOR CLARITY</span></footer>
      </>}</div>
    </main>
  </div>;
}
