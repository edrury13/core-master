# UNO (Universal Network Objects) Architecture

## Overview

UNO is LibreOffice's component model and the foundation for its extensibility. It provides language-independent interfaces, remote procedure calls, and a service-based architecture that allows components written in different languages to interact seamlessly.

## Core Concepts

### 1. Interfaces

UNO interfaces define contracts between components:

```idl
// Example IDL definition
interface XComponent : com::sun::star::uno::XInterface
{
    void dispose();
    void addEventListener([in] XEventListener xListener);
    void removeEventListener([in] XEventListener xListener);
};
```

Key characteristics:
- Defined in UNOIDL (UNO Interface Definition Language)
- All interfaces inherit from XInterface
- Support multiple inheritance of interfaces
- Immutable once published

### 2. Services

Services are concrete implementations of interfaces:

```idl
service Desktop : XDesktop2, XComponent
{
    // Service constructor
    create();
};
```

Types of services:
- **Old-style services**: Property bag with interfaces
- **New-style services**: Constructor-based with defined interface

### 3. Types

UNO type system includes:
- **Basic types**: boolean, byte, short, long, hyper, float, double, char, string
- **Composite types**: struct, exception, enum, typedef
- **Complex types**: interface, service, singleton
- **Containers**: sequence<T>, any

### 4. Properties

Properties provide named access to object state:
- Accessed via XPropertySet interface
- Support change notifications
- Can be bound, constrained, or maybedefault

## Architecture Layers

### 1. Binary UNO (C++ Level)

Located in: `cppu/`, `cppuhelper/`

Core components:
- **uno::Reference<>**: Smart pointer for interface references
- **uno::Any**: Type-safe variant type
- **uno::Sequence<>**: Dynamic array implementation
- **WeakReference**: Weak reference support

Memory management:
- Reference counting via acquire()/release()
- Thread-safe reference counting
- Weak reference support for lifecycle management

### 2. UNO Runtime

Key components:

**Service Manager** (`cppuhelper/source/servicemanager.cxx`):
- Central registry for all services
- Factory pattern implementation
- Singleton management

**Type Description Manager**:
- Runtime type information
- Type conversion and compatibility checking
- Reflection services

**Bridge Infrastructure** (`bridges/`):
- Language bridges (C++, Java, Python)
- Process bridges (in-process, out-of-process)
- Remote bridges (URP - UNO Remote Protocol)

### 3. Language Bindings

**C++ Binding**:
```cpp
Reference<XComponent> xComponent(
    xFactory->createInstance("com.sun.star.frame.Desktop"),
    UNO_QUERY);
if (xComponent.is()) {
    xComponent->dispose();
}
```

**Java Binding** (`javaunohelper/`):
```java
XComponent xComponent = UnoRuntime.queryInterface(
    XComponent.class,
    xFactory.createInstance("com.sun.star.frame.Desktop"));
```

**Python Binding** (`pyuno/`):
```python
desktop = ctx.ServiceManager.createInstanceWithContext(
    "com.sun.star.frame.Desktop", ctx)
```

## Component Registration

### 1. Component Description

Components are described in `.component` files:

```xml
<component loader="com.sun.star.loader.SharedLibrary">
  <implementation name="com.sun.star.comp.framework.Desktop">
    <service name="com.sun.star.frame.Desktop"/>
    <singleton name="com.sun.star.frame.theDesktop"/>
  </implementation>
</component>
```

### 2. Factory Pattern

Each component provides factory methods:

```cpp
extern "C" SAL_DLLPUBLIC_EXPORT void* component_getFactory(
    const char* pImplName,
    void* pServiceManager,
    void* pRegistryKey)
{
    return cppu::component_getFactoryHelper(
        pImplName, pServiceManager, pRegistryKey,
        services);
}
```

### 3. Service Instantiation Flow

```
1. Application requests service
        ↓
2. ServiceManager lookup
        ↓
3. Load component library
        ↓
4. Call component_getFactory
        ↓
5. Get XFactory interface
        ↓
6. Create instance
        ↓
7. Query desired interface
```

## Threading Model

### Thread Safety Guarantees

1. **Thread-safe**: Service Manager, Type Description
2. **Thread-affine**: UI components (main thread only)
3. **Thread-unsafe**: Most document model objects

### Apartment Threading

UNO supports apartment threading:
- **MTA** (Multi-Threaded Apartment): Thread-safe components
- **STA** (Single-Threaded Apartment): Thread-affine components
- **Thread-unsafe**: Requires external synchronization

Implementation via:
```cpp
class MyComponent : public cppu::WeakImplHelper<XMyInterface>
{
    mutable ::osl::Mutex m_aMutex;
    // Use m_aMutex for synchronization
};
```

## Inter-Process Communication

### UNO Remote Protocol (URP)

