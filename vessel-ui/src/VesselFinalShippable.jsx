import React, { useState, useEffect, useRef } from 'react';
import { Activity, Power, ChevronUp, ChevronDown, X, Waveform } from 'lucide-react';

const VesselFinalShippable = () => {
  // --- GOLD MASTER STATE ---
  const [morph, setMorph] = useState(50);     // Main Vowel Position
  const [flux, setFlux] = useState(15);       // Internal Random Modulation
  const [cutoff, setCutoff] = useState(65);   // Filter Freq
  const [res, setRes] = useState(30);         // Resonance
  const [drive, setDrive] = useState(20);     // Input Saturation
  const [output, setOutput] = useState(80);   // Output Trim
  const [circuit, setCircuit] = useState(0);  // 0: LIN, 1: SAT, 2: FLD
  const [preset, setPreset] = useState(1);    // Preset Memory
  const [bypass, setBypass] = useState(false);
  
  const canvasRef = useRef(null);
  const params = useRef({ morph, cutoff, res, flux });
  
  // Sync refs for animation loop
  useEffect(() => { params.current = { morph, cutoff, res, flux }; }, [morph, cutoff, res, flux]);

  // --- PRESET LOGIC ---
  const changePreset = (dir) => {
      setPreset(p => {
          const next = p + dir;
          return next > 99 ? 1 : next < 1 ? 99 : next;
      });
  };

  // --- VISUALIZER: FERROFLUID FLUX ENGINE ---
  useEffect(() => {
    const canvas = canvasRef.current;
    const ctx = canvas.getContext('2d', { alpha: false });
    let frameId;
    let time = 0;

    // 2x Resolution for retina sharpness, scaled down by CSS
    const w = 320;
    const h = 160;
    canvas.width = w;
    canvas.height = h;

    const render = () => {
      // 1. Background (Ceramic/E-ink Grey)
      ctx.fillStyle = '#c8c8c6'; 
      ctx.fillRect(0, 0, w, h);

      if (bypass) {
          ctx.fillStyle = 'rgba(0,0,0,0.1)';
          ctx.font = 'bold 20px monospace';
          ctx.textAlign = 'center';
          ctx.fillText("OFFLINE", w/2, h/2 + 6);
          frameId = requestAnimationFrame(render);
          return;
      }

      const { morph, res, cutoff, flux } = params.current;
      const mVal = morph / 100;
      const rVal = res / 100;
      const cVal = cutoff / 100;
      const fVal = flux / 100;

      // 2. Physics: 14 Magnetic Poles
      const points = [];
      const count = 14;
      
      for(let k=0; k<count; k++) {
          // Flux adds jitter/life to the movement speed
          const agitation = 0.05 + (rVal * 0.1) + (fVal * 0.3); 
          const t = time * agitation + (k * 0.7);
          
          // Spread logic
          const spreadX = w * (0.12 + cVal * 0.45);
          const spreadY = h * (0.25 + fVal * 0.2); // Flux expands vertical range

          // Lissajous Movement (The Morph)
          const px = (w/2) + Math.sin(t + (mVal * Math.PI) + k) * spreadX;
          const py = (h/2) + Math.cos(t * 0.8 + k) * spreadY;
          points.push({x: px, y: py});
      }

      // 3. Render Ink (Thresholding)
      ctx.fillStyle = '#2a2a2a';
      
      // Optimization: Scanline skip for "screen" texture
      for(let y=0; y<h; y+=2) {
          for(let x=0; x<w; x+=2) {
              let force = 0;
              for(let p of points) {
                  const dx = x - p.x;
                  const dy = y - p.y;
                  // Force Field Function
                  force += (1000 * (1 + rVal)) / (dx*dx + dy*dy + 1); 
              }

              // Threshold Layers
              if (force > 18.0) {
                  ctx.fillRect(x, y, 2, 2); // Solid Core
              } else if (force > 6.0) {
                  // Dithered Halo (Bayer Pattern)
                  if ((x/2 + y/2) % 2 === 0) ctx.fillRect(x, y, 2, 2);
              }
          }
      }

      // 4. Data Overlay
      ctx.fillStyle = 'rgba(0,0,0,0.3)';
      ctx.font = '10px monospace';
      ctx.textAlign = 'left';
      ctx.fillText(`FLUX: ${(fVal*100).toFixed(0)}%`, 14, h - 14);
      ctx.textAlign = 'right';
      ctx.fillText(`Z-POS: ${(mVal*100).toFixed(0)}`, w - 14, h - 14);

      time += 0.02;
      frameId = requestAnimationFrame(render);
    };
    render();
    return () => cancelAnimationFrame(frameId);
  }, [bypass]);

  return (
    <div className="min-h-screen bg-[#f0f0f0] flex items-center justify-center font-sans p-8 select-none">
      
      {/* CHASSIS: Clean White/Grey Industrial */}
      <div className="w-[820px] bg-[#f4f4f4] rounded-sm shadow-[0_40px_80px_-20px_rgba(0,0,0,0.15),0_0_0_1px_rgba(0,0,0,0.05)] flex flex-col relative overflow-hidden">
        
        {/* --- HEADER STRIP --- */}
        <div className="h-10 bg-[#f8f8f8] border-b border-[#e5e5e5] flex justify-between items-center px-4">
            <div className="flex items-center gap-3">
                 <div className="w-4 h-4 rounded-full bg-[#e0e0e0] flex items-center justify-center text-[#999]"><X size={10} strokeWidth={3}/></div>
                 <Activity size={14} className="text-[#666]" />
            </div>
            <div className={`w-2 h-2 rounded-full transition-all duration-300 ${bypass ? 'bg-[#ddd]' : 'bg-[#00ff41] shadow-[0_0_8px_#00ff41]'}`}></div>
        </div>

        {/* --- UPPER DECK: MONITOR & CONTROLS --- */}
        <div className="h-[320px] flex bg-[#fbfbfb]">
            
            {/* LEFT: SCREEN AREA */}
            <div className="flex-1 p-8 flex items-center justify-center border-r border-[#ececec]">
                <div className="bg-[#dcdcdc] p-2 shadow-inner rounded-sm w-full h-full max-h-[220px] flex items-center justify-center">
                    <div className="w-full h-full bg-[#c8c8c6] relative overflow-hidden border border-[#bbb] shadow-[inset_0_2px_4px_rgba(0,0,0,0.1)]">
                        <canvas ref={canvasRef} className="w-full h-full opacity-90 mix-blend-multiply" />
                    </div>
                </div>
            </div>

            {/* RIGHT: CONTROL LAB */}
            <div className="w-[340px] bg-[#f7f7f7] p-8 flex flex-col justify-center gap-8">
                
                {/* ROW 1: PRESET & DRIVE */}
                <div className="flex justify-between items-end">
                    <div className="flex flex-col gap-2">
                        <Label>PRESET</Label>
                        <div className="bg-[#e8e8e8] h-[36px] w-28 rounded flex items-center justify-between px-3 border border-[#dcdcdc] shadow-sm">
                             <span className="font-bold text-[#333] text-sm">P-{preset.toString().padStart(2, '0')}</span>
                             <div className="flex flex-col -mr-1">
                                 <button onClick={() => changePreset(1)} className="text-[#999] hover:text-[#555] active:text-black"><ChevronUp size={12}/></button>
                                 <button onClick={() => changePreset(-1)} className="text-[#999] hover:text-[#555] active:text-black"><ChevronDown size={12}/></button>
                             </div>
                        </div>
                    </div>
                    <div className="flex flex-col gap-2 w-32">
                        <Label>DRIVE</Label>
                        <TrimmerKnob value={drive} onChange={setDrive} />
                    </div>
                </div>

                {/* ROW 2: FREQ & RESO */}
                <div className="flex justify-between items-end border-t border-[#eee] pt-6">
                    <div className="flex flex-col gap-2 w-32">
                        <Label>FREQ</Label>
                        <TrimmerKnob value={cutoff} onChange={setCutoff} />
                    </div>
                    <div className="flex flex-col gap-2 w-32">
                        <Label>RESO</Label>
                        <TrimmerKnob value={res} onChange={setRes} />
                    </div>
                </div>

                {/* ROW 3: CIRCUIT */}
                <div className="flex flex-col gap-2 border-t border-[#eee] pt-6">
                    <Label icon={<Zap size={10} className="mr-1"/>}>CIRCUIT</Label>
                    <SlideSwitch value={circuit} onChange={setCircuit} options={["LIN", "SAT", "FLD"]} />
                </div>

            </div>
        </div>

        {/* --- LOWER DECK: PERFORMANCE STRIP --- */}
        <div className="h-[140px] bg-[#ebebeb] border-t border-[#dcdcdc] relative flex flex-col justify-center px-12 shadow-[inset_0_4px_10px_rgba(0,0,0,0.03)]">
            
            <div className="flex items-center gap-10 h-full">
                
                {/* FLUX CONTROL (Left) */}
                <div className="flex flex-col items-center gap-3">
                    <FluxKnob value={flux} onChange={setFlux} />
                    <span className="text-[10px] font-bold text-[#888] tracking-widest uppercase">Flux</span>
                </div>

                {/* MAIN FADER (Center) */}
                <div className="flex-1 flex flex-col gap-3 pt-2">
                    <div className="flex justify-between text-[9px] font-bold text-[#999] px-0.5 tracking-wider">
                        <span>START</span>
                        <span>INTERPOLATION</span>
                        <span>END</span>
                    </div>
                    <PrecisionFader value={morph} onChange={setMorph} />
                </div>

                {/* OUTPUT & POWER (Right) */}
                <div className="flex flex-col items-end gap-3 pl-4">
                     <div className="flex items-center gap-3">
                        <span className="text-[10px] font-bold text-[#888] tracking-widest uppercase">OUT</span>
                        <TrimmerKnob value={output} onChange={setOutput} width="w-16" />
                     </div>
                     <div className="pt-2">
                        <Power 
                            size={18} 
                            className={`cursor-pointer transition-colors duration-200 ${bypass ? 'text-[#999]' : 'text-[#333]'}`}
                            onClick={() => setBypass(!bypass)}
                        />
                     </div>
                </div>

            </div>
            
        </div>

        {/* --- TEXTURE OVERLAY --- */}
        <div className="absolute inset-0 opacity-[0.03] pointer-events-none z-50 mix-blend-multiply" 
             style={{backgroundImage: `url("data:image/svg+xml,%3Csvg viewBox='0 0 200 200' xmlns='http://www.w3.org/2000/svg'%3E%3Cfilter id='noise'%3E%3CfeTurbulence type='fractalNoise' baseFrequency='0.8' numOctaves='3' stitchTiles='stitch'/%3E%3C/filter%3E%3Crect width='100%25' height='100%25' filter='url(%23noise)'/%3E%3C/svg%3E")`}}>
        </div>

      </div>
    </div>
  );
};

