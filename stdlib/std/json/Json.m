namespace std.json

import std.core.Result
import std.collections.{ List, Map }

/// A parsed JSON value.
public enum JsonValue {
    Null,
    Bool(value: Bool),
    Number(value: Float),
    Text(value: String),
    Array(items: List<JsonValue>),
    Object(fields: Map<String, JsonValue>)
}

public class Json {
    public static fn parse(text: String): Result<JsonValue, JsonError> {
        return JsonParser(text).parseValue()
    }

    public static fn stringify(value: JsonValue): String {
        let builder: StringBuilder()
        writeValue(value, builder)
        return builder.build()
    }
}

public struct JsonError {
    public readonly let message: String
    public readonly let offset: Int
}
