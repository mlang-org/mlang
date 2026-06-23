namespace std.log

public enum Level { Debug, Info, Warn, Error }

/// A structured logger writing to a configurable sink.
public class Logger {
    private readonly let name: String
    private var minLevel: Level

    public constructor(name: String) {
        this.name = name
        this.minLevel = Level.Info
    }

    public fn info(message: String) {
        log(Level.Info, message)
    }

    public fn error(message: String) {
        log(Level.Error, message)
    }

    private fn log(level: Level, message: String) {
        if ((level as Int) >= (minLevel as Int)) {
            Sink.current().emit(name, level, message)
        }
    }
}
