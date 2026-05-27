# AGENTS.md - Image Watch for Visual Studio

## Project Overview

Image Watch is a Visual Studio extension that provides a watch window for visualizing in-memory images (bitmaps) when debugging native C++ code. It consists of a C# VS SDK extension and native C++ image processing components.

## Build Commands

### Prerequisites
- Visual Studio 2022 or later
- Required workloads: .NET Desktop Development, Desktop Development with C++, Visual Studio Extension Development
- Windows 10 SDK
- .NET Framework 4.7.2

### Build Solution
```bash
# Debug build
msbuild ImageWatch.sln /p:Configuration=Debug /p:Platform=x64

# Release build
msbuild ImageWatch.sln /p:Configuration=Release /p:Platform=x64
```

### Run Tests
Tests are run via Visual Studio Test Explorer:
- Open "Test" > "Test Explorer"
- Select "Run All" or run specific tests
- Tests can be grouped by Traits; Categories "Perf" and "Slow" are suited for Release configuration

### Run Extension
1. Set `ImageWatch.Packaging` project as startup project
2. Select "Debug" > "Start Debugging" (launches experimental VS instance)

## Code Structure

### Main Projects

| Project | Language | Purpose |
|---------|----------|---------|
| `ImageWatch/` | C# | Main VS extension package, UI controls, debugger integration |
| `ImageWatchNativeHelpers/` | C++/CLI | Bridge between C# and native image processing |
| `vtiview/NativeImage/` | C++/CLI | Native image viewing and rendering (color maps, transforms) |
| `VisionTools/` | C++ | Core image processing library |
| `ImageWatch.Packaging/` | C# | VSIX packaging |
| `contrib/JXRLib/` | C++ | JPEG XR codec library |

### Key Directories

- `ImageWatch/Interface/` - WPF UI components (ImageViewer, WatchList, MainControl)
- `ImageWatchNativeHelpers/` - WatchedImage types and operators
- `vtiview/NativeImage/` - NativeImageView, ColorMap rendering
- `VisionTools/src/core/` - Core image processing algorithms
- `Test/` - Test applications (ConsoleApplication1, VTApp1, VTApp2)

## Code Style Guidelines

### C# Code Style

```csharp
// Namespace: Microsoft.ImageWatch for main code
namespace Microsoft.ImageWatch.Interface
{
    // PascalCase for public members, camelCase with underscore for private fields
    public class WatchListItem : BindableBase
    {
        private string expression_ = "";  // private fields with underscore suffix
        public string Expression { get; set; }  // PascalCase properties
        
        // Methods: PascalCase
        public void MarkContextOutOfDate() { }
        
        // Event handlers: On<Event> pattern
        private void OnItemPropertyChanged(object sender, PropertyChangedEventArgs e) { }
    }
}
```

**Naming Conventions:**
- Classes/Methods/Properties: `PascalCase`
- Private fields: `camelCase_` (with trailing underscore)
- Local variables: `camelCase`
- Constants: `UPPER_CASE` or `PascalCase`
- Interfaces: `IPrefix`

**Imports:**
```csharp
using System;
using System.Collections.Generic;
using System.Windows;
using Microsoft.VisualStudio.Debugger;
```
- Group imports: System first, then third-party, then project-specific
- No region directives for imports

**Formatting:**
- Indent: 4 spaces
- Braces: Allman style (opening brace on new line)
- Always use braces for control structures
- Max line length: ~120 characters

### C++/CLI Code Style

```cpp
// Header guard style
#pragma once

// Use namespaces
namespace Microsoft 
{
    namespace ImageWatch
    {
        public ref class WatchedImageInfo
        {
        public:
            property String^ ElementFormat { String^ get(); }
            
            // Native methods use VT_HR_BEGIN/END pattern for HRESULT
        private:
            String^ elementFormat_;
        };
    }
}
```

**Naming Conventions:**
- Classes: `PascalCase`
- Methods: `PascalCase`
- Properties: `PascalCase`
- Private member variables: `memberName_` (trailing underscore)
- Macros: `UPPER_CASE`

### C++ (Native) Code Style

