import socket, struct, os, hashlib
os.chdir(r'c:\Users\12159\Desktop\TOY')
s=socket.socket(); s.settimeout(10); s.connect(('118.25.42.70',9000))
# children of ZERO
s.sendall(struct.pack('>BI',5,32)+b'\x00'*32)
h=s.recv(5); st,ln=struct.unpack('>BI',h)
body=b''; 
while len(body)<ln: body+=s.recv(ln-len(body))
cnt=struct.unpack('>I',body[:4])[0] if ln>=4 else 0
with open('_diag.txt','w') as f:
    f.write(f'ZERO children: status={st} count={cnt}\n')
    if cnt>0:
        first=body[4:36]
        f.write(f'first hash={first.hex()}\n')
        s.sendall(struct.pack('>BI',3,32)+first)
        h=s.recv(5); s2,l2=struct.unpack('>BI',h)
        b2=b''; 
        while len(b2)<l2: b2+=s.recv(l2-len(b2))
        f.write(f'FILE status={s2} len={l2}\n')
        if s2==0:
            with open('_from_server.bin','wb') as f2: f2.write(b2)
            local=open('zero.bin','rb').read()
            f.write(f'local zero.bin={len(local)} sha256={hashlib.sha256(local).hexdigest()}\n')
            f.write(f'server block={len(b2)} sha256={hashlib.sha256(b2).hexdigest()}\n')
            f.write(f'match={local==b2}\n')
            if len(b2)<200: f.write(f'server block hex={b2.hex()}\n')
s.close()
print('done')