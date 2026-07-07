# 3DM-Loader

3Dmigoto 注入器，可在 3Dmigoto 加载后额外注入 `unlocker_dll`、`extra_dll` 等 DLL。

## 使用方式

1. 将编译后的 `3DMigoto Loader.exe`、3Dmigoto 的 `d3d11.dll` 和 `d3dx.ini` 放在同一目录。
2. 在 `d3dx.ini` 中配置 `[Loader]`。
3. 运行 `3DMigoto Loader.exe`。如果配置了 `require_admin = true`，建议右键以管理员身份运行。

## d3dx.ini 示例

```ini
[Loader]
; Target process to load into:
target = YuanShen.exe

; 3Dmigoto module to load:
module = d3d11.dll

; Optional: launch the game automatically.
launch = D:\Game\YuanShen.exe
launch_args = -popupwindow

; Optional: keep the loader open for N seconds after injection check.
; Use -1 to keep waiting.
delay = 5

; Optional: remind the user/admin manifest to use administrator privileges.
require_admin = true

; Optional: extra DLLs to inject after the target process appears.
unlocker_dll = Plugin\FufuLauncher.UnlockerIsland.dll
extra_dll = D:\HoYoShadeHub\HoYoShade\ReShade64.dll
extra_dll1 = Plugin\AnotherPlugin.dll
extra_dll_2 = Plugin\YetAnotherPlugin.dll
```

## 说明

- `extra_dll`、`extra_dll1`、`extra_dll_1` 都可识别，最多扫描到序号 32。
- 程序启动时会显示配置摘要、目标启动状态和额外 DLL 注入结果。
- 本项目派生自 3Dmigoto：<https://github.com/bo3b/3Dmigoto>
