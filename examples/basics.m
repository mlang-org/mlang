namespace examples.basics

import std.io.Console

// Variables: `let` is immutable, `var` is mutable. A colon introduces the value.
public fn variables() {
    let name: "Mert"
    let age: 20
    let online: true
    let balance: 45.75
    var counter: 0
    counter = counter + 1
    Console.println("${name} is ${age}")
}

// Null safety: `T?` may be absent; the operators make absence explicit.
public fn nullSafety(input: String?) {
    let length: input?.length() ?? 0
    let value: input ?: "default"
    Console.println("len=${length} value=${value}")
}

// Control flow.
public fn controlFlow(n: Int): Int {
    var total: 0
    for (i in range(0, n)) {
        if (i % 2 == 0) {
            total = total + i
        } else {
            total = total - 1
        }
    }
    var k: n
    while (k > 0) {
        k = k - 1
    }
    return total
}

public fn main() {
    variables()
    nullSafety(null)
    Console.println("sum: ${controlFlow(10)}")
}
