# SeImporter
## 特性 Features

支持导入文件类型：SEModel，SEAnim，Cast

Supported file types for import: SEModel, SEAnim, Cast

Cast文件请参考：[cast/libraries at master · dtzxporter/cast (github.com)](https://github.com/dtzxporter/cast/tree/master/libraries)

For Cast files, please refer to: [cast/libraries at master · dtzxporter/cast (github.com)](https://github.com/dtzxporter/cast/tree/master/libraries)

推荐使用Cast导入，而不是SEModel和SEAnim格式

It is recommended to use Cast for importing instead of SEModel and SEAnim formats.

- 绝对动画，导入后动画即可正常播放
- **Absolute animations**: Animations will play correctly after import.
- 相对动画，导入后需要在动画蓝图使用转换插件，将它变为正常动画：[girlgg/SeAnimModule: Convert Refpose to Normal Pose (github.com)](https://github.com/girlgg/SeAnimModule)。相对动画结合这款插件，即可实现动画的叠加（主要用于第一人称动画）：https://github.com/dest1yo/MDA
- **Relative animations**: After importing, you'll need to use a conversion plugin in the animation blueprint to turn it into a normal animation: [girlgg/SeAnimModule: Convert Refpose to Normal Pose (github.com)](https://github.com/girlgg/SeAnimModule). With this plugin, you can combine relative animations for layered animation effects (primarily used for first-person animations): https://github.com/dest1yo/MDA

你可以使用AddRefPosToAnimations选项，使用本插件在导入动画时将相对动画转为绝对动画

You can use **AddRefPosToAnimations** to convert relative animations into absolute animations during import with this plugin.

使用blender导出为FBX时，请注意根骨骼名字必须是**Armature**

When exporting from Blender to FBX, please ensure the root bone is named **Armature**.

支持自动识别并导入材质和贴图

The plugin supports automatic detection and import of materials and textures.

当前支持iw9引擎（游戏版本19-21），iw8引擎（游戏版本16-18）

Currently supports IW9 engine (game versions 19-21) and IW8 engine (game versions 16-18).

插件只支持UE5.4.4及以上版本。由于UE的资产特性，如果你使用以下版本，材质将不会被自动创建，但贴图仍然会被导入

The plugin only supports Unreal Engine 5.4.4 and above. Due to the asset nature of Unreal, if you use a lower version, materials will not be created automatically, but textures will still be imported.

## 等待修复 Pending Fixes

SEModel无法导入骨骼网格体

SEModel cannot import skeletal meshes.

由于早期使用该插件的导入方法，现在与该插件存在命名空间冲突： https://github.com/djhaled/Unreal-SeTools

Due to the import method used in earlier versions of this plugin, there is now a namespace conflict with this plugin: https://github.com/djhaled/Unreal-SeTools