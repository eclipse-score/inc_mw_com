// Copyright (c) 2025 Qorix GmbH
//
// This program and the accompanying materials are made available under the
// terms of the Apache License, Version 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0.
//
// SPDX-License-Identifier: Apache-2.0

use std::hash::Hasher;
use std::mem::size_of;
use std::ops::BitXor;

/// The speedy FXHash hash algorithm. For fast hashing we are
/// not concerned about attacks against the hash. 
///
/// This is the same as the algorithm used by RUSTC.
///
pub struct FxHasher {
    hash: usize,
}

impl Default for FxHasher {
    #[inline]
    fn default() -> FxHasher {
        FxHasher {
            hash: Self::DEFAULT,
        }
    }
}

impl FxHasher {
    #[cfg(target_pointer_width = "32")]
    const PRIME: usize = 0x9e3779b9;
    #[cfg(target_pointer_width = "32")]
    const DEFAULT: usize = 0;
    #[cfg(target_pointer_width = "64")]
    const PRIME: usize = 0x517cc1b727220a95;
    #[cfg(target_pointer_width = "64")]
    const DEFAULT: usize = 0;

    #[inline]
    fn add_to_hash(&mut self, value: usize) {
        self.hash = self
            .hash
            .rotate_left(5)
            .bitxor(value)
            .wrapping_mul(Self::PRIME);
    }
}

impl Hasher for FxHasher {
    #[inline]
    fn write(&mut self, mut bytes: &[u8]) {
        #[cfg(target_pointer_width = "32")]
        let read_usize = |bytes: &[u8]| u32::from_ne_bytes(bytes[..4].try_into().unwrap());
        #[cfg(target_pointer_width = "64")]
        let read_usize = |bytes: &[u8]| u64::from_ne_bytes(bytes[..8].try_into().unwrap());

        let mut hash = FxHasher { hash: self.hash };
        assert!(size_of::<usize>() <= 8);
        while bytes.len() >= size_of::<usize>() {
            hash.add_to_hash(read_usize(bytes) as usize);
            bytes = &bytes[size_of::<usize>()..];
        }
        if (size_of::<usize>() > 4) && (bytes.len() >= 4) {
            hash.add_to_hash(u32::from_ne_bytes(bytes[..4].try_into().unwrap()) as usize);
            bytes = &bytes[4..];
        }
        if (size_of::<usize>() > 2) && bytes.len() >= 2 {
            hash.add_to_hash(u16::from_ne_bytes(bytes[..2].try_into().unwrap()) as usize);
            bytes = &bytes[2..];
        }
        if (size_of::<usize>() > 1) && !bytes.is_empty() {
            hash.add_to_hash(bytes[0] as usize);
        }
        self.hash = hash.hash;
    }

    #[inline]
    fn write_u8(&mut self, i: u8) {
        self.add_to_hash(i as usize);
    }

    #[inline]
    fn write_u16(&mut self, i: u16) {
        self.add_to_hash(i as usize);
    }

    #[inline]
    fn write_u32(&mut self, i: u32) {
        self.add_to_hash(i as usize);
    }

    #[cfg(target_pointer_width = "32")]
    #[inline]
    fn write_u64(&mut self, i: u64) {
        self.add_to_hash(i as usize);
        self.add_to_hash((i >> 32) as usize);
    }

    #[cfg(target_pointer_width = "64")]
    #[inline]
    fn write_u64(&mut self, i: u64) {
        self.add_to_hash(i as usize);
    }

    #[inline]
    fn write_usize(&mut self, i: usize) {
        self.add_to_hash(i);
    }

    #[inline]
    fn finish(&self) -> u64 {
        self.hash as u64
    }
}

/// FNV1a as 64 bit hasher for short strings
pub struct Fnv1a64Hasher {
    hash: u64,
}

impl Default for Fnv1a64Hasher {
    #[inline]
    fn default() -> Self {
        Fnv1a64Hasher {
            hash: Self::DEFAULT,
        }
    }
}

impl Fnv1a64Hasher {
    const DEFAULT: u64 = 0xcbf29ce484222325;
    const PRIME: u64 = 0x100000001b3;

    #[inline]
    fn add_to_hash(&mut self, byte: u8) {
        self.hash = self.hash.bitxor(byte as u64).wrapping_mul(Self::PRIME);
    }
}

impl std::hash::Hasher for Fnv1a64Hasher {
    #[inline]
    fn write(&mut self, bytes: &[u8]) {
        for byte in bytes {
            self.add_to_hash(*byte)
        }
    }

    #[inline]
    fn finish(&self) -> u64 {
        self.hash
    }
}

/// FNV1a as 32 bit hasher for short strings
pub struct Fnv1a32Hasher {
    hash: u32,
}

impl Default for Fnv1a32Hasher {
    #[inline]
    fn default() -> Self {
        Fnv1a32Hasher {
            hash: Self::DEFAULT,
        }
    }
}

impl Fnv1a32Hasher {
    const DEFAULT: u32 = 0x811c9dc5;
    const PRIME: u32 = 0x01000193;

    #[inline]
    fn add_to_hash(&mut self, byte: u8) {
        self.hash = self.hash.bitxor(byte as u32).wrapping_mul(Self::PRIME);
    }
}

impl std::hash::Hasher for Fnv1a32Hasher {
    fn write(&mut self, bytes: &[u8]) {
        for byte in bytes {
            self.add_to_hash(*byte)
        }
    }

    fn finish(&self) -> u64 {
        self.hash as u64
    }
}

/// FNV1a Hashing function for use in constant expressions.
///
/// This is not compatible with `std::hash::Hasher`. Instead, all functions
/// have constant attribute.
/// Use this implementation if you want hash codes calculated in const expressions at compile time.
///
pub struct Fnv1a64ConstHasher {
    hash: u64,
}

