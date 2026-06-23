namespace examples.types

import std.io.Console

// A value-type struct.
public struct Vector3 {
    public readonly let x: Float
    public readonly let y: Float
    public readonly let z: Float

    public fn length(): Float {
        return Math.sqrt(x * x + y * y + z * z)
    }
}

// An enum.
public enum Rank {
    PLAYER,
    VIP,
    ADMIN,
    OWNER
}

// An interface.
public interface Command {
    public fn execute()
}

// An abstract base class.
public abstract class Entity {
    public abstract fn tick()
}

// A class with inheritance, an interface, properties, constructor, destructor.
public class Player ext Entity impl Command {
    public readonly let uuid: String
    private var health: Int
    public var rank: Rank

    public constructor(uuid: String) {
        this.uuid = uuid
        this.health = 100
        this.rank = Rank.PLAYER
    }

    public destructor() {
        Console.println("player ${uuid} despawned")
    }

    public fn tick() {
        health = health + 1
    }

    public fn execute() {
        Console.println("executing as ${uuid}")
    }
}

// An extension on a built-in type.
extension String {
    public fn isNumeric(): Bool {
        return length() > 0 && chars().all((c) => c.isDigit())
    }
}

public fn main() {
    let p: Player("alice")
    p.tick()
    p.execute()
}
