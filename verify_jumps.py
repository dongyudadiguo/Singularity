import struct
data=open('zero.bin','rb').read()
off=0
items=[]
while off+36<=len(data):
    token=data[off:off+32]
    if token==b'\x00'*32: break
    span=struct.unpack('<I',data[off+32:off+36])[0]
    name=token.rstrip(b'\x00').decode('ascii','ignore')
    payload=data[off+36:off+32+span]
    items.append((off,name,span))
    off+=32+span
# Find UI:POLL (LOOP label)
loop_off=None
for o,n,s in items:
    if n=='UI:POLL': loop_off=o; break
lines=[f'LOOP (UI:POLL) at offset: {loop_off}']
for o,n,s in items:
    if n in ('FLOW:JMP','FLOW:JZ','FLOW:JNZ') and s>=8:
        tgt=struct.unpack('<I',data[o+36:o+40])[0]
        is_loop='<-- LOOP' if tgt==loop_off else ''
        lines.append(f'  {o:5d} {n:10s} -> {tgt:5d} {is_loop}')
open('_verify.txt','w').write('\n'.join(lines))
print('done')