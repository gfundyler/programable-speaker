# -*- mode: python -*-

block_cipher = None


a = Analysis(['speaker_appliance.py'],
             pathex=['C:\\Users\\crb02\\Documents\\Side Quests\\Clark Speaker Array\\Python\\listener'],
             binaries=[],
             datas=[],
             hiddenimports=['scipy.special._ufuncs_cxx',
                'scipy.linalg.cython_blas',
                'scipy.linalg.cython_lapack',
                'scipy.integrate',
                'scipy.integrate.quadrature',
                'scipy.integrate.odepack',
                'scipy.integrate._odepack',
                'scipy.integrate.quadpack',
                'scipy.integrate._quadpack',
                'scipy.integrate._ode',
                'scipy.integrate.vode',
                'scipy.integrate._dop',
                'scipy.integrate.lsoda'],
             hookspath=[],
             runtime_hooks=[],
             excludes=[],
             win_no_prefer_redirects=False,
             win_private_assemblies=False,
             cipher=block_cipher)
pyz = PYZ(a.pure, a.zipped_data,
             cipher=block_cipher)
exe = EXE(pyz,
          a.scripts,
          exclude_binaries=True,
          name='speaker_appliance',
          debug=False,
          strip=False,
          upx=True,
          console=True )
coll = COLLECT(exe,
               a.binaries,
               a.zipfiles,
               a.datas,
               strip=False,
               upx=True,
               name='speaker_appliance')
