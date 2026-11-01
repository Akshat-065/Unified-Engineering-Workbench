import { useEffect, useRef } from 'react';
import * as THREE from 'three';
import { OrbitControls } from 'three/addons/controls/OrbitControls.js';

export default function ThreeViewport({ model, kind, onInspect }) {
  const hostRef = useRef(null);
  const fitRef = useRef(() => {});
  useEffect(() => {
    const host = hostRef.current;
    if (!host || !model) return undefined;
    const scene = new THREE.Scene(); scene.background = new THREE.Color('#f3f6f5');
    const camera = new THREE.PerspectiveCamera(45, 1, 0.001, 1e8); camera.position.set(3, 3, 4);
    const renderer = new THREE.WebGLRenderer({ antialias: true }); renderer.setPixelRatio(Math.min(devicePixelRatio, 2)); host.appendChild(renderer.domElement);
    const controls = new OrbitControls(camera, renderer.domElement); controls.enableDamping = true;
    scene.add(new THREE.HemisphereLight(0xffffff, 0x52616a, 2));
    const light = new THREE.DirectionalLight(0xffffff, 2); light.position.set(3, 5, 7); scene.add(light);
    scene.add(new THREE.GridHelper(10, 20, '#9ba9aa', '#d8dfdd'));
    const objects = new THREE.Group(); scene.add(objects);
    let pointsObject;
    if (kind === 'stl') {
      const geometry = new THREE.BufferGeometry();
      geometry.setAttribute('position', new THREE.Float32BufferAttribute(model.positions, 3));
      geometry.setAttribute('normal', new THREE.Float32BufferAttribute(model.normals, 3));
      const mesh = new THREE.Mesh(geometry, new THREE.MeshStandardMaterial({ color: '#9bbd69', metalness: .12, roughness: .58, side: THREE.DoubleSide }));
      objects.add(mesh); objects.add(new THREE.LineSegments(new THREE.EdgesGeometry(geometry), new THREE.LineBasicMaterial({ color: '#354337', transparent: true, opacity: .38 })));
    } else {
      const temperatures = model.nodes.map((n) => n.temperature_C); const lo = Math.min(...temperatures), hi = Math.max(...temperatures);
      const positions = [], colors = [];
      for (const n of model.nodes) {
        positions.push(n.x_m, n.y_m, n.z_m);
        const color = new THREE.Color().setHSL(.67 - (hi === lo ? .5 : (n.temperature_C - lo) / (hi - lo)) * .67, .86, .52);
        colors.push(color.r, color.g, color.b);
      }
      const geometry = new THREE.BufferGeometry(); geometry.setAttribute('position', new THREE.Float32BufferAttribute(positions, 3)); geometry.setAttribute('color', new THREE.Float32BufferAttribute(colors, 3));
      pointsObject = new THREE.Points(geometry, new THREE.PointsMaterial({ size: 10, sizeAttenuation: false, vertexColors: true })); objects.add(pointsObject);
    }
    const box = new THREE.Box3().setFromObject(objects);
    const fit = () => {
      const size = box.getSize(new THREE.Vector3()); const center = box.getCenter(new THREE.Vector3());
      const span = Math.max(size.x, size.y, size.z, 1e-6); const fov = THREE.MathUtils.degToRad(camera.fov);
      const distance = span / (2 * Math.tan(fov / 2)) * 1.8;
      camera.position.copy(center).add(new THREE.Vector3(distance, distance * .75, distance)); camera.near = Math.max(span / 10000, .0001); camera.far = Math.max(span * 100, 100); camera.updateProjectionMatrix(); controls.target.copy(center); controls.update();
    };
    fitRef.current = fit; fit();
    const resize = () => { const w = host.clientWidth, h = host.clientHeight; if (!w || !h) return; renderer.setSize(w, h); camera.aspect = w / h; camera.updateProjectionMatrix(); };
    const observer = new ResizeObserver(resize); observer.observe(host); resize();
    let frame; const animate = () => { frame = requestAnimationFrame(animate); controls.update(); renderer.render(scene, camera); }; animate();
    const click = (event) => {
      if (kind !== 'csv' || !pointsObject) return;
      const rect = renderer.domElement.getBoundingClientRect(); const mouse = new THREE.Vector2((event.clientX - rect.left) / rect.width * 2 - 1, -((event.clientY - rect.top) / rect.height) * 2 + 1);
      const raycaster = new THREE.Raycaster(); raycaster.params.Points.threshold = Math.max(box.getSize(new THREE.Vector3()).length() * .018, .002); raycaster.setFromCamera(mouse, camera);
      const hit = raycaster.intersectObject(pointsObject)[0]; if (hit) onInspect?.(model.nodes[hit.index]);
    };
    renderer.domElement.addEventListener('click', click);
    return () => { cancelAnimationFrame(frame); observer.disconnect(); controls.dispose(); renderer.dispose(); renderer.domElement.removeEventListener('click', click); host.removeChild(renderer.domElement); objects.traverse((o) => { o.geometry?.dispose(); if (Array.isArray(o.material)) o.material.forEach((m) => m.dispose()); else o.material?.dispose(); }); };
  }, [model, kind, onInspect]);
  return <div className="viewport-wrap"><div className="viewport" ref={hostRef} />{model && <button className="fit-view" onClick={() => fitRef.current()}>Fit to view</button>}{model && <span className="viewport-hint">Drag to orbit · Shift-drag to pan · Scroll to zoom</span>}</div>;
}
