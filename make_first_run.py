import hashlib, os, socket, struct, sys
from pathlib import Path

ROOT=Path(__file__).resolve().parent
MODS=ROOT/'mods'
SERVER=('118.25.42.70',9000)

def hfile(p): return hashlib.sha256(Path(p).read_bytes()).digest()
def hx(b): return b.hex()
def instr(token,payload=b''): return token+struct.pack('<I',len(payload))+payload
def block(items): return b''.join(items)+b'\0'*32
def frame(s,op,body=b''):
    s.sendall(bytes([op])+len(body).to_bytes(4,'big')+body)
    hdr=s.recv(5)
    if len(hdr)<5: raise RuntimeError('short frame')
    st=hdr[0]; n=int.from_bytes(hdr[1:5],'big'); data=b''
    while len(data)<n:
        part=s.recv(n-len(data))
        if not part: raise RuntimeError('short body')
        data+=part
    return st,data
def upload(s,b):
    st,out=frame(s,2,b)
    if st or len(out)<32: raise RuntimeError(f'upload failed {st}')
    return out[:32]
def edge(s,p,c):
    st,out=frame(s,4,p+c)
    if st: raise RuntimeError(f'edge failed {st}')
def uset(s,user,key,val):
    st,out=frame(s,7,user+key+val)
    if st: raise RuntimeError(f'uset failed {st}')

def token_for(name):
    mf=ROOT/'mod_tokens.txt'
    if mf.exists():
        for line in mf.read_text().splitlines():
            parts=line.split()
            if len(parts)==2 and parts[0]==name: return bytes.fromhex(parts[1])
    direct=MODS/(name+'.dll')
    if direct.exists(): return hfile(direct)
    raise KeyError(name)

names=['editor_init','editor_frame','editor_should_halt','cond_payload','reexec','halt','ret']
tokens={n:token_for(n) for n in names}
# loop block: frame, should_halt, if true halt, reexec
loop_key=hashlib.sha256(b'#SingularityFirstRunLoop').digest()
loop=block([
    instr(tokens['editor_frame']),
    instr(tokens['editor_should_halt']),
    instr(tokens['cond_payload'], loop_key),
    instr(tokens['reexec']),
])
# halt block bound to same logical key in cond_payload payload mutation semantics is not suitable; create separate halt key.
halt_key=hashlib.sha256(b'#SingularityHalt').digest()
loop=block([
    instr(tokens['editor_frame']),
    instr(tokens['editor_should_halt']),
    instr(tokens['cond_payload'], halt_key),
    instr(tokens['reexec']),
])
halt_block=block([instr(tokens['halt'])])
first_key=hashlib.sha256(b'#SingularityFirstRun').digest()
first=block([instr(tokens['editor_init']), instr(loop_key), instr(tokens['ret'])])

(ROOT/'first_run_editor.block').write_bytes(first)
(ROOT/'first_run_loop.block').write_bytes(loop)
(ROOT/'first_run_halt.block').write_bytes(halt_block)
(ROOT/'first_block.bin').write_bytes(first_key)

user=(ROOT/'id.bin').read_bytes()[:32]
with socket.create_connection(SERVER,timeout=10) as s:
    first_hash=upload(s,first); loop_hash=upload(s,loop); halt_hash=upload(s,halt_block)
    edge(s,first_key,first_hash); edge(s,loop_key,loop_hash); edge(s,halt_key,halt_hash)
    uset(s,user,first_key,first_hash); uset(s,user,loop_key,loop_hash); uset(s,user,halt_key,halt_hash)
    # also expose tag path #TAG -> #editor -> executable tokens, where non-# children are instruction tokens.
    tag=hashlib.sha256(b'#TAG').digest(); editor_tag=upload(s,b'#editor')
    edge(s,tag,editor_tag)
    for name,tok in tokens.items():
        name_hash=upload(s,('#'+name).encode())
        edge(s,editor_tag,name_hash)
        edge(s,name_hash,tok)
print('first_key',hx(first_key))
for n,t in tokens.items(): print(n,hx(t))
