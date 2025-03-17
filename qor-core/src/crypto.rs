// Copyright (c) 2025 Qorix GmbH
//
// This program and the accompanying materials are made available under the
// terms of the Apache License, Version 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0.
//
// SPDX-License-Identifier: Apache-2.0

use crate::types::{bf16, f16, tf32};

/// The CryptoHash trait is used by types that support the creation of cryptographic hashes.
///
/// Other than Hash where the hash value is always as u64, cryptographic hashes are of differente sizes dependent on the algorithm used.
pub trait CryptoHash {
    /// Compute the cryptographic hash of the value.
    ///
    /// The hash type is defined by `Output` type of the `CryptoHasher` implementation.
    fn crypto_hash<H: CryptoHasher>(&self, state: &mut H);

    /// Compute the cryptographic hash of a slice of values
    #[inline]
    fn crypto_hash_slice<H: CryptoHasher>(data: &[Self], state: &mut H)
    where
        Self: Sized,
    {
        for piece in data {
            piece.crypto_hash(state)
        }
    }
}

/// macro to implement the `CryptoHash` trait for a list of types
#[macro_export]
macro_rules! impl_crypto_hash {
    ($($t:ty) *) => {
        $(
            impl CryptoHash for $t {
                #[inline]
                fn crypto_hash<H: CryptoHasher>(&self, state: &mut H) {
                    state.write(&self.to_be_bytes());
                }
            }
        )*
    };
}
impl_crypto_hash!(u8 u16 u32 u64 u128 usize i8 i16 i32 i64 i128 isize);

impl CryptoHash for bool {
    #[inline]
    fn crypto_hash<H: CryptoHasher>(&self, state: &mut H) {
        state.write_u8(*self as u8);
    }
}

impl CryptoHash for f16 {
    #[inline]
    fn crypto_hash<H: CryptoHasher>(&self, state: &mut H) {
        state.write_u16(self.as_raw());
    }
}

impl CryptoHash for bf16 {
    #[inline]
    fn crypto_hash<H: CryptoHasher>(&self, state: &mut H) {
        state.write_u16(self.as_raw());
    }
}

impl CryptoHash for tf32 {
    #[inline]
    fn crypto_hash<H: CryptoHasher>(&self, state: &mut H) {
        state.write_u32(self.as_raw());
    }
}

impl CryptoHash for char {
    #[inline]
    fn crypto_hash<H: CryptoHasher>(&self, state: &mut H) {
        state.write_u32(*self as u32);
    }
}

impl CryptoHash for str {
    #[inline]
    fn crypto_hash<H: CryptoHasher>(&self, state: &mut H) {
        state.write_str(self);
    }
}

/// A cryptographic Hasher
pub trait CryptoHasher {
    /// The output type of the hasher
    type Output;

    /// finish the hash generation
    fn finish(&mut self);
    fn fetch(&self) -> Self::Output;

    /// finish the hash generation and return the result immediately
    #[inline]
    fn finish_and_fetch(&mut self) -> Self::Output {
        self.finish();
        self.fetch()
    }

    /// Write a byte slice.
    fn write(&mut self, data: &[u8]);

    /// Write a single byte.
    #[inline]
    fn write_u8(&mut self, byte: u8) {
        self.write(&[byte])
    }

    /// Write an unsigned 16 bit value.
    #[inline]
    fn write_u16(&mut self, value: u16) {
        self.write(&value.to_be_bytes());
    }

    /// Write an unsigned 32 bit value.
    #[inline]
    fn write_u32(&mut self, value: u32) {
        self.write(&value.to_be_bytes());
    }

    /// Write an unsigned 64 bit value.
    #[inline]
    fn write_u64(&mut self, value: u64) {
        self.write(&value.to_be_bytes());
    }

    /// Write an unsigned 128 bit value.
    #[inline]
    fn write_u128(&mut self, value: u128) {
        self.write(&value.to_be_bytes());
    }

    /// Write an unsigned size value.
    #[inline]
    fn write_usize(&mut self, value: usize) {
        self.write(&value.to_be_bytes());
    }

    /// Write a signed 8 bit value.
    #[inline]
    fn write_i8(&mut self, value: i8) {
        self.write_u8(value as u8);
    }

    /// Write a signed 16 bit value.
    #[inline]
    fn write_i16(&mut self, value: i16) {
        self.write_u16(value as u16);
    }

    /// Write a signed 32 bit value.
    #[inline]
    fn write_i32(&mut self, value: i32) {
        self.write_u32(value as u32);
    }

