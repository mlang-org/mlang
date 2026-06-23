namespace std.math

/// Common mathematical constants and functions. Backed by compiler intrinsics
/// that map to hardware instructions where available.
public class Math {
    public static readonly let PI: 3.141592653589793
    public static readonly let E: 2.718281828459045

    public static fn abs(x: Float): Float {
        return x < 0.0 ? -x : x
    }

    public static fn min<T: Comparable>(a: T, b: T): T {
        return a.compareTo(b) <= 0 ? a : b
    }

    public static fn max<T: Comparable>(a: T, b: T): T {
        return a.compareTo(b) >= 0 ? a : b
    }

    public static fn clamp<T: Comparable>(value: T, lo: T, hi: T): T {
        return max(lo, min(value, hi))
    }

    public static fn sqrt(x: Float): Float {
        return Intrinsics.sqrt(x)
    }

    public static fn pow(base: Float, exponent: Float): Float {
        return Intrinsics.pow(base, exponent)
    }
}