// --- COMPONENT LIBRARY ---

const Label = ({ children, icon }) => (
    <div className="text-[10px] font-bold text-[#999] uppercase tracking-widest flex items-center select-none">
        {icon}
        {children}
    </div>
);

const Zap = ({size, className}) => (
    <svg width={size} height={size} viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2.5" strokeLinecap="round" strokeLinejoin="round" className={className}><polygon points="13 2 3 14 12 14 11 22 21 10 12 10 13 2"></polygon></svg>
);

// 1. PRECISION FADER (Long Horizontal)
const PrecisionFader = ({ value, onChange }) => {
    const handleDrag = (e) => {
        if(e.buttons !== 1) return;
        const rect = e.currentTarget.getBoundingClientRect();
        let v = Math.max(0, Math.min(100, ((e.clientX - rect.left) / rect.width) * 100));
        onChange(v); // Smooth, no snap
    };

    return (
        <div 
            className="h-10 w-full relative cursor-ew-resize group flex items-center"
            onMouseMove={handleDrag}
            onMouseDown={handleDrag}
        >
            {/* Track Line */}
            <div className="absolute w-full h-[3px] bg-[#ccc] rounded-full overflow-hidden">
                 <div className="h-full bg-[#444]" style={{width: `${value}%`}}></div>
            </div>
            
            {/* Ticks */}
            <div className="absolute w-full h-full pointer-events-none flex justify-between px-[1px] items-center">
               {[...Array(21)].map((_, i) => (
                   <div key={i} className={`w-[1px] bg-[#999] ${i % 10 === 0 ? 'h-4 opacity-80' : i % 5 === 0 ? 'h-2.5 opacity-60' : 'h-1.5 opacity-30'}`}></div>
               ))}
            </div>

            {/* Handle */}
            <div 
                className="absolute w-12 h-6 bg-white border border-[#ccc] shadow-[0_2px_5px_rgba(0,0,0,0.15)] rounded-[2px] flex items-center justify-center z-10 -translate-x-1/2 cursor-grab active:cursor-grabbing hover:border-[#bbb] transition-transform active:scale-95"
                style={{ left: `${value}%` }}
            >
                <div className="w-[2px] h-[10px] bg-[#ff4400] opacity-80"></div>
            </div>
        </div>
    );
};