    /// Write a signed 64 bit value.
    #[inline]
    fn write_i64(&mut self, value: i64) {
        self.write_u64(value as u64);
    }

    /// Write a signed 128 bit value.
    #[inline]
    fn write_i128(&mut self, value: i128) {
        self.write_u128(value as u128);
    }

    /// Write a signed size value.
    #[inline]
    fn write_isize(&mut self, value: isize) {
        self.write_usize(value as usize);
    }

    /// Write a string slice.
    ///
    /// As UTF-8 never ends with `0xFF` we append a `0xFF` byte to the end of the string.
    /// This creates a different hash for the empty string than for `()`.
    #[inline]
    fn write_str(&mut self, value: &str) {
        self.write(value.as_bytes());
        self.write_u8(0xff);
    }
}

/// Cryptographic hash function SHA-1
#[derive(Debug)]
pub struct Sha1 {
    state: [u32; 5],
    data: Vec<u8>,
    len: u64,
    finalized: bool,
}

impl CryptoHasher for Sha1 {
    type Output = [u32; 5];

    fn finish(&mut self) {
        if self.finalized {
            return;
        }

        // append the bit '1' to the message
        let bit_len = self.len;
        self.data.push(0x80);

        // append 0 ≤ k < 512 bits '0', such that the resulting message length in bits
        while self.data.len() % 64 != 56 {
            self.data.push(0);
        }

        // append length of message (before pre-processing), in bits, as 64-bit big-endian integer
        self.data.extend_from_slice(&bit_len.to_be_bytes());

        // process the final chunk
        while self.data.len() >= 64 {
            let chunk = self.data.drain(..64).collect::<Vec<_>>();
            self.process_chunk(&chunk);
        }

        // finalize the hash
        self.finalized = true;
    }

    fn fetch(&self) -> Self::Output {
        // SHA-1 has only one output size, so we can return it directly
        self.state
    }

    /// Write a byte slice.
    fn write(&mut self, input: &[u8]) {
        self.data.extend_from_slice(input);
        self.len += input.len() as u64 * 8;
        while self.data.len() >= 64 {
            let chunk = self.data.drain(..64).collect::<Vec<_>>();
            self.process_chunk(&chunk);
        }
    }

    /// Write a single byte.
    fn write_u8(&mut self, byte: u8) {
        self.write(&[byte]);
    }
}

impl Sha1 {
    /// Create a new SHA-1 hasher
    pub const fn new() -> Self {
        Self {
            state: [0x67452301, 0xefcdab89, 0x98badcfe, 0x10325476, 0xc3d2e1f0],
            data: Vec::new(),
            len: 0,
            finalized: false,
        }
    }

    /// Create a new SHA-1 hasher with a specific seed
    pub const fn from_seed(seed: [u32; 5]) -> Self {
        Self {
            state: seed,
            data: Vec::new(),
            len: 0,
            finalized: false,
        }
    }

    /// The SHA-1 hash function processes data in 512-bit blocks, which is 64 bytes.
    fn process_chunk(&mut self, chunk: &[u8]) {
        let mut w = [0u32; 80];
        for (i, chunk) in chunk.chunks(4).enumerate() {
            w[i] = u32::from_be_bytes([chunk[0], chunk[1], chunk[2], chunk[3]]);
        }
        for i in 16..80 {
            w[i] = (w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16]).rotate_left(1);
        }
        let mut a = self.state[0];
        let mut b = self.state[1];
        let mut c = self.state[2];
        let mut d = self.state[3];
        let mut e = self.state[4];
        for i in 0..80 {
            let (f, k) = match i {
                0..=19 => ((b & c) | (!b & d), 0x5a827999),
                20..=39 => (b ^ c ^ d, 0x6ed9eba1),
                40..=59 => ((b & c) | (b & d) | (c & d), 0x8f1bbcdc),
                _ => (b ^ c ^ d, 0xca62c1d6),
            };
            let temp = a
                .rotate_left(5)
                .wrapping_add(f)
                .wrapping_add(e)
                .wrapping_add(k)
                .wrapping_add(w[i]);
            e = d;
            d = c;
            c = b.rotate_left(30);
            b = a;
            a = temp;
        }
        self.state[0] = self.state[0].wrapping_add(a);
        self.state[1] = self.state[1].wrapping_add(b);
        self.state[2] = self.state[2].wrapping_add(c);
        self.state[3] = self.state[3].wrapping_add(d);
        self.state[4] = self.state[4].wrapping_add(e);
    }
}