Binary protocol for remote UNO communication:
- Efficient binary encoding
- Support for all UNO types
- Exception propagation
- Reference lifecycle management

Protocol stack:
```
Application
    ↓
UNO Bridge
    ↓
URP Protocol
    ↓
Socket/Pipe
```

### Bridge Types

1. **In-process bridges**: Direct function calls
2. **Binary UNO bridge**: C++ ABI compatibility
3. **Java bridge**: JNI-based communication
4. **Remote bridge**: Network communication

## Type System Details

### Type Mapping

Cross-language type mapping:

| UNO Type | C++ | Java | Python |
|----------|-----|------|--------|
| boolean | sal_Bool | boolean | bool |
| byte | sal_Int8 | byte | int |
| short | sal_Int16 | short | int |
| long | sal_Int32 | int | int |
| hyper | sal_Int64 | long | long |
| string | OUString | String | str |
| any | Any | Object | any |

### Type Description

Runtime type information structure:
```cpp
struct typelib_TypeDescription
{
    typelib_TypeClass eTypeClass;
    sal_Int32 nSize;
    sal_Int32 nAlignment;
    typelib_TypeDescriptionReference* pWeakRef;
    // Type-specific data
};
```

## Extension Mechanism

### Extension Structure

```
extension.oxt
├── META-INF/
│   └── manifest.xml
├── description.xml
├── registration/
│   └── license.txt
├── libs/
│   └── mycomponent.so
└── registry/
    └── data/
        └── org/openoffice/
            └── Office/
                └── Addons.xcu
```

### Extension Loading

1. **Package Manager** parses extension
2. **Component Registration** via .component files
3. **Configuration Merge** for UI elements
4. **Service Registration** in ServiceManager

## Performance Considerations

### Optimization Techniques

1. **Interface Caching**:
```cpp
mutable Reference<XPropertySet> m_xCachedProps;
if (!m_xCachedProps.is())
    m_xCachedProps.set(this, UNO_QUERY);
```

2. **Sequence Optimization**:
```cpp
// Avoid copying
uno::Sequence<sal_Int32> aSeq(100);
sal_Int32* pData = aSeq.getArray(); // Direct access
```

3. **Any Optimization**:
```cpp
// Check type before extraction
if (aAny.getValueType() == cppu::UnoType<sal_Int32>::get())
    sal_Int32 n = *static_cast<const sal_Int32*>(aAny.getValue());
```

### Common Pitfalls

1. **Reference Cycles**: Use weak references
2. **Threading Issues**: Proper mutex usage
3. **Any Type Extraction**: Always check type
4. **Sequence Copying**: Use getConstArray() when possible

## Debugging UNO

### Tools and Techniques

1. **uno executable**: Test UNO components
```bash
uno -s com.sun.star.frame.Desktop --accept="pipe,name=test"
```

2. **Component Context**:
```cpp
Reference<XComponentContext> xContext = 
    comphelper::getProcessComponentContext();
```

3. **Service Inspection**:
```cpp
Reference<XIntrospection> xIntro = 
    Introspection::create(xContext);
Reference<XIntrospectionAccess> xAccess = 
    xIntro->inspect(makeAny(xComponent));
```

### Common Debugging Patterns

1. Check interface availability:
```cpp
if (Reference<XMyInterface> xMy(xObj, UNO_QUERY); xMy.is())
{
    // Interface is supported
}
```

2. Exception handling:
```cpp
try {
    xComponent->doSomething();
} catch (const Exception& e) {
    SAL_WARN("module", "UNO exception: " << e.Message);
}
```

## Best Practices

### Design Guidelines

1. **Interface Segregation**: Keep interfaces focused
2. **Backward Compatibility**: Never change published interfaces
3. **Service Evolution**: Use new-style services
4. **Property Design**: Group related properties

### Implementation Guidelines

1. **Helper Classes**: Use cppu::WeakImplHelper
2. **Thread Safety**: Document threading requirements
3. **Resource Management**: Implement XComponent when needed
4. **Error Handling**: Throw appropriate exceptions

### Performance Guidelines

1. **Minimize Queries**: Cache interface references
2. **Batch Operations**: Use XMultiPropertySet
3. **Avoid Any**: Use specific types when possible
4. **Local vs Remote**: Design for local use first

## Future Directions

### Ongoing Improvements

1. **Type Safety**: More compile-time type checking
2. **Performance**: Reduced overhead for common operations
3. **Language Support**: Better modern C++ integration
4. **Documentation**: Improved API documentation

### Experimental Features

1. **Coroutine Support**: Async UNO calls
2. **WebAssembly Bridge**: Browser integration
3. **gRPC Transport**: Modern RPC mechanism
4. **Reflection Improvements**: Better introspection

---

This documentation provides an in-depth look at UNO's architecture. For specific API usage, consult the API documentation at https://api.libreoffice.org/