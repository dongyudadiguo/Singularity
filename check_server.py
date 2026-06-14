import socket, struct, os, hashlib
os.chdir(r'c:\Users\12159\Desktop\TOY')
s=socket.socket(); s.settimeout(8); s.connect(('118.25.42.70',9000))
# children of ZERO
s.sendall(struct.pack('>BI',5,32)+b'\x00'*32)
h=s.recv(5); st,ln=struct.unpack('>BI',h)
body=b''
while len(body)<ln: body+=s.recv(ln-len(body))
cnt=struct.unpack('>I',body[:4])[0] if ln>=4 else 0
print(f'ZERO children count={cnt} status={st}')
local_sha=hashlib.sha256(open('zero.bin','rb').read()).hexdigest()
for i in range(cnt):
    off=4+i*40
    hh=body[off:off+32].hex()
    # fetch file
    s2=socket.socket(); s2.settimeout(8); s2.connect(('118.25.42.70',9000))
    s2.sendall(struct.pack('>BI',3,32)+bytes.fromhex(hh))
    h2=s2.recv(5); s2s,l2=struct.unpack('>BI',h2)
    b2=b''; 
    while len(b2)<l2: b2+=s2.recv(l2-len(b2))
    sha=hashlib.sha256(b2).hexdigest()
    is_current=' <- CURRENT' if sha==local_sha else ''
    # check first JMP target in this block
    if s2s==0 and l2>=945:
        # find FLOW:JMP at what should be offset 865
        data=b2
        oo=0; jmp_targets=[]
        while oo+36<=len(data):
            tok=data[oo:oo+32]
            if tok==b'\x00'*32: break
            sp=struct.unpack('<I',data[oo+32:oo+36])[0]
            nm=tok.rstrip(b'\x00').decode('ascii','ignore')
            if nm in ('FLOW:JMP','FLOW:JZ','FLOW:JNZ') and sp>=8:
                tgt=struct.unpack('<I',data[oo+36:oo+40])[0]
                jmp_targets.append(f'{nm}@{oo}->{tgt}')
            oo+=32+sp
        print(f'  child[{i}] hash={hh} sha={sha}{is_current} jumps={jmp_targets[:3]}...')
    else:
        print(f'  child[{i}] hash={hh} sha={sha}{is_current}')
    s2.close()
s.close()
print('done')