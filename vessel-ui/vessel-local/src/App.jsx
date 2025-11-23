import React, { useState, useEffect, useRef } from 'react';
import { Hexagon, Activity, Zap, Power, ChevronUp, ChevronDown } from 'lucide-react';

// MAIN COMPONENT (Must be named App for preview)
export default function App() {
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
      ctx.fillStyle = '#dcdcd9'; 
      ctx.fillRect(0, 0, w, h);

      if (bypass) {
          ctx.fillStyle = 'rgba(0,0,0,0.1)';
          ctx.font = 'bold 20px monospace';
          ctx.textAlign = 'center';
          ctx.fillText("DISENGAGED", w/2, h/2 + 6);
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
      ctx.fillStyle = '#151515';
      
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
      ctx.fillStyle = 'rgba(0,0,0,0.4)';
      ctx.font = '9px monospace';
      ctx.textAlign = 'left';
      ctx.fillText(`FLUX: ${(fVal*100).toFixed(0)}%`, 10, h - 12);
      ctx.textAlign = 'right';
      ctx.fillText(`Z-POS: ${(mVal*100).toFixed(0)}`, w - 10, h - 12);

      time += 0.02;
      frameId = requestAnimationFrame(render);
    };
    render();
    return () => cancelAnimationFrame(frameId);
  }, [bypass]);

  return (
    <div className="min-h-screen bg-[#b0b0b0] flex items-center justify-center font-sans p-8 select-none">
      
      {/* CHASSIS: Ceramic White with Noise Texture */}
      <div className="w-[760px] bg-[#efefed] rounded-[3px] shadow-[0_50px_100px_-20px_rgba(0,0,0,0.35),0_0_0_1px_rgba(255,255,255,0.2)] flex flex-col relative overflow-hidden border-t border-white/60">
        
        {/* Texture Overlay (Crucial for the 'Hardware' look) */}
        <div className="absolute inset-0 opacity-[0.04] pointer-events-none z-50 mix-blend-multiply" 
             style={{backgroundImage: `url("data:image/svg+xml,%3Csvg viewBox='0 0 200 200' xmlns='http://www.w3.org/2000/svg'%3E%3Cfilter id='noise'%3E%3CfeTurbulence type='fractalNoise' baseFrequency='0.8' numOctaves='3' stitchTiles='stitch'/%3E%3C/filter%3E%3Crect width='100%25' height='100%25' filter='url(%23noise)'/%3E%3C/svg%3E")`}}>
        </div>

        {/* --- UPPER DECK: MONITORING & CALIBRATION --- */}
        <div className="h-[260px] flex bg-[#f7f7f5]">
            
            {/* LEFT: SCREEN (Monitoring) */}
            <div className="flex-1 p-8 flex flex-col justify-center relative border-r border-[#e0e0e0]">
                <Screw tl /> <Screw bl />
                
                {/* Screen Header */}
                <div className="flex justify-between items-center mb-3 px-1">
                    <Label icon={<Activity size={10}/>}>Core Monitor</Label>
                    <div className={`w-1.5 h-1.5 rounded-full transition-all duration-300 ${bypass ? 'bg-[#ccc]' : 'bg-[#00ff41] shadow-[0_0_6px_#00ff41]'}`}></div>
                </div>
                
                {/* Deep Inset Screen */}
                <div className="flex-1 bg-[#e8e8e6] rounded-[2px] shadow-[inset_0_2px_8px_rgba(0,0,0,0.1)] p-[4px] border border-[#fff] relative">
                    <div className="w-full h-full bg-[#dcdcd9] relative overflow-hidden border border-[#bfbfbd] rounded-[1px]">
                        <canvas ref={canvasRef} className="w-full h-full opacity-90 mix-blend-multiply" />
                        <div className="absolute inset-0 bg-gradient-to-br from-white/20 to-transparent pointer-events-none"></div>
                    </div>
                </div>
            </div>

            {/* RIGHT: THE LAB (Controls) */}
            <div className="w-[280px] bg-[#f0f0ee] p-8 flex flex-col justify-between relative">
                <Screw tr /> <Screw br />

                {/* 1. PRESET & SOURCE */}
                <div className="flex justify-between items-start border-b border-[#e0e0e0] pb-5">
                    <div className="flex flex-col gap-2">
                        <Label>Preset</Label>
                        <div className="bg-[#e2e2e0] rounded-[2px] border border-[#dcdcdc] shadow-inner flex items-center p-1 w-20 justify-between">
                             <div className="font-mono text-[10px] font-bold text-[#333] pl-1">P-{preset.toString().padStart(2, '0')}</div>
                             <div className="flex flex-col">
                                 <button onClick={() => changePreset(1)} className="text-[#888] hover:text-[#333]"><ChevronUp size={8}/></button>
                                 <button onClick={() => changePreset(-1)} className="text-[#888] hover:text-[#333]"><ChevronDown size={8}/></button>
                             </div>
                        </div>
                    </div>
                    
                    <ControlGroup label="DRIVE">
                        <TrimmerKnob value={drive} onChange={setDrive} />
                    </ControlGroup>
                </div>

                {/* 2. SHAPING BLOCK */}
                <div className="flex justify-between items-start pt-1">
                    <ControlGroup label="FREQ">
                        <TrimmerKnob value={cutoff} onChange={setCutoff} />
                    </ControlGroup>
                    <ControlGroup label="RESO">
                        <TrimmerKnob value={res} onChange={setRes} />
                    </ControlGroup>
                </div>

                {/* 3. MODE BLOCK */}
                <div className="space-y-2 mt-1">
                    <Label icon={<Zap size={10}/>}>Circuit</Label>
                    <SlideSwitch value={circuit} onChange={setCircuit} options={["LIN", "SAT", "FLD"]} />
                </div>
            </div>
        </div>

        {/* --- LOWER DECK: PERFORMANCE --- */}
        <div className="h-[120px] bg-[#e6e6e4] border-t border-[#d4d4d2] relative flex flex-col justify-center px-10 shadow-[inset_0_3px_6px_rgba(0,0,0,0.03)]">
            
            <div className="flex items-center gap-8">
                
                {/* FLUX KNOB (LFO/Movement) - Placed next to Morph for "Motion" context */}
                <div className="flex flex-col items-center gap-2 w-16">
                    <Knob value={flux} onChange={setFlux} size="medium" />
                    <span className="text-[9px] font-bold text-[#999] tracking-widest">FLUX</span>
                </div>

                {/* MORPH SLIDER (The Main Event) */}
                <div className="flex-1 flex flex-col gap-2 pt-1">
                    <div className="flex justify-between text-[8px] font-bold text-[#aaa] px-1 font-mono">
                        <span>START</span>
                        <span>INTERPOLATION</span>
                        <span>END</span>
                    </div>
                    <PrecisionFader value={morph} onChange={setMorph} />
                </div>

            </div>
            
            {/* Output Trim (Hidden in plain sight - Bottom Right) */}
            <div className="absolute bottom-3 right-10 flex items-center gap-4">
                <div className="flex items-center gap-2 opacity-50 hover:opacity-100 transition-opacity group">
                    <span className="text-[8px] font-bold uppercase">Out</span>
                    <TrimmerKnob value={output} onChange={setOutput} width="w-12" />
                </div>
                <Power 
                    size={12} 
                    className={`cursor-pointer transition-colors ${bypass ? 'text-[#bbb]' : 'text-[#333]'}`}
                    onClick={() => setBypass(!bypass)}
                />
            </div>
        </div>

      </div>
    </div>
  );
};