```cpp
// VisionTools uses vt:: namespace
namespace vt
{
    class CImg
    {
    public:
        // Methods: PascalCase with Vt prefix for public API
        HRESULT VtConvertImage(CImg& dst);
        
        // Internal methods: camelCase
        int getWidth() const { return width_; }
        
    private:
        int width_;  // trailing underscore for members
    };
}
```

**Naming Conventions:**
- Classes: `PascalCase` (often with C prefix like `CImg`)
- Functions: `PascalCase` or `VtPascalCase` for public API
- Member variables: `camelCase_`
- Local variables: `camelCase`
- Constants: `UPPER_CASE` or `kPascalCase`

## Type System

### WatchedImage Types

The extension supports multiple image types through the `WatchedImageTypeMap`:

```csharp
// Built-in types (regex patterns)
@"vt::(CImgInCache(\W|$)|CTypedImgInCache<.+)" -> VTCImgInCache
@"ID3D11Resource(\W|$)|ID3D11Texture2D(\W|$)" -> D3D11Resource

// Operator types (prefixed with @)
@band, @clamp, @scale, @abs, @file, @diff, @norm8, @norm16, @thresh, @mem, @fliph, @flipv, @flipd, @rot90, @rot180, @rot270

// User-defined types loaded from natvis files
```

### Image Operators

To add a new operator:
1. Create `WatchedImageXxxOp.h/.cpp` in `ImageWatchNativeHelpers/`
2. Inherit from `WatchedImageUnaryOp` or `WatchedImageBinaryOp`
3. Register in `WatchedImageTypeMap.Initialize()`:
   ```csharp
   AddOperator("xxx", typeof(WatchedImageXxxOp));
   ```

## Error Handling

### C# Error Handling
```csharp
// Use try-catch for debugger operations
try
{
    var sf = ImageWatchPackage.DTE.Debugger.CurrentStackFrame;
    if (sf == null) return;
    // ...
}
catch (Exception)
{
    // Clean up and return
    process_ = null;
}
```

### C++ Error Handling
```cpp
// Use HRESULT with VT_HR_BEGIN/END macros
HRESULT MyFunction()
{
    VT_HR_BEGIN();
    
    VT_HR_EXIT(DoSomething());
    
    VT_HR_END();
}

// Assert for internal errors
VT_ASSERT(ptr != nullptr);
```

## Debugging and Logging

```csharp
// Conditional debug logging
#if DEBUG
    System.Diagnostics.Debug.WriteLine("Message: {0}", arg);
#endif
```

## Key Patterns

### BindableBase Pattern (MVVM)
```csharp
public class BindableBase : INotifyPropertyChanged
{
    protected bool SetProperty<T>(ref T storage, T value, [CallerMemberName] string propertyName = null)
    {
        if (EqualityComparer<T>.Default.Equals(storage, value)) return false;
        storage = value;
        OnPropertyChanged(propertyName);
        return true;
    }
}
```

### WatchListItem Lifecycle
1. Created with expression string
2. `UpdateContext()` - parses expression
3. `UpdateInfo()` - loads image metadata
4. `UpdatePixels()` - loads actual pixel data
5. `Dispose()` - cleanup

### Native Image View Pipeline
1. `NativeImageBase` - base class for image containers
2. `NativeImageView` - renders image with transforms (zoom, pan)
3. `ColorMap` - converts pixel values to RGBA for display
4. `InteropBitmap` - WPF interop for rendering

## Testing

### Test Projects
- `VisionTools/tests/coretest/` - Core library tests
- `VisionTools/tests/numericstest/` - Numerics tests
- `Test/ConsoleApplication1/` - OpenCV sample app for manual testing
- `Test/VTApp1/`, `Test/VTApp2/` - Additional test applications

### Adding Tests
- Use Visual Studio native test framework
- Tests can have traits: `[TestCategory("Perf")]`, `[TestCategory("Slow")]`

## Resources

- Natvis files: `ImageWatch/Internal/ImageWatch.natvis`
- User types: Loaded from `%USERPROFILE%\Documents\Visual Studio 2022\Visualizers\`
- Default visualizer path: Set via `ImageWatchPackage.DefaultVisualizerPath`
