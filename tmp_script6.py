import re, math
from pathlib import Path
text = Path('source/DSP/EMUAuthenticTables.h').read_text()
body = re.search(r'AUTHENTIC_EMU_SHAPES\[32\]\[12\]\s*=\s*\{([\s\S]*?)\};', text).group(1)
nums = [float(x.replace('f','')) for x in re.findall(r'-?\d+\.\d+f?', body)]
shapes = [nums[i*12:(i+1)*12] for i in range(32)]
pairs_body = re.search(r'MORPH_PAIRS\[6\]\[2\]\s*=\s*\{([\s\S]*?)\};', text).group(1)
pairs = [(int(a),int(b)) for a,b in re.findall(r'\{\s*(\d+)\s*,\s*(\d+)\s*\}', pairs_body)]
AUTH_SR = 44100.0
PI = math.pi
kThetaSwing = 0.35
kRadiusSwing = 0.08

def clamp(v, lo, hi): return max(lo, min(hi, v))

def convert_pole_to_section(r, theta):
    freqHz = theta * AUTH_SR / (2.0 * PI)
    freqHz = clamp(freqHz, 20.0, 20000.0)
    k1 = math.log10(freqHz/20.0)/3.0
    k1 = clamp(k1, 0.0, 1.0)
    k2 = clamp(r/0.98, 0.0, 1.0)
    return {'k1': k1, 'k2': k2, 'gain': 1.0}

def load_polar(idx):
    arr = shapes[idx]
    poles = [(arr[i*2], arr[i*2+1]) for i in range(6)]
    poles.append((0.99, 0.0))
    return poles

def convert_to_cube(idxA, idxB):
    shapeA = load_polar(idxA)
    shapeB = load_polar(idxB)
    cube = []
    for corner in range(8):
        x = 1 if (corner & 1) else 0
        y = 1 if (corner & 2) else 0
        z = 1 if (corner & 4) else 0
        base = shapeB if x else shapeA
        frame = []
        for r, theta in base:
            theta = clamp(theta + (kThetaSwing if y else -kThetaSwing), 0.0, math.pi)
            r = clamp(r * (1.0 + (kRadiusSwing if z else -kRadiusSwing)), 0.10, 0.995)
            frame.append(convert_pole_to_section(r, theta))
        cube.append(frame)
    return cube

def lerp(a,b,t): return a + t*(b-a)

def interpolate_cube(cube, x,y,z):
    frame=[]
    for s in range(7):
        k1_vals=[corner[s]['k1'] for corner in cube]
        k2_vals=[corner[s]['k2'] for corner in cube]
        g_vals=[corner[s]['gain'] for corner in cube]
        c000,c100,c010,c110,c001,c101,c011,c111 = (k1_vals[i] for i in range(8))
        def tri(vals):
            c000,c100,c010,c110,c001,c101,c011,c111 = vals
            c00 = lerp(c000, c100, x)
            c10 = lerp(c010, c110, x)
            c01 = lerp(c001, c101, x)
            c11 = lerp(c011, c111, x)
            c0 = lerp(c00, c10, y)
            c1 = lerp(c01, c11, y)
            return lerp(c0, c1, z)
        k1 = tri((c000,c100,c010,c110,c001,c101,c011,c111))
        k2 = tri(tuple(k2_vals))
        g  = tri(tuple(g_vals))
        frame.append({'k1':k1,'k2':k2,'gain':g})
    return frame

def freq_response(coeffs, omega):
    z1 = math.cos(-omega) + 1j*math.sin(-omega)
    z2 = math.cos(-2*omega) + 1j*math.sin(-2*omega)
    num = coeffs[0] + coeffs[1]*z1 + coeffs[2]*z2
    den = 1 - (coeffs[3]*z1 + coeffs[4]*z2)
    return num/den

def decode_section(sec, sample_rate=48000.0):
    R = (sec['k2']**1.35) * 0.995
    R = clamp(R, 0.10, 0.995)
    freqHz = 20.0 * (1000.0 ** sec['k1'])
    freqHz = clamp(freqHz, 20.0, sample_rate*0.49)
    theta = 2.0*math.pi*freqHz/sample_rate
    b1 = 2.0*R*math.cos(theta)
    b2 = -(R*R)
    alpha = (1.0 - R)
    a0 = alpha * sec['gain']
    a1 = 0.0
    a2 = -alpha * sec['gain']
    return (a0,a1,a2,b1,b2)

def probe_avg(mode_idx=0, morph=0.5, freq=0.5, trans=0.5, sr=48000.0, sections=7):
    a,b=pairs[mode_idx]
    cube=convert_to_cube(a,b)
    frame=interpolate_cube(cube,morph,freq,trans)
    coeffs=[decode_section(sec,sr) for sec in frame][:sections]
    probe=[80,200,500,1200,2400,5000,10000]
    mags=[]
    for f in probe:
        omega=2*math.pi*f/sr
        H=1+0j
        for c in coeffs:
            H*=freq_response(c,omega)
        mags.append(20*math.log10(abs(H)+1e-12))
    return sum(mags)/len(mags), max(mags), min(mags)

for sections in [7,4,3,2,1]:
    avg,mx,mn=probe_avg(mode_idx=2, sections=sections)
    print('sections',sections,'avg',avg,'max',mx,'min',mn)
