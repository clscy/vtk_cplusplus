## 记录一些后续可能用到的API接口:
```
vtkFillHolesFilter
```

## 记录一些医学技术术语:
```
CPR----曲面重建
```

## 记录一些调试过程:
```
0.mkdir build
1.cmake .. -DCMAKE_BUILD_TYPE=debug
2.配置launch.json和tasks.json
3.make
4.调试C++无报错提示解决办法,settings.json添加以下内容:
"C_Cpp.intelliSenseEngine":"Default",
"editor.suggest.snippetsPreventQuickSuggestions":true,
```

## 2023.01.28~2023.02.03工作计划:
```
0.调通Widget及Rep, 主要报错为"undefined reference to typeinfo for XXX";
    0.CMakeLists.txt中SOURCES和HEAD分别添加自定义的类和头文件;
    1.widget中的头文件和类文件都改名是自定义的Rep名.
1.调试报错为vtkNew.h:89: undefined reference to `vtkXXX::New()',
    0.修改CMakeLists.txt,添加对应的库,例如:
        vtkNew.h:89: undefined reference to `vtkPLYReader::New()'
        CMakeLists.txt内find_package添加IOPLY
```
