# 19. Reflection

Reflection lets a program inspect (and to a controlled degree, manipulate) types
and values at runtime. MLang offers reflection that is **opt-in, typed, and
pay-for-what-you-use**, so the default binary carries no reflection metadata it
does not need.

## 19.1 Design constraints

- **No mandatory bloat.** A program that does not use reflection on a type emits
  no reflection metadata for it. Metadata is generated only for types annotated
  `@reflect` or used by a reflective API, so the cost is opt-in.
- **Type safety preserved.** Reflective access returns typed handles
  (`TypeInfo`, `FieldInfo`, `MethodInfo`) and dynamic values (`Dynamic`) that
  must be checked/cast back to a static type before ordinary use; reflection
  cannot silently violate the type system.
- **AOT-friendly.** Because MLang is ahead-of-time compiled and monomorphized,
  "create an instance of a type named at runtime" requires that type's metadata
  and constructor to have been retained. The build records reflective roots so
  dead-code elimination does not strip something reflection needs.

## 19.2 The metadata model

For each reflected type the compiler emits, into the binary's metadata section
(Chapter 29), a record consumed by `mlang-runtime`'s `meta/` module:

```
TypeInfo {
    name, namespace, kind (class|struct|enum|interface)
    size, alignment
    typeParameters[]            // instantiated generic args
    fields[]   -> FieldInfo { name, type, offset, visibility, attributes }
    methods[]  -> MethodInfo { name, signature, fnptr, visibility, attributes }
    interfaces[]                // conformances
    attributes[]               // user annotations retained for reflection
}
```

## 19.3 The reflection API (in MLang)

```mlang
import std.reflect.{ typeOf, TypeInfo }

let t: TypeInfo = typeOf(player)
Console.println("type: ${t.name}")

for (f in t.fields) {
    Console.println("${f.name}: ${f.type.name} = ${f.get(player)}")
}

let m = t.method("login")?
let result = m.invoke(player, ["alice", "secret"])
```

Capabilities: enumerate fields/methods/attributes, read and (for `var`/non-
`readonly` fields) write field values, invoke methods, query and test
conformances, and construct instances from a `TypeInfo` plus constructor args.
Each capability is gated by visibility and by what metadata was retained.

## 19.4 Attributes (annotations)

User-defined attributes are how frameworks drive reflection (serialization,
DI, ORM, test discovery):

```mlang
@Serializable
class User {
    @Json(name: "user_name") public let name: String
    @Json(skip) private var cache: String?
}
```

Attributes are ordinary types (often value types) attached to declarations;
retained ones appear in `MethodInfo`/`FieldInfo.attributes`. The test framework
(Chapter 26) uses `@Test` this way; the JSON library (Chapter 20) uses
`@Serializable`/`@Json`.

## 19.5 Dynamic values

`Dynamic` is a type-erased value carrying its `TypeInfo`. It is the bridge between
reflective and static code:

```mlang
let d: Dynamic = readValueFromConfig()
match (d.asType<Int>()) {
    case let n: Int => useInt(n)
    case null       => Console.error("expected an int")
}
```

`Dynamic` is the only dynamically-typed value in the language and it is explicit;
there is no implicit dynamic typing anywhere else.

## 19.6 Compile-time reflection (planned)

Beyond runtime reflection, a `comptime` facility (Chapter 33) will let macros and
generic code inspect types *at compile time* to generate code (e.g. derive a JSON
codec with no runtime metadata at all). This is strictly more efficient than
runtime reflection for the cases that can be resolved statically, and is the
preferred path for serialization once it lands; runtime reflection remains for
genuinely dynamic scenarios.

## 19.7 Why opt-in

Languages that reflect everything by default (Java) pay in binary size, slower
startup, and an attack surface (reflection is a classic exploitation vector).
Languages with none (C, Rust without crates) make frameworks awkward. MLang's
opt-in model gives frameworks what they need while keeping the default lean and
the security model (Chapter 31) tight: you can audit exactly which types are
reflectable.