// 2. TRIMMER KNOB (Linear Slider style)
const TrimmerKnob = ({ value, onChange, width="w-full" }) => {
    const handleDrag = (e) => {
        if(e.buttons !== 1) return;
        const rect = e.currentTarget.getBoundingClientRect();
        const v = Math.max(0, Math.min(100, ((e.clientX - rect.left) / rect.width) * 100));
        onChange(Math.round(v));
    };

    return (
        <div 
            className={`${width} h-[36px] bg-[#e6e6e6] rounded-[3px] relative cursor-ew-resize flex items-center overflow-hidden hover:bg-[#e2e2e2] transition-colors`}
            onMouseMove={handleDrag}
            onMouseDown={handleDrag}
        >
            {/* Indicator Line */}
            <div className="absolute top-2 bottom-2 w-[3px] bg-[#ff4400] z-10 pointer-events-none shadow-[0_0_2px_rgba(0,0,0,0.1)] transition-all duration-75" style={{ left: `calc(${value}% - 1.5px)` }}></div>
            
            {/* Value Text */}
            <div className="absolute right-3 text-xs font-bold text-[#444] pointer-events-none tabular-nums opacity-60">{Math.round(value)}</div>
        </div>
    );
};

// 3. FLUX KNOB (Circular Capsule)
const FluxKnob = ({ value, onChange }) => {
    const handleDrag = (e) => {
        if(e.buttons !== 1) return;
        const rect = e.currentTarget.getBoundingClientRect();
        // Drag up/down logic
        const delta = ((e.movementY) * -1); 
        const newVal = Math.max(0, Math.min(100, value + delta));
        onChange(newVal);
    };

    return (
        <div 
            className="w-16 h-8 bg-[#e8e8e8] rounded-full border border-[#dcdcdc] shadow-[inset_0_1px_3px_rgba(0,0,0,0.05)] flex items-center p-1 relative cursor-ns-resize group hover:border-[#ccc]"
            onMouseMove={handleDrag}
            onMouseDown={handleDrag}
        >
            <div 
                className="h-full aspect-square rounded-full bg-[#333] shadow-sm transform transition-transform duration-75 ease-out"
                style={{ transform: `translateX(${value * 0.32}px)` }} // 32px is roughly range inside 64px width
            ></div>
        </div>
    );
};

// 4. SLIDE SWITCH (Segmented Control)
const SlideSwitch = ({ value, onChange, options }) => {
    return (
        <div className="w-full h-[36px] bg-[#e6e6e6] rounded-[3px] flex items-center p-1 cursor-pointer">
            <div className="relative w-full h-full flex items-center">
                 {/* Moving Background */}
                <div 
                    className="absolute top-0 bottom-0 w-[33.33%] bg-white rounded-[2px] shadow-sm transition-all duration-200 ease-out border border-[#ddd]"
                    style={{ left: `${value * 33.33}%` }}
                ></div>
                
                {/* Labels */}
                {options.map((opt, i) => (
                    <div 
                        key={i} 
                        className={`flex-1 text-center text-[10px] font-bold z-10 transition-colors uppercase tracking-wide select-none ${value === i ? 'text-[#222]' : 'text-[#888]'}`}
                        onClick={() => onChange(i)}
                    >
                        {opt}
                    </div>
                ))}
            </div>
        </div>
    );
};

export default VesselFinalShippable;