impl Fnv1a64ConstHasher {
    /// New const FNV1a hasher
    pub const fn new() -> Self {
        Self {
            hash: Fnv1a64Hasher::DEFAULT,
        }
    }

    /// Create new FNV1a hasher from seed
    pub const fn from_seed(seed: u64) -> Self {
        Self { hash: seed }
    }

    /// Create new FNV1a hasher from byte array
    pub const fn from_const(bytes: &[u8]) -> Self {
        Self::new().write(bytes)
    }

    /// Create new FNV1a hasher from string
    pub const fn from_str(value: &str) -> Self {
        Self::new().write(value.as_bytes())
    }

    /// Write the given bytes into the the hasher state
    pub const fn write(&self, bytes: &[u8]) -> Self {
        let mut hash = self.hash;
        let mut i = 0;
        while i < bytes.len() {
            hash = (hash ^ bytes[i] as u64).wrapping_mul(Fnv1a64Hasher::PRIME);
            i += 1;
        }

        Self { hash }
    }

    /// Finalize the hasher and return hash code.
    pub const fn finish(&self) -> u64 {
        self.hash
    }
}

impl Default for Fnv1a64ConstHasher {
    fn default() -> Self {
        Self::new()
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use std::hash::Hash;
    use std::time::Instant;

    const SHORT_TEXT: &str = "Identifier";
    const LOREM_IPSUM: &str = r#"
Lorem ipsum dolor sit amet, consectetur adipisici elit, sed eiusmod tempor incidunt ut labore et dolore magna aliqua. 
Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquid ex ea commodi consequat. Quis aute 
iure reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur. Excepteur sint obcaecat cupiditat 
non proident, sunt in culpa qui officia deserunt mollit anim id est laborum.

Duis autem vel eum iriure dolor in hendrerit in vulputate velit esse molestie consequat, vel illum dolore eu feugiat 
nulla facilisis at vero eros et accumsan et iusto odio dignissim qui blandit praesent luptatum zzril delenit augue 
duis dolore te feugait nulla facilisi. Lorem ipsum dolor sit amet, consectetuer adipiscing elit, sed diam nonummy 
nibh euismod tincidunt ut laoreet dolore magna aliquam erat volutpat.

Ut wisi enim ad minim veniam, quis nostrud exerci tation ullamcorper suscipit lobortis nisl ut aliquip ex ea commodo 
consequat. Duis autem vel eum iriure dolor in hendrerit in vulputate velit esse molestie consequat, vel illum dolore 
eu feugiat nulla facilisis at vero eros et accumsan et iusto odio dignissim qui blandit praesent luptatum zzril 
delenit augue duis dolore te feugait nulla facilisi.

Nam liber tempor cum soluta nobis eleifend option congue nihil imperdiet doming id quod mazim placerat facer possim assum. 
Lorem ipsum dolor sit amet, consectetuer adipiscing elit, sed diam nonummy nibh euismod tincidunt ut laoreet dolore magna 
aliquam erat volutpat. Ut wisi enim ad minim veniam, quis nostrud exerci tation ullamcorper suscipit lobortis nisl ut aliquip 
ex ea commodo consequat.
"#;
    const ITERATIONS: usize = if cfg!(miri) { 10 } else { 1_000_000 };

    fn run_performance<H>(hasher: &mut H)
    where
        H: Hasher,
    {
        // performance
        let start = Instant::now();
        for _ in 0..ITERATIONS {
            LOREM_IPSUM.hash(hasher);
        }
        let duration = start.elapsed();
        println!(
            "Performance on {ITERATIONS} iterations over {} characters: {:?} produced 0x{:016x}",
            LOREM_IPSUM.len(),
            duration,
            hasher.finish()
        );
    }

    #[test]
    fn fxhash_hashing() {
        let hasher = FxHasher::default();
        assert_eq!(hasher.finish(), FxHasher::DEFAULT as u64);

        let mut hasher = FxHasher::default();
        SHORT_TEXT.hash(&mut hasher);
        assert_eq!(hasher.finish(), 0x9cd9284cc45a8dec);

        let mut hasher = FxHasher::default();
        LOREM_IPSUM.hash(&mut hasher);
        assert_eq!(hasher.finish(), 0xe8fdc19deb596b57);

        // performance
        run_performance(&mut hasher);
    }

    #[test]
    fn fnv1a32hash_hashing() {
        let hasher = Fnv1a32Hasher::default();
        assert_eq!(hasher.finish(), Fnv1a32Hasher::DEFAULT as u64);

        let mut hasher = Fnv1a32Hasher::default();
        SHORT_TEXT.hash(&mut hasher);
        assert_eq!(hasher.finish(), 0x8ea26d73);

        let mut hasher = Fnv1a32Hasher::default();
        LOREM_IPSUM.hash(&mut hasher);
        assert_eq!(hasher.finish(), 0x2c01582a);

        // performance
        run_performance(&mut hasher);
    }

    #[test]
    fn fnv1a64hash_hashing() {
        let hasher = Fnv1a64Hasher::default();
        assert_eq!(hasher.finish(), Fnv1a64Hasher::DEFAULT);

        let mut hasher = Fnv1a64Hasher::default();
        SHORT_TEXT.hash(&mut hasher);
        assert_eq!(hasher.finish(), 0xae427b5fb59f2833);

        let mut hasher = Fnv1a64Hasher::default();
        LOREM_IPSUM.hash(&mut hasher);
        assert_eq!(hasher.finish(), 0xbfca6c52bbc24a2a);

        // performance
        run_performance(&mut hasher);
    }
}
