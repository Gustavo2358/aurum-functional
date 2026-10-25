"""Mechanical ctypes ABI binding. This module contains no financial policy."""
from pathlib import Path
import ctypes as C
import re,os
ROOT=Path(__file__).resolve().parents[1]
header=re.sub(r'/\*.*?\*/','', (ROOT/'subject/include/aurum.h').read_text(), flags=re.S)
T={'int64_t':C.c_int64,'Money':C.c_int64,'size_t':C.c_size_t,'int':C.c_int,'bool':C.c_bool,'char':C.c_char,'void':None}
ENUMS={}
for body,name in re.findall(r'typedef enum\s*\{([^}]+)\}\s*(\w+);',header):
    members=[v.strip() for v in body.split(',')]
    T[name]=C.c_int;ENUMS[name]=members
    for i,member in enumerate(members):globals()[member]=i
structures=re.findall(r'typedef struct\s*\{([^}]+)\}\s*(\w+);',header)
for body,name in structures:T[name]=type(name,(C.Structure,),{});globals()[name]=T[name]
def ctype(s):
    s=s.replace('const ','').strip();p=s.count('*');s=s.replace('*','').strip()
    if s=='char' and p==1:return C.c_char_p
    if s=='void' and p==1:return C.c_void_p
    t=T[s]
    for _ in range(p):t=C.POINTER(t)
    return t
for body,name in structures:
    fs=[]
    for declaration in body.split(';'):
        if not declaration.strip():continue
        typ,variables=declaration.strip().split(None,1)
        for var in variables.split(','):
            match=re.fullmatch(r'\s*(\**)(\w+)(?:\[(\w+)\])?\s*',var)
            if not match:raise ValueError(declaration)
            stars,field,n=match.groups();tp=ctype(typ+stars)
            if n:tp=tp*({'ID_SIZE':49}.get(n,int(n) if n.isdigit() else 0))
            fs.append((field,tp))
    T[name]._fields_=fs
lib=C.CDLL(os.environ.get('AURUM_LIB',str(ROOT/'build/libaurum.so')))
for result,stars,name,args in re.findall(r'^((?:const )?\w+)\s+(\**)(\w+)\(([^;]+)\);',header,re.M):
    result=result+stars
    if 'FILE' in args or result not in T and '*' not in result:continue
    try:fn=getattr(lib,name)
    except AttributeError:continue
    argtypes=[]
    if args!='void':
        for arg in args.split(','):
            tp=re.sub(r'\b\w+$','',arg.strip()).strip();argtypes.append(ctype(tp))
    fn.argtypes=argtypes;fn.restype=ctype(result)
    globals()[name]=fn

def decode(x):
    if isinstance(x,bytes):return x.decode('ascii')
    if isinstance(x,C.Structure):return {n:decode(getattr(x,n)) for n,t in x._fields_}
    return x

def record(cls,**values):
    x=cls()
    for k,v in values.items():setattr(x,k,v.encode() if isinstance(v,str) else v)
    return x

def append(e,collection,value):
    off=lambda field:C.byref(e,getattr(Engine,field).offset)
    capacity=getattr(e.config,collection+'_capacity')
    ok=array_append(C.cast(off(collection),C.POINTER(C.c_void_p)),C.cast(off(collection+'_len'),C.POINTER(C.c_size_t)),C.cast(off(collection+'_alloc'),C.POINTER(C.c_size_t)),capacity,C.sizeof(value),C.byref(value))
    if not ok:raise AssertionError('fixture prestate capacity exhausted: '+collection)

def entries(e,collection):return [decode(getattr(e,collection)[i]) for i in range(getattr(e,collection+'_len'))]