// --- ATOMIC COMPONENTS ---

const Label = ({ children, icon }) => (
    <div className="text-[9px] font-bold text-[#999] uppercase tracking-widest flex items-center gap-2 mb-1 select-none">
        {icon}
        {children}
    </div>
);

const ControlGroup = ({ label, children }) => (
    <div className="flex flex-col gap-2 group w-[45%]">
        <span className="text-[9px] font-bold text-[#999] group-hover:text-[#555] transition-colors pl-1">{label}</span>
        {children}
    </div>
);

const Screw = ({ tl, tr, bl, br }) => {
    const pos = tl ? "top-3 left-3" : tr ? "top-3 right-3" : bl ? "bottom-3 left-3" : "bottom-3 right-3";
    return (
        <div className={`absolute ${pos} w-2.5 h-2.5 rounded-full bg-[#dcdcdc] flex items-center justify-center shadow-[inset_1px_1px_1px_#fff,0_1px_1px_rgba(0,0,0,0.1)] opacity-60`}>
             <div className="w-[1px] h-[60%] bg-[#aaa] rotate-45"></div>
             <div className="h-[1px] w-[60%] bg-[#aaa] rotate-45 absolute"></div>
        </div>
    );
}

// 1. HERO FADER (Mechanical Snap)
const PrecisionFader = ({ value, onChange }) => {
    const handleDrag = (e) => {
        if(e.buttons !== 1) return;
        const rect = e.currentTarget.getBoundingClientRect();
        let v = Math.max(0, Math.min(100, ((e.clientX - rect.left) / rect.width) * 100));
        // 5% Snap logic for tactile feel
        v = Math.round(v / 5) * 5;
        onChange(v);
    };

    return (
        <div 
            className="h-8 w-full relative cursor-ew-resize group"
            onMouseMove={handleDrag}
            onMouseDown={handleDrag}
        >
            {/* Track */}
            <div className="absolute top-1/2 left-0 w-full h-[4px] bg-[#ccc] -translate-y-1/2 rounded-full shadow-[inset_0_1px_2px_rgba(0,0,0,0.1)]"></div>
            <div className="absolute top-1/2 left-0 h-[4px] bg-[#333] -translate-y-1/2 rounded-full" style={{width: `${value}%`}}></div>
            
            {/* Ticks */}
            <div className="absolute top-1/2 left-0 w-full h-3 -translate-y-1/2 pointer-events-none flex justify-between px-[1px]">
               {[...Array(21)].map((_, i) => (
                   <div key={i} className={`w-[1px] bg-[#999] ${i % 5 === 0 ? 'h-3' : 'h-1 opacity-50'}`}></div>
               ))}
            </div>

            {/* Cap */}
            <div 
                className="absolute top-1/2 h-5 w-10 bg-[#fcfcfc] -translate-y-1/2 -translate-x-1/2 rounded-[1px] shadow-[0_2px_4px_rgba(0,0,0,0.2),inset_0_1px_0_#fff,0_0_0_1px_rgba(0,0,0,0.05)] flex flex-col items-center justify-center z-10 transition-transform active:scale-95 border border-[#ddd]"
                style={{ left: `${value}%` }}
            >
                <div className="absolute top-[-2px] w-[2px] h-[4px] bg-[#ff4400]"></div>
                <div className="w-full h-[1px] bg-[#e0e0e0]"></div>
            </div>
        </div>
    );
};

// 2. FLUSH KNOB (Used for Flux)
const Knob = ({ value, onChange, size="small" }) => {
    const handleDrag = (e) => {
        if(e.buttons !== 1) return;
        const rect = e.currentTarget.getBoundingClientRect();
        const v = Math.max(0, Math.min(100, 100 - ((e.clientY - rect.top)/rect.height)*100));
        onChange(Math.round(v));
    };

    return (
        <div 
            className={`w-full h-8 bg-[#e8e8e6] rounded-full relative cursor-ns-resize flex items-center justify-center shadow-[0_2px_5px_rgba(0,0,0,0.1),inset_0_1px_0_#fff] border border-[#dcdcdc] hover:border-[#ccc] transition-colors`}
            onMouseMove={handleDrag}
        >
            <div className="absolute top-0 left-0 w-full h-full">
                 <div 
                    className="w-1.5 h-1.5 bg-[#333] rounded-full absolute top-[20%] left-1/2 -translate-x-1/2 origin-[50%_150%]" 
                    style={{ transform: `translateX(-50%) rotate(${(value * 2.8) - 140}deg)` }}
                 ></div>
            </div>
        </div>
    );
};

// 3. TRIMMER KNOB (Used for Calibration)
const TrimmerKnob = ({ value, onChange, width="w-full" }) => {
    const handleDrag = (e) => {
        if(e.buttons !== 1) return;
        const rect = e.currentTarget.getBoundingClientRect();
        const v = Math.max(0, Math.min(100, 100 - ((e.clientY - rect.top)/rect.height)*100));
        onChange(Math.round(v));
    };

    return (
        <div 
            className={`${width} h-7 bg-[#e2e2e0] rounded-[2px] border border-[#dcdcdc] relative cursor-ns-resize flex items-center px-1 shadow-[inset_0_1px_2px_rgba(0,0,0,0.06)] overflow-hidden hover:border-[#ccc] transition-colors`}
            onMouseMove={handleDrag}
        >
            <div className="h-full bg-[#d4d4d2] absolute left-0 top-0 transition-all duration-75 ease-out" style={{ width: `${value}%` }}></div>
            <div className="relative z-10 text-[9px] font-bold text-[#444] w-full text-right pr-2 font-mono select-none">{Math.floor(value)}</div>
            <div className="absolute top-0 bottom-0 w-[2px] bg-[#ff4400] transition-all duration-75 ease-out shadow-[0_0_2px_rgba(0,0,0,0.1)]" style={{ left: `${value}%` }}></div>
        </div>
    );
};

// 4. SLIDE SWITCH (Modes)
const SlideSwitch = ({ value, onChange, options }) => {
    return (
        <div className="relative w-full h-6 bg-[#e0e0de] rounded-[2px] shadow-[inset_0_1px_2px_rgba(0,0,0,0.1)] flex items-center p-[2px] cursor-pointer border border-[#fff]">
            <div 
                className="absolute top-[2px] bottom-[2px] w-[33%] bg-[#fcfcfc] rounded-[1px] shadow-[0_1px_2px_rgba(0,0,0,0.15)] transition-all duration-150 ease-out z-10 border border-[#eee]"
                style={{ left: `${value * 33.33}%` }}
            ></div>
            {options.map((opt, i) => (
                <div 
                    key={i} 
                    className={`flex-1 text-center text-[8px] font-bold z-20 relative select-none transition-colors ${value === i ? 'text-[#222]' : 'text-[#999]'}`}
                    onClick={() => onChange(i)}
                >
                    {opt}
                </div>
            ))}
        </div>
    );
};