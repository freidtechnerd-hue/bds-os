import pathlib,sys,tempfile,unittest,struct
ROOT=pathlib.Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'tools'))
from disk import Disk
class Integrity(unittest.TestCase):
    def setUp(self):self.d=Disk(ROOT/'build/seed.img')
    def test_seed(self):self.d.check()
    def test_overlap(self):
        f=[e for e in self.d.entries if e[2]==1];f[1][4]=f[0][4]
        with self.assertRaises(AssertionError):self.d.check()
    def test_cycle(self):
        i=next(i for i,e in enumerate(self.d.entries) if e[2]==2);self.d.entries[i][1]=i+1
        with self.assertRaises(AssertionError):self.d.check()
    def test_size(self):
        e=next(e for e in self.d.entries if e[2]==1);e[3]=e[5]*512+1
        with self.assertRaises(AssertionError):self.d.check()
    def test_full(self):
        self.d.blocks=20
        with self.assertRaises(AssertionError):self.d.put('/HUGE',b'x'*8192)
    def test_architecture(self):
        for name in ['hello','calc','editor','sysinfo','game','doom','nes','browser','image','player','cube','puzzle']:
            raw=(ROOT/f'build/apps/{name}.run').read_bytes();h=struct.unpack('<4sHHIIIIII',raw[:32])
            self.assertEqual(h[:3],(b'BDSR',2,2));self.assertEqual(h[3],len(raw)-32-h[8]);self.assertEqual(raw[32+h[3]:],(ROOT/f"assets/icons/{name.upper()}.ICO").read_bytes());self.assertLess(h[5],h[3]);self.assertGreaterEqual(h[6],h[3]+h[4]);self.assertLessEqual(h[6],8*1024*1024)
            checksum=0
            for c in raw[32:]:checksum=(checksum*31+c)&0xffffffff
            self.assertEqual(checksum,h[7])
if __name__=='__main__':unittest.main(verbosity=2)
