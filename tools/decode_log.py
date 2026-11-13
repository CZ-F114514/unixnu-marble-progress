#!/usr/bin/env python3
"""Decode UniXNU OneSend QR frames locally. No network or device access.

Input: decoded binary QR frames (.bin); PNG/JPG additionally need zxing-cpp.
The output path is explicit; a filename received from a QR is never used as a path.
"""
from pathlib import Path
import argparse,struct,zlib

def decode_frames(frames):
    pages={}; identity=None
    for frame in frames:
        if len(frame)!=752 or frame[:4]!=b'OS\x02\x00': raise ValueError('invalid frame format')
        if zlib.crc32(frame[:748])!=struct.unpack_from('<I',frame,748)[0]: raise ValueError('frame CRC mismatch')
        session,sequence,count,block,reserved,size,crc=struct.unpack_from('<IIIHHII',frame,4)
        if block!=720 or reserved or not 1<=size<=1048576 or count!=(size+719)//720: raise ValueError('invalid frame bounds')
        key=(session,count,size,crc)
        if identity is not None and key!=identity: raise ValueError('mixed QR sessions')
        identity=key
        if sequence%5==4: raise ValueError('unexpected parity frame')
        index=sequence//5*4+sequence%5
        if index>=count: raise ValueError('page outside payload')
        page=frame[28:748]
        if index in pages and pages[index]!=page: raise ValueError('conflicting duplicate')
        pages[index]=page
    if identity is None: raise ValueError('no frames')
    _,count,size,crc=identity
    missing=sorted(set(range(count))-pages.keys())
    if missing: raise ValueError(f'missing pages: {missing}')
    payload=b''.join(pages[i] for i in range(count))[:size]
    if zlib.crc32(payload)!=crc: raise ValueError('payload CRC mismatch')
    if len(payload)<26 or payload[:6]!=b'LMS1\x02\x00': raise ValueError('unsupported log envelope')
    n,m,length,check,raw_length,raw_check=struct.unpack_from('<HHIIII',payload,6)
    start=26+n+m
    if start>len(payload) or len(payload)-start!=length or raw_length!=length or raw_check!=check: raise ValueError('invalid envelope bounds')
    if payload[26+n:start]!=b'text/plain': raise ValueError('unexpected MIME')
    content=payload[start:]
    if zlib.crc32(content)!=check: raise ValueError('log CRC mismatch')
    return payload[26:26+n].decode('utf-8',errors='replace'),content

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('inputs',nargs='+',type=Path)
    parser.add_argument('--output',required=True,type=Path)
    args=parser.parse_args();frames=[]
    for path in args.inputs:
        if path.suffix.lower()=='.bin': frames.append(path.read_bytes());continue
        try:
            import zxingcpp
            from PIL import Image
        except ImportError:
            parser.error('image decoding needs local packages zxing-cpp and Pillow; .bin frames need no packages')
        with Image.open(path) as img:
            frames.extend(result.bytes for result in zxingcpp.read_barcodes(img) if result.bytes[:2]==b'OS')
    try: name,content=decode_frames(frames)
    except ValueError as e: parser.error(str(e))
    with args.output.open('xb') as f: f.write(content)
    print(f'Saved {len(content)} bytes from {name} to {args.output}; all CRCs verified')

if __name__=='__main__':main()
