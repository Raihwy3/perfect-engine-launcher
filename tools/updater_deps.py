"""Build-only updater dependencies; no downloads happen in this helper."""
from pathlib import Path
import os,json
ROOT=Path(__file__).resolve().parents[1]
def prefix():
 for p in [Path(os.environ.get('PD_UPDATE_PORTLIBS','/opt/devkitpro/portlibs/3ds')),ROOT/'local/update-deps/opt/devkitpro/portlibs/3ds']:
  if all((p/'lib'/n).exists() for n in ['libcurl.a','libminizip.a','libmbedtls.a','libmbedx509.a','libmbedcrypto.a','libz.a']):return p
 raise RuntimeError('Install 3ds-curl, 3ds-mbedtls and 3ds-zlib with devkitPro pacman, or set PD_UPDATE_PORTLIBS to their prefix.')
def prepare():
 out=ROOT/'build/updater';out.mkdir(exist_ok=True)
 ca=(ROOT/'vendor/updater/cacert.pem').read_text()
 (out/'native_update_ca.h').write_text('static const char native_update_ca[]=\n'+'\n'.join(json.dumps(line+'\n') for line in ca.splitlines())+';\n')
 return ['-I'+str(out),'-I'+str(ROOT/'vendor/updater'),'-I'+str(prefix()/'include'),'-DCURL_STATICLIB']
def libraries():return ['-L'+str(prefix()/'lib'),'-lminizip','-lcurl','-lmbedtls','-lmbedx509','-lmbedcrypto','-lz']
