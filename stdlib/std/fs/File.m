namespace std.fs

import std.core.Result

/// Filesystem access. I/O is async and integrates with the scheduler; the
/// handle closes deterministically via its destructor (docs/design/13).
public class File {
    private readonly let handle: FileHandle

    public destructor() {
        handle.close()
    }

    public static async fn open(path: String, mode: OpenMode): Result<File, IoError> {
        return await FileSystem.open(path, mode)
    }

    public static async fn readToString(path: String): Result<String, IoError> {
        let file: await open(path, OpenMode.Read)?
        return await file.readAll()
    }

    public async fn readAll(): Result<String, IoError> {
        return await handle.readAllUtf8()
    }

    public async fn write(text: String): Result<Void, IoError> {
        return await handle.writeUtf8(text)
    }
}

public enum OpenMode { Read, Write, Append }

public struct IoError {
    public readonly let message: String
    public readonly let code: Int
}
