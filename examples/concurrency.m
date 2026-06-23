namespace examples.concurrency

import std.io.Console
import std.concurrent.{ Channel, gather }

// async/await: the body does not run until awaited.
public async fn fetchUser(id: Int): String {
    let row: await Database.query(id)
    return row.name
}

// Structured concurrency: children cannot outlive the scope.
public async fn handleRequest(id: Int): String {
    return scope {
        let user: launch fetchUser(id)
        let prefs: launch fetchPrefs(id)
        format(await user, await prefs)
    }
}

// CSP channels: producer / consumer.
public async fn pipeline() {
    let ch: Channel<String>(16)

    launch {
        for (line in source()) {
            await ch.send(line)
        }
        ch.close()
    }

    for (line in ch) {
        process(line)
    }
}

public async fn main() {
    let titles: await gather([handleRequest(1), handleRequest(2), handleRequest(3)])
    for (title in titles) {
        Console.println(title)
    }
}
