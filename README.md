# 3DM-Loader

3Dmigoto注入器，可额外一键注入FufuLauncher.UnlockerIsland.dll等dll

示例（需修改d3dx.ini）：
[Loader]
; Target process to load into:
target = YuanShen.exe

module = d3d11.dll
unlocker_dll = Plugin\FufuLauncher.UnlockerIsland.dll

extra_dll = D:\HoYoShadeHub\HoYoShade\ReShade64.dll
extra_dll1 = Plugin\XXXX.dll

require_admin = true


This project is derived from 3Dmigoto project:

https://github.com/bo3b/3Dmigoto


