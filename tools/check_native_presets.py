#!/usr/bin/env python3
"""Validate the actual portable MPC preset files, including their VST chunks."""
import hashlib,json,sys,xml.etree.ElementTree as ET
import build
sys.path.insert(0,str(build.MPC/'tools'))
import xpl

def check():
    cfg=build.CFG
    folder=f"{cfg['vendor']} - VST - {cfg['name']}"
    directory=build.ROOT/'build/skin'/folder/'Presets'
    manifest=json.loads((build.ROOT/'build/native-preset-manifest.json').read_text())
    options=json.loads((build.ROOT/'params.json').read_text())['params'][8]['options']
    expected=[f'{i:04d}-{name}.xpl' for i,name in enumerate(options)]+['1000-Starter Kit (16 Pads).xpl']
    factory=json.loads((build.ROOT/'build/factory-native-manifest.json').read_text())
    assert len(factory)==109
    expected += [item['file'] for item in factory]
    assert len(manifest)==118 and [item['file'] for item in manifest]==expected
    assert sorted(p.name for p in directory.iterdir())==sorted(expected)
    for item in manifest:
        tree=ET.parse(directory/item['file']).getroot()
        plugin=tree.find('PLUGIN')
        assert plugin is not None
        assert plugin.get('file')==f'%payload-path%/{folder}/{cfg["so"]}'
        assert plugin.get('name')==cfg['name'] and plugin.get('manufacturer')==cfg['vendor']
        assert plugin.get('uid')==cfg['uid'].encode().hex()
        assert tree.findtext('preset')==item['name']
        bank=xpl.b64dec(tree.findtext('state'))
        assert bank[:4]==b'CcnK' and bank[8:12]==b'FBCh'
        assert int.from_bytes(bank[16:20],'big')==int.from_bytes(cfg['uid'].encode(),'big')
        assert int.from_bytes(bank[20:24],'big')==cfg['version']
        raw=bank[160:]
        assert int.from_bytes(bank[156:160],'big')==len(raw)==item['bytes']
        assert raw.startswith(b'GKMP9 ') and raw.endswith(b'\0')
        assert b' KIT1 ' in raw and b' MAP1 ' in raw
        assert hashlib.sha256(raw).hexdigest()==item['state_sha256']
        assert raw==(build.ROOT/'build/native-preset-export'/item['file'].replace('.xpl','.chunk')).read_bytes()
    assert (directory.parent/'Factory-Licenses.txt').is_file()
    print('Native MPC presets:118 portable XPL files including109 original factory presets/kits; names/UID/version/payloads/hashes PASS')

if __name__=='__main__':check()
