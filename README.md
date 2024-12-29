# SeImporter

有问题请在Github提交Issue，或者加入QQ群：`817519915`

If you have any questions, please submit an issue on GitHub.

喜欢这个项目请点star，这对我非常有帮助

If you like this project, please give it a star; it would be very helpful to me.

## 特性 Features

支持导入文件类型：`SEModel`，`SEAnim`，`Cast`

Supported file types for import: SEModel, SEAnim, Cast

Cast文件请参考：[cast/libraries at master · dtzxporter/cast (github.com)](https://github.com/dtzxporter/cast/tree/master/libraries)

For Cast files, please refer to: [cast/libraries at master · dtzxporter/cast (github.com)](https://github.com/dtzxporter/cast/tree/master/libraries)

推荐使用Cast导入，而不是SEModel和SEAnim格式

It is recommended to use Cast for importing instead of SEModel and SEAnim formats.

- 选项：`ConvertRefPosition`，勾选将会转换位置信息，如果导入后模型被拉伸，尝试取消这个选项

- ConvertRefPosition - Checking this option will convert position information. If the model gets stretched after import, try unchecking this option.

- 选项：`ConvertRefAnim`，勾选将会转换旋转信息，如果导入后模型旋转有误，尝试取消这个选项

- `ConvertRefAnim` - Checking this option will convert rotation information. If the model’s rotation is incorrect after import, try unchecking this option.

每个动画以上两个选项勾选方式不一样！！！

The checking methods for these two options are different for each animation!!!

使用以上两个选项，现在动画已经可以完全转换为UnrealEngine格式并正常预览，无需其他插件转换

By using the above two options, animations can now be fully converted to Unreal Engine format and previewed correctly without the need for other plugin conversions.

使用虚幻引擎Apply Additive节点即可实现动画叠加，无需其他插件进行叠加处理

Animation layering can be achieved using the Apply Additive node in Unreal Engine, without the need for other plugins for layering.

动画通知的导入仍在测试中

The import of animation notifications is still under testing.

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