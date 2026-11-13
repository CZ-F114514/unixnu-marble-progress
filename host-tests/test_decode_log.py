import unittest,struct,zlib
from decode_log import decode_frames
def fixture():
    content=(b'UXTRACE SEC entry el=1 fdt=0x80600000\nUNIXNU: UXTRACE menu enter\n'*40)
    name=b'marble.log'; mime=b'text/plain'; crc=zlib.crc32(content)
    payload=struct.pack('<4sBBHHIIII',b'LMS1',2,0,len(name),len(mime),len(content),crc,len(content),crc)+name+mime+content
    count=(len(payload)+719)//720; frames=[]
    for i in range(count):
        header=struct.pack('<2sBBIIIHHII',b'OS',2,0,123,i+i//4,count,720,0,len(payload),zlib.crc32(payload))
        frame=header+payload[i*720:(i+1)*720].ljust(720,b'\0')
        frames.append(frame+struct.pack('<I',zlib.crc32(frame)))
    return frames,content
class DecodeTests(unittest.TestCase):
    def test_out_of_order_and_duplicate(self):
        frames,content=fixture()
        self.assertEqual(decode_frames(list(reversed(frames))+frames[:1])[1],content)
    def test_reject_corruption(self):
        frames,_=fixture(); b=bytearray(frames[0]);b[60]^=1;frames[0]=bytes(b)
        with self.assertRaises(ValueError): decode_frames(frames)
    def test_reject_missing_page(self):
        frames,_=fixture()
        with self.assertRaises(ValueError): decode_frames(frames[:-1])
    def test_reject_mixed_sessions(self):
        frames,_=fixture(); b=bytearray(frames[0]);b[4]^=1;struct.pack_into('<I',b,748,zlib.crc32(b[:748]));frames.append(bytes(b))
        with self.assertRaises(ValueError): decode_frames(frames)
if __name__=='__main__': unittest.main()
