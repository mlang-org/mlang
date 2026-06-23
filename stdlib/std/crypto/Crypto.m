namespace std.crypto

/// Cryptographic primitives. Hashes and AEAD are constant-time; randomness is a
/// CSPRNG; sensitive buffers are zeroized on drop (docs/design/31-security.md).
public class Sha256 {
    private var state: HashState

    public fn update(data: Bytes): Sha256 {
        state.absorb(data)
        return this
    }

    public fn digest(): Bytes {
        return state.finalize()
    }

    public static fn hash(data: Bytes): Bytes {
        return Sha256().update(data).digest()
    }
}

/// A cryptographically secure random source.
public class SecureRandom {
    public fn nextBytes(count: Int): Bytes {
        return Entropy.fill(count)
    }
}
