namespace std.reflect

import std.collections.List

/// Runtime type information. Opt-in and pay-for-what-you-use: metadata is
/// emitted only for @reflect types or those used reflectively
/// (docs/design/19-reflection.md).
public class TypeInfo {
    public readonly let name: String
    public readonly let namespace: String

    public fn fields(): List<FieldInfo> {
        return Metadata.fieldsOf(this)
    }

    public fn method(name: String): MethodInfo? {
        return Metadata.methodOf(this, name)
    }
}

public class FieldInfo {
    public readonly let name: String
    public readonly let type: TypeInfo

    public fn get(instance: Any): Dynamic {
        return Metadata.readField(instance, this)
    }
}

public class MethodInfo {
    public readonly let name: String

    public fn invoke(instance: Any, args: List<Dynamic>): Dynamic {
        return Metadata.call(instance, this, args)
    }
}

/// Returns the runtime type of a value.
public fn typeOf(value: Any): TypeInfo {
    return Metadata.typeOf(value)
}